
// MemReader.cpp

#include "pch.h"
#include "MemReader.h"
#include "Logger.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <vector>

// Singleton Instance
MemReader& MemReader::getInstance() {
    static MemReader instance;
    return instance;
}

// Constructor
MemReader::MemReader() : baseAddress_(0) {}

// Destructor
MemReader::~MemReader() = default;

// Helper to convert an integer to a hexadecimal string
std::string addressToHex(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}

// Initialize MemReader
void MemReader::initialize() {
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule == nullptr) {
        Logger::getInstance().log("Failed to get module handle");
        return;
    }
    baseAddress_ = reinterpret_cast<uintptr_t>(hModule);

    // Use addressToHex to convert the base address to a string
    Logger::getInstance().log("MemReader initialized with base address: " + addressToHex(baseAddress_));
}

// Helper to convert a string to its hexadecimal representation
std::string stringToHex(const std::string& input) {
    std::ostringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        hexStream << std::setw(2) << static_cast<int>(c);
    }
    return hexStream.str();
}

// Helper to validate server name
bool isValid(const std::string& str) {
    // Check that string only contains valid ASCII characters
    for (char c : str) {
        if (c != '\0' && (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 125)) {
            return false;
        }
    }

    // Ensure there's at least one non-null character
    return std::any_of(str.begin(), str.end(), [](char c) { return c != '\0'; });
}

// Read a string at a specific memory offset
std::string MemReader::readStringAtAddress(bool relative,
    uintptr_t offset,
    size_t size,
    const std::string& label,
    bool truncateAtNull)
{
    // Conditionally use baseAddress
    uintptr_t targetAddress = relative ? (baseAddress_ + offset) : offset;
    Logger::getInstance().log(
        "Looking up " + label + " at " + addressToHex(targetAddress)
        + " (offset +" + addressToHex(offset)
        + ", size " + std::to_string(size) + "): ",
        false
    );

    // Read raw characters from memory
    const char* src = reinterpret_cast<const char*>(targetAddress);
    std::string result(src, size);

    Logger::getInstance().log(stringToHex(result));

    // Conditionally truncate at the first null terminator
    if (truncateAtNull) {
        size_t nullPos = result.find('\0');
        if (nullPos != std::string::npos) {
            result.erase(nullPos);
        }
    }

    return result;
}

// Search process memory for a string and return address, value
std::tuple<uintptr_t, std::string> MemReader::searchMemory(const std::string& searchString, size_t offset, size_t size, const std::string& label, bool truncateAtNull) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t startAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    uintptr_t endAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
    uintptr_t currentAddress = startAddress;

    Logger::getInstance().log("Searching for " + label + " with pattern " + stringToHex(searchString));

    while (currentAddress < endAddress) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
            break;
        }

        if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {
            std::vector<char> buffer(mbi.RegionSize);
            SIZE_T bytesRead = 0;

            if (ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(mbi.BaseAddress),
                buffer.data(), mbi.RegionSize, &bytesRead))
            {
                std::string memoryChunk(buffer.data(), bytesRead);
                size_t position = 0;

                while ((position = memoryChunk.find(searchString, position)) != std::string::npos) {
                    uintptr_t foundAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + position;

                    Logger::getInstance().log("Match at address " + addressToHex(foundAddress));

                    // Ensure we have enough room for offset + size
                    if (foundAddress + offset + size <= reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize) {
                        std::string extractedData(buffer.data() + position + offset, size);

                        Logger::getInstance().log("Value at address " + addressToHex(foundAddress + offset)
                            + " (offset +" + addressToHex(offset) + ", size " + std::to_string(size) + ") : " + stringToHex(extractedData));

                        if (isValid(extractedData)) {
                            Logger::getInstance().log("Valid occurrence found");

                            // Conditionally truncate at the first null terminator
                            if (truncateAtNull) {
                                size_t nullPos = extractedData.find('\0');
                                if (nullPos != std::string::npos) {
                                    extractedData.erase(nullPos);
                                }
                            }

                            return { foundAddress, extractedData };
                        }
                    }
                    // Move past this occurrence
                    position += searchString.length();
                }
            }
        }
        // Move on to the next region
        currentAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }

    // Pattern not found
    Logger::getInstance().log("Pattern not found");
    return { 0, "Unknown" };
}

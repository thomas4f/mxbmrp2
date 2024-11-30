
// MemReader.cpp

#include "pch.h"
#include "MemReader.h"
#include "Logger.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <vector>

// Minimum valid length for a server name
constexpr size_t MIN_VALID_LENGTH = 4;

// Singleton Instance
MemReader& MemReader::getInstance() {
    static MemReader instance;
    return instance;
}

// Constructor
MemReader::MemReader() : baseAddress_(0) {}

// Destructor
MemReader::~MemReader() = default;

// Utility function to convert an integer to a hexadecimal string
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

// Utility function to convert a string to its hexadecimal representation
std::string stringToHex(const std::string& input) {
    std::ostringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        hexStream << std::setw(2) << static_cast<int>(c);
    }
    return hexStream.str();
}

// Helper function to validate server name
bool isValid(const std::string& str) {
    if (str.length() < MIN_VALID_LENGTH) {
        return false; // Server name too short
    }

    bool hasPrintable = false;
    for (char c : str) {
        if (c == '\0') {
            break;
        }
        if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 127) {
            return false; // Contains non-printable ASCII characters
        }
        hasPrintable = true;
    }
    return hasPrintable;
}


// Read a string at a specific memory offset
std::string MemReader::readStringAtOffset(uintptr_t offset, size_t size, const std::string& label) {
    if (baseAddress_ == 0) {
        Logger::getInstance().log("Base address is not initialized");
        return "Uknown";
    }

    Logger::getInstance().log("Looking up " + label + " at offset " + addressToHex(offset));


    uintptr_t targetAddress = baseAddress_ + offset;
    std::vector<char> buffer(size, 0);
    SIZE_T bytesRead = 0;

    if (ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(targetAddress), buffer.data(), size, &bytesRead)) {
        return std::string(buffer.data(), bytesRead); // Return the string including all bytes read
    }
    else {
        Logger::getInstance().log("Failed to read " + label);
        return "Unknown";
    }
}

// Searches process memory for a string
std::string MemReader::searchMemory(const std::string& searchString, size_t size, size_t offset) {
    if (baseAddress_ == 0) {
        Logger::getInstance().log("Base address is not initialized");
        return "Uknown";
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t startAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    uintptr_t endAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
    uintptr_t currentAddress = startAddress;

    // Log the message using the Logger singleton
    Logger::getInstance().log("Searching for server_name with pattern: " + stringToHex(searchString));

    while (currentAddress < endAddress) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
            break;
        }

        if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {

            std::vector<char> buffer(mbi.RegionSize);
            SIZE_T bytesRead;

            if (ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(mbi.BaseAddress), buffer.data(), mbi.RegionSize, &bytesRead)) {
                std::string memoryChunk(buffer.data(), bytesRead);
                size_t position = 0;

                while ((position = memoryChunk.find(searchString, position)) != std::string::npos) {
                    uintptr_t foundAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + position;

                    if (foundAddress + offset + size <= reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize) {
                        std::string extractedData(buffer.data() + position + offset, size);

                        Logger::getInstance().log("Value at address " + addressToHex(foundAddress) + ": " + stringToHex(extractedData));

                        // Validate the extracted data
                        if (isValid(extractedData)) {
                            // Find the position of the first null byte
                            size_t nullPos = extractedData.find('\0');
                            if (nullPos != std::string::npos) {
                                extractedData.erase(nullPos);
                            }

                            // Log the valid occurrence
                            Logger::getInstance().log("Valid occurrence found at address " + addressToHex(foundAddress) + ": " + extractedData);

                            return extractedData;
                        }
                    }
                    position += searchString.length();
                }
            }
        }

        currentAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }

    Logger::getInstance().log("Pattern not found");

    return "Unknown";
}
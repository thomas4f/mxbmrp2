
// MemReader.cpp

#include "pch.h"

#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <chrono>

#include "MemReader.h"
#include "Logger.h"
#include "Constants.h"

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

// Helper to validate server name
static bool isValid(const std::string& s) {
    auto pos = s.find('\0');
    // must have at least one ASCII before the NUL, and at least one NUL
    if (pos == std::string::npos || pos == 0) return false;

    // all bytes before pos must be printable ASCII
    if (!std::all_of(s.begin(), s.begin() + pos,
        [](unsigned char c) { return c >= 32 && c <= 125; }))
        return false;

    // all bytes from pos to end must be NUL
    if (!std::all_of(s.begin() + pos, s.end(),
        [](unsigned char c) { return c == '\0'; }))
        return false;

    return true;
}

// Helper to convert a byte buffer to a hex string
static std::string bufferToHex(const std::vector<uint8_t>& buf) {
    std::ostringstream oss;
    for (uint8_t b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << std::uppercase << static_cast<int>(b);
    }
    return oss.str();
}

// Hex-dump logging helper
static void logHexDump(
    const char* fnName,
    uintptr_t addr,
    const std::vector<uint8_t>& buf,
    const char* suffix = nullptr
) {
    std::ostringstream oss;
    oss << fnName << "() @ " << addressToHex(addr)
        << " [" << std::dec << buf.size() << " bytes]";
    if (suffix) {
        oss << " " << suffix;
    }
    oss << ": " << bufferToHex(buf);
    Logger::getInstance().log(oss.str());
}

static bool safeMemcpy(void* dst, const void* src, size_t bytes) {
    __try
    {
        std::memcpy(dst, src, bytes);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;        // access violation, guard page, etc.
    }
}

// Read raw bytes at a specific memory offset
std::vector<uint8_t> MemReader::readRawBytesAtAddress(
    bool        relative,
    uintptr_t   offset,
    size_t      size,
    const char* callerName
) {
    const uintptr_t targetAddress = relative ? (baseAddress_ + offset) : offset;
    std::vector<uint8_t> buffer(size);

    if (!safeMemcpy(buffer.data(),
        reinterpret_cast<const void*>(targetAddress),
        size))
    {
        return {};
    }

    if (LOG_MEMORY_VALUES && callerName)
    {
        logHexDump(callerName, targetAddress, buffer);
    }
    return buffer;
}



// Search process memory for a pattern and return address, value
std::tuple<uintptr_t, std::string>
MemReader::searchMemoryRaw(
    const std::vector<uint8_t>& pattern,
    size_t readOffset,
    size_t readSize,
    const char* callerName
) {
    // Start timing
    auto t_start = std::chrono::high_resolution_clock::now();

    // Guard against empty pattern
    if (pattern.empty()) {
        Logger::getInstance().log(std::string(callerName) + "Empty pattern");
        return { 0, {} };
    }

    // Precompute KMP "lps" array
    std::vector<size_t> lps(pattern.size(), 0);
    for (size_t i = 1, len = 0; i < pattern.size(); ) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        }
        else if (len > 0) {
            len = lps[len - 1];
        }
        else {
            lps[i++] = 0;
        }
    }

    // Address bounds
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t addr = reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);
    uintptr_t end = reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

    const size_t CHUNK_SIZE = 1 * 1024 * 1024; // 1 MB
    const size_t minRegionSize = pattern.size() + readOffset + readSize;
    size_t totalBytesRead = 0;

    while (addr < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == 0)
            break;

        // Tight filter: only private, committed, exact RW, no guards, and large enough
        if (mbi.State != MEM_COMMIT ||
            mbi.Type != MEM_PRIVATE ||
            mbi.Protect != PAGE_READWRITE ||
            (mbi.Protect & PAGE_GUARD) ||
            mbi.RegionSize < minRegionSize)
        {
            addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            continue;
        }

        size_t regionSize = mbi.RegionSize;

        // Read in sliding chunks with overlap for pattern boundary
        for (size_t regionOff = 0; regionOff < regionSize; regionOff += CHUNK_SIZE) {
            size_t bytesLeft = regionSize - regionOff;
            size_t toRead = std::min<size_t>(CHUNK_SIZE + pattern.size() - 1, bytesLeft);

            auto chunk = readRawBytesAtAddress(false, addr + regionOff, toRead);
            totalBytesRead += chunk.size();
            if (chunk.empty()) break;

            // KMP search within this chunk
            size_t R = chunk.size(), P = pattern.size();
            size_t i = 0, j = 0;
            while (i < R) {
                if (chunk[i] == pattern[j]) {
                    ++i; ++j;
                    if (j == P) {
                        uintptr_t foundAddr = addr + regionOff + (i - j);
                        uintptr_t blobAddr = foundAddr + readOffset;

                        if (blobAddr + readSize <= addr + regionSize) {
                            auto blob = readRawBytesAtAddress(false, blobAddr, readSize);
                            totalBytesRead += blob.size();
                            std::string candidate(
                                reinterpret_cast<const char*>(blob.data()),
                                blob.size()
                            );

                            if (isValid(candidate)) {
                                if (LOG_MEMORY_VALUES && callerName) {
                                    logHexDump(callerName, foundAddr, blob, "valid");
                                }

                                // Log bytes read + elapsed time
                                {
                                    double mb = totalBytesRead / (1024.0 * 1024.0);
                                    auto t_end = std::chrono::high_resolution_clock::now();
                                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        t_end - t_start
                                    ).count();

                                    std::ostringstream oss;
                                    oss << "Read " << mb << " MB, "
                                        << "Time elapsed: " << elapsed_ms << " ms";
                                    Logger::getInstance().log(oss.str());
                                }

                                // Truncate at NUL
                                if (auto pos = candidate.find('\0'); pos != std::string::npos) {
                                    candidate.resize(pos);
                                }

                                return { foundAddr, candidate };
                            }
                        }

                        j = lps[j - 1];
                    }
                }
                else if (j > 0) {
                    j = lps[j - 1];
                }
                else {
                    ++i;
                }
            }
        }

        addr = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + regionSize;
    }

    Logger::getInstance().log(std::string(callerName) + "Pattern not found");

    return { 0, {} };
}

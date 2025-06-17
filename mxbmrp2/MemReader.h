
// MemReader.h

#pragma once

#include <string>
#include <cstdint>
#include <vector>

class MemReader {
public:
    // Singleton Instance
    static MemReader& getInstance();

    // Initialize the MemReader
    void initialize();

    // Read a value from a specific memory offset
    std::vector<uint8_t> readRawBytesAtAddress(
        bool relative,
        uintptr_t offset,
        size_t size,
        const char* callerName = nullptr
    );

    // Search memory for a specific string pattern
    std::tuple<uintptr_t, std::string> searchMemoryRaw(
        const std::vector<uint8_t>& pattern,
        size_t readOffset,
        size_t readSize,
        const char* callerName = nullptr
    );

    // Destructor
    ~MemReader();

private:
    // Private constructor for Singleton pattern
    MemReader();

    // Delete copy constructor and assignment operator
    MemReader(const MemReader&) = delete;
    MemReader& operator=(const MemReader&) = delete;

    uintptr_t baseAddress_;
};

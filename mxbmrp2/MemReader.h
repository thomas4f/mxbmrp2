
// MemReader.h

#pragma once

#include <string>
#include <cstdint>

class MemReader {
public:
    // Singleton Instance
    static MemReader& getInstance();

    // Initialize the MemReader
    void initialize();

    // Read a string from a specific memory offset
    std::string readStringAtAddress(bool relative, uintptr_t offset, size_t size = 64, const std::string& label = "", bool truncateAtNull = true);

    // Search memory for a specific string pattern
    std::tuple<uintptr_t, std::string> searchMemory(const std::string& searchString, size_t offset = 0, size_t size = 32, const std::string& label = "", bool truncateAtNull = true);

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

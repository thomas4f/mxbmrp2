
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
    // Optionally specify the number of bytes to read (default: 64)
    std::string readStringAtOffset(uintptr_t offset, size_t size = 64, const std::string& label = "");


    // Search memory for a specific string pattern
    std::string searchMemory(const std::string& searchString, size_t size, size_t offset = 0);

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
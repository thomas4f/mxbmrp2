
// MemReaderHelpers.h

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <tuple>

namespace MemReaderHelpers {

    // Reads a null-terminated string of up to `size` bytes
    // at `offset` (relative to baseAddress_ if relative=true).
    std::string readNullTermString(
        bool relative,
        std::uintptr_t offset,
        std::size_t size,
        const char* callerName = nullptr
    );

    using ByteBuf = std::vector<uint8_t>;

    std::string getConnectURIString();
    std::string getLocalServerName();
    std::string getServerCategories();
    std::string getServerTrackID();
    std::string getLocalServerPassword();
    std::string getLocalServerLocation();
    int getLocalServerClientsMax();
    ByteBuf getRemoteServerSocketAddress();
    std::tuple<uintptr_t, std::string> getRemoteServerNameAndAddress(const ByteBuf& remoteIPv6Hex);
    std::string getRemoteServerPassword();
    std::string getRemoteServerLocation(uintptr_t remoteIPv6MemoryAddress);
    std::string getTrackDeformation();
    int getRemoteServerClientsMax(uintptr_t remoteIPv6MemoryAddress);
    std::string getRemoteServerPing();
    int getServerClientsCount();
    std::string getRemainingTearoffs(const std::string& connectionType);
}

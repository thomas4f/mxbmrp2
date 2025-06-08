
// MemReaderHelpers.cpp

#include "pch.h"

#include "MemReaderHelpers.h"
#include "MemReader.h"
#include "ConfigManager.h"
#include "Constants.h"

namespace {
    auto& configManager = ConfigManager::getInstance();
    auto& memReader = MemReader::getInstance();
}

namespace MemReaderHelpers {

    // Generic helper: read raw bytes, find first 0x00, return string up to that point
    std::string readNullTermString(
        bool relative,
        std::uintptr_t offset,
        std::size_t size,
        const char* callerName
    ) {
        // New reader returns an empty vector on error
        auto raw = memReader.readRawBytesAtAddress(relative, offset, size, callerName);
        if (raw.empty()) {
            return {};  // failed or zero-length
        }

        auto term = std::find(raw.begin(), raw.end(), 0);
        return std::string(raw.begin(), term);
    }

    // getConnectURIString
    std::string getConnectURIString() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("connection_string_offset"),
            SIZE_CONNECTION_STRING,
            __func__
        );
    }

    // getLocalServerName
    std::string getLocalServerName() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("local_server_name_offset"),
            SIZE_LOCAL_SERVER_NAME,
            __func__
        );
    }

    // getServerCategories
    std::string getServerCategories() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("server_categories_offset"),
            SIZE_SERVER_CATEGORIES,
            __func__
        );
    }

    // getServerTrackID
    std::string getServerTrackID() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("server_track_id"),
            SIZE_SERVER_TRACK_ID,
            __func__
        );
    }
    // getLocalServerPassword
    std::string getLocalServerPassword() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("local_server_password_offset"),
            SIZE_LOCAL_SERVER_PASSWORD,
            __func__
        );
    }

    // getLocalServerLocation
    std::string getLocalServerLocation() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("local_server_location_offset"),
            SIZE_LOCAL_SERVER_LOCATION,
            __func__
        );
    }

    // getLocalServerClientsMax
    std::string getLocalServerClientsMax() {
        // Read raw bytes directly at the module-relative offset
        auto raw = memReader.readRawBytesAtAddress(
            true,
            configManager.getValue<unsigned long>("local_server_clients_max_offset"),
            SIZE_LOCAL_SERVER_CLIENTS_MAX,
            __func__
        );

        if (raw.empty()) {
            return "0";
        }

        // The first byte is your client count
        return std::to_string(raw[0]);
    }

    // getRemoteServerSocketAddress
    ByteBuf getRemoteServerSocketAddress() {
        ByteBuf raw = memReader.readRawBytesAtAddress(
            true,
            configManager.getValue<unsigned long>("remote_server_sockaddr_offset"),
            SIZE_REMOTE_SERVER_SOCKADDR,
            __func__
        );

        // All zeroes, not online
        if (std::all_of(raw.begin(), raw.end(), [](auto b) { return b == 0; })) {
            return {};
        }

        // Not implemented yet
        if (raw[22] != 0xFF || raw[23] != 0xFF) {
            Logger::getInstance().log("Not an IPv6-mapped-IPv4 address. Won't be able do determine server name, etc!");
            return {};
        }

        ByteBuf out;
        out.reserve(8);

        out.insert(out.end(),
            raw.begin() + 22,
            raw.begin() + 28);

        out.insert(out.end(),
            raw.begin() + 6,
            raw.begin() + 8);

        return out;
    }

    // getRemoteServerNameAndAddress
    std::tuple<uintptr_t, std::string>
        getRemoteServerNameAndAddress(const ByteBuf& remoteIPv6Hex)
    {
        if (remoteIPv6Hex.empty()) {
            return { 0, {} };
        }

        // Now we already have the pattern bytes:
        const std::vector<uint8_t>& pattern = remoteIPv6Hex;

        // invoke the new raw search
        return memReader.searchMemoryRaw(
            pattern,
            configManager.getValue<unsigned long>("remote_server_name_offset"),
            SIZE_REMOTE_SERVER_NAME,
            __func__
        );
    }

    // getRemoteServerPassword
    std::string getRemoteServerPassword() {
        return readNullTermString(
            true,
            configManager.getValue<unsigned long>("remote_server_password_offset"),
            SIZE_REMOTE_SERVER_PASSWORD,
            __func__
        );
    }

    // getRemoteServerLocation
    std::string getRemoteServerLocation(uintptr_t remoteIPv6MemoryAddress) {
        return readNullTermString(
            false,
            configManager.getValue<unsigned long>("remote_server_location_offset") + remoteIPv6MemoryAddress,
            SIZE_REMOTE_SERVER_LOCATION,
            __func__
        );
    }

    // getRemoteServerClientsMax
    std::string getRemoteServerClientsMax(uintptr_t remoteIPv6MemoryAddress) {
        auto raw = memReader.readRawBytesAtAddress(
            false,
            configManager.getValue<unsigned long>("remote_server_clients_max_offset") + remoteIPv6MemoryAddress,
            SIZE_REMOTE_SERVER_CLIENTS_MAX,
            __func__
        );

        if (raw.empty()) {
            return "?";
        }

        return std::to_string(raw[0]);
    }

    // getRemoteServerPing
    std::string getRemoteServerPing() {
        auto raw = memReader.readRawBytesAtAddress(
            true,
            configManager.getValue<unsigned long>("remote_server_ping_offset"),
            SIZE_REMOTE_SERVER_PING
        );

        if (raw.empty()) {
            return "?";
        }

        uint16_t ping = static_cast<uint16_t>(raw[0]) | (static_cast<uint16_t>(raw[1]) << 8);
        return std::to_string(ping) + " ms";
    }

    // getServerClientsCount
    std::string getServerClientsCount() {
        auto raw = memReader.readRawBytesAtAddress(
            true,
            configManager.getValue<unsigned long>("server_clients_offset"),
            SIZE_SERVER_CLIENTS
        );

        if (raw.empty()) {
            return "?";
        }

        int count = 1; // Local player is always present
        for (size_t i = 0; i + SIZE_SERVER_CLIENTS_BLOCK <= raw.size(); i += SIZE_SERVER_CLIENTS_BLOCK) {
            if (raw[i] != 0) {
                ++count;
            }
        }
        return std::to_string(count);
    }
}

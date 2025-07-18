
// JSONWriter.h

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

class ConfigManager;

namespace JsonWriter {

    std::string escapeJson(std::string_view s);
    std::string renderJson(
        const std::unordered_map<std::string, std::string>& all,
        const std::vector<std::pair<std::string, std::string>>& order,
        ConfigManager& cfg);

    void atomicWrite(const std::filesystem::path& path,
        const std::string& data);

}



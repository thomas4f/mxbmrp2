
// htmlWriter.h

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include "configManager.h"
#include "Constants.h"

namespace HtmlWriter {

    // Render the page
    std::string renderHtml(
        const std::unordered_map<std::string, std::string>& all,
        const std::vector<std::pair<std::string, std::string>>& order,
        ConfigManager& cfg);

    // "No data" placeholder
    std::string renderNoData();

    // Write data to disk
    void atomicWrite(const std::filesystem::path& path,
        const std::string& data);

}

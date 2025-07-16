
// htmlWriter.cpp

#include "pch.h"

#include <sstream>
#include <fstream>

#include "Constants.h"
#include "HtmlWriter.h"
#include "ConfigManager.h"

namespace {
    // Escape XML special characters
    std::string escapeXml(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:    out += c;
            }
        }
        return out;
    }
}

namespace HtmlWriter {

    // Build the HTML page
    std::string renderHtml(
        const std::unordered_map<std::string, std::string>& all,
        const std::vector<std::pair<std::string, std::string>>& order,
        ConfigManager& cfg)
    {
        std::ostringstream body;
        body << "<div class=\"data\">\n";

        bool hasRealData = false;   // ignored if we only have the banner

        for (const auto& [key, displayName] : order) {
            if (!cfg.getValue<bool>(key)) continue;
            auto it = all.find(key);
            if (it == all.end() || it->second.empty()) continue;

            if (key != "plugin_banner")
                hasRealData = true;

            body << "  <div class=\"data__item data__item--" << key << "\">\n";

            if (key == "plugin_banner") {
                body << "    <span class=\"data__value\">"
                    << escapeXml(it->second) << "</span>\n";
            }
            else {
                body << "    <span class=\"data__label\">"
                    << escapeXml(displayName) << " </span>\n"
                    << "    <span class=\"data__value\">"
                    << escapeXml(it->second) << "</span>\n";
            }
            body << "  </div>\n";
        }

        // If we only have the banner, add a placeholder line
        if (!hasRealData) {
            body << "  <span class=\"data__no-data\">No data</span>\n";
        }

        body << "</div>\n";

        // Splice into the shared template
        std::string out = HTML_TEMPLATE;
        auto pos = out.find("{{BODY}}");
        out.replace(pos, strlen("{{BODY}}"), body.str());
        return out;
    }

    // Shown on shutdown when the plugin wipes the file
    std::string renderNoData()
    {
        std::ostringstream body;
        body << "<div class=\"data\">\n"
            << "  <div class=\"data__item data__item--plugin_banner\">\n"
            << "    <span class=\"data__value\">" << PLUGIN_VERSION << "</span>\n"
            << "  </div>\n"
            << "  <span class=\"data__no-data\">No data</span>\n"
            << "</div>\n";

        std::string out = HTML_TEMPLATE;
        auto pos = out.find("{{BODY}}");
        out.replace(pos, strlen("{{BODY}}"), body.str());
        return out;
    }

    void atomicWrite(const std::filesystem::path& path, const std::string& data)
    {
        auto tmp = path;
        tmp += ".tmp";

        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
        ofs.close();

        std::filesystem::rename(tmp, path);
    }

} // namespace HtmlWriter

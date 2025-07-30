
// jsonWriter.cpp

#include "pch.h"

#include <sstream>
#include <fstream>
#include <iomanip>

#include "Constants.h"
#include "ConfigManager.h"
#include "JSONWriter.h"

namespace JsonWriter {
    std::string escapeJson(std::string_view s)
    {
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    std::ostringstream tmp;
                    tmp << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c);
                    out += tmp.str();
                }
                else
                    out += c;
            }
        }
        return out;
    }

    std::string renderJson(
        const std::unordered_map<std::string, std::string>& all,
        const std::vector<std::pair<std::string, std::string>>& order,
        ConfigManager& cfg)
    {
        std::ostringstream out;
        out << "{\n";
        bool first = true;

        for (const auto& [key, /*displayName*/ _] : order)
        {
            if (!cfg.getValue<bool>(key)) continue;
            auto it = all.find(key);
            if (it == all.end() || it->second.empty()) continue;

            if (!first) out << ",\n";
            first = false;
            out << "  \"" << key << "\": \"" << escapeJson(it->second) << "\"";
        }
        out << "\n}\n";
        return out.str();
    }

    std::string renderNoData() {
        return std::string("{\n  \"plugin_banner\": \"") + PLUGIN_VERSION + "\"\n}\n";
    }


    void atomicWrite(const std::filesystem::path& path,
        const std::string& data)
    {
        auto tmp = path;
        tmp += ".tmp";
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
        ofs.close();
        std::filesystem::rename(tmp, path);
    }

} // namespace JsonWriter

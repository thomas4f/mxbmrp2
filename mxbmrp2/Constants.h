
// Constants.h

#pragma once

#include <cstddef>
#include <filesystem>

// Build info
#if defined(_DEBUG)
inline constexpr const char* BUILD_TYPE = "Debug";
#elif defined(NDEBUG)
inline constexpr const char* BUILD_TYPE = "Release";
#endif

// Plugin
inline constexpr const char* PLUGIN_VERSION = "mxbmrp2-v0.9.17";
inline constexpr const char* HOST_VERSION = "MX Bikes beta20b";
inline constexpr const char* DATA_DIR = "mxbmrp2_data";
inline constexpr const char* PROFILE_DIR = "mxbmrp2";
inline const std::filesystem::path LOG_FILE = "mxbmrp2.log";
inline const std::filesystem::path CONFIG_FILE = "mxbmrp2.ini";
inline const std::filesystem::path TIME_TRACKER_FILE = "mxbmrp2.dat";
inline const std::filesystem::path HTML_FILE = "mxbmrp2.html";
inline const std::filesystem::path JSON_FILE = "mxbmrp2.json";

inline constexpr UINT HOTKEY = 'R';
inline constexpr float LINE_HEIGHT_MULTIPLIER = 1.1f;
inline constexpr size_t MAX_STRING_LENGTH = 48;
inline constexpr uint32_t SETUP_DEFAULT_HIGHLIGHT_MS = 5000;

inline constexpr int PERIODIC_TASK_INTERVAL = 1000;
inline constexpr const char* DEFAULT_PLAYER_ACTIVITY = "In Menus";
inline constexpr bool LOG_MEMORY_VALUES = true;

// Discord RP
inline constexpr uint64_t DISCORD_APP_ID = 1286928297288011817ULL;

// MXB_interface
inline constexpr const char* MOD_ID = "mxbikes";
inline constexpr int MOD_DATA_VERSION = 8;
inline constexpr int INTERFACE_VERSION = 9;
inline constexpr int REFRESH_RATE = 3;

// Memory sizes
inline constexpr std::size_t SIZE_LOCAL_SERVER_NAME = 64;
inline constexpr std::size_t SIZE_LOCAL_SERVER_PASSWORD = 32;
inline constexpr std::size_t SIZE_LOCAL_SERVER_LOCATION = 32;
inline constexpr std::size_t SIZE_LOCAL_SERVER_CLIENTS_MAX = 1;
inline constexpr std::size_t SIZE_REMOTE_SERVER_SOCKADDR = 28;
inline constexpr std::size_t SIZE_REMOTE_SERVER_PASSWORD = 32;
inline constexpr std::size_t SIZE_REMOTE_SERVER_NAME = 64;
inline constexpr std::size_t SIZE_REMOTE_SERVER_LOCATION = 32;
inline constexpr std::size_t SIZE_REMOTE_SERVER_PING = 2;
inline constexpr std::size_t SIZE_REMOTE_SERVER_CLIENTS_MAX = 1;
inline constexpr std::size_t SIZE_SERVER_CATEGORIES = 128;
inline constexpr std::size_t SIZE_SERVER_CLIENTS = 3200;
inline constexpr std::size_t SIZE_SERVER_CLIENTS_BLOCK = 64;
inline constexpr std::size_t SIZE_TRACK_DEFORMATION = 4;
inline constexpr std::size_t SIZE_SERVER_TRACK_ID = 32;
inline constexpr std::size_t SIZE_CONNECTION_STRING = 128;
inline constexpr std::size_t SIZE_REMAINING_TEAROFFS = 1;

// ConfigManager
static constexpr char DEFAULT_INI_TEMPLATE[] = R"(# mxbmrp2.ini

# Draw configuration
plugin_banner={{plugin_banner}}
race_number={{race_number}}
rider_name={{rider_name}}
bike_category={{bike_category}}
bike_id={{bike_id}}
bike_name={{bike_name}}
setup_name={{setup_name}}
remaining_tearoffs={{remaining_tearoffs}}
track_id={{track_id}}
track_name={{track_name}}
track_length={{track_length}}
connection_type={{connection_type}}
server_name={{server_name}}
server_password={{server_password}}
server_location={{server_location}}
server_ping={{server_ping}}
server_clients={{server_clients}}
event_type={{event_type}}
session_type={{session_type}}
session_state={{session_state}}
session_duration={{session_duration}}
conditions={{conditions}}
air_temperature={{air_temperature}}
track_deformation={{track_deformation}}
combo_time={{combo_time}}
total_time={{total_time}}
discord_status={{discord_status}}

# HUD visibility and placement
default_enabled={{default_enabled}}
position_x={{position_x}}
position_y={{position_y}}

# Other settings
font_name={{font_name}}
font_size={{font_size}}
font_color={{font_color}}
background_color={{background_color}}

# Discord Rich Presence
enable_discord_rich_presence={{enable_discord_rich_presence}}

# HTML/JSON Export
enable_html_export={{enable_html_export}}
enable_json_export={{enable_json_export}}

# Memory addresses (don't touch!)
local_server_name_offset={{local_server_name_offset}}
local_server_password_offset={{local_server_password_offset}}
local_server_location_offset={{local_server_location_offset}}
local_server_clients_max_offset={{local_server_clients_max_offset}}

remote_server_sockaddr_offset={{remote_server_sockaddr_offset}}
remote_server_password_offset={{remote_server_password_offset}}
remote_server_name_offset={{remote_server_name_offset}}
remote_server_location_offset={{remote_server_location_offset}}
remote_server_ping_offset={{remote_server_ping_offset}}
remote_server_clients_max_offset={{remote_server_clients_max_offset}}

local_server_remaining_tearoffs_offset={{local_server_remaining_tearoffs_offset}}
remote_server_remaining_tearoffs_offset={{remote_server_remaining_tearoffs_offset}}

track_deformation_offset={{track_deformation_offset}}

server_clients_offset={{server_clients_offset}}
server_track_id_offset={{server_track_id_offset}}
server_categories_offset={{server_categories_offset}}

connection_string_offset={{connection_string_offset}}
)";

// HTML export
static constexpr char HTML_TEMPLATE[] = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script>setInterval(() => location.reload(), {{INTERVAL}});</script>
    <title>MXBMRP2</title>
    <link rel="stylesheet" href="mxbmrp2.css">
</head>
<body>
    {{BODY}}
</body>
</html>)";

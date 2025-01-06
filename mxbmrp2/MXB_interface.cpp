
// MXB_interface.cpp

#include "pch.h"
#include "Constants.h"
#include "MXB_interface.h"
#include "Logger.h"
#include "Plugin.h"

// Exported functions

// GetModID: Called to get the module ID
__declspec(dllexport) const char* GetModID() {
    return MOD_ID;
}

// GetModDataVersion: Called to get the module data version
__declspec(dllexport) int GetModDataVersion() {
    return MOD_DATA_VERSION;
}

// GetInterfaceVersion: Called to get the interface version
__declspec(dllexport) int GetInterfaceVersion() {
    return INTERFACE_VERSION;
}

// Startup: Called when software is started
__declspec(dllexport) int Startup(char* _szSavePath) {
    if (_szSavePath != nullptr) {
        Logger::getInstance().log("Save path: " + std::string(_szSavePath));
    }

    return REFRESH_RATE;
}

// Shutdown: Called when software is closed
__declspec(dllexport) void Shutdown() {
}

// EventInit: Called when event is initialized (Testing/Race)
__declspec(dllexport) void EventInit(void* _pData, int _iDataSize) {
    if (_pData == nullptr) {
        Logger::getInstance().log("EventInit received null data");
        return;
    }

    if (_iDataSize < sizeof(SPluginsBikeEvent_t)) {
        Logger::getInstance().log("EventInit received insufficient data size");
        return;
    }

    SPluginsBikeEvent_t* psEventData = static_cast<SPluginsBikeEvent_t*>(_pData);
    Plugin::getInstance().onEventInit(*psEventData);
}

// EventDeinit: Called when event is closed
__declspec(dllexport) void EventDeinit() {
    Plugin::getInstance().onEventDeinit();
}

// DrawInit: Called when drawing is initialized
__declspec(dllexport) int DrawInit(int* _piNumSprites, char** _pszSpriteName, int* _piNumFonts, char** _pszFontName) {
    // Retrieve display strings from Plugin
    Plugin& plugin = Plugin::getInstance();
    const std::vector<std::string>& displayStrs = plugin.getDisplayKeys();

    // Access cached Draw configuration
    const Plugin::displayConfig& config = plugin.displayConfig_;

    // Specify no sprites
    *_piNumSprites = 0;
    *_pszSpriteName = nullptr;

    // Specify one font
    *_piNumFonts = 1;

    // Allocate memory for font name
    const char* fontName = config.fontName.c_str();
    size_t fontNameLength = strlen(fontName) + 1; // +1 for the null terminator

    // Allocate memory for the font name string
    char* fonts = new char[fontNameLength];
    strcpy_s(fonts, fontNameLength, fontName);

    *_pszFontName = fonts;

    return 0;
}

// Draw: Called to perform drawing
__declspec(dllexport) void Draw(int _iState, int* _piNumQuads, void** _ppQuad, int* _piNumString, void** _ppString) {
    // Retrieve display strings from Plugin
    Plugin& plugin = Plugin::getInstance();
    const std::vector<std::string>& displayStrs = plugin.getDisplayKeys();

    if (displayStrs.empty()) {
        *_piNumQuads = 0;
        *_ppQuad = nullptr;
        *_piNumString = 0;
        *_ppString = nullptr;
        return;
    }

    // Access cached Draw configuration
    const Plugin::displayConfig& config = plugin.displayConfig_;

    // Calculate quad dimensions
    float startX = config.positionX;
    float startY = config.positionY;
    float fontSize = config.fontSize;
    float lineHeight = config.lineHeight;
    float quadWidth = config.quadWidth;
    float quadHeight = lineHeight * displayStrs.size();

    // Allocate memory for quads using smart pointer
    std::unique_ptr<SPluginQuad_t[]> quads(new (std::nothrow) SPluginQuad_t[1]);
    if (!quads) {
        Logger::getInstance().log("Failed to allocate memory for quads");
        *_piNumQuads = 0;
        *_ppQuad = nullptr;
        *_piNumString = 0;
        *_ppString = nullptr;
        return;
    }

    // Initialize quad
    quads[0].m_aafPos[0][0] = startX;
    quads[0].m_aafPos[0][1] = startY;
    quads[0].m_aafPos[1][0] = startX;
    quads[0].m_aafPos[1][1] = startY + quadHeight;
    quads[0].m_aafPos[2][0] = startX + quadWidth;
    quads[0].m_aafPos[2][1] = startY + quadHeight;
    quads[0].m_aafPos[3][0] = startX + quadWidth;
    quads[0].m_aafPos[3][1] = startY;
    quads[0].m_iSprite = 0;
    quads[0].m_ulColor = config.backgroundColor;

    *_piNumQuads = 1;
    *_ppQuad = quads.release(); // Transfer ownership to the caller

    // Allocate memory for strings using smart pointer
    size_t numStrings = displayStrs.size();
    std::unique_ptr<SPluginString_t[]> strings(new (std::nothrow) SPluginString_t[numStrings]);
    if (!strings) {
        Logger::getInstance().log("Failed to allocate memory for strings");
        // Free previously allocated quads
        delete[] reinterpret_cast<SPluginQuad_t*>(*_ppQuad);
        *_piNumQuads = 0;
        *_ppQuad = nullptr;
        *_piNumString = 0;
        *_ppString = nullptr;
        return;
    }

    // Initialize the strings
    for (size_t i = 0; i < numStrings; ++i) {
        const std::string& currentStr = displayStrs[i];
        std::string truncatedStr = currentStr.substr(0, 99); // Truncate to max length
        strncpy_s(strings[i].m_szString, sizeof(strings[i].m_szString), truncatedStr.c_str(), _TRUNCATE);
        strings[i].m_afPos[0] = startX + (fontSize / 4); // X position
        strings[i].m_afPos[1] = startY + (i * lineHeight); // Y position
        strings[i].m_iFont = 1;
        strings[i].m_fSize = fontSize;
        strings[i].m_iJustify = 0; // Left-justified
        strings[i].m_ulColor = config.fontColor;

        // Check if the string starts with "mxbmrp2" (case-sensitive)
        if (i == 0 && currentStr.compare(0, 7, "mxbmrp2") == 0) {
            strings[i].m_ulColor = 0xFF0081CC; /* ABGR */
        }
    }

    *_piNumString = static_cast<int>(numStrings);
    *_ppString = strings.release(); // Transfer ownership to the caller
}

// RunInit: Called when bike goes to track
__declspec(dllexport) void RunInit(void* _pData, int _iDataSize) {
    if (_pData == nullptr) {
        Logger::getInstance().log("RunInit received null data");
        return;
    }

    if (_iDataSize < sizeof(SPluginsBikeSession_t)) {
        Logger::getInstance().log("RunInit received insufficient data size");
        return;
    }

    // Delegate to Plugin
    SPluginsBikeSession_t* psSessionData = static_cast<SPluginsBikeSession_t*>(_pData);
    Plugin::getInstance().onRunInit(*psSessionData);
}

// RaceSession: Called when Session Starts
__declspec(dllexport) void RaceSession(void* _pData, int _iDataSize) {
    if (_pData == nullptr) {
        Logger::getInstance().log("RaceSession received null data");
        return;
    }

    if (_iDataSize < sizeof(SPluginsRaceSession_t)) {
        Logger::getInstance().log("RaceSession received insufficient data size");
        return;
    }

    // Delegate to Plugin
    SPluginsRaceSession_t* psRaceSession = static_cast<SPluginsRaceSession_t*>(_pData);
    Plugin::getInstance().onRaceSession(*psRaceSession);
}

// RaceSessionState: Called when Session Ends
__declspec(dllexport) void RaceSessionState(void* _pData, int _iDataSize) {
    if (_pData == nullptr) {
        Logger::getInstance().log("RaceSessionState received null data");
        return;
    }

    if (_iDataSize < sizeof(SPluginsRaceSessionState_t)) {
        Logger::getInstance().log("RaceSessionState received insufficient data size");
        return;
    }

    // Delegate to Plugin
    SPluginsRaceSessionState_t* psRaceSessionState = static_cast<SPluginsRaceSessionState_t*>(_pData);
    Plugin::getInstance().onRaceSessionState(*psRaceSessionState);
}

// RaceAddEntry: Called when a new entry is added to the race
__declspec(dllexport) void RaceAddEntry(void* _pData, int _iDataSize) {
    if (_pData == nullptr) {
        Logger::getInstance().log("RaceAddEntry received null data");
        return;
    }

    if (_iDataSize < sizeof(SPluginsRaceAddEntry_t)) {
        Logger::getInstance().log("RaceAddEntry received insufficient data size");
        return;
    }

    // Delegate to Plugin
    SPluginsRaceAddEntry_t* psRaceAddEntry = static_cast<SPluginsRaceAddEntry_t*>(_pData);
    Plugin::getInstance().onRaceAddEntry(*psRaceAddEntry);
}

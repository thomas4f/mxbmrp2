
// MXB_interface.cpp

#include "pch.h"

#include "Constants.h"
#include "MXB_interface.h"
#include "Logger.h"
#include "Plugin.h"

namespace {
    static std::vector<char>            g_fontNameBuf;
    static std::vector<SPluginQuad_t>   g_quadsBuf;
    static std::vector<SPluginString_t> g_strsBuf;
}

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
    Plugin::getInstance().onStartup(_szSavePath);
    return REFRESH_RATE;
}

// Shutdown: Called when software is closed
__declspec(dllexport) void Shutdown() {
    Plugin::getInstance().onShutdown();
}

// EventInit: Called when event is initialized (Testing/Race)
__declspec(dllexport) void EventInit(void* _pData, int _iDataSize) {
    SPluginsBikeEvent_t* psEventData = (SPluginsBikeEvent_t*)_pData;
    Plugin::getInstance().onEventInit(*psEventData);
}

// EventDeinit: Called when event is closed
__declspec(dllexport) void EventDeinit() {
    Plugin::getInstance().onEventDeinit();
}

// DrawInit: called when software is started.
__declspec(dllexport) int DrawInit(int* outNumSprites,
    char** outSpriteNames,
    int* outNumFonts,
    char** outFontNames)
{
    // we don't draw any sprites
    *outNumSprites = 0;
    *outSpriteNames = nullptr;

    // exactly one font
    *outNumFonts = 1;
    auto& cfg = Plugin::getInstance().displayConfig_;
    const auto& fn = cfg.fontName;

    // copy into persistent buffer with null terminator
    g_fontNameBuf.assign(fn.begin(), fn.end());
    g_fontNameBuf.push_back('\0');
    *outFontNames = g_fontNameBuf.data();

    return 0;
}

// Draw: Called every frame
__declspec(dllexport) void Draw(int state,
    int* outNumQuads,
    void** outQuads,
    int* outNumStrings,
    void** outStrings)
{
    // only update on state change
    static int lastState = -1;
    auto& plugin = Plugin::getInstance();
    if (state != lastState) {
        plugin.onStateChange(state);
        lastState = state;
    }

    // get what we should display
    auto display = plugin.getDisplayKeys();
    if (display.empty()) {
        *outNumQuads = 0; *outQuads = nullptr;
        *outNumStrings = 0; *outStrings = nullptr;
        return;
    }

    auto& cfg = plugin.displayConfig_;
    float x = cfg.positionX;
    float y = cfg.positionY;
    float lh = cfg.lineHeight;
    float w = cfg.quadWidth;
    float h = lh * static_cast<float>(display.size());

    // single background quad
    g_quadsBuf.clear();
    g_quadsBuf.emplace_back();
    auto& q = g_quadsBuf[0];
    q.m_aafPos[0][0] = x;     q.m_aafPos[0][1] = y;     // top-left
    q.m_aafPos[1][0] = x;     q.m_aafPos[1][1] = y + h; // bottom-left
    q.m_aafPos[2][0] = x + w; q.m_aafPos[2][1] = y + h; // bottom-right
    q.m_aafPos[3][0] = x + w; q.m_aafPos[3][1] = y;     // top-right
    q.m_iSprite = 0;
    q.m_ulColor = cfg.backgroundColor;

    *outNumQuads = 1;
    *outQuads = g_quadsBuf.data();

    // then each string
    size_t n = display.size();
    g_strsBuf.clear();
    g_strsBuf.reserve(display.size());
    for (size_t i = 0; i < n; ++i) {
        g_strsBuf.emplace_back();
        auto& s = g_strsBuf.back();
        // copy up to 99 chars, _TRUNCATE will add null terminator
        strncpy_s(s.m_szString, sizeof(s.m_szString),
            display[i].c_str(), _TRUNCATE);

        s.m_afPos[0] = x + (cfg.fontSize * 0.25f);
        s.m_afPos[1] = y + (static_cast<float>(i) * lh);
        s.m_iFont = 1;
        s.m_fSize = cfg.fontSize;
        s.m_iJustify = 0;
        s.m_ulColor = cfg.fontColor;

        // if first line is our banner, color it specially
        if (i == 0 && display[0].rfind("mxbmrp2", 0) == 0) {
            s.m_ulColor = 0xFF0081CC;  // ABGR
        }
    }

    *outNumStrings = static_cast<int>(n);
    *outStrings = g_strsBuf.data();
}

// RunInit: Called when bike goes to track
__declspec(dllexport) void RunInit(void* _pData, int _iDataSize) {
    SPluginsBikeSession_t* psSessionData = (SPluginsBikeSession_t*)_pData;
    Plugin::getInstance().onRunInit(*psSessionData);
}

// RunDeInit: Called when bike leaves the track
__declspec(dllexport) void RunDeinit() {
    Plugin::getInstance().onRunDeinit();
}

// RaceSession: Called when Session Starts
__declspec(dllexport) void RaceSession(void* _pData, int _iDataSize) {
    SPluginsRaceSession_t* psRaceSession = (SPluginsRaceSession_t*)_pData;
    Plugin::getInstance().onRaceSession(*psRaceSession);
}

// RaceSessionState: Called when Session Ends
__declspec(dllexport) void RaceSessionState(void* _pData, int _iDataSize) {
    SPluginsRaceSessionState_t* psRaceSessionState = (SPluginsRaceSessionState_t*)_pData;
    Plugin::getInstance().onRaceSessionState(*psRaceSessionState);
}

// RaceEvent: Called when a race or replay is initialized
__declspec(dllexport) void RaceEvent(void* _pData, int _iDataSize) {
    SPluginsRaceEvent_t* psRaceEvent = (SPluginsRaceEvent_t*)_pData;
    Plugin::getInstance().onRaceEvent(*psRaceEvent);
}

// RaceAddEntry: Called when a new entry is added to the race
__declspec(dllexport) void RaceAddEntry(void* _pData, int _iDataSize) {
    SPluginsRaceAddEntry_t* psRaceAddEntry = (SPluginsRaceAddEntry_t*)_pData;
    Plugin::getInstance().onRaceAddEntry(*psRaceAddEntry);
}

// RaceRemoveEntry: Called when a race entry is removed
__declspec(dllexport) void RaceRemoveEntry(void* _pData, int _iDataSize) {
    SPluginsRaceRemoveEntry_t* psRaceRemoveEntry = (SPluginsRaceRemoveEntry_t*)_pData;
    Plugin::getInstance().onRaceRemoveEntry(*psRaceRemoveEntry);
}

__declspec(dllexport) void RunStart() {
    Plugin::getInstance().onRunStart();
}

__declspec(dllexport) void RunStop() {
    Plugin::getInstance().onRunStop();
}

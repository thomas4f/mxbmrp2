
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
__declspec(dllexport) void Draw(int state, int* outNumQuads, void** outQuads, int* outNumStrings, void** outStrings) {
    // only update on state change
    static int lastState = -1;
    auto& plugin = Plugin::getInstance();
    if (state != lastState) {
        plugin.onStateChange(state);
        lastState = state;
    }

    // get what we should display
    const auto display = plugin.getDisplayKeys();
    if (display.empty()) {
        *outNumQuads = 0; *outQuads = nullptr;
        *outNumStrings = 0; *outStrings = nullptr;
        return;
    }

    // Cache frequently-used config values
    const auto& cfg = plugin.displayConfig_;
    const float x0 = cfg.positionX;
    const float y0 = cfg.positionY;
    const float lineH = cfg.lineHeight;
    const float quadW = cfg.quadWidth;
    const float bgH = lineH * static_cast<float>(display.size());
    const float textX = x0 + cfg.fontSize * 0.25f;

    // single background quad
    g_quadsBuf.assign(1, {});
    auto& bg = g_quadsBuf.front();
    bg.m_aafPos[0][0] = x0; bg.m_aafPos[0][1] = y0;               // TL
    bg.m_aafPos[1][0] = x0; bg.m_aafPos[1][1] = y0 + bgH;         // BL
    bg.m_aafPos[2][0] = x0 + quadW; bg.m_aafPos[2][1] = y0 + bgH; // BR
    bg.m_aafPos[3][0] = x0 + quadW; bg.m_aafPos[3][1] = y0;       // TR
    bg.m_iSprite = 0;
    bg.m_ulColor = cfg.backgroundColor;

    *outNumQuads = 1;
    *outQuads = g_quadsBuf.data();

    // Helper to append a string to be drawn
    auto pushString = [&](size_t row, const char* text, uint32_t color) {
            auto& fs = g_strsBuf.emplace_back();
            strncpy_s(fs.m_szString, sizeof(fs.m_szString), text, _TRUNCATE);
            fs.m_afPos[0] = textX;
            fs.m_afPos[1] = y0 + static_cast<float>(row) * lineH;
            fs.m_iFont = 1;
            fs.m_fSize = cfg.fontSize;
            fs.m_iJustify = 0;
            fs.m_ulColor = color;
        };

    // Build the string buffer
    g_strsBuf.clear();
    g_strsBuf.reserve(display.size() + 1); // +1 if a line is split

    for (size_t row = 0; row < display.size(); ++row) {
        const std::string& line = display[row];

        // Special-case: split "Setup Name: Default"
        if (line == "Setup Name: Default") {
            // How long since we went on-track?
            uint64_t startMs = plugin.lastRunInitMs_.load(std::memory_order_relaxed);
            uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            bool highlight = (startMs != 0) && (nowMs - startMs < SETUP_DEFAULT_HIGHLIGHT_MS);

            // Hacky way of overlapping the two pieces of text
            pushString(row, "Setup Name: ", cfg.fontColor);
            pushString(row, "            Default", highlight ? 0xFF0000FF : cfg.fontColor); /// red or white
            continue;
        }

        // Normal path, with optional banner colour on the first row
        uint32_t colour = cfg.fontColor;
        if (row == 0 && line.rfind("mxbmrp2", 0) == 0)
            colour = 0xFF0081CC; // orange

        pushString(row, line.c_str(), colour);
    }

    // Hand buffers back to the caller
    *outNumStrings = static_cast<int>(g_strsBuf.size());
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

// RaceClassification
__declspec(dllexport) void RaceClassification(void* _pData, int   _iDataSize, void* _pArray, int   _iElemSize) {
    SPluginsRaceClassification_t* psRaceClassification =
        (SPluginsRaceClassification_t*)_pData;

    // Dont flood the plugin
    static int lastSecond = -1;
    int curSecond = psRaceClassification->m_iSessionTime / 1000;
    if (curSecond != lastSecond) {
        lastSecond = curSecond;
        Plugin::getInstance().onRaceClassification(*psRaceClassification);
    }
}

// RunLap
__declspec(dllexport) void RunLap(void* _pData, int _iDataSize) {
    SPluginsBikeLap_t* psLapData = (SPluginsBikeLap_t*)_pData;
    Plugin::getInstance().onRunLap(*psLapData);
}

// RunSplit: Called when a split is crossed
__declspec(dllexport) void RunSplit(void* _pData, int _iDataSize) {
    SPluginsBikeSplit_t* psSplitData = (SPluginsBikeSplit_t*)_pData;
    Plugin::getInstance().onRunSplit(*psSplitData);
}

// RaceCommunication: Called when a penalty or state change occurs
__declspec(dllexport) void RaceCommunication(void* _pData, int _iDataSize) {
    SPluginsRaceCommunication_t* psRaceCommunication = (SPluginsRaceCommunication_t*)_pData;

    if (psRaceCommunication->m_iCommunication == 2 && psRaceCommunication->m_iOffence == 2 && psRaceCommunication->m_iTime > 0) {
        Plugin::getInstance().onRaceCommunication(*psRaceCommunication);
    }
}

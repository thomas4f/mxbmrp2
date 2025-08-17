
// MXB_interface.h

#pragma once

extern "C" {

    // Structures related to specific functions

    // EventInit
    typedef struct {
        char m_szRiderName[100];
        char m_szBikeID[100];
        char m_szBikeName[100];
        int m_iNumberOfGears;
        int m_iMaxRPM;
        int m_iLimiter;
        int m_iShiftRPM;
        float m_fEngineOptTemperature;
        float m_afEngineTemperatureAlarm[2];
        float m_fMaxFuel;
        float m_afSuspMaxTravel[2];
        float m_fSteerLock;
        char m_szCategory[100];
        char m_szTrackID[100];
        char m_szTrackName[100];
        float m_fTrackLength;
        int m_iType;
    } SPluginsBikeEvent_t;

    // RunInit
    typedef struct {
        int m_iSession;
        int m_iConditions;
        float m_fAirTemperature;
        char m_szSetupFileName[100];
    } SPluginsBikeSession_t;

    // DrawInit
    typedef struct {
        float m_aafPos[4][2];
        int m_iSprite;
        unsigned long m_ulColor;
    } SPluginQuad_t;

    // Draw
    typedef struct {
        char m_szString[100];
        float m_afPos[2];
        int m_iFont;
        float m_fSize;
        int m_iJustify;
        unsigned long m_ulColor;
    } SPluginString_t;

    // RaceSession
    typedef struct {
        int m_iSession;
        int m_iSessionState;
        int m_iSessionLength;
        int m_iSessionNumLaps;
        int m_iConditions;
        float m_fAirTemperature;
    } SPluginsRaceSession_t;

    // RaceSessionState
    typedef struct {
        int m_iSession;
        int m_iSessionState;
        int m_iSessionLength;
    } SPluginsRaceSessionState_t;

    typedef struct {
        int   m_iType;
        char  m_szName[100];
        char  m_szTrackName[100];
        float m_fTrackLength;
    } SPluginsRaceEvent_t;

    // RaceAddEntry
    typedef struct {
        int m_iRaceNum;
        char m_szName[100];
        char m_szBikeName[100];
        char m_szBikeShortName[100];
        char m_szCategory[100];
        int m_iUnactive;
        int m_iNumberOfGears;
        int m_iMaxRPM;
    } SPluginsRaceAddEntry_t;

    // RaceRemoveEntry
    typedef struct {
        int m_iRaceNum;
    } SPluginsRaceRemoveEntry_t;

    // RaceClassification
    typedef struct {
        int m_iSession;
        int m_iSessionState;
        int m_iSessionTime;
        int m_iNumEntries;
    } SPluginsRaceClassification_t;

    typedef struct {
        int m_iLapNum;
        int m_iInvalid;
        int m_iLapTime;
        int m_iBest;
    } SPluginsBikeLap_t;

    typedef struct {
        int m_iSplit;
        int m_iSplitTime;
        int m_iBestDiff;
    } SPluginsBikeSplit_t;

    // Functions exported to the game
    __declspec(dllexport) const char* GetModID();
    __declspec(dllexport) int GetModDataVersion();
    __declspec(dllexport) int GetInterfaceVersion();
    __declspec(dllexport) int Startup(char* _szSavePath);
    __declspec(dllexport) void Shutdown();
    __declspec(dllexport) void EventInit(void* _pData, int _iDataSize);
    __declspec(dllexport) void EventDeinit();
    __declspec(dllexport) int DrawInit(int* _piNumSprites, char** _pszSpriteName, int* _piNumFonts, char** _pszFontName);
    __declspec(dllexport) void Draw(int _iState, int* _piNumQuads, void** _ppQuad, int* _piNumString, void** _ppString);
    __declspec(dllexport) void RunInit(void* _pData, int _iDataSize);
    __declspec(dllexport) void RunDeinit();
    __declspec(dllexport) void RaceSession(void* _pData, int _iDataSize);
    __declspec(dllexport) void RaceSessionState(void* _pData, int _iDataSize);
    __declspec(dllexport) void RaceEvent(void* _pData, int _iDataSize);
    __declspec(dllexport) void RaceAddEntry(void* _pData, int _iDataSize);
    __declspec(dllexport) void RaceRemoveEntry(void* _pData, int _iDataSize);
    __declspec(dllexport) void RunStart();
    __declspec(dllexport) void RunStop();
    __declspec(dllexport) void RaceClassification(void* _pData, int _iDataSize, void* _pArray, int   _iElemSize);
   __declspec(dllexport) void RunLap(void* _pData, int _iDataSize);
   __declspec(dllexport) void RunSplit(void* _pData, int _iDataSize);
}

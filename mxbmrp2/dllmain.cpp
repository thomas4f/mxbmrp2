
// dllmain.cpp

#include "pch.h"
#include "Plugin.h"

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        Plugin::getInstance().initialize();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        Plugin::getInstance().shutdown();
        break;
    }
    return TRUE;
}

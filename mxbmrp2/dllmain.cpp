
// dllmain.cpp

#include "pch.h"

#include "Plugin.h"

#ifdef _DEBUG
static void InitDebugConsole()
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    // Sync C++ streams with C stdio
    std::ios::sync_with_stdio();

    // Disable buffering so logs appear instantly
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
        InitDebugConsole();
#endif
        DisableThreadLibraryCalls(hModule);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

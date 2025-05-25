
// KeyPressHandler.cpp

#include "pch.h"

#include "KeyPressHandler.h"
#include "Logger.h"

KeyPressHandler::KeyPressHandler(HotkeyCallback cb, UINT hotkey)
    : callback_(std::move(cb)), hotkey_(hotkey)
{
    // Launch the thread
    thread_ = std::thread(&KeyPressHandler::messageLoop, this);

    // Wait until messageLoop() signals that threadId_ is set
    {
        std::unique_lock<std::mutex> lk(initMutex_);
        initCv_.wait(lk, [this]() { return initialized_; });
    }
}

KeyPressHandler::~KeyPressHandler()
{
    running_ = false;
    if (DWORD id = threadId_.load())
        PostThreadMessage(id, WM_QUIT, 0, 0);

    if (thread_.joinable())
        thread_.join();
}

void KeyPressHandler::messageLoop() {
    // 1) Record our thread ID
    threadId_ = GetCurrentThreadId();

    // 2) Notify the constructor that we're up and running
    {
        std::lock_guard<std::mutex> lk(initMutex_);
        initialized_ = true;
    }
    initCv_.notify_one();

    // 3) Register the hotkey
    const UINT hotkeyId = 0xBEEF;
    if (!RegisterHotKey(nullptr, hotkeyId, MOD_CONTROL, hotkey_)) {
        Logger::getInstance().log(
            std::string("Failed to register hotkey: CTRL + ")
            + static_cast<char>(hotkey_)
        );
    }
    else {
        Logger::getInstance().log(
            std::string("KeyPressHandler thread started with hotkey: CTRL + ")
            + static_cast<char>(hotkey_)
        );
    }

    // 4) Message loop
    MSG msg{};
    while (running_) {
        MsgWaitForMultipleObjectsEx(
            0, nullptr, INFINITE,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE
        );

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            if (msg.message == WM_HOTKEY &&
                msg.wParam == hotkeyId &&
                callback_)
            {
                HWND fg = GetForegroundWindow();
                DWORD fgPid = 0;
                GetWindowThreadProcessId(fg, &fgPid);
                if (fgPid == GetCurrentProcessId()) {
                    callback_();
                }
            }
        }
    }

    UnregisterHotKey(nullptr, hotkeyId);
    Logger::getInstance().log("KeyPressHandler thread stopped");
}

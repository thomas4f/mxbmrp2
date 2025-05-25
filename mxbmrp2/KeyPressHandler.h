
// KeyPressHandler.h

#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

class KeyPressHandler {
public:
    using HotkeyCallback = std::function<void()>;

    KeyPressHandler(HotkeyCallback callback, UINT hotkey);
    ~KeyPressHandler();

private:
    void messageLoop();

    std::thread        thread_;
    std::atomic<bool>  running_{ true };
    std::atomic<DWORD> threadId_{ 0 };

    HotkeyCallback    callback_;
    UINT              hotkey_;

    // Synchronization for thread startup
    std::mutex              initMutex_;
    std::condition_variable initCv_;
    bool                    initialized_{ false };
};
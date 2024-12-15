
// KeyPressHandler.h

#pragma once

#include <windows.h>
#include <thread>
#include <atomic>
#include <functional>

class KeyPressHandler {
public:
    using HotkeyCallback = std::function<void()>;

    KeyPressHandler(HotkeyCallback callback, UINT hotkey);
    ~KeyPressHandler();

private:
    void messageLoop();
    std::thread thread_;
    std::atomic<bool> running_;
    HotkeyCallback callback_;
    UINT hotkey_;
};

// KeyPressHandler.cpp

#include "pch.h"
#include "KeyPressHandler.h"
#include "Logger.h"

KeyPressHandler::KeyPressHandler(HotkeyCallback callback, UINT hotkey)
    : running_(true), callback_(callback), hotkey_(hotkey) {
    // Start the message loop in a new thread
    thread_ = std::thread(&KeyPressHandler::messageLoop, this);
}

KeyPressHandler::~KeyPressHandler() {
    running_ = false;

    // Post a quit message to exit the message loop
    PostThreadMessage(GetThreadId(thread_.native_handle()), WM_QUIT, 0, 0);

    if (thread_.joinable()) {
        thread_.join();
    }
}

void KeyPressHandler::messageLoop() {
    // Register Ctrl + R as a hotkey with ID 1
    if (!RegisterHotKey(NULL, 1, MOD_CONTROL, hotkey_)) {
        Logger::getInstance().log("Failed to register hotkey");
        return;
    }
    else {
        Logger::getInstance().log("Hotkey registered successfully");
    }

    MSG msg;
    while (running_) {
        // Wait for messages (hotkey events)
        if (GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_HOTKEY) {
                if (msg.wParam == 1) { // Hotkey ID
                    Logger::getInstance().log("Keypress handler triggered");

                    // Get the process ID of the foreground window
                    DWORD foregroundProcessId = 0;
                    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcessId);

                    // Get the current process ID
                    DWORD currentProcessId = GetCurrentProcessId();

                    // Check if the foreground window belongs to the current process
                    if (foregroundProcessId == currentProcessId) {
                        if (callback_) {
                            callback_(); // Invoke the callback
                        }
                    }
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Unregister the hotkey upon exiting the loop
    UnregisterHotKey(NULL, 1);
    Logger::getInstance().log("Hotkey unregistered");
}

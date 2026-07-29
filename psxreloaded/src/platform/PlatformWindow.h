#pragma once

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gl_loader.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#endif

// Class to handle opening, closing, getting input, and other operations regarding windowing from win32
class PlatformWindow {
public:
    PlatformWindow() {}; // Global constructor
    PlatformWindow(const char* title, int width, int height); // Initialize the window
    ~PlatformWindow(); // Destroy the window

    void Show(); // Shows the window
    void PollEvents(); // Polls for events like input with the keyboard and mouse
    bool ShouldClose() const; // Checks if the window is supposed to close
    void NotifyClose();

#ifdef _WIN32
    HWND GetHandle() const { return m_hwnd; } // Gets the raw HWND handle
    HDC GetDeviceContext() const { return m_hdc; }

    typedef void (*EventCallbackFn)(UINT msg, WPARAM wParam, LPARAM lParam);
    void SetEventCallback(EventCallbackFn callback) { m_eventCallback = callback; } // Sets the window's procedure
#elif __EMSCRIPTEN__
    void SetMainLoop(void(*main_loop_func)(void), int fps, bool simulate_infinite_loop);
    static EM_BOOL MouseMoveCallback(int eventType, const EmscriptenMouseEvent* e, void* userData);
    static EM_BOOL MouseButtonCallback(int eventType, const EmscriptenMouseEvent* e, void* userData);
    static EM_BOOL MouseWheelCallback(int eventType, const EmscriptenWheelEvent* e, void* userData);
    static EM_BOOL ResizeCallback(int eventType, const EmscriptenUiEvent* e, void* userData);
    static EM_BOOL FullscreenResizeCallback(int eventType, const EmscriptenFullscreenChangeEvent* fullscreenChangeEvent __attribute__((nonnull)), void* userData);
        static EM_BOOL KeyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData);
#endif // _WIN32

    bool SetupGLContext(); // Sets up OpenGL with a core 4.6 context (WINDOWS) Sets up WebGL2 (EMSCRIPTEN)
    void SwapDC() const;

    bool IsKeyDown(int vk_code) const;
    void GetMousePosition(int& x, int& y) const;
    bool IsMouseButtonDown(int button) const;
    float GetMouseWheelDelta() const;
    void ResetMouseWheelDelta();
    const bool* GetKeyboardState() const { return &m_keys[0]; }

    using KeyCallbackFunc = void(*)(int vk_code, bool pressed);
    using MouseMoveCallbackFunc = void(*)(int x, int y);
    using MouseButtonCallbackFunc = void(*)(int button, bool pressed);
    using WindowResizeCallbackFunc = void(*)(int width, int height);

    double GetTime() const;
    double GetTimeMS() const;

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    int GetMonitorRefreshRate() const;

    void SetKeyCallback(KeyCallbackFunc cb) { m_keyCallback = cb; }
    void SetMouseMoveCallback(MouseMoveCallbackFunc cb) { m_mouseMoveCallback = cb; }
    void SetMouseButtonCallback(MouseButtonCallbackFunc cb) { m_mouseButtonCallback = cb; }
    void SetWindowResizeAndRenderDuringResizeCallback(WindowResizeCallbackFunc cb) { m_windowResizeCallback = cb; }
private:
#ifdef _WIN32
    HWND m_hwnd;
    HDC  m_hdc;
    HGLRC m_glrc;
    HINSTANCE m_hInstance;
    const char* m_className;
    EventCallbackFn m_eventCallback;
#elif __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_glContext;
#endif // _WIN32
    bool m_shouldClose;

    bool m_keys[256] = { false };
    bool m_mouseButtons[3] = { false };
    bool m_in_sizemove = false;
    int m_mouseX = 0;
    int m_mouseY = 0;
    float m_mouseDelta;
    int m_width = 0, m_height = 0;

    KeyCallbackFunc m_keyCallback = nullptr;
    MouseMoveCallbackFunc m_mouseMoveCallback = nullptr;
    MouseButtonCallbackFunc m_mouseButtonCallback = nullptr;
    WindowResizeCallbackFunc m_windowResizeCallback = nullptr;

    void RegisterWindowClass();
    void CreateAppWindow(const char* title, int width, int height);

#ifdef _WIN32
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _WIN32
};

namespace PlatformInput {
    // Keyboard keys (common and extended)
    enum Key {
        None = 0,
        Backspace = 0x08,
        Tab = 0x09,
        Enter = 0x0D,
        Shift = 0x10,
        Control = 0x11,
        Alt = 0x12,
        Pause = 0x13,
        CapsLock = 0x14,
        Escape = 0x1B,
        Space = 0x20,
        PageUp = 0x21,
        PageDown = 0x22,
        End = 0x23,
        Home = 0x24,
        LeftArrow = 0x25,
        UpArrow = 0x26,
        RightArrow = 0x27,
        DownArrow = 0x28,
        PrintScreen = 0x2C,
        Insert = 0x2D,
        Delete = 0x2E,

        // Numbers (top row)
        Num0 = 0x30,
        Num1 = 0x31,
        Num2 = 0x32,
        Num3 = 0x33,
        Num4 = 0x34,
        Num5 = 0x35,
        Num6 = 0x36,
        Num7 = 0x37,
        Num8 = 0x38,
        Num9 = 0x39,

        // Letters
        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4A,
        K = 0x4B,
        L = 0x4C,
        M = 0x4D,
        N = 0x4E,
        O = 0x4F,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5A,

#ifndef __EMSCRIPTEN__
        LeftWin = 0x5B,
        RightWin = 0x5C,
        Apps = 0x5D,
        Sleep = 0x5F,

        Numpad0 = 0x60,
        Numpad1 = 0x61,
        Numpad2 = 0x62,
        Numpad3 = 0x63,
        Numpad4 = 0x64,
        Numpad5 = 0x65,
        Numpad6 = 0x66,
        Numpad7 = 0x67,
        Numpad8 = 0x68,
        Numpad9 = 0x69,
        Multiply = 0x6A,
        Add = 0x6B,
        Separator = 0x6C,
        Subtract = 0x6D,
        Decimal = 0x6E,
        Divide = 0x6F,
#endif // __EMSCRIPTEN__

        F1 = 0x70,
        F2 = 0x71,
        F3 = 0x72,
        F4 = 0x73,
        F5 = 0x74,
        F6 = 0x75,
        F7 = 0x76,
        F8 = 0x77,
        F9 = 0x78,
        F10 = 0x79,
        F11 = 0x7A,
        F12 = 0x7B,
#ifndef __EMSCRIPTEN__
        F13 = 0x7C,
        F14 = 0x7D,
        F15 = 0x7E,
        F16 = 0x7F,
        F17 = 0x80,
        F18 = 0x81,
        F19 = 0x82,
        F20 = 0x83,
        F21 = 0x84,
        F22 = 0x85,
        F23 = 0x86,
        F24 = 0x87,

        NumLock = 0x90,
        ScrollLock = 0x91,
#endif // __EMSCRIPTEN__

        LeftShift = 0xA0,
        RightShift = 0xA1,
        LeftControl = 0xA2,
        RightControl = 0xA3,
        LeftAlt = 0xA4,
        RightAlt = 0xA5,

        // Add more keys if needed
    };

    // Mouse buttons
    enum MouseButton {
        MouseLeft = 0,
        MouseRight = 1,
        MouseMiddle = 2,
        MouseXButton1 = 3,
        MouseXButton2 = 4,
    };
}
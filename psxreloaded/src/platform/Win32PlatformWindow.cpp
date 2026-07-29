#include "PlatformWindow.h"

#ifdef _WIN32
#include <timeapi.h>
#include <windowsx.h>
#include <gl/GL.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "Winmm.lib")

// WGL pixel format attributes
#define WGL_DRAW_TO_WINDOW_ARB        0x2001
#define WGL_SUPPORT_OPENGL_ARB        0x2010
#define WGL_DOUBLE_BUFFER_ARB         0x2011
#define WGL_PIXEL_TYPE_ARB            0x2013
#define WGL_COLOR_BITS_ARB            0x2014
#define WGL_DEPTH_BITS_ARB            0x2022
#define WGL_STENCIL_BITS_ARB          0x2023
#define WGL_TYPE_RGBA_ARB             0x202B

// WGL context creation attributes
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB  0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

static PlatformWindow* g_windowInstance = nullptr;
static LARGE_INTEGER qpc_frequency;

PlatformWindow::PlatformWindow(const char* title, int width, int height)
    : m_hwnd(nullptr),
    m_hdc(nullptr),
    m_glrc(nullptr),
    m_hInstance(GetModuleHandleA(0)),
    m_className("PlatformWindowClass"),
    m_shouldClose(false),
    m_eventCallback(nullptr),
    m_width(width),
    m_height(height)
{
    g_windowInstance = this;
    RegisterWindowClass();
    CreateAppWindow(title, width, height);

    timeBeginPeriod(1);
    QueryPerformanceFrequency(&qpc_frequency);
}

PlatformWindow::~PlatformWindow() {
    if (m_glrc) {
        wglMakeCurrent(0, 0);
        wglDeleteContext(m_glrc);
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
    UnregisterClassA(m_className, m_hInstance);
    timeEndPeriod(1);
}

void PlatformWindow::RegisterWindowClass() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = m_className;
    RegisterClassA(&wc);
}

void PlatformWindow::CreateAppWindow(const char* title, int width, int height) {
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowExA(
        0,
        m_className,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0,
        0,
        m_hInstance,
        0
    );

    m_hdc = GetDC(m_hwnd);

    SetWindowLongPtrA(m_hwnd, GWLP_USERDATA, (long long)this);
}

void PlatformWindow::Show() {
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

void PlatformWindow::PollEvents() {
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_shouldClose = true;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

bool PlatformWindow::ShouldClose() const {
    return m_shouldClose;
}

void PlatformWindow::NotifyClose() {
    m_shouldClose = true;
}

double PlatformWindow::GetTime() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return ((double)(now.QuadPart)) / (double)(qpc_frequency.QuadPart);
}

double PlatformWindow::GetTimeMS() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return ((double)(now.QuadPart) * 1000.0) / (double)(qpc_frequency.QuadPart);
}

int PlatformWindow::GetMonitorRefreshRate() const
{
    HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor)
        return 0;

    MONITORINFOEX monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(monitorInfo);

    if (!GetMonitorInfo(hMonitor, (MONITORINFO*)&monitorInfo))
        return 0;

    DEVMODE dm = { 0 };
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm))
        return 0;

    return dm.dmDisplayFrequency;
}

#ifdef _DEBUG

#include <cstdio>

static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        printf("Debug message %u | %s\n", id, message);
    }

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION && severity != GL_DEBUG_SEVERITY_LOW)
    {
        __debugbreak();
    }
}

#endif // _DEBUG

bool PlatformWindow::SetupGLContext() {
    // 1. Dummy window for extension loading
    WNDCLASSA dummyClass = { 0 };
    dummyClass.style = CS_OWNDC;
    dummyClass.lpfnWndProc = DefWindowProcA;
    dummyClass.hInstance = m_hInstance;
    dummyClass.lpszClassName = "DummyGL";
    RegisterClassA(&dummyClass);

    HWND dummyWnd = CreateWindowA("DummyGL", "", WS_OVERLAPPEDWINDOW,
        0, 0, 1, 1, 0, 0, m_hInstance, 0);
    HDC dummyDC = GetDC(dummyWnd);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pf = ChoosePixelFormat(dummyDC, &pfd);
    SetPixelFormat(dummyDC, pf, &pfd);
    HGLRC dummyRC = wglCreateContext(dummyDC);
    wglMakeCurrent(dummyDC, dummyRC);

    typedef BOOL(WINAPI* wglSwapIntervalEXTProc)(int);
    wglSwapIntervalEXTProc wglSwapIntervalEXT =
        (wglSwapIntervalEXTProc)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1); // 0 = VSync off, 1 = VSync on
    }

    // 2. Load wgl extensions
    typedef HGLRC(WINAPI* wglCreateContextAttribsARBProc)(HDC, HGLRC, const int*);
    typedef BOOL(WINAPI* wglChoosePixelFormatARBProc)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);

    wglCreateContextAttribsARBProc wglCreateContextAttribsARB =
        (wglCreateContextAttribsARBProc)wglGetProcAddress("wglCreateContextAttribsARB");

    if (!wglCreateContextAttribsARB) {
        MessageBoxA(0, "wglCreateContextAttribsARB not available", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 3. Set pixel format for real window
    int attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_SUPPORT_OPENGL_ARB, 1,
        WGL_DOUBLE_BUFFER_ARB, 1,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };

    int pixelFormat;
    UINT numFormats;
    wglChoosePixelFormatARBProc wglChoosePixelFormatARB =
        (wglChoosePixelFormatARBProc)wglGetProcAddress("wglChoosePixelFormatARB");

    if (!wglChoosePixelFormatARB ||
        !wglChoosePixelFormatARB(m_hdc, attribs, 0, 1, &pixelFormat, &numFormats)) {
        MessageBoxA(0, "wglChoosePixelFormatARB failed", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    DescribePixelFormat(m_hdc, pixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(m_hdc, pixelFormat, &pfd);

    // 4. Create real OpenGL 4.5 core context
    int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    m_glrc = wglCreateContextAttribsARB(m_hdc, 0, contextAttribs);
    if (!m_glrc) {
        MessageBoxA(0, "Failed to create OpenGL 4.5 context", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    wglMakeCurrent(m_hdc, m_glrc);

    // 5. Clean up dummy context
    wglMakeCurrent(0, 0);
    wglDeleteContext(dummyRC);
    ReleaseDC(dummyWnd, dummyDC);
    DestroyWindow(dummyWnd);
    UnregisterClassA("DummyGL", m_hInstance);

    wglMakeCurrent(m_hdc, m_glrc);
    if (!gl_loaderLoadGL()) {
        MessageBoxA(0, "Failed to initialize GLAD", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE); // Enable all messages
#endif

    glViewport(0, 0, m_width, m_height);

    return true;
}

void PlatformWindow::SwapDC() const
{
    SwapBuffers(GetDeviceContext());
}

LRESULT CALLBACK PlatformWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PlatformWindow* window = (PlatformWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (window) {
        return window->WndProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PlatformWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (m_eventCallback) {
        m_eventCallback(msg, wParam, lParam);
    }

    switch (msg) {
    case WM_CLOSE:
        m_shouldClose = true;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        int vk_code = (int)wParam;
        if (vk_code >= 0 && vk_code < 256) {
            m_keys[vk_code] = true;
            if (m_keyCallback) m_keyCallback(vk_code, true);
        }
        return 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        int vk_code = (int)wParam;
        if (vk_code >= 0 && vk_code < 256) {
            m_keys[vk_code] = false;
            if (m_keyCallback) m_keyCallback(vk_code, false);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        m_mouseX = GET_X_LPARAM(lParam);
        m_mouseY = GET_Y_LPARAM(lParam);
        if (m_mouseMoveCallback) m_mouseMoveCallback(m_mouseX, m_mouseY);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        m_mouseDelta += (float)delta / (float)WHEEL_DELTA;
        return 0;
    }
    case WM_LBUTTONDOWN:
        m_mouseButtons[0] = true;
        if (m_mouseButtonCallback) m_mouseButtonCallback(0, true);
        return 0;
    case WM_LBUTTONUP:
        m_mouseButtons[0] = false;
        if (m_mouseButtonCallback) m_mouseButtonCallback(0, false);
        return 0;
    case WM_RBUTTONDOWN:
        m_mouseButtons[1] = true;
        if (m_mouseButtonCallback) m_mouseButtonCallback(1, true);
        return 0;
    case WM_RBUTTONUP:
        m_mouseButtons[1] = false;
        if (m_mouseButtonCallback) m_mouseButtonCallback(1, false);
        return 0;
    case WM_MBUTTONDOWN:
        m_mouseButtons[2] = true;
        if (m_mouseButtonCallback) m_mouseButtonCallback(2, true);
        return 0;
    case WM_MBUTTONUP:
        m_mouseButtons[2] = false;
        if (m_mouseButtonCallback) m_mouseButtonCallback(2, false);
        return 0;
    case WM_ENTERSIZEMOVE:
        m_in_sizemove = true;
        return 0;
    case WM_EXITSIZEMOVE:
        m_in_sizemove = false;
        return 0;
    case WM_SIZE:
    {
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);

        if (m_width == 0 || m_height == 0)
            return 0;

        if (glViewport) { glViewport(0, 0, m_width, m_height); } // OpenGL may not be initialized yet

        if (m_windowResizeCallback) { m_windowResizeCallback(m_width, m_height); }

        SwapBuffers(m_hdc); // Keep rendering

        return 0;
    }
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

bool PlatformWindow::IsKeyDown(int vk_code) const {
    if (vk_code < 0 || vk_code >= 256) return false;
    return m_keys[vk_code];
}

void PlatformWindow::GetMousePosition(int& x, int& y) const {
    x = m_mouseX;
    y = m_mouseY;
}

bool PlatformWindow::IsMouseButtonDown(int button) const {
    if (button < 0 || button >= 3) return false;
    return m_mouseButtons[button];
}

float PlatformWindow::GetMouseWheelDelta() const {
    return m_mouseDelta;
}

void PlatformWindow::ResetMouseWheelDelta()
{
    m_mouseDelta = 0.0f;
}
#endif // _WIN32
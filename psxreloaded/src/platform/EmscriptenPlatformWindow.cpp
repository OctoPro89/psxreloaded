#include "PlatformWindow.h"

// TODO: Fullscreening broken, mouse wheel broken

#ifdef __EMSCRIPTEN__

#include <GLES3/gl3.h>
#include <emscripten/html5.h>

// This prevents right click from opening the browser menu
EM_JS(void, disable_context_menu, (), {
    document.getElementById('canvas').addEventListener('contextmenu', function(e) {
        e.preventDefault();
    });
});

EM_JS(void, prevent_browser_keys, (), {
    document.addEventListener('keydown', function(e) {
        if (
            e.code.startsWith("Arrow") ||
            e.code === "Space" ||
            e.code.startsWith("F") ||
            e.code.startsWith("Control")
        ) {
            e.preventDefault();
        }
    });

    document.addEventListener('keyup', function(e) {
        if (
            e.code.startsWith("Arrow") ||
            e.code === "Space" ||
            e.code.startsWith("F") ||
            e.code.startsWith("Control")
        ) {
            e.preventDefault();
        }
    });
});

EM_JS(void, __applicationRequestFullscreen__, (), {
    const canvas = document.getElementById("canvas");
    if (canvas.requestFullscreen)
        canvas.requestFullscreen();
    else if (canvas.webkitRequestFullscreen)
        canvas.webkitRequestFullscreen();
    else if (canvas.mozRequestFullScreen)
        canvas.mozRequestFullScreen();
});

void applicationRequestFullscreen() { __applicationRequestFullscreen__(); }

EM_BOOL PlatformWindow::MouseMoveCallback(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
    PlatformWindow* window = static_cast<PlatformWindow*>(userData);

    window->m_mouseX = e->targetX;
    window->m_mouseY = e->targetY;

    if (window->m_mouseMoveCallback)
        window->m_mouseMoveCallback(window->m_mouseX, window->m_mouseY);

    return EM_TRUE;
}

EM_BOOL PlatformWindow::MouseButtonCallback(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
    PlatformWindow* window = static_cast<PlatformWindow*>(userData);

    int button = -1;

    switch (e->button)
    {
    case 0:
        button = 0;
        break;
    case 2:
        button = 1;
        break;
    case 1:
        button = 2;
        break;
    }

    if (button >= 0 && button < 3)
    {
        bool pressed = eventType == EMSCRIPTEN_EVENT_MOUSEDOWN;

        window->m_mouseButtons[button] = pressed;

        if (window->m_mouseButtonCallback)
            window->m_mouseButtonCallback(button, pressed);
    }

    return EM_TRUE;
}

// designed to be like WM_MOUSEWHEEL in the Win32 API
EM_BOOL PlatformWindow::MouseWheelCallback( int, const EmscriptenWheelEvent* e, void* userData)
{
    auto* window = static_cast<PlatformWindow*>(userData);

    double delta = e->deltaY;

    switch (e->deltaMode)
    {
    case DOM_DELTA_PIXEL:
        delta /= 100.0;      // roughly one wheel notch
        break;

    case DOM_DELTA_LINE:
        break;

    case DOM_DELTA_PAGE:
        delta *= 3.0;
        break;
    }

    // Browser positive is "scroll down".
    window->m_mouseDelta -= (float)delta;

    return EM_TRUE;
}

#include <cstdio>

EM_BOOL PlatformWindow::FullscreenResizeCallback(int eventType, const EmscriptenFullscreenChangeEvent* e, void* userData)
{
    PlatformWindow* window = static_cast<PlatformWindow*>(userData);

    int width;
    int height;

    emscripten_get_canvas_element_size(
        "#canvas",
        &width,
        &height
    );

    window->m_width = width;
    window->m_height = height;

    emscripten_webgl_make_context_current(window->m_glContext);

    glViewport(0, 0, width, height);

    if (window->m_windowResizeCallback)
        window->m_windowResizeCallback(width, height);

    return EM_TRUE;
}

EM_BOOL PlatformWindow::ResizeCallback(int eventType, const EmscriptenUiEvent* e, void* userData)
{
    PlatformWindow* window = static_cast<PlatformWindow*>(userData);

    double cssWidth;
    double cssHeight;

    emscripten_get_element_css_size(
        "#canvas",
        &cssWidth,
        &cssHeight
    );

    int width = (int)cssWidth;
    int height = (int)cssHeight;

    if (width <= 0 || height <= 0)
        return EM_TRUE;

    emscripten_set_canvas_element_size(
        "#canvas",
        width,
        height
    );

    window->m_width = width;
    window->m_height = height;

    emscripten_webgl_make_context_current(window->m_glContext);

    glViewport(0, 0, width, height);

    if (window->m_windowResizeCallback)
        window->m_windowResizeCallback(width, height);

    return EM_TRUE;
}

EM_BOOL PlatformWindow::KeyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData)
{
    PlatformWindow* window = static_cast<PlatformWindow*>(userData);

    int key = 0;

    // TODO: possibly translate
    if (e->keyCode > 0 && e->keyCode < 256)
        key = e->keyCode;

    if (key)
    {
        bool pressed = eventType == EMSCRIPTEN_EVENT_KEYDOWN;

        window->m_keys[key] = pressed;

        if (window->m_keyCallback)
            window->m_keyCallback(key, pressed);
    }

    return EM_TRUE;
}

PlatformWindow::PlatformWindow(const char* title, int width, int height)
    : m_width(width), m_height(height), m_shouldClose(false)
{
    emscripten_set_window_title(title);
    emscripten_set_canvas_element_size("#canvas", width, height);

    // Input, NOTE: Get mouse input from CANVAS not WINDOW since otherwise it'll be wrong
    emscripten_set_mousemove_callback("#canvas", this, false, MouseMoveCallback);
    emscripten_set_mousedown_callback("#canvas", this, false, MouseButtonCallback);
    emscripten_set_mouseup_callback("#canvas", this, false, MouseButtonCallback);
    emscripten_set_wheel_callback("#canvas", this, false, MouseWheelCallback);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true, KeyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true, KeyCallback);

    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        this,
        true,
        ResizeCallback
    );

    emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
        this,
        true,
        FullscreenResizeCallback
    );

    disable_context_menu();
    prevent_browser_keys();
}

PlatformWindow::~PlatformWindow() {}

void PlatformWindow::Show() {}
void PlatformWindow::PollEvents() {}

bool PlatformWindow::ShouldClose() const { return m_shouldClose; }
void PlatformWindow::NotifyClose() { m_shouldClose = true; }

void PlatformWindow::SetMainLoop(void(*main_loop_func)(void), int fps, bool simulate_infinite_loop)
{
    emscripten_set_main_loop(main_loop_func, fps, simulate_infinite_loop);
}

bool PlatformWindow::SetupGLContext()
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);

    attrs.alpha = false;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = true;

    attrs.majorVersion = 2;    // WebGL2

    m_glContext = emscripten_webgl_create_context("#canvas", &attrs);

    if (m_glContext <= 0)
        return false;

    emscripten_webgl_make_context_current(m_glContext);

    return true;
}

void PlatformWindow::SwapDC() const {}

bool PlatformWindow::IsKeyDown(int vk_code) const
{
    if (vk_code < 0 || vk_code >= 256) return false;
    return m_keys[vk_code];
}

void PlatformWindow::GetMousePosition(int& x, int& y) const 
{
    x = m_mouseX;
    y = m_mouseY;
}

bool PlatformWindow::IsMouseButtonDown(int button) const
{
    if (button < 0 || button >= 3) return false;
    return m_mouseButtons[button];
}

float PlatformWindow::GetMouseWheelDelta() const
{
    return m_mouseDelta;
}

void PlatformWindow::ResetMouseWheelDelta()
{
    m_mouseDelta = 0.0f;
}

double PlatformWindow::GetTime() const
{
    return emscripten_get_now() * 0.001;
}

double PlatformWindow::GetTimeMS() const
{
    return emscripten_get_now();
}

// TODO:
int PlatformWindow::GetMonitorRefreshRate() const
{
    return 60;
}

#endif // __EMSCRIPTEN__
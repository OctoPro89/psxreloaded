#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <windows.h>
#include "gl_loader.h"
#include <stdlib.h>
#include <stdio.h>

#include "platform.h"

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "uuid.lib")

DEFINE_GUID(CLSID_MMDeviceEnumerator,
    0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);

DEFINE_GUID(IID_IMMDeviceEnumerator,
    0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);

DEFINE_GUID(IID_IAudioClient,
    0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x3b, 0xd5, 0xc6, 0x86);

DEFINE_GUID(IID_IAudioClient2,
    0x726778CD, 0xF60A, 0x4eda, 0x82, 0xDE,
    0xe4, 0x76, 0x10, 0xcd, 0x78, 0xaa);

DEFINE_GUID(IID_IAudioRenderClient,
    0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;

/*========================
    INTERNAL TYPES
========================*/

struct PlatformWindow {
    HWND hwnd;
    HDC dc;
    HGLRC glrc;
    RECT windowed_rect;
    DWORD windowed_style;
    bool fullscreen;
    PlatformState* state;
};

struct PlatformThread {
    HANDLE handle;
};

struct PlatformMutex {
    CRITICAL_SECTION cs;
};

/*========================
    GLOBALS
========================*/

static bool g_running = true;
static LARGE_INTEGER g_freq;

static PlatformAudioCallback g_audio_callback = NULL;
static void* g_audio_user_data = NULL;

static int g_audio_channels = 2;
static int g_audio_sample_rate = 44100;

static IAudioClient* audio_client = NULL;
static IAudioRenderClient* render_client = NULL;
static HANDLE audio_event = NULL;
static HANDLE audio_thread = NULL;
static bool audio_running = false;

/*========================
    INPUT
========================*/

static void input_begin_frame(PlatformInput* in) {
    for (int i = 0; i < KEY_COUNT; i++) {
        in->keys[i].pressed = false;
        in->keys[i].released = false;
    }

    in->mouse_dx = 0;
    in->mouse_dy = 0;
    in->wheel = 0;
}

static KeyCode win32_translate_key(WPARAM w) {
    if (w >= 'A' && w <= 'Z') return (KeyCode)(KEY_A + (w - 'A'));
    if (w >= '0' && w <= '9') return (KeyCode)(KEY_0 + (w - '0'));

    switch (w) {
        case VK_ESCAPE: return KEY_ESCAPE;
        case VK_RETURN: return KEY_ENTER;
        case VK_TAB: return KEY_TAB;
        case VK_BACK: return KEY_BACKSPACE;
        case VK_LEFT: return KEY_LEFT;
        case VK_RIGHT: return KEY_RIGHT;
        case VK_UP: return KEY_UP;
        case VK_DOWN: return KEY_DOWN;
        case VK_SHIFT: return KEY_LSHIFT;
        case VK_CONTROL: return KEY_LCTRL;
        case VK_MENU: return KEY_LALT;
    }

    return KEY_UNKNOWN;
}

/*========================
    WINDOW PROC
========================*/

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    PlatformWindow* win = (PlatformWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    PlatformInput* input = (win && win->state) ? &win->state->input : NULL;

    switch (msg) {
        case WM_CLOSE: {
            g_running = false;
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (input) {
                KeyCode k = win32_translate_key(wparam);
                if (k != KEY_UNKNOWN) {
                    ButtonState* b = &input->keys[k];
                    if (!b->down) b->pressed = true;
                    b->down = true;
                }
            }
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (input) {
                KeyCode k = win32_translate_key(wparam);
                if (k != KEY_UNKNOWN) {
                    ButtonState* b = &input->keys[k];
                    b->down = false;
                    b->released = true;
                }
            }
            break;
        }
        case WM_INPUT: {
            UINT size;
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

            BYTE stack_buffer[256];
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, stack_buffer, &size, sizeof(RAWINPUTHEADER));

            RAWINPUT* raw = (RAWINPUT*)stack_buffer;

            if (input && raw->header.dwType == RIM_TYPEMOUSE) {
                input->mouse_dx += raw->data.mouse.lLastX;
                input->mouse_dy += raw->data.mouse.lLastY;
            }
            break;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

/*========================
    CORE
========================*/

bool platform_init(void) {
    QueryPerformanceFrequency(&g_freq);
    return true;
}

void platform_shutdown(void) {
    platform_audio_shutdown();
}

/*========================
    WINDOW
========================*/

PlatformWindow* platform_create_window(PlatformWindowDesc* desc, PlatformState* state) {
    HINSTANCE inst = GetModuleHandle(NULL);

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = wndproc;
    wc.hInstance = inst;
    wc.lpszClassName = "plat_win32";
    wc.style = CS_OWNDC;

    RegisterClassA(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc->resizable) style &= ~WS_THICKFRAME;

    HWND hwnd = CreateWindowExA(0, wc.lpszClassName, desc->title, style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, desc->width, desc->height, 0, 0, inst, 0);

    PlatformWindow* w = malloc(sizeof(*w));
    w->hwnd = hwnd;
    w->dc = GetDC(hwnd);
    w->glrc = NULL;
    w->state = state;
    ZeroMemory(&state->input, sizeof(PlatformInput));

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)w);

    RAWINPUTDEVICE rid = { 0 };
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02; // mouse
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    return w;
}

void platform_destroy_window(PlatformWindow* w) {
    if (!w) return;

    if (w->glrc) {
        wglDeleteContext(w->glrc);
    }

    ReleaseDC(w->hwnd, w->dc);
    DestroyWindow(w->hwnd);
    free(w);
}

void platform_set_fullscreen(PlatformWindow* w, bool fs) {
    if (fs == w->fullscreen) return;

    if (fs) {
        w->windowed_style = GetWindowLong(w->hwnd, GWL_STYLE);
        GetWindowRect(w->hwnd, &w->windowed_rect);

        SetWindowLong(w->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(w->hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

        SetWindowPos(w->hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED);
    }
    else {
        SetWindowLong(w->hwnd, GWL_STYLE, w->windowed_style);

        SetWindowPos(w->hwnd, NULL, w->windowed_rect.left, w->windowed_rect.top, w->windowed_rect.right - w->windowed_rect.left, w->windowed_rect.bottom - w->windowed_rect.top, SWP_FRAMECHANGED);
    }

    w->fullscreen = fs;
}

void platform_set_title(PlatformWindow* w, const char* title) {
    SetWindowTextA(w->hwnd, title);
}

void platform_get_size(PlatformWindow* w, int* width, int* height) {
    RECT r;
    GetClientRect(w->hwnd, &r);
    *width = r.right - r.left;
    *height = r.bottom - r.top;
}

void platform_set_window_size(PlatformWindow* w, int width, int height) {
    SetWindowPos(w->hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void platform_set_window_pos(PlatformWindow* w, int x, int y) {
    SetWindowPos(w->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

DWORD WINAPI platform_audio_thread(void* arg);

/*========================
    AUDIO
========================*/
bool platform_audio_init(int sample_rate, int channels, PlatformAudioCallback callback, void* user_data) {
    g_audio_sample_rate = sample_rate;
    g_audio_channels = channels;
    g_audio_callback = callback;
    g_audio_user_data = user_data;

    if (FAILED(CoInitialize(NULL))) return false;

    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;

    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&enumerator))) {
        return false;
    }

    if (FAILED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
        return false;
    }

    if (FAILED(device->lpVtbl->Activate(device, &IID_IAudioClient2, CLSCTX_ALL, NULL, (void**)&audio_client))) {
        return false;
    }

    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nChannels = (WORD)channels;
    wfx.nSamplesPerSec = 48000;
    wfx.wBitsPerSample = 32;
    wfx.nBlockAlign = wfx.nChannels * sizeof(float);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    REFERENCE_TIME buffer_duration = 50000; // ~5ms

    if ((audio_client->lpVtbl->Initialize(audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, buffer_duration, 0, &wfx, NULL))) {
        return false;
    }

    audio_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    audio_client->lpVtbl->SetEventHandle(audio_client, audio_event);

    if (FAILED(audio_client->lpVtbl->GetService(
        audio_client, &IID_IAudioRenderClient, (void**)&render_client)))
        return false;

    audio_running = true;
    audio_client->lpVtbl->Start(audio_client);

    audio_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)platform_audio_thread, NULL, 0, NULL);

    return true;
}

DWORD WINAPI platform_audio_thread(void* arg) {
    DWORD task_index = 0;
    HANDLE avrt = AvSetMmThreadCharacteristicsA("Pro Audio", &task_index);

    while (audio_running) {
        WaitForSingleObject(audio_event, INFINITE);

        UINT32 padding = 0;
        audio_client->lpVtbl->GetCurrentPadding(audio_client, &padding);

        UINT32 buffer_frames;
        audio_client->lpVtbl->GetBufferSize(audio_client, &buffer_frames);

        UINT32 frames_to_write = buffer_frames - padding;
        if (frames_to_write == 0)
            continue;

        float* out = NULL;
        render_client->lpVtbl->GetBuffer(render_client, frames_to_write, (BYTE**)&out);

        if (g_audio_callback) {
            g_audio_callback(out, frames_to_write, g_audio_channels, g_audio_sample_rate, g_audio_user_data);
        }
        else {
            // safety fallback
            for (UINT32 i = 0; i < frames_to_write * g_audio_channels; i++) {
                out[i] = 0.0f;
            }
        }

        render_client->lpVtbl->ReleaseBuffer(render_client, frames_to_write, 0);
    }

    AvRevertMmThreadCharacteristics(avrt);
    return 0;
}

void platform_audio_shutdown(void) {
    if (!audio_running) return;

    audio_running = false;

    // wake thread so it exits
    if (audio_event) {
        SetEvent(audio_event);
    }

    // wait for thread
    if (audio_thread) {
        WaitForSingleObject(audio_thread, INFINITE);
        CloseHandle(audio_thread);
        audio_thread = NULL;
    }

    // stop WASAPI
    if (audio_client) {
        audio_client->lpVtbl->Stop(audio_client);
    }

    // release interfaces (IMPORTANT ORDER)
    if (render_client) {
        render_client->lpVtbl->Release(render_client);
        render_client = NULL;
    }

    if (audio_client) {
        audio_client->lpVtbl->Release(audio_client);
        audio_client = NULL;
    }

    if (audio_event) {
        CloseHandle(audio_event);
        audio_event = NULL;
    }

    CoUninitialize();
}

/*========================
    EVENTS
========================*/

void platform_poll_events(PlatformState* state) {
    input_begin_frame(&state->input);

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    state->running = g_running;
}

/*========================
    TIME
========================*/

double platform_get_time(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)g_freq.QuadPart;
}

double platform_get_time_ms() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)g_freq.QuadPart;
}

/*========================
    OPENGL
========================*/

bool platform_gl_create_context(PlatformWindow* w) {
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int fmt = ChoosePixelFormat(w->dc, &pfd);
    SetPixelFormat(w->dc, fmt, &pfd);

    w->glrc = wglCreateContext(w->dc);
    wglMakeCurrent(w->dc, w->glrc);
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (!gl_loaderLoadGL()) {
        printf("Failed to initialize OpenGL!\n");
        return false;
    }

    wglSwapIntervalEXT(1);

    return true;
}

void platform_gl_swap_buffers(PlatformWindow* w) {
    SwapBuffers(w->dc);
}

/*========================
    THREADING
========================*/

typedef struct {
    PlatformThreadFunc func;
    void* arg;
} ThreadStart;

static DWORD WINAPI thread_proc(LPVOID p) {
    ThreadStart* s = p;
    int r = s->func(s->arg);
    free(s);
    return (DWORD)r;
}

PlatformThread* platform_thread_create(PlatformThreadFunc func, void* arg) {
    PlatformThread* t = malloc(sizeof(*t));

    ThreadStart* s = malloc(sizeof(*s));
    s->func = func;
    s->arg = arg;

    t->handle = CreateThread(0, 0, thread_proc, s, 0, 0);
    return t;
}

void platform_thread_join(PlatformThread* t) {
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    free(t);
}

/*========================
    MUTEX
========================*/

PlatformMutex* platform_mutex_create(void) {
    PlatformMutex* m = malloc(sizeof(*m));
    InitializeCriticalSection(&m->cs);
    return m;
}

void platform_mutex_lock(PlatformMutex* m) {
    EnterCriticalSection(&m->cs);
}

void platform_mutex_unlock(PlatformMutex* m) {
    LeaveCriticalSection(&m->cs);
}
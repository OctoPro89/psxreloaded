#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
    #define PLATFORM_API extern "C"
#else
    #define PLATFORM_API
#endif // __cplusplus

/*========================
    INPUT
========================*/

typedef enum {
    KEY_UNKNOWN = 0,

    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
    KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
    KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    KEY_ESCAPE,
    KEY_ENTER,
    KEY_TAB,
    KEY_BACKSPACE,

    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL, KEY_RCTRL,
    KEY_LALT, KEY_RALT,

    KEY_COUNT
} KeyCode;

typedef struct {
    bool down;
    bool pressed;
    bool released;
} ButtonState;

typedef struct {
    ButtonState keys[KEY_COUNT];

    ButtonState mouse_buttons[8];
    int mouse_x, mouse_y;
    int mouse_dx, mouse_dy;
    float wheel;
} PlatformInput;

/*========================
    WINDOW
========================*/

typedef struct PlatformWindow PlatformWindow;

typedef struct {
    int width;
    int height;
    const char* title;

    bool resizable;
    bool fullscreen;
} PlatformWindowDesc;

/*========================
    THREADING
========================*/

typedef struct PlatformThread PlatformThread;
typedef struct PlatformMutex PlatformMutex;

typedef int (*PlatformThreadFunc)(void*);

/*========================
    PLATFORM STATE
========================*/

typedef struct {
    bool running;
    float delta_time;
    PlatformInput input;
} PlatformState;

/*========================
    API
========================*/

// lifecycle
PLATFORM_API bool platform_init(void);
PLATFORM_API void platform_shutdown(void);

// window
PLATFORM_API PlatformWindow* platform_create_window(PlatformWindowDesc* desc, PlatformState* state);
PLATFORM_API void platform_destroy_window(PlatformWindow* window);

PLATFORM_API void platform_set_fullscreen(PlatformWindow*, bool enabled);
PLATFORM_API void platform_set_title(PlatformWindow*, const char* title);
PLATFORM_API void platform_get_size(PlatformWindow*, int* w, int* h);
PLATFORM_API void platform_set_window_size(PlatformWindow* w, int width, int height);
PLATFORM_API void platform_set_window_pos(PlatformWindow* w, int x, int y);

// audio
typedef void (*PlatformAudioCallback)(
    float* output,      // interleaved buffer
    int frames,         // number of frames requested
    int channels,
    int sample_rate,
    void* user_data
);

PLATFORM_API bool platform_audio_init(
    int sample_rate,
    int channels,
    PlatformAudioCallback callback,
    void* user_data
);

PLATFORM_API void platform_audio_shutdown(void);

// loop
PLATFORM_API void platform_poll_events(PlatformState* state);

// timing
PLATFORM_API double platform_get_time(void);
PLATFORM_API double platform_get_time_ms(void);

// opengl
PLATFORM_API bool platform_gl_create_context(PlatformWindow*);
PLATFORM_API void platform_gl_swap_buffers(PlatformWindow*);

// threading
PLATFORM_API PlatformThread* platform_thread_create(PlatformThreadFunc func, void* arg);
PLATFORM_API void platform_thread_join(PlatformThread*);

PLATFORM_API PlatformMutex* platform_mutex_create(void);
PLATFORM_API void platform_mutex_lock(PlatformMutex*);
PLATFORM_API void platform_mutex_unlock(PlatformMutex*);

#endif
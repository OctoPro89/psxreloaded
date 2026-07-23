#pragma once

#include "core/ClassHelpers.h"

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif // __cplusplus

// TODO: multitap
#define INPUT_MAX_CONTROLLERS 2

typedef enum
{
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,

    BUTTON_START,
    BUTTON_SELECT,

    BUTTON_SQUARE,
    BUTTON_TRIANGLE,
    BUTTON_CIRCLE,
    BUTTON_CROSS,

    BUTTON_L1,
    BUTTON_L2,
    BUTTON_L3,

    BUTTON_R1,
    BUTTON_R2,
    BUTTON_R3,

    BUTTON_COUNT
} InputButton;

typedef struct
{
    bool connected;

    bool buttons[BUTTON_COUNT];
    bool previous[BUTTON_COUNT];

    float left_x;
    float left_y;

    float right_x;
    float right_y;

    float left_trigger;
    float right_trigger;
} InputState;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
bool controller_input_init();
void controller_input_shutdown();
void controller_input_update();
const InputState* controller_input_get_controller(int index);
void controller_input_set_rumble(int controller, float left, float right);
#ifdef __cplusplus
}
#endif // __cplusplus
#define INPUT_PRESSED(pPad, button) (pad->buttons[b] && !pad->previous[b])
#define INPUT_RELEASED(pPad, button) (!pad->buttons[b] && pad->previous[b])
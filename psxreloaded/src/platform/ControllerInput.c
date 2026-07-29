#include "ControllerInput.h"
#include <string.h>

extern const bool* m_keys;

static InputState gControllers[INPUT_MAX_CONTROLLERS];

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Xinput.h>

#pragma comment(lib, "Xinput.lib")

static inline float normalizeStick(short value, short deadzone)
{
    if (value > deadzone) { return (float)(value - deadzone) / (32767.0f - deadzone); }
    if (value < -deadzone) { return (float)(value + deadzone) / (32768.0f - deadzone); }
    return 0.0f;
}

static inline float normalizeTrigger(BYTE value)
{
    return value / 255.0f;
}

bool controller_input_init(void)
{
    memset(gControllers, 0, sizeof(gControllers));

    return true;
}

void controller_input_update()
{
    for (DWORD i = 0; i < INPUT_MAX_CONTROLLERS; i++)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));

        DWORD result = XInputGetState(i, &state);

        InputState* pad = &gControllers[i];

        memcpy(pad->previous, pad->buttons, sizeof(pad->buttons));

        if (result != ERROR_SUCCESS)
        {
            pad->connected = false;

            memset(pad->buttons, 0, sizeof(pad->buttons));

            pad->left_x = 0.0f;
            pad->left_y = 0.0f;
            pad->right_x = 0.0f;
            pad->right_y = 0.0f;

            pad->left_trigger = 0.0f;
            pad->right_trigger = 0.0f;

            continue;
        }

        pad->connected = true;

        WORD b = state.Gamepad.wButtons;

        pad->buttons[BUTTON_UP] = b & XINPUT_GAMEPAD_DPAD_UP;
        pad->buttons[BUTTON_DOWN] = b & XINPUT_GAMEPAD_DPAD_DOWN;
        pad->buttons[BUTTON_LEFT] = b & XINPUT_GAMEPAD_DPAD_LEFT;
        pad->buttons[BUTTON_RIGHT] = b & XINPUT_GAMEPAD_DPAD_RIGHT;

        pad->buttons[BUTTON_START] = b & XINPUT_GAMEPAD_START;
        pad->buttons[BUTTON_SELECT] = b & XINPUT_GAMEPAD_BACK;

        pad->buttons[BUTTON_L1] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
        pad->buttons[BUTTON_R1] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;

        pad->buttons[BUTTON_L3] = b & XINPUT_GAMEPAD_LEFT_THUMB;
        pad->buttons[BUTTON_R3] = b & XINPUT_GAMEPAD_RIGHT_THUMB;

        // Face buttons

        pad->buttons[BUTTON_CROSS] = b & XINPUT_GAMEPAD_A;
        pad->buttons[BUTTON_CIRCLE] = b & XINPUT_GAMEPAD_B;
        pad->buttons[BUTTON_SQUARE] = b & XINPUT_GAMEPAD_X;
        pad->buttons[BUTTON_TRIANGLE] = b & XINPUT_GAMEPAD_Y;

        // PS1 L2/R2

        pad->buttons[BUTTON_L2] = state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        pad->buttons[BUTTON_R2] = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        pad->left_trigger = normalizeTrigger(state.Gamepad.bLeftTrigger);
        pad->right_trigger = normalizeTrigger(state.Gamepad.bRightTrigger);
        pad->left_x = normalizeStick(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        pad->left_y = normalizeStick(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        pad->right_x = normalizeStick(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        pad->right_y = normalizeStick(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    }
}

const InputState* controller_input_get_controller(int index)
{
    if (index < 0 || index >= INPUT_MAX_CONTROLLERS)
        return NULL;

    return &gControllers[index];
}

void controller_input_shutdown()
{
    // clear vibration
    XINPUT_VIBRATION vibration = { 0 };

    for (DWORD i = 0; i < INPUT_MAX_CONTROLLERS; i++)
    {
        XInputSetState(i, &vibration);
    }
}

void controller_input_set_rumble(int controller, float left, float right)
{
    XINPUT_VIBRATION vibration;
    vibration.wLeftMotorSpeed = (WORD)(left * 65535);
    vibration.wRightMotorSpeed = (WORD)(right * 65535);
    XInputSetState(controller, &vibration);
}

#endif // _WIN32

// TODO:
#ifdef __EMSCRIPTEN__
bool controller_input_init(void)
{
    memset(gControllers, 0, sizeof(gControllers));

    return true;
}

void controller_input_update()
{

}

const InputState* controller_input_get_controller(int index)
{
    if (index < 0 || index >= INPUT_MAX_CONTROLLERS)
        return (const InputState*)0;

    return &gControllers[index];
}

void controller_input_shutdown()
{

}

void controller_input_set_rumble(int controller, float left, float right)
{

}
#endif // __EMSCRIPTEN__
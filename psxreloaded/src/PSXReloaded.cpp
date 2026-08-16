#include "GUI/MenuBar.h"
#include "Host.h"
#include "Renderer.h"

#include "psx-utils/Sideload.h"

#include "psx/Bus.h"

#include "core/Parse.h"
#include "core/Log.h"
#include "core/StringHelpers.h"
#include "core/Helpers.h" // HP_UNUSED

#include "platform/ControllerInput.h"
#include "platform/PlatformWindow.h"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <platform/gl_loader.h>
#endif // __EMSCRIPTEN__

#include <XGUI/xgui.h>

#include <stdio.h>
#include <stdlib.h> // EXIT_FAILURE
#include <mutex>

#include <dynarec/Dynarec.h>
#include <renderering/HardwareRenderer.h>

struct CommandLineArgs
{
	const char* biosPath = nullptr;
	const char* discPath = nullptr;
	const char* sideloadExePath = nullptr;
	const char* memoryCardPath = nullptr;
	unsigned int windowWidth = 0;
	unsigned int windowHeight = 0;
	bool windowMaximised = false;
	bool respectDisplayDpiScale = false; // should be false for pixel perfect fonts
	bool amidogTestDebug = false;
};

struct HostControllerInput
{
	bool buttonSelect = false;  // PlayStation Select
	bool buttonL3 = false; // left stick press
	bool buttonR3 = false; // right stick press
	bool buttonStart = false; // PlayStation Start
	bool joypadUp = false;
	bool joypadRight = false;
	bool joypadDown = false;
	bool joypadLeft = false;
	bool buttonL2 = false;
	bool buttonR2 = false;
	bool buttonL1 = false;
	bool buttonR1 = false;
	bool buttonNorth = false; // PlayStation Triangle / Nintendo Y / Xbox Y
	bool buttonEast = false;  // PlayStation Circle / Nintendo A / Xbox B
	bool buttonSouth = false; // PlayStation Cross / Nintendo B / Xbox A
	bool buttonWest = false;  // PlayStation Square / Nintendo X / Xbox X

	// Analogue sticks
	float m_leftStickX = 0.0f;
	float m_leftStickY = 0.0f;
	float m_rightStickX = 0.0f;
	float m_rightStickY = 0.0f;
};

static const unsigned int kMaxControllers = 2;

struct HostInput
{
	HostControllerInput controllers[kMaxControllers];
};

// TODO cleanup
MenuBar::FrameStats stats;

static bool s_quitOnEscape = false;
static bool s_quit = false;
static bool s_mainMenuBarVisible = true;
static HostInput s_hostInput;
static bool s_hostLeftAnalogueStickToDpadInDigitalMode[kMaxControllers]{ true, true }; // Convert host left analogue stick to PSX digital DPAD input.

// -----
char state_char = 0;
bool shift = false;
bool enter = false;
bool control = false;
bool left = false;
bool right = false;
bool backspace = false;
bool del = false;

const bool* m_keys;
double frameTimeSeconds = 0.0;
int uiControllerMode;

PlatformWindow* window{};

#ifdef _WIN32
wchar_t TranslateKeyToChar(UINT vkCode)
{
	BYTE keyboardState[256];
	GetKeyboardState(keyboardState); // get current state (shift, caps, etc.)

	keyboardState[VK_CONTROL] = false;
	keyboardState[VK_LCONTROL] = false;
	keyboardState[VK_RCONTROL] = false;

	wchar_t buff[5] = { 0 };
	int ret = ToUnicode(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), keyboardState, buff, 4, 0);
	if (ret > 0)
		return buff[0];
	return 0;
}
#endif // _WIN32

void keyboardCallback(int vkCode, bool isPressed)
{
#ifdef _WIN32
	bool ctrl = m_keys[VK_CONTROL] || m_keys[VK_LCONTROL] || m_keys[VK_RCONTROL];
	bool shft = m_keys[VK_SHIFT] || m_keys[VK_LSHIFT] || m_keys[VK_RSHIFT];

	control = ctrl;
	shft = shft;
	enter = m_keys[VK_RETURN];
	left = m_keys[VK_LEFT];
	right = m_keys[VK_RIGHT];
	del = m_keys[VK_DELETE];
	backspace = m_keys[VK_BACK];

	// Ignore non-printable or control keys for text input
	if (!isPressed) return;

	wchar_t ch = TranslateKeyToChar(vkCode);
	if (ch != 0)
	{
		char c = (char)ch;
		state_char = c;
	}
#endif // _WIN32
#ifdef __EMSCRIPTEN__
	control = window->IsKeyDown(PlatformInput::Control);
#endif // __EMSCRIPTEN__
}

xgui::InputState pollEventsAndGetKeyboard()
{
	auto& ctx = xgui::Context::get();

	m_keys = window->GetKeyboardState();

	window->PollEvents();

	// Process input
	xgui::InputState input{};
	if (!uiControllerMode)
	{
		input.mouse_down[0] = window->IsMouseButtonDown(0);
		input.mouse_down[1] = window->IsMouseButtonDown(1);
		input.mouse_down[2] = window->IsMouseButtonDown(2);

		int x = 0, y = 0;
		window->GetMousePosition(x, y);
		input.mouse_pos.x = (float)x;
		input.mouse_pos.y = (float)y;
		input.mouse_wheel = window->GetMouseWheelDelta();
	}
	else if (uiControllerMode == 2) // virtual mouse
	{
		const InputState* controller0 = controller_input_get_controller(0);
		input.mouse_down[0] = controller0->buttons[BUTTON_L1];

		static float x = 0;
		static float y = 0;
		x += controller0->left_x * 5.0f;
		y -= controller0->left_y * 5.0f;
		input.mouse_pos.x = x;
		input.mouse_pos.y = y;
		if (input.mouse_pos.x > window->GetWidth()) { x = (float)window->GetWidth(); }
		if (input.mouse_pos.y > window->GetWidth()) { y = (float)window->GetWidth(); }
		if (input.mouse_pos.x < 0) { x = 0.0f; }
		if (input.mouse_pos.y < 0) { y = 0.0f; }
		ctx.commandRecorder = xgui::XGUI_COMMAND_RECORDER_TOP;
		xgui::internal::renderRect({ input.mouse_pos.x - 5.0f, input.mouse_pos.y - 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
		ctx.commandRecorder = xgui::XGUI_COMMAND_RECORDER_DEFAULT;
	}

	input.input_char = control ? 0 : state_char;
	input.key_ctrl = control;
	input.key_enter = enter;
	input.key_shift = shift;
	input.key_left = left;
	input.key_right = right;
	input.key_c = state_char == 'c';
	input.key_x = state_char == 'x';
	input.key_v = state_char == 'v';
	input.key_a = state_char == 'a';
	input.key_delete = del;
	input.key_backspace = backspace;

	return input;
}

void resizeCallback(int width, int height)
{
	auto& ctx = xgui::Context::get();
	ctx.screen_width = (float)width;
	ctx.screen_height = (float)height;

	ctx.projection_updated_map.values[0] = false; // Rebuild projection matrix
	ctx.projection_updated_map.values[1] = false;
	ctx.projection_updated_map.values[2] = false;
	ctx.projection_updated_map.values[3] = false;
}

// -----

static void printUsage()
{
	puts("Usage: psxreloaded [options]\n\n");
	puts("Options:\n\n"
		"  --help                           Shows this message\n"
		"  --bios <path>                    Specify bios path. Default: bios/SCPH1001.bin\n"
		"  --disc <path>                    Insert disc (.cue file)\n"
		"  --exe <path>                     Sideload executable\n"
		"  --mcd <path>                     Load memory card from file\n"
		"  --amidog-debug                   Enable Amidog debug output for debugging test failures\n"
		"  -w --window-width                Window width. Default: 75%% display width\n"
		"  -h --window-height               Window height. Default: 75%% display height\n"
		"  -m --window-maximised            Maximise window. Default: false\n"
		"  -d --respect-display-dpi-scale   Default: false\n"
		"  --log-level <value>              Specify log level: 2 (trace), 1 (debug), 0 (info), -1 (warn), -2 (error) -3 (none)  Default: 0\n"
	);
}

static void parseCommandLineArgs(int argc, char** argv, CommandLineArgs& commandLineArgs)
{
	for (int i = 1; i < argc; i++)
	{
		const char* arg = argv[i];

		if (strcmp(arg, "--help") == 0)
		{
			printUsage();
			exit(EXIT_SUCCESS);
		}
		else if (strcmp(arg, "--log-level") == 0)
		{
			if (i + 1 == argc)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}

			arg = argv[++i];
			if (arg[0] == '-')
			{
				printUsage();
				exit(EXIT_FAILURE);
			}

			int logLevel;
			if (!ParseInt(arg, logLevel) || logLevel < LOG_LEVEL_MIN || logLevel > LOG_LEVEL_MAX)
			{
				LOG_ERROR("Invalid log-level value\n");
				printUsage();
				exit(EXIT_FAILURE);
			}
			SetLogLevel(logLevel);
		}
		else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--window-width") == 0)
		{
			if (i + 1 == argc)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}

			arg = argv[++i];
			if (!ParseUnsignedInt(arg, commandLineArgs.windowWidth))
			{
				LOG_ERROR("Specified window width is invalid\n");
				printUsage();
				exit(EXIT_FAILURE);
			}
		}
		else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--window-height") == 0)
		{
			if (i + 1 == argc)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}

			arg = argv[++i];
			if (!ParseUnsignedInt(arg, commandLineArgs.windowHeight))
			{
				LOG_ERROR("Specified window height is invalid\n");
				printUsage();
				exit(EXIT_FAILURE);
			}
		}
		else if (strcmp(arg, "--maximised") == 0 || strcmp(arg, "-m") == 0)
		{
			commandLineArgs.windowMaximised = true;
		}
		else if (strcmp(arg, "--respect-display-dpi-scale") == 0 || strcmp(arg, "-d") == 0)
		{
			commandLineArgs.respectDisplayDpiScale = true;
		}
		else if (strcmp(arg, "--bios") == 0)
		{
			if (i + 1 == argc || commandLineArgs.biosPath)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			arg = argv[++i];
			commandLineArgs.biosPath = arg;
		}
		else if (strcmp(arg, "--disc") == 0)
		{
			if (i + 1 == argc || commandLineArgs.discPath)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			arg = argv[++i];
			commandLineArgs.discPath = arg;
		}
		else if (strcmp(arg, "--exe") == 0)
		{
			if (i + 1 == argc)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			arg = argv[++i];
			commandLineArgs.sideloadExePath = arg;
		}
		else if (strcmp(arg, "--mcd") == 0)
		{
			if (i + 1 == argc)
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			arg = argv[++i];
			commandLineArgs.memoryCardPath = arg;
		}
		else if (strcmp(arg, "--amidog-debug") == 0)
		{
			commandLineArgs.amidogTestDebug = true;
		}
		else
		{
			LOG_ERROR("Unrecognised command line arg: %s\n", arg);
			printUsage();
			exit(EXIT_FAILURE);
		}
	}
}

// Maps [-1.0f,+1.0f] to [0,255]
static inline u8 stickFloatToU8(float val)
{
	if (val < -1.0f)
		val = -1.0f;
	if (val > 1.0f)
		val = 1.0f;
	u8 ret = (u8)((val + 1.0f) * 127.5f);
	return ret;
}

static void handleInput()
{
	// TODO: actual input layer, controller input fr
	const HostControllerInput hostController0_prev = s_hostInput.controllers[0];
	HostControllerInput& hostController0 = s_hostInput.controllers[0];

	const HostControllerInput hostController1_prev = s_hostInput.controllers[1];
	HostControllerInput& hostController1 = s_hostInput.controllers[1];

	if (Host::GetBus().GetSIO().GetPort(0).IsControllerConnected())
	{
		hostController0.buttonSelect = window->IsKeyDown(PlatformInput::Shift);
		//hostController0.buttonL3 = ;
		//hostController0.buttonR3 = Input::GetKeyState(SDL_SCANCODE_RCTRL) || Input::GetButtonState(0, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
		hostController0.buttonStart = window->IsKeyDown(PlatformInput::Enter);
		hostController0.joypadUp = window->IsKeyDown(PlatformInput::W);
		hostController0.joypadRight = window->IsKeyDown(PlatformInput::D);
		hostController0.joypadDown = window->IsKeyDown(PlatformInput::S);
		hostController0.joypadLeft = window->IsKeyDown(PlatformInput::A);
		//if (s_hostLeftAnalogueStickToDpadInDigitalMode[0] && Host::GetBus().GetSIO().GetPort(0).GetController().GetType() == Controller::Type::Digital)
		//{
		//	hostController0.joypadUp |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY) < -0.5f;;
		//	hostController0.joypadRight |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX) > 0.5f;
		//	hostController0.joypadDown |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY) > 0.5f;
		//	hostController0.joypadLeft |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX) < -0.5f;
		//}
		hostController0.buttonL2 = window->IsKeyDown(PlatformInput::Num2);
		hostController0.buttonR2 = window->IsKeyDown(PlatformInput::Num9);
		hostController0.buttonL1 = window->IsKeyDown(PlatformInput::Num1);
		hostController0.buttonR1 = window->IsKeyDown(PlatformInput::Num0);
		hostController0.buttonNorth = window->IsKeyDown(PlatformInput::UpArrow); // PlayStation Triangle / Nintendo Y / Xbox Y
		hostController0.buttonEast = window->IsKeyDown(PlatformInput::RightArrow);  // PlayStation Circle / Nintendo A / Xbox B
		hostController0.buttonSouth = window->IsKeyDown(PlatformInput::DownArrow); // PlayStation Cross / Nintendo B / Xbox A
		hostController0.buttonWest = window->IsKeyDown(PlatformInput::LeftArrow);  // PlayStation Square / Nintendo X / Xbox X
		//hostController0.m_leftStickX = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX);
		//hostController0.m_leftStickY = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY);
		//hostController0.m_rightStickX = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_RIGHTX);
		//hostController0.m_rightStickY = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_RIGHTY);

		// Do controller input after if connected
		// TODO: Controller UI, rumble
		const InputState* controller = controller_input_get_controller(0);
		if (controller->connected && !uiControllerMode)
		{
			hostController0.buttonSelect = controller->buttons[BUTTON_SELECT];
			hostController0.buttonStart = controller->buttons[BUTTON_START];

			hostController0.joypadUp = controller->buttons[BUTTON_UP];
			hostController0.joypadRight = controller->buttons[BUTTON_RIGHT];
			hostController0.joypadDown = controller->buttons[BUTTON_DOWN];
			hostController0.joypadLeft = controller->buttons[BUTTON_LEFT];

			if (s_hostLeftAnalogueStickToDpadInDigitalMode[0] && Host::GetBus().GetSIO().GetPort(0).GetController().GetType() == Controller::Type::Digital)
			{
				const float threshold = 0.5f;
				hostController0.joypadUp |= controller->left_y > threshold;
				hostController0.joypadDown |= controller->left_y < -threshold;
				hostController0.joypadLeft |= controller->left_x < -threshold;
				hostController0.joypadRight |= controller->left_x > threshold;
			}

			hostController0.buttonL1 = controller->buttons[BUTTON_L1];
			hostController0.buttonL2 = controller->buttons[BUTTON_L2];
			hostController0.buttonR1 = controller->buttons[BUTTON_R1];
			hostController0.buttonR2 = controller->buttons[BUTTON_R2];
			hostController0.buttonL3 = controller->buttons[BUTTON_L3];
			hostController0.buttonR3 = controller->buttons[BUTTON_R3];

			hostController0.buttonNorth = controller->buttons[BUTTON_TRIANGLE]; // PlayStation Triangle / Nintendo Y / Xbox Y
			hostController0.buttonEast = controller->buttons[BUTTON_CIRCLE];  // PlayStation Circle / Nintendo A / Xbox B
			hostController0.buttonSouth = controller->buttons[BUTTON_CROSS]; // PlayStation Cross / Nintendo B / Xbox A
			hostController0.buttonWest = controller->buttons[BUTTON_SQUARE];  // PlayStation Square / Nintendo X / Xbox X

			hostController0.m_leftStickX = controller->left_x;
			hostController0.m_leftStickY = controller->left_y;
			hostController0.m_rightStickX = controller->right_x;
			hostController0.m_rightStickY = controller->right_y;
		}
	}
	if (Host::GetBus().GetSIO().GetPort(1).IsControllerConnected())
	{
		hostController1.buttonSelect = window->IsKeyDown(PlatformInput::Shift);
		//hostController1.buttonL3 = ;
		//hostController1.buttonR3 = Input::GetKeyState(SDL_SCANCODE_RCTRL) || Input::GetButtonState(1, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
		hostController1.buttonStart = window->IsKeyDown(PlatformInput::Enter);
		hostController1.joypadUp = window->IsKeyDown(PlatformInput::W);
		hostController1.joypadRight = window->IsKeyDown(PlatformInput::D);
		hostController1.joypadDown = window->IsKeyDown(PlatformInput::S);
		hostController1.joypadLeft = window->IsKeyDown(PlatformInput::A);
		//if (s_hostLeftAnalogueStickToDpadInDigitalMode[1] && Host::GetBus().GetSIO().GetPort(1).GetController().GetType() == Controller::Type::Digital)
		//{
		//	hostController1.joypadUp |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY) < -0.5f;;
		//	hostController1.joypadRight |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX) > 0.5f;
		//	hostController1.joypadDown |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY) > 0.5f;
		//	hostController1.joypadLeft |= Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX) < -0.5f;
		//}
		hostController1.buttonL2 = window->IsKeyDown(PlatformInput::Num2);
		hostController1.buttonR2 = window->IsKeyDown(PlatformInput::Num9);
		hostController1.buttonL1 = window->IsKeyDown(PlatformInput::Num1);
		hostController1.buttonR1 = window->IsKeyDown(PlatformInput::Num1);
		hostController1.buttonNorth = window->IsKeyDown(PlatformInput::UpArrow); // PlayStation Triangle / Nintendo Y / Xbox Y
		hostController1.buttonEast = window->IsKeyDown(PlatformInput::RightArrow);  // PlayStation Circle / Nintendo A / Xbox B
		hostController1.buttonSouth = window->IsKeyDown(PlatformInput::DownArrow); // PlayStation Cross / Nintendo B / Xbox A
		hostController1.buttonWest = window->IsKeyDown(PlatformInput::LeftArrow);  // PlayStation Square / Nintendo X / Xbox X
		//hostController1.m_leftStickX = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTX);
		//hostController1.m_leftStickY = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_LEFTY);
		//hostController1.m_rightStickX = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_RIGHTX);
		//hostController1.m_rightStickY = Input::GetAxisValue(0, SDL_GAMEPAD_AXIS_RIGHTY);

		// Do controller input after if connected
		// TODO: Controller UI, rumble
		const InputState* controller = controller_input_get_controller(0);
		if (controller->connected && !uiControllerMode)
		{
			hostController0.buttonSelect = controller->buttons[BUTTON_SELECT];
			hostController1.buttonStart = controller->buttons[BUTTON_START];

			hostController1.joypadUp = controller->buttons[BUTTON_UP];
			hostController1.joypadRight = controller->buttons[BUTTON_RIGHT];
			hostController1.joypadDown = controller->buttons[BUTTON_DOWN];
			hostController1.joypadLeft = controller->buttons[BUTTON_LEFT];

			if (s_hostLeftAnalogueStickToDpadInDigitalMode[1] && Host::GetBus().GetSIO().GetPort(1).GetController().GetType() == Controller::Type::Digital)
			{
				const float threshold = 0.5f;
				hostController0.joypadUp |= controller->left_y > threshold;
				hostController1.joypadDown |= controller->left_y < -threshold;
				hostController1.joypadLeft |= controller->left_x < -threshold;
				hostController1.joypadRight |= controller->left_x > threshold;
			}

			hostController1.buttonL1 = controller->buttons[BUTTON_L1];
			hostController1.buttonL2 = controller->buttons[BUTTON_L2];
			hostController1.buttonR1 = controller->buttons[BUTTON_R1];
			hostController1.buttonR2 = controller->buttons[BUTTON_R2];
			hostController1.buttonL3 = controller->buttons[BUTTON_L3];
			hostController1.buttonR3 = controller->buttons[BUTTON_R3];

			hostController1.buttonNorth = controller->buttons[BUTTON_TRIANGLE]; // PlayStation Triangle / Nintendo Y / Xbox Y
			hostController1.buttonEast = controller->buttons[BUTTON_CIRCLE];  // PlayStation Circle / Nintendo A / Xbox B
			hostController1.buttonSouth = controller->buttons[BUTTON_CROSS]; // PlayStation Cross / Nintendo B / Xbox A
			hostController1.buttonWest = controller->buttons[BUTTON_SQUARE];  // PlayStation Square / Nintendo X / Xbox X

			hostController1.m_leftStickX = controller->left_x;
			hostController1.m_leftStickY = controller->left_y;
			hostController1.m_rightStickX = controller->right_x;
			hostController1.m_rightStickY = controller->right_y;
		}
	}

	// Pass input state changes to emulator
	SIO& sio = Host::GetBus().GetSIO();

	Controller& controller0 = sio.GetPort(0).GetController();
	if (hostController0.buttonSelect != hostController0_prev.buttonSelect)
		hostController0.buttonSelect ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::SelectButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::SelectButton);
	if (hostController0.buttonL3 != hostController0_prev.buttonL3)
		hostController0.buttonL3 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::L3) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::L3);
	if (hostController0.buttonR3 != hostController0_prev.buttonR3)
		hostController0.buttonR3 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::R3) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::R3);
	if (hostController0.buttonStart != hostController0_prev.buttonStart)
		hostController0.buttonStart ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::StartButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::StartButton);
	if (hostController0.joypadUp != hostController0_prev.joypadUp)
		hostController0.joypadUp ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::JoypadUp) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::JoypadUp);
	if (hostController0.joypadRight != hostController0_prev.joypadRight)
		hostController0.joypadRight ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::JoypadRight) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::JoypadRight);
	if (hostController0.joypadDown != hostController0_prev.joypadDown)
		hostController0.joypadDown ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::JoypadDown) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::JoypadDown);
	if (hostController0.joypadLeft != hostController0_prev.joypadLeft)
		hostController0.joypadLeft ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::JoypadLeft) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::JoypadLeft);
	if (hostController0.buttonL2 != hostController0_prev.buttonL2)
		hostController0.buttonL2 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::L2Button) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::L2Button);
	if (hostController0.buttonR2 != hostController0_prev.buttonR2)
		hostController0.buttonR2 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::R2Button) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::R2Button);
	if (hostController0.buttonL1 != hostController0_prev.buttonL1)
		hostController0.buttonL1 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::L1Button) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::L1Button);
	if (hostController0.buttonR1 != hostController0_prev.buttonR1)
		hostController0.buttonR1 ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::R1Button) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::R1Button);
	if (hostController0.buttonNorth != hostController0_prev.buttonNorth)
		hostController0.buttonNorth ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::TriangleButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::TriangleButton);
	if (hostController0.buttonEast != hostController0_prev.buttonEast)
		hostController0.buttonEast ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::CircleButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::CircleButton);
	if (hostController0.buttonSouth != hostController0_prev.buttonSouth)
		hostController0.buttonSouth ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::CrossButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::CrossButton);
	if (hostController0.buttonWest != hostController0_prev.buttonWest)
		hostController0.buttonWest ? controller0.DigitalSwitchDown(Controller::DigitalSwitch::SquareButton) : controller0.DigitalSwitchUp(Controller::DigitalSwitch::SquareButton);
	if (hostController0.m_leftStickX != hostController0_prev.m_leftStickX)
		controller0.SetLeftJoyX(stickFloatToU8(hostController0.m_leftStickX));
	if (hostController0.m_leftStickY != hostController0_prev.m_leftStickY)
		controller0.SetLeftJoyY(stickFloatToU8(hostController0.m_leftStickY));
	if (hostController0.m_rightStickX != hostController0_prev.m_rightStickX)
		controller0.SetRightJoyX(stickFloatToU8(hostController0.m_rightStickX));
	if (hostController0.m_rightStickY != hostController0_prev.m_rightStickY)
		controller0.SetRightJoyY(stickFloatToU8(hostController0.m_rightStickY));

	Controller& controller1 = sio.GetPort(1).GetController();
	if (hostController1.buttonSelect != hostController1_prev.buttonSelect)
		hostController1.buttonSelect ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::SelectButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::SelectButton);
	if (hostController1.buttonL3 != hostController1_prev.buttonL3)
		hostController1.buttonL3 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::L3) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::L3);
	if (hostController1.buttonR3 != hostController1_prev.buttonR3)
		hostController1.buttonR3 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::R3) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::R3);
	if (hostController1.buttonStart != hostController1_prev.buttonStart)
		hostController1.buttonStart ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::StartButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::StartButton);
	if (hostController1.joypadUp != hostController1_prev.joypadUp)
		hostController1.joypadUp ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::JoypadUp) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::JoypadUp);
	if (hostController1.joypadRight != hostController1_prev.joypadRight)
		hostController1.joypadRight ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::JoypadRight) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::JoypadRight);
	if (hostController1.joypadDown != hostController1_prev.joypadDown)
		hostController1.joypadDown ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::JoypadDown) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::JoypadDown);
	if (hostController1.joypadLeft != hostController1_prev.joypadLeft)
		hostController1.joypadLeft ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::JoypadLeft) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::JoypadLeft);
	if (hostController1.buttonL2 != hostController1_prev.buttonL2)
		hostController1.buttonL2 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::L2Button) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::L2Button);
	if (hostController1.buttonR2 != hostController1_prev.buttonR2)
		hostController1.buttonR2 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::R2Button) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::R2Button);
	if (hostController1.buttonL1 != hostController1_prev.buttonL1)
		hostController1.buttonL1 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::L1Button) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::L1Button);
	if (hostController1.buttonR1 != hostController1_prev.buttonR1)
		hostController1.buttonR1 ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::R1Button) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::R1Button);
	if (hostController1.buttonNorth != hostController1_prev.buttonNorth)
		hostController1.buttonNorth ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::TriangleButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::TriangleButton);
	if (hostController1.buttonEast != hostController1_prev.buttonEast)
		hostController1.buttonEast ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::CircleButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::CircleButton);
	if (hostController1.buttonSouth != hostController1_prev.buttonSouth)
		hostController1.buttonSouth ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::CrossButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::CrossButton);
	if (hostController1.buttonWest != hostController1_prev.buttonWest)
		hostController1.buttonWest ? controller1.DigitalSwitchDown(Controller::DigitalSwitch::SquareButton) : controller1.DigitalSwitchUp(Controller::DigitalSwitch::SquareButton);
	if (hostController1.m_leftStickX != hostController1_prev.m_leftStickX)
		controller1.SetLeftJoyX(stickFloatToU8(hostController1.m_leftStickX));
	if (hostController1.m_leftStickY != hostController1_prev.m_leftStickY)
		controller1.SetLeftJoyY(stickFloatToU8(hostController1.m_leftStickY));
	if (hostController1.m_rightStickX != hostController1_prev.m_rightStickX)
		controller1.SetRightJoyX(stickFloatToU8(hostController1.m_rightStickX));
	if (hostController1.m_rightStickY != hostController1_prev.m_rightStickY)
		controller1.SetRightJoyY(stickFloatToU8(hostController1.m_rightStickY));
}

static void updateGUI()
{
	Bus& bus = Host::GetBus();
	MenuBar::Update(s_mainMenuBarVisible);
	/*
	InsertDiscDialog::Update();
	MemoryCardFileDialog::Update();
	SnapshotDialog::Update(bus.GetGPU());
	SideloadDialog::Update();
	ImGuiDemoWindow::Update();
	CDROMWindow::Update(bus.GetCDROM());
	CDWindow::Update(bus.GetCDROM().GetCD());
	CPUWindow::Update(bus.GetCPU());
	GPUWindow::Update(bus.GetGPU());
	SPUWindow::Update(bus.GetSPU());
	DMAWindow::Update(bus.GetDMAC());
	MemoryCardWindow::Update(bus.GetSIO());
	HostWindow::Update();
	*/
}

static void displayEmulatorView()
{
	auto& ctx = xgui::Context::get();
	auto& gpu = Host::GetBus().GetGPU();

	// TODO: account for aspect ratio
	// don't draw whole texture, just visible region i.e. respect GPU display state
	unsigned int w = gpu.GetHorizontalResolution();
	unsigned int h = gpu.GetVerticalResolution();

	float menuHeight = s_mainMenuBarVisible ? 20.0f : 0.0f;

	float availX = 0.0f;
	float availY = menuHeight;
	float availW = ctx.screen_width;
	float availH = ctx.screen_height - menuHeight;

	constexpr float targetAspect = 4.0f / 3.0f;

	float viewportW = availW;
	float viewportH = viewportW / targetAspect;

	if (viewportH > availH)
	{
		viewportH = availH;
		viewportW = viewportH * targetAspect;
	}

	float viewportX = availX + (availW - viewportW) * 0.5f;
	float viewportY = availY + (availH - viewportH) * 0.5f;

	float u1 = float(w) / 640.0f;
	float v1 = float(h) / 480.0f;

	xgui::imageView(
		Host::GetDisplayTexture()->GetGLTexture(),
		viewportX + viewportW * 0.5f,
		viewportY + viewportH * 0.5f,
		viewportW,
		viewportH,
		0.0f, 0.0f,
		u1, v1
	);

	
	// real emu scaling
	/*
	unsigned int gpuW = gpu.GetHorizontalResolution();
	unsigned int gpuH = gpu.GetVerticalResolution();

	int scale = std::max(1, std::min(
		(int)(availW / gpuW),
		(int)(availH / gpuH)
	));

	viewportW = gpuW * scale;
	viewportH = gpuH * scale;
	*/
}

int main(int argc, char** argv)
{
	CommandLineArgs commandLineArgs;
	parseCommandLineArgs(argc, argv, commandLineArgs);

	// Size window to show display above VRAM + ImGui main menu
	static const unsigned int kDefaultWindowWidth = kVRAMWidth16bpp + 512 + 128; // extra for ImGui windows
	static const unsigned int kDefaultWindowHeight = 20 + 64 + GPU::kMaxDisplayHeightPixels + kVRAMHeightLines + 32; // + some extra for main menu + overscan

	// Default window size
	int windowWidth = (int)kDefaultWindowWidth;
	int windowHeight = (int)kDefaultWindowHeight;
	if (commandLineArgs.windowWidth > 0)
		windowWidth = commandLineArgs.windowWidth;
	if (commandLineArgs.windowHeight > 0)
		windowHeight = commandLineArgs.windowHeight;

#ifdef DEBUG
	#ifdef __EMSCRIPTEN__
		const char* title = "PSXReloaded [WebAssembly]" " | Nightly" " | Debug";
	#else
		const char* title = "PSXReloaded" " | Nightly" " | Debug";
	#endif // __EMSCRIPTEN__
#elif RELEASE
#ifdef __EMSCRIPTEN__
	const char* title = "PSXReloaded [WebAssembly]" " | Nightly" " | Release";
#else
	const char* title = "PSXReloaded" " | Nightly" " | Release";
#endif // __EMSCRIPTEN__
#endif

	window = new PlatformWindow(title, windowWidth, windowHeight);
	if (!window)
	{
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to open window!", "", MB_OK);
#elif __EMSCRIPTEN__
		emscripten_run_script("alert('Failed to open window!')");
#endif // _WIN32
		return EXIT_FAILURE;
	}

	if (!window->SetupGLContext())
	{
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to initialize OpenGL", "", MB_OK);
#elif __EMSCRIPTEN__
		emscripten_run_script("alert('Failed to initialize OpenGL!')");
#endif // _WIN32
		return EXIT_FAILURE;
	}

	window->Show();
	window->SetKeyCallback(keyboardCallback);
	window->SetWindowResizeAndRenderDuringResizeCallback(resizeCallback);

	printf("Window created, starting application!\n");

// #ifdef __EMSCRIPTEN__
	// const bool audioSubSystemInitialised = false;
// #else
	const bool audioSubSystemInitialised = true;
// #endif

	if (!Host::Init(audioSubSystemInitialised, commandLineArgs.biosPath))
	{
		LOG_ERROR("Failed to initialise host\n");
		return EXIT_FAILURE;
	}

	// Connect a controller to port 0
	// #TODO: Make this a command line option or load from settings
	ControllerPort& port0 = Host::GetBus().GetSIO().GetPort(0);
	port0.SetControllerConnected(true);

	if (commandLineArgs.memoryCardPath)
	{
		MemoryCard& memoryCard = port0.GetMemoryCard();
		if (!memoryCard.LoadFromFile(commandLineArgs.memoryCardPath))
		{
			LOG_ERROR("Failed to load memory card image: %s\n", commandLineArgs.memoryCardPath);
			return EXIT_FAILURE;
		}
		port0.SetMemoryCardInserted(true);
	}

	if (commandLineArgs.discPath)
	{
		CD& cd = Host::GetCD();
		if (!cd.LoadFromFile(commandLineArgs.discPath))
		{
			LOG_ERROR("Failed to load disc image: %s\n", commandLineArgs.discPath);
			return EXIT_FAILURE;
		}
		Host::GetBus().GetCDROM().InsertDisc(cd);
	}

	if (commandLineArgs.sideloadExePath)
	{
		if (!Sideload::SideloadExecutable(commandLineArgs.sideloadExePath, Host::GetBus(), /*pTTYLogger*/nullptr))
		{
			LOG_ERROR("Failed to sideload executable: %s\n", commandLineArgs.sideloadExePath);
			return EXIT_FAILURE;
		}

		if (commandLineArgs.amidogTestDebug)
			Sideload::EnableAmidogTestDebugOutput(Host::GetBus());
	}

	printf("Loading virtual disc image: 'Crash Bandicoot (USA).bin'\n");

	// TODO: remove
#if 0
#ifdef __EMSCRIPTEN__
	CD& cd = Host::GetCD();
	if (!cd.LoadFromFile("Crash Bandicoot (USA).bin"))
	{
		LOG_ERROR("Failed to load disc image: %s\n", commandLineArgs.discPath);
		return EXIT_FAILURE;
	}
	Host::GetBus().GetCDROM().InsertDisc(cd);
#endif // __EMSCRIPTEN__
#endif
	printf("Initializing UI Framework\n");
	if (!xgui::init())
	{
		printf("Failed to initialize UI Framework!\n");
		return EXIT_FAILURE;
	}
	printf("Initialized UI Framework\n");

	auto& ctx = xgui::Context::get();

	xgui::Style style{};
	style.corner_radius = 5.0f;

	ctx.style = style;

	ctx.screen_width = (float)window->GetWidth();
	ctx.screen_height = (float)window->GetHeight();

	ctx.style.menu_bar_margin = 16.0f;

	ctx.projection_updated_map.values[0] = false; // Rebuild projection matrix
	ctx.projection_updated_map.values[1] = false;
	ctx.projection_updated_map.values[2] = false;
	ctx.projection_updated_map.values[3] = false;

	// Main loop
	s_quit = false;
	double prevTime = window->GetTime();

	// Colors
	xgui::Colors::MenuBar = { 0.0509803922f, 0.0666666667f, 0.0901960784f, 1.0f };
	xgui::Colors::MenuBarItem = { 0.0509803922f, 0.0666666667f, 0.0901960784f, 1.0f };
	xgui::Colors::MenuBarItemHover = { 0.0509803922f + 0.15f, 0.0666666667f + 0.15f, 0.0901960784f + 0.15f, 1.0f };
	xgui::Colors::MenuBarItemClicked = { 0.0509803922f - 0.05f, 0.0666666667f - 0.05f, 0.0901960784f - 0.05f, 1.0f };
	xgui::Colors::Menu = { 0.0509803922f, 0.0666666667f, 0.0901960784f, 1.0f };
	xgui::Colors::MenuHover = { 0.0509803922f + 0.15f, 0.0666666667f + 0.15f, 0.0901960784f + 0.15f, 1.0f };

	// TODO: multi-monitor better support
	double refreshRate = (double)window->GetMonitorRefreshRate();

	/*
	dynarec::Compiler compiler = dynarec::Compiler(Host::GetBus().GetCPU());
	dynarec::CompiledBlock block = compiler.CompileBlock(0xbfc0'0000);

	for (size_t i = 0; i < block.instructions.size(); ++i)
	{
		const auto& ins = block.instructions[i];
		const dynarec::Opcode* def = dynarec::Compiler::DecodeOpcode(ins.raw);

		printf("%08X : %08X [%s]\n",
			ins.pc,
			ins.raw,
			def ? def->instruction : "unknown");
	}
	*/

#ifdef EXPERIMENTAL_HW_RENDERER
	hw_renderer_singleton.Init();
#endif // EXPERIMENTAL_HW_RENDERER

#ifndef __EMSCRIPTEN__
	while (!s_quit)
	{
#else
	auto frame = [&]()
	{
#endif // __EMSCRIPTEN__
		s_quit = window->ShouldClose();

		xgui::InputState input = pollEventsAndGetKeyboard();
		xgui::beginFrame(input);

		handleInput();
		controller_input_update();

		updateGUI();

		displayEmulatorView();

		bool ctrl = input.key_ctrl;
		bool shift = input.key_shift;

		static bool lastFrameP = false;
		static bool lastFrameCtrl = false;

		if (s_quitOnEscape && input.key_escape && !ctrl && !shift)
		{
			LOG_INFO("Escape pressed, quitting\n");
			s_quit = true;
		}

		if (input.key_ctrl && !lastFrameCtrl && !shift)
			s_mainMenuBarVisible = !s_mainMenuBarVisible;

		if (window->IsKeyDown(PlatformInput::P) && !lastFrameP)
		{
			if (!ctrl && !shift) // alone
				Host::s_paused = !Host::s_paused;
			else if (ctrl && shift) // Ctrl+Shift+F5
				Host::ResetEmulator();
		}

		double currentTime = window->GetTime();
		frameTimeSeconds = currentTime - prevTime;
		prevTime = currentTime;
		// TODO: frame skip
		if (frameTimeSeconds > 0.5)
			frameTimeSeconds = 1.0 / refreshRate; // Probably debugging

		// -- FRAMERATE CALCULATION --
		stats.accumulator += frameTimeSeconds;
		stats.frames++;

		if (stats.accumulator >= 0.5)
		{
			stats.fps = stats.frames / stats.accumulator;
			stats.frameTimeMS = (stats.accumulator * 1000.0) / stats.frames;

			stats.accumulator = 0.0;
			stats.frames = 0;
		}
		// -- FRAMERATE CALCULATION --

		// TODO: Get refresh rate
		Host::Update(/* displayFramePeriodSeconds */ 1.0 / refreshRate, frameTimeSeconds);
		//Host::Render(menuBarHeight);

#ifdef EXPERIMENTAL_HW_RENDERER
		hw_renderer_singleton.Render();
		unsigned int texwidth = 1024;//Host::GetBus().GetGPU().GetHorizontalResolution();
		unsigned int texheight = 512;//Host::GetBus().GetGPU().GetVerticalResolution();
		xgui::imageView(hw_renderer_singleton.GetTexture(), (float)texwidth / 2.0f, (float)texheight / 2.0f, (float)texwidth, (float)texheight, 0.0f, 1.0f, 1.0f, 0.0f);
#endif // EXPERIMENTAL_HW_RENDERER
		glViewport(0, 0, window->GetWidth(), window->GetHeight());
		xgui::endFrame();
		window->SwapDC();

		lastFrameP = window->IsKeyDown(PlatformInput::P);
		lastFrameCtrl = window->IsKeyDown(PlatformInput::Control);

		// revert state
		state_char = 0;
		shift = false;
		enter = false;
		control = false;
		left = false;
		right = false;
		backspace = false;
		window->ResetMouseWheelDelta();
#ifdef __EMSCRIPTEN__
	};
#else
	}
#endif // __EMSCRIPTEN__

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(
	[](void* arg)
	{
		(*static_cast<decltype(frame)*>(arg))();
	},
	&frame,
	0,
	true);
#endif // __EMSCRIPTEN__

	xgui::shutdown();
	Host::Shutdown();

	delete window;

	return EXIT_SUCCESS;
}

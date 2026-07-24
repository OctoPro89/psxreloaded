#include "MenuBar.h"
#include <XGUI/xgui.h>
#include "GUI/InsertDiscDialog.h"
#include "GUI/MemoryCardFileDialog.h"
#include "GUI/SideloadDialog.h"
#include "GUI/SnapshotDialog.h"
#include "GUI/CDROMWindow.h"
#include "GUI/CDWindow.h"
#include "GUI/CPUWindow.h"
#include "GUI/DMAWindow.h"
#include "GUI/GPUWindow.h"
#include "GUI/HostWindow.h"
#include "GUI/MemoryCardWindow.h"
#include "GUI/SPUWindow.h"
#include "GUI/GuiUtils.h"
#include "Host.h"
#include "platform/ControllerInput.h"
#include "platform/platform.h"
#include "psx/Bus.h"
#include "core/Log.h"

extern double frameTimeSeconds;
extern int uiControllerMode;
static bool prevUiUsingController;

bool MenuBar::Update(bool s_mainMenuBarVisible)
{
	xgui::Context& ctx = xgui::Context::get();

	if (s_mainMenuBarVisible)
	{
		f32 file_menu_x_off = 0.0f;

		static bool fileMenuOpen = false;
		static bool infoMenuOpen = false;
		static bool emulatorMenuOpen = false;
		static bool controllersMenuOpen = false;
		static bool memoryCardsMenuOpen = false;
		static bool helpMenuOpen = false;
		static bool failedPopupOpen = false;
		static const char* errorMsg = NULL;

		xgui::menuBar(20.0f);
		if (xgui::menuBarItem("File", &file_menu_x_off))
		{
			fileMenuOpen = !fileMenuOpen;
			emulatorMenuOpen = false;
			memoryCardsMenuOpen = false;
		}

		f32 info_menu_x_off = file_menu_x_off;
		if (xgui::menuBarItem("Info", &info_menu_x_off))
		{
			infoMenuOpen = !infoMenuOpen;
		}

		f32 emulator_menu_x_off = info_menu_x_off;
		if (xgui::menuBarItem("Emulator", &emulator_menu_x_off))
		{
			emulatorMenuOpen = !emulatorMenuOpen;
			fileMenuOpen = false;
			memoryCardsMenuOpen = false;
		}

		f32 controllers_menu_x_off = emulator_menu_x_off;
		if (xgui::menuBarItem("Controllers", &controllers_menu_x_off))
		{
			controllersMenuOpen = !controllersMenuOpen;
		}

		f32 memory_cards_menu_x_off = controllers_menu_x_off;
		if (xgui::menuBarItem("Memory Cards", &memory_cards_menu_x_off))
		{
			memoryCardsMenuOpen = !memoryCardsMenuOpen;
			emulatorMenuOpen = false;
			fileMenuOpen = false;
		}

		f32 help_menu_x_off = memory_cards_menu_x_off;
		if (xgui::menuBarItem("Help", &help_menu_x_off))
		{
			helpMenuOpen = !helpMenuOpen;
		}

		// UI control with controller
		/*
			offsets calculated from getTextLength()
			File & Info & Help: 40.3361359 / 2 = 20.16806795
			Emulator: 80.6722717 / 2 = 40.33613585
			Controllers: 110.924362 / 2 = 55.462181
			Memory Cards: 121.008392 / 2 = 60.504196
		*/
		const InputState* controller0_state = controller_input_get_controller(0);
		if (controller0_state->buttons[BUTTON_START] && controller0_state->buttons[BUTTON_R2] && !uiControllerMode)
		{
			uiControllerMode = 1;
		}
		else if (INPUT_RELEASED(controller0_state, BUTTON_START) || INPUT_RELEASED(controller0_state, BUTTON_R2)) { uiControllerMode = 0; }

		if (uiControllerMode)
		{
			ctx.commandRecorder = xgui::XGUI_COMMAND_RECORDER_TOP; // place on top of menubar

			static int selected = 0;
			if (controller0_state->left_x > 0.2f || controller0_state->left_x < -0.2f || controller0_state->left_y > 0.2f || controller0_state->left_y < -0.2f) { uiControllerMode = 2; }
			if (uiControllerMode == 2)
			{ 
				ctx.commandRecorder = xgui::XGUI_COMMAND_RECORDER_DEFAULT;
				goto menus;
			}
			if (INPUT_PRESSED(controller0_state, BUTTON_LEFT))
			{
				uiControllerMode = 1;
				if ((selected - 1) > -1) { selected -= 1; }
			}
			if (INPUT_PRESSED(controller0_state, BUTTON_RIGHT))
			{
				uiControllerMode = 1;
				if ((selected + 1) < 6) { selected += 1; }
			}

			switch (selected)
			{
			case 0:
				xgui::internal::renderRect({ file_menu_x_off + 25.16806795f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			case 1:
				xgui::internal::renderRect({ info_menu_x_off + 25.16806795f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			case 2:
				xgui::internal::renderRect({ emulator_menu_x_off + 45.33613585f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			case 3:
				xgui::internal::renderRect({ controllers_menu_x_off + 60.462181f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			case 4:
				xgui::internal::renderRect({ memory_cards_menu_x_off + 65.504196f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			case 5:
				xgui::internal::renderRect({ help_menu_x_off + 25.16806795f, 5.0f, 10.0f, 10.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, 5.0f);
				break;
			default:
				break;
			}

			if (INPUT_PRESSED(controller0_state, BUTTON_DOWN))
			{
				switch (selected)
				{
				case 0:
					fileMenuOpen = !fileMenuOpen;
					emulatorMenuOpen = false;
					memoryCardsMenuOpen = false;
					break;
				case 1:
					infoMenuOpen = !infoMenuOpen;
					break;
				case 2:
					emulatorMenuOpen = !emulatorMenuOpen;
					fileMenuOpen = false;
					memoryCardsMenuOpen = false;
					break;
				case 3:
					controllersMenuOpen = !controllersMenuOpen;
					break;
				case 4:
					memoryCardsMenuOpen = !memoryCardsMenuOpen;
					emulatorMenuOpen = false;
					fileMenuOpen = false;
					break;
				case 5:
					helpMenuOpen = !helpMenuOpen;
					break;
				}
			}

		}

menus:
		if (fileMenuOpen)
		{
			static xgui::MenuItem saveSubmenu[] = {
				{ "Save display..." },
				{ "Save VRAM 16 bpp" },
				{ "Save VRAM 24 bpp" },
			};

			static xgui::MenuItem fileMenu[] = {
				{ "Insert disc..." },
				{ "Eject disc" },
				{ "Sideload executable..." },
				{ "Screen capture", saveSubmenu, _countof(saveSubmenu) },
				{ "Exit" }
			};

			int selected = xgui::menu(fileMenu, _countof(fileMenu), file_menu_x_off, ctx.current_menu_bar.height);
			if (selected != -1) {
				int parent = XGUI_MENUBAR_PARENT(selected);
				int child = XGUI_MENUBAR_CHILD(selected);

				Bus& bus = Host::GetBus();

				// TODO:
				switch (parent)
				{
				case 0:
				{
					if (const char* fp = xgui::filePicker("Insert disc...", "Raw Binary (*.bin)\0*.bin\0Cue Sheets (*.cue)\0*.cue\0All Files\0*.*\0\0"))
					{
						CD& cd = Host::GetCD();
						if (!cd.LoadFromFile(fp))
						{
							failedPopupOpen = true;
							errorMsg = "Failed to load disc image!";
							LOG_ERROR("Failed to load disc image: %s\n", fp);
						}
						else
						{
							bus.GetCDROM().InsertDisc(cd);
						}
					}
					break;
				}
				case 1:
				{
					bus.GetCDROM().EjectDisc();
					break;
				}
				case 2:
				{
					// TODO:
					break;
				}
				case 4:
				{
					return true;
					break;
				}
				// TODO
				default:
					break;
				}

				fileMenuOpen = false;
			}
		}

		if (infoMenuOpen)
		{
			xgui::beginWindow("Info", 500.0f, 500.0f, 750.0f, 500.0f, true);

			xgui::WindowedUILayout wlayout{};
			wlayout.UIbegin(0.95f, 20.0f);

			wlayout.UItext("PSXReloaded v1.0", 15.0f);
			wlayout.UItext("UI Framework - XGUI", 15.0f);
			wlayout.UItext("Graphics Info:", 15.0f);
			wlayout.UItext("    Graphics API: OpenGL", 15.0f);
			wlayout.UItext("    Renderer: emulated PSX rasterizer", 15.0f);
			char buf[100];
			snprintf(buf, 100, "    Frame time (ms): %.2f", (float)frameTimeSeconds * 1000);
			wlayout.UItext(buf, 15.0f);
			wlayout.UItext("Audio Info:", 15.0f);
			wlayout.UItext("    Audio backend: WASAPI", 15.0f);
			snprintf(buf, 100, "    Current host sampling rate: %d kHz", platform_audio_output_sample_rate);
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Resampling method: Static %d kHz (host) to 44100 kHz (PSX)", platform_audio_output_sample_rate);
			wlayout.UItext(buf, 15.0f);
			wlayout.UItext("Virtual controller port 1 info:", 15.0f);
			wlayout.UItext("    Controller API: XInput", 15.0f);
			wlayout.UItext(controller0_state->connected ? "    Connected: true" : "   Connected: false", 15.0f);
			snprintf(buf, 100, "    Cross: %s | Circle: %s",
				controller0_state->buttons[BUTTON_CROSS] ? "true" : "false",
				controller0_state->buttons[BUTTON_CIRCLE] ? "true" : "false"
			);
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Square: %s | Triangle | %s",
				controller0_state->buttons[BUTTON_SQUARE] ? "true" : "false",
				controller0_state->buttons[BUTTON_TRIANGLE] ? "true" : "false"
			);

			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Left stick X: %.2f | Y: %.2f", controller0_state->left_x, controller0_state->left_y);
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Right stick X: %.2f | Y: %.2f", controller0_state->right_x, controller0_state->right_y);
			wlayout.UItext(buf, 15.0f);

			snprintf(buf, 100, "    Left trigger: %.2f", controller0_state->left_trigger);
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Right trigger: %.2f", controller0_state->right_trigger);
			wlayout.UItext(buf, 15.0f);

			snprintf(buf, 100, "    L1 (converted to true / false): %s | L2: %s | L3: %s",
				controller0_state->buttons[BUTTON_L1] ? "true" : "false",
				controller0_state->buttons[BUTTON_L2] ? "true" : "false",
				controller0_state->buttons[BUTTON_L3] ? "true" : "false"
			);
			wlayout.UItext(buf, 15.0f);

			snprintf(buf, 100, "    R1 (converted to true / false): %s | R2: %s | R3: %s",
				controller0_state->buttons[BUTTON_R1] ? "true" : "false",
				controller0_state->buttons[BUTTON_R2] ? "true" : "false",
				controller0_state->buttons[BUTTON_R3] ? "true" : "false"
			);
			wlayout.UItext(buf, 15.0f);

			snprintf(buf, 100, "    Start: %s", controller0_state->buttons[BUTTON_START] ? "true" : "false");
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Select: %s", controller0_state->buttons[BUTTON_SELECT] ? "true" : "false");
			wlayout.UItext(buf, 15.0f);

			if (wlayout.UIbutton(Host::s_playTestTone ? "Stop" : "Test audio tone", 15.0f))
			{
				Host::s_playTestTone = !Host::s_playTestTone;
			}

			if (wlayout.UIbutton("Close", 15.0f))
			{
				infoMenuOpen = false;
				ctx.active_id = 0;
				ctx.hot_id = 0;
			}
			xgui::endWindow();
		}

		if (emulatorMenuOpen)
		{
			static xgui::MenuItem menu[] = {
				{ "Pause" },
				{ "Reset Emulator" },
			};

			int selected = xgui::menu(menu, _countof(menu), emulator_menu_x_off, ctx.current_menu_bar.height);
			if (selected != -1) {
				int parent = XGUI_MENUBAR_PARENT(selected);
				int child = XGUI_MENUBAR_CHILD(selected);

				switch (parent)
				{
					case 0:
						Host::s_paused = !Host::s_paused;
						break;
					case 1:
						Host::ResetEmulator();
						break;
					default:
						break;
				}

				emulatorMenuOpen = false;
			}
		}

		if (controllersMenuOpen)
		{
			xgui::beginWindow("Controllers", 500.0f, 500.0f, 600.0f, 500.0f, true);

			xgui::WindowedUILayout wlayout{};
			wlayout.UIbegin(0.95f, 20.0f);

			wlayout.UItext("Port 1:", 15.0f);
			const char* labels[] = {
				"Digital",
				"Analog",
				"None",
			};

			const f32 y_positions[] = {
				wlayout.current_y - 8.0f,
				wlayout.current_y + wlayout.spacing,
				wlayout.current_y + 28.0f + wlayout.spacing,
			};

			static bool values[3] = { true, false, false };

			wlayout.current_y += 50.0f + (wlayout.spacing * 2);

			xgui::radioButtons(labels, values, 3, wlayout.start_x - 190.0f, (f32*)y_positions, 24.0f);

			wlayout.UItext("Key bindings:", 15.0f);
			wlayout.UItext("    DPAD (Digital): W/A/S/D", 15.0f);
			wlayout.UItext("    Cross (Digital): Down Arrow", 15.0f);
			wlayout.UItext("    Circle (Digital): Right Arrow", 15.0f);
			wlayout.UItext("    Triangle (Digital): Up Arrow", 15.0f);
			wlayout.UItext("    Square (Digital): Left Arrow", 15.0f);

			wlayout.UItext("See Info tab for gamepad info", 15.0f);
			// TODO finish...

			// TODO implement

			// TODO port 2

			if (wlayout.UIbutton("Close", 15.0f))
			{
				controllersMenuOpen = false;
				ctx.active_id = 0;
				ctx.hot_id = 0;
			}
			xgui::endWindow();
		}

		if (memoryCardsMenuOpen)
		{
			SIO& sio = Host::GetBus().GetSIO();

			static xgui::MenuItem memoryCardSubmenu[] = {
				{ "Load..." },
				{ "Save..." },
				{ "Insert" },
				{ "Eject" },
				{ "Format" }
			};

			static xgui::MenuItem fileMenu[] = {
				{ "Slot 1", memoryCardSubmenu, _countof(memoryCardSubmenu) },
				{ "Slot 2", memoryCardSubmenu, _countof(memoryCardSubmenu) }
			};

			int selected = xgui::menu(fileMenu, _countof(fileMenu), memory_cards_menu_x_off, ctx.current_menu_bar.height);
			if (selected != -1) {
				int parent = XGUI_MENUBAR_PARENT(selected);
				int child = XGUI_MENUBAR_CHILD(selected);

				if (parent == 0)
				{
					ControllerPort& port = sio.GetPort(0);
					MemoryCard& card = port.GetMemoryCard();
					switch (child)
					{
					case 0:
						if (const char* fp = xgui::filePicker("Load memory card...", "PSX Memory Cards (*.mcd, *.mc, *.mcr)\0*.mcd;*.mc;*.mcr\0Cue Sheets\0*.cue\0All Files\0*.*\0\0"))
						{
							if (card.LoadFromFile(fp))
							{
								if (!port.IsMemoryCardInserted()) { port.SetMemoryCardInserted(true); }
							}
							else
							{
								errorMsg = "Failed to load memory card!";
								failedPopupOpen = true;
							}
						}
						break;
					case 1:
						if (const char* fp = xgui::saveFilePicker("Save memory card...", "PSX Memory Cards (*.mcd, *.mc, *.mcr)\0*.mcd;*.mc;*.mcr\0Cue Sheets\0*.cue\0All Files\0*.*\0\0", "mcd"))
						{
							if (!card.SaveToFile(fp))
							{
								errorMsg = "Failed to save memory card!";
								failedPopupOpen = true;
							}
						}
						break;
					case 2:
						port.SetMemoryCardInserted(true);
						break;
					case 3:
						port.SetMemoryCardInserted(false);
						break;
					case 4:
						card.Format();
						break;
					}
				}
				else if (parent == 1)
				{
					ControllerPort& port = sio.GetPort(1);
					MemoryCard& card = port.GetMemoryCard();
					switch (child)
					{
					case 0:
						if (const char* fp = xgui::filePicker("Load memory card...", "PSX Memory Cards (*.mcd, *.mc, *.mcr)\0*.mcd;*.mc;*.mcr\0Cue Sheets\0*.cue\0All Files\0*.*\0\0"))
						{
							if (card.LoadFromFile(fp))
							{
								if (!port.IsMemoryCardInserted()) { port.SetMemoryCardInserted(true); }
							}
							else
							{
								errorMsg = "Failed to load memory card!";
								failedPopupOpen = true;
							}
						}
						break;
					case 1:
						if (const char* fp = xgui::saveFilePicker("Save memory card...", "PSX Memory Cards (*.mcd, *.mc, *.mcr)\0*.mcd;*.mc;*.mcr\0Cue Sheets\0*.cue\0All Files\0*.*\0\0", "mcd"))
						{
							if (!card.SaveToFile(fp))
							{
								errorMsg = "Failed to save memory card!";
								failedPopupOpen = true;
							}
						}
						break;
					case 2:
						port.SetMemoryCardInserted(true);
						break;
					case 3:
						port.SetMemoryCardInserted(false);
						break;
					case 4:
						card.Format();
						break;
					}
				}

				emulatorMenuOpen = false;
			}
		}

		if (helpMenuOpen)
		{
			xgui::beginWindow("Help", 500.0f, 600.0f, 600.0f, 500.0f, true);

			xgui::WindowedUILayout wlayout{};
			wlayout.UIbegin(0.95f, 20.0f);
			wlayout.UItext("Using a controller on the menubar:", 15.0f);
			wlayout.UItext("    Hold START + R2 and use the D-Pad to control the red dot", 15.0f);
			wlayout.UItext("    Press D-Pad down to select an option", 15.0f);
			wlayout.UItext("    Once a menu is open, use the left stick to control the virtual mouse", 15.0f);
			wlayout.UItext("    With the virtual mouse, use L1 to click", 15.0f);
			
			xgui::endWindow();
		}

		if (ctx.active_id == 0 && ctx.hot_id == 0 && ctx.input.mouse_down[0])
		{
			fileMenuOpen = false;
			emulatorMenuOpen = false;
			memoryCardsMenuOpen = false;
		}

		if (failedPopupOpen)
		{
			GuiUtils::ShowErrorPopup(errorMsg, failedPopupOpen);
		}
	}

	return false;
}
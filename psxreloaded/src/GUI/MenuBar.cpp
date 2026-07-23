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
#include "platform/platform.h"
#include "psx/Bus.h"
#include "core/Log.h"


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
		static bool failedPopupOpen = false;
		static const char* errorMsg = NULL;

		xgui::menuBar(20.0f);
		if (xgui::menuBarItem("File", &file_menu_x_off))
		{
			fileMenuOpen = !fileMenuOpen;
			emulatorMenuOpen = false;
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
		}

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
			wlayout.UIbegin(0.8f, 20.0f);

			wlayout.UItext("PSXReloaded v1.0", 15.0f);
			wlayout.UItext("UI Framework - XGUI", 15.0f);
			wlayout.UItext("Graphics Info:", 15.0f);
			wlayout.UItext("    Graphics API: OpenGL", 15.0f);
			wlayout.UItext("    Renderer: emulated PSX rasterizer", 15.0f);
			wlayout.UItext("Audio Info:", 15.0f);
			wlayout.UItext("    Audio backend: WASAPI", 15.0f);
			char buf[100];
			snprintf(buf, 100, "    Current host sampling rate: %d kHz", platform_audio_output_sample_rate);
			wlayout.UItext(buf, 15.0f);
			snprintf(buf, 100, "    Resampling method: Static %d kHz (host) to 44100 kHz (PSX)", platform_audio_output_sample_rate);
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
			static xgui::MenuItem fileMenu[] = {
				{ "Pause" },
				{ "Reset Emulator" },
			};

			int selected = xgui::menu(fileMenu, _countof(fileMenu), emulator_menu_x_off, ctx.current_menu_bar.height);
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
			wlayout.UIbegin(0.8f, 20.0f);

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
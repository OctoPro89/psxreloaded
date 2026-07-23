#pragma once

#include <XGUI/xgui.h>

class GuiUtils
{
public:
	static inline void ShowErrorPopup(const char* errorMsg, bool& popupOpen)
	{
		xgui::beginPopup("Error", 300.0f, 300.0f, false);
		xgui::WindowedUILayout wlayout{};
		wlayout.UIbegin(0.8f, 15.0f);
		wlayout.UItext(errorMsg, 15.0f);
		if (wlayout.UIbutton("Ok", 15.0f))
		{
			xgui::Context::get().active_id = 0;
			xgui::Context::get().hot_id = 0;
			popupOpen = false;
		}
		xgui::endPopup();
	}
};
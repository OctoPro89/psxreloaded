#pragma once

#include <XGUI/xgui_common.h>
#include <XGUI/xgui_core.h>

namespace xgui
{
	enum xgui_render_type
	{
		XGUI_RENDER_TYPE_RECT,
		XGUI_RENDER_TYPE_TEXT,
		XGUI_RENDER_TYPE_IMAGEVIEW,
		XGUI_RENDER_TYPE_CHECKMARK,
		XGUI_RENDER_TYPE_RARROW,
	};

	struct xgui_render_command
	{
		f32 u0 = 0.0f;
		f32 v0 = 0.0f;
		f32 u1 = 1.0f;
		f32 v1 = 1.0f;
		f32 x;
		f32 y;
		u64 submission_id;
		f32 w;
		f32 h;
		f32 size;
		f32 corner_radius;
		Colors::Color color;
		Rect rect;
		std::string text;
		u32 gl_id;
		xgui_render_type render_type;
		bool use_clip = false;
		Rect clip_rect;
	};

	extern u64 global_submission_counter;
} // namespace xgui
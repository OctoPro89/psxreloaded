#pragma once

#include <XGUI/xgui.h>

#define XGUI_IMPL_OPENGL

#ifdef XGUI_IMPL_OPENGL

namespace xgui
{
	namespace impl
	{
		namespace opengl
		{
			XAPI bool init_context_vars();
			XAPI void shutdown();

			XAPI void beginFrame();

			#define applyClip(ctx, r) { glEnable(GL_SCISSOR_TEST); glScissor((int)r.x, (int)(ctx.screen_height - (r.y + r.h)), (int)r.w, (int)r.h); }
			#define disableClip() { glDisable(GL_SCISSOR_TEST); }
			// XAPI void applyClip(const Context& ctx, const Rect& r);
			// XAPI void disableClip();

			XAPI void renderRect(Context& ctx, xgui_render_command& cmd);
			XAPI void renderCheckmark(Context& ctx, xgui_render_command& cmd);
			XAPI void renderRightArrow(Context& ctx, xgui_render_command& cmd);
			XAPI void renderText(Context& ctx, xgui_render_command& cmd);
			XAPI void renderImageView(Context& ctx, xgui_render_command& cmd);
			XAPI void execCommands(std::vector<xgui_render_command>& render_commands);
		}
	}
}

#endif // XGUI_IMPL_OPENGL
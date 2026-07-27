#include <XGUI/xgui_rendering_engine.h>
#include <XGUI/xgui_core.h>
#include <XGUI/impl/xgui_impl_opengl.h>

#include <string>
#include <cstring>
#include <algorithm>

namespace xgui
{
    namespace
    {
        struct ParsedLabel
        {
            const char* visible;
            const char* id_part;
        };

        static ParsedLabel parseLabel(const char* label)
        {
            ParsedLabel out{};
            const char* sep = strstr(label, "##");

            if (!sep)
            {
                out.visible = label;
                out.id_part = label;
            }
            else
            {
                static char buf[256];
                size_t len = sep - label;
                strncpy(buf, label, len);
                buf[len] = '\0';
                out.visible = buf;

                out.id_part = sep + 2;
            }

            return out;
        }

        std::vector<xgui_render_command> render_commands;
        std::vector<xgui_render_command> menu_render_commands;
        std::vector<xgui_render_command> top_render_commands;
    } // anonymous namespace

    static inline void pushCommand(const xgui_render_command& cmd)
    {
        Context& ctx = Context::get();
        switch (ctx.commandRecorder)
        {
        case XGUI_COMMAND_RECORDER_DEFAULT:
            render_commands.push_back(cmd);
            break;
        case XGUI_COMMAND_RECORDER_WINDOW:
            ctx.current_window->commands.push_back(cmd);
            break;
        case XGUI_COMMAND_RECORDER_MENU:
            menu_render_commands.push_back(cmd);
            break;
        case XGUI_COMMAND_RECORDER_TOP:
            top_render_commands.push_back(cmd);
            break;
        }
    }

    static void renderCheckmark(f32 x, f32 y, f32 size, const Colors::Color& color)
    {
        xgui_render_command cmd{};
        cmd.x = x;
        cmd.y = y;
        cmd.submission_id = global_submission_counter++;
        cmd.size = size;
        cmd.color = color;
        cmd.render_type = XGUI_RENDER_TYPE_CHECKMARK;
        pushCommand(cmd);
    }

    static void renderImageView(u32 gl_id, f32 x, f32 y, f32 w, f32 h, f32 u0, f32 v0, f32 u1, f32 v1)
    {
        xgui_render_command cmd{};
        cmd.x = x;
        cmd.y = y;
        cmd.submission_id = global_submission_counter++;
        cmd.w = w;
        cmd.h = h;
        cmd.u0 = u0;
        cmd.v0 = v0;
        cmd.u1 = u1;
        cmd.v1 = v1;
        cmd.gl_id = gl_id;
        cmd.render_type = XGUI_RENDER_TYPE_IMAGEVIEW;
        pushCommand(cmd);
    }

    static void renderArrowRight(f32 x, f32 y, f32 size, const Colors::Color& color)
    {
        xgui_render_command cmd{};
        cmd.x = x;
        cmd.y = y;
        cmd.submission_id = global_submission_counter++;
        cmd.size = size;
        cmd.color = color;
        cmd.render_type = XGUI_RENDER_TYPE_RARROW;
        pushCommand(cmd);
    }

    // Static context instance
    static Context* s_context = nullptr;

    bool Context::init()
    {
        if (!s_context)
        {
            s_context = new Context();

#ifdef XGUI_IMPL_OPENGL
            if (!impl::opengl::init_context_vars()) { return false; }
#endif // XGUI_IMPL_OPENGL

            // could use c:\\Windows\\Fonts\\segoeui.ttf on windows as a default
            return font::loadFontSDF("C:/Users/vince/Downloads/Cascadia_Code/static/CascadiaCode-Medium.ttf", s_context->text_size, &s_context->font_texture, &s_context->glyphs[0]);
        }

        return false;
    }

    void Context::shutdown()
    {
        if (s_context)
        {
#ifdef XGUI_IMPL_OPENGL
            impl::opengl::shutdown();
#endif // XGUI_IMPL_OPENGL

            // TODO: Continue cleanup

            delete s_context;
            s_context = nullptr;
        }
    }

    Context& Context::get()
    {
        return *s_context;
    }

    bool init()
    {
        return Context::init();
    }

#ifdef XGUIBUILDDYNAMIC
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>

    bool loadDylibGLFuncs()
    {
        if (!gladLoadGL()) {
            MessageBoxA(0, "Failed to initialize GLAD", "Error", MB_OK | MB_ICONERROR);
            return false;
        }

        return true;
    }
#endif

    void shutdown()
    {
        Context::shutdown();
    }

    static std::vector<Window> windows;

    static bool windowCanReceiveInput(Window* win)
    {
        Context& ctx = Context::get();

        // Iterate from front to back.
        for (auto it = windows.rbegin(); it != windows.rend(); ++it)
        {
            Rect wholeWindow = { it->rect.x, it->rect.y, it->rect.w, it->rect.h };

            if (internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, wholeWindow))
            {
                // first window hit is the front-most one
                return &(*it) == win;
            }
        }

        return false;
    }

    struct Textbox
    {
        const char* label;
        size_t cursor_pos = 0;
        size_t selection_start = 0;
        size_t selection_end = 0;
        f32 scroll_x = 0.0f;
        u32 id = 0;
        bool has_focus = false;
        bool caret_visible = true;
        double last_blink_time = 0.0;
        bool dragging = false;
    };

    static std::vector<Textbox> textboxes;

    void beginFrame(const InputState& input)
    {
        Context& ctx = Context::get();
        ctx.input = input;

#ifdef XGUI_IMPL_OPENGL
        impl::opengl::beginFrame();
#endif // XGUI_IMPL_OPENGL
    }

    void endFrame()
    {
        Context& ctx = Context::get();
        ctx.hot_id = 0;
        ctx.is_using_cursor = false;

#ifdef XGUI_IMPL_OPENGL
        impl::opengl::execCommands(render_commands);
#endif // XGUI_IMPL_OPENGL

        // add window commands here
        for (auto& wdw : windows)
        {
#ifdef XGUI_IMPL_OPENGL
            impl::opengl::execCommands(wdw.commands);
#endif // XGUI_IMPL_OPENGL
        }

#ifdef XGUI_IMPL_OPENGL
        impl::opengl::execCommands(menu_render_commands);
#endif // XGUI_IMPL_OPENGL

#ifdef XGUI_IMPL_OPENGL
        impl::opengl::execCommands(top_render_commands);
#endif // XGUI_IMPL_OPENGL
    }

    static void drawScrollbar(Window* win)
    {
        Context& ctx = Context::get();

        const f32 view_h = win->rect.h - 22.0f; // minus title bar
        const f32 content_h = std::max(win->content_height, view_h);

        if (content_h <= view_h)
            return;

        const f32 scrollbar_w = 8.0f;

        Rect bar_rect =
        {
            win->rect.x + win->rect.w - scrollbar_w,
            win->rect.y + 22.0f,
            scrollbar_w,
            view_h
        };

        // background
        internal::renderRect(bar_rect, { 0.15f,0.15f,0.15f,1 }, 4.0f + 0.02f);

        // thumb size proportional
        f32 thumb_h = std::max((view_h / content_h) * view_h, 20.0f);

        f32 max_scroll = content_h - view_h;
        f32 scroll_ratio = win->scroll_y / max_scroll;

        f32 thumb_y = bar_rect.y + scroll_ratio * (view_h - thumb_h);

        Rect thumb =
        {
            bar_rect.x,
            thumb_y,
            scrollbar_w,
            thumb_h
        };

        const u32 id = internal::hashString("scrollbar");

        bool hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, thumb) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

        static f32 drag_offset = 0.0f;

        if (hovered && ctx.input.mouse_down[0] && ctx.active_id == 0)
        {
            ctx.active_id = id;
            drag_offset = ctx.input.mouse_pos.y - thumb_y;
        }

        if (ctx.active_id == id)
        {
            ctx.is_using_cursor = true;

            f32 new_y = ctx.input.mouse_pos.y - drag_offset;
            f32 t = (new_y - bar_rect.y) / (view_h - thumb_h);
            t = std::clamp(t, 0.0f, 1.0f);

            win->scroll_y = t * max_scroll;
        }

        if (!ctx.input.mouse_down[0] && ctx.active_id == id)
            ctx.active_id = 0;

        internal::renderRect(thumb, hovered ? Colors::Color{ 0.6f,0.6f,0.6f,1 } : Colors::Color{ 0.4f,0.4f,0.4f,1 }, 4.0f);
    }

    void beginWindow(const char* title, f32 x, f32 y, f32 w, f32 h, bool draggable)
    {
        x -= (w / 2.0f);
        y -= (h / 2.0f);

        ParsedLabel parsed = parseLabel(title);
        u32 id = internal::hashString(parsed.id_part);

        Context& ctx = Context::get();
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_WINDOW;

        // Find or create
        Window* win = nullptr;
        for (auto& wdw : windows)
            if (wdw.title == parsed.visible) { win = &wdw; break; }
        if (!win)
        {
            windows.push_back({ parsed.visible, id, {x, y, w, h}, { 0 }, false, draggable });
            win = &windows.back();
        }

        f32 title_bar_height = 22.0f; // smaller than before
        f32 view_h = win->rect.h - title_bar_height;
        f32 max_scroll = std::max(0.0f, win->content_height - view_h);

        Rect client = { win->rect.x, win->rect.y + title_bar_height, win->rect.w, win->rect.h - title_bar_height };

        if (win->content_height > win->rect.h && internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, client))
        {
            win->scroll_y -= ctx.input.mouse_wheel * 20.0f;
            win->scroll_y = std::clamp(win->scroll_y, 0.0f, max_scroll);
        }

        win->content_height = 0.0f;

        // update state, don't update clip yet or title and title bar won't be rendererd
        ctx.current_window = win;

        // Title bar rect
        Rect title_bar = { win->rect.x, win->rect.y, win->rect.w, title_bar_height };
        Rect old_bar = title_bar;

        bool mouse_over_title = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, title_bar);

        // Start dragging
        if (draggable && mouse_over_title && ctx.input.mouse_down[0] && !win->dragging && ctx.active_id == 0 && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window)))
        {
            win->dragging = true;
            win->drag_offset = { ctx.input.mouse_pos.x - win->rect.x, ctx.input.mouse_pos.y - win->rect.y };
            ctx.active_id = id;

            bringCurrentWindowToFront();
        }

        // Stop dragging
        if (!ctx.input.mouse_down[0])
        {
            win->dragging = false;
            ctx.active_id = 0;
        }

        Colors::Color bar_color = win->focused ? Colors::Color{ 0.3f, 0.3f, 0.6f, 1.0f } : Colors::Color{ 0.2f, 0.2f, 0.2f, 1.0f };

        // Drag move / docking
        if (win->dragging)
        {
            ctx.is_using_cursor = true;
            win->rect.x = ctx.input.mouse_pos.x - win->drag_offset.x;
            win->rect.y = ctx.input.mouse_pos.y - win->drag_offset.y;
            title_bar = { win->rect.x, win->rect.y, win->rect.w, title_bar_height };

            bool overDock = false;

            for (const auto& d : ctx.dock_points)
            {
                // Dock position in pixels (center)
                f32 dock_x = d.x_percent * ctx.screen_width;
                f32 dock_y = d.y_percent * (ctx.screen_height - ctx.current_menu_bar.height);

                // Size to grow window to when docked, relative to screen size
                f32 dock_w = d.grow_percent_x * ctx.screen_width;
                f32 dock_h = d.grow_percent_y * (ctx.screen_height - ctx.current_menu_bar.height);

                // Center dock rectangle on dock point
                Rect dockRect = {
                    dock_x - dock_w * 0.5f,
                    (dock_y - dock_h * 0.5f) + ctx.current_menu_bar.height,
                    dock_w,
                    dock_h
                };

                // Draw dock rect with some transparency
                internal::renderRect(dockRect, { bar_color.r, bar_color.g, bar_color.b, 0.5f }, ctx.style.corner_radius);

                // Draw dock label roughly centered
                f32 text_width = internal::getTextWidth(d.dock_text);
                f32 text_height = s_context->text_size / 2.0f;
                internal::renderText(d.dock_text, dock_x - text_width * 0.5f, dock_y - text_height * 0.5f, Colors::Text);

                // Check if mouse is over dock rect
                if (internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, dockRect)) {
                    if (!win->docked) {
                        title_bar = { win->rect.x, win->rect.y, win->rect.w, title_bar_height };
                        win->old_rect = win->rect;
                    }
                    win->rect = dockRect;
                    win->docked = true;
                    overDock = true;
                    title_bar = old_bar;
                }
            }

            if (win->docked && !overDock) {
                // Restore old size and position when undocked
                win->rect = win->old_rect;
                win->docked = false;
            }
        }

        // Render
        internal::renderRect(title_bar, bar_color, 0.0f);
        internal::renderText(title, win->rect.x + 6.0f, win->rect.y + title_bar_height - 6.0f, Colors::White);

        // Window background
        Rect bg = { win->rect.x, win->rect.y + title_bar_height, win->rect.w, win->rect.h - title_bar_height };
        internal::renderRect(bg, Colors::Color{ 0.1f, 0.1f, 0.1f, 1.0f }, 0.0f);

        ctx.current_clip = bg;
        ctx.clip = true;
    }

    void endWindow() {
        Context& ctx = Context::get();
        
        // Scrollbar if needed
        if (ctx.current_window->content_height > ctx.current_window->rect.h)
        {
            drawScrollbar(ctx.current_window);
        }

        ctx.current_window = nullptr;
        ctx.clip = false;
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
    }

    void beginPopup(const char* title, f32 w, f32 h, bool draggable)
    {
        Context& ctx = Context::get();
        beginWindow(title, ctx.screen_width * 0.5f, ctx.screen_height * 0.5f, w, h, draggable);
        bringCurrentWindowToFront();
    }

    void endPopup() { endWindow(); }

    bool button(const char* label, f32 x, f32 y, f32 w, f32 h)
    {
        // Adjust to top left coordinate sytem
        x -= (w / 2.0f);
        y -= (h / 2.0f);

        Context& ctx = Context::get();
        ParsedLabel parsed = parseLabel(label);
        u32 id = internal::hashString(parsed.id_part);

        const Rect rect = { x, y, w, h };
        const bool is_hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, rect) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

        // Handle interaction
        bool clicked = false;
        if (is_hovered && ctx.active_id == 0) // Only interact if nothing else is active
        {
            ctx.hot_id = id;
            if (ctx.input.mouse_down[0])
            {
                ctx.is_using_cursor = true;
                clicked = true;
                ctx.active_id = id;
            }
        }

        if (!ctx.input.mouse_down[0] && ctx.active_id == id)
        {
            ctx.active_id = 0;
        }

        // Render
        Colors::Color color = Colors::Button;
        if (ctx.active_id == id)
            color = Colors::ButtonClicked;
        else if (ctx.hot_id == id && is_hovered)
            color = Colors::ButtonHover;

        f32 text_width = internal::getTextWidth(parsed.visible);
        f32 text_height = s_context->text_size / 2.0f;

        f32 text_x = x + (w - text_width) * 0.5f;
        f32 text_y = y + (h + text_height) * 0.5f;

        internal::renderRect(rect, color, ctx.style.corner_radius);
        internal::renderText(parsed.visible, text_x, text_y, Colors::Text);

        return clicked;
    }

    bool checkbox(const char* label, bool* value, f32 x, f32 y, f32 size, f32 corner_radius)
    {
        // Adjust to top left coordinate sytem
        x -= (size / 2.0f);
        y -= (size / 2.0f);

        Context& ctx = Context::get();
        ParsedLabel parsed = parseLabel(label);
        u32 id = internal::hashString(parsed.id_part);
        const Rect box_rect = { x, y, size, size };
        const Rect label_rect = { x + size + 8.0f, y + (size / 2.0f) + (s_context->text_size / 4.0f), size - size - 8.0f, size };

        const bool is_hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, box_rect) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

        bool toggled = false;

        // Interaction
        if (is_hovered && ctx.active_id == 0) // Only interact if nothing else is active
        {
            ctx.hot_id = id;
            if (ctx.input.mouse_down[0])
            {
                ctx.is_using_cursor = true;
                ctx.active_id = id;
                *value = !(*value);
                toggled = true;
            }
        }

        if (!ctx.input.mouse_down[0] && ctx.active_id == id)
        {
            ctx.active_id = 0;
        }

        // Style
        Colors::Color box_color = Colors::Checkbox;
        if (ctx.active_id == id)
            box_color = Colors::CheckboxClicked;
        else if (ctx.hot_id == id && is_hovered)
            box_color = Colors::CheckboxHover;

        internal::renderRect(box_rect, box_color, corner_radius != -1.0f ? corner_radius : ctx.style.corner_radius);

        if (*value)
        {
            renderCheckmark(x, y, size, Colors::White);
        }

        internal::renderText(parsed.visible, label_rect.x, label_rect.y, Colors::Text);

        return toggled;
    }

    int radioButtons(const char** labels, bool* values, int count, f32 x, f32* y_positions, f32 size)
    {
        Context& ctx = Context::get();
        int retval = -1;
        f32 half_size = size * 0.5f;

        // Adjust to top left coordinate sytem
        x -= (half_size);

        for (int i = 0; i < count; ++i)
        {
            y_positions[i] -= (half_size);
        }

        for (int i = 0; i < count; ++i)
        {
            ParsedLabel parsed = parseLabel(labels[i]);
            u32 id = internal::hashString(parsed.id_part);
            const Rect box_rect = { x, y_positions[i], size, size};
            const Rect label_rect = { x + size + 8.0f, y_positions[i] + (half_size) + (s_context->text_size / 4.0f), size - size - 8.0f, size};

            const bool is_hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, box_rect) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

            // Interaction
            if (is_hovered && ctx.active_id == 0) // Only interact if nothing else is active
            {
                ctx.hot_id = id;
                if (ctx.input.mouse_down[0])
                {
                    ctx.is_using_cursor = true;
                    ctx.active_id = id;
                    for (int j = 0; j < count; ++j) { values[j] = false; }
                    values[i] = !(values[i]);
                    retval = i;
                }
            }

            if (!ctx.input.mouse_down[0] && ctx.active_id == id)
            {
                ctx.active_id = 0;
            }

            // Style
            Colors::Color box_color = Colors::Checkbox;
            if (ctx.active_id == id)
                box_color = Colors::CheckboxClicked;
            else if (ctx.hot_id == id && is_hovered)
                box_color = Colors::CheckboxHover;

            internal::renderRect(box_rect, box_color, 12.0f);

            if (values[i])
            {
                f32 quarter_size = half_size * 0.5f;
                internal::renderRect({ x + quarter_size, y_positions[i] + quarter_size, size * 0.5f, size * 0.5f }, Colors::White, 12.0f);
            }

            internal::renderText(parsed.visible, label_rect.x, label_rect.y, Colors::Text);
        }

        return retval;
    }

    bool sliderFloat(const char* label, f32* value, f32 min, f32 max, f32 x, f32 y, f32 w, f32 h, f32 corner_radius)
    {
        // Adjust to top-left coordinate system
        x -= (w / 2.0f);
        y -= (h / 2.0f);

        Context& ctx = Context::get();
        ParsedLabel parsed = parseLabel(label);
        u32 id = internal::hashString(parsed.id_part);

        corner_radius = (corner_radius != -1.0f) ? corner_radius : ctx.style.corner_radius;

        const Rect slider_rect = { x, y, w, h };
        const bool is_hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, slider_rect) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

        // Knob dimensions
        f32 knob_width = h / 2.0f;
        f32 knob_half = knob_width * 0.5f;

        // Calculate current value fraction (0..1)
        f32 value_frac = std::clamp((*value - min) / (max - min), 0.0f, 1.0f);

        // Static so offset persists during drag
        static f32 grab_offset = 0.0f;

        // Start dragging
        if (is_hovered && ctx.input.mouse_down[0] && ctx.active_id == 0)
        {
            ctx.active_id = id;
            // Offset from mouse to knob center
            grab_offset = ctx.input.mouse_pos.x - (x + value_frac * (w - knob_width) + knob_half);
        }

        // Dragging logic
        if (ctx.active_id == id)
        {
            ctx.is_using_cursor = true;
            f32 rel_x = ctx.input.mouse_pos.x - x - grab_offset - knob_half;
            f32 t = std::clamp(rel_x / (w - knob_width), 0.0f, 1.0f);
            *value = min + t * (max - min);
            value_frac = t; // update for rendering
        }

        // Stop dragging
        if (!ctx.input.mouse_down[0] && ctx.active_id == id)
        {
            ctx.active_id = 0;
        }

        // Background bar
        internal::renderRect(slider_rect, Colors::Slider, corner_radius);

        // Knob position (centered on value_frac)
        Rect knob_rect = {
            x + value_frac * (w - knob_width),
            y,
            knob_width,
            h
        };
        internal::renderRect(knob_rect, is_hovered ? Colors::SliderHover : Colors::SliderKnob, corner_radius);

        // Draw label and value
        char value_str[32];
        snprintf(value_str, sizeof(value_str), "%.2f", *value);

        // Text positions
        f32 label_y = y - (h * 0.1f);
        f32 label_x = x + w * 0.5f - (w * 0.5f); // naive centering, replace if you have text width

        f32 value_x = x + w * 0.5f - (internal::getTextWidth(value_str) * 0.5f);
        f32 value_y = y + (h + (ctx.text_size * 0.5f)) * 0.5f;

        internal::renderText(parsed.visible, label_x, label_y, Colors::Text);
        internal::renderText(value_str, value_x, value_y, Colors::Text);

        return (ctx.active_id == id); // True if being dragged
    }

    bool textbox(const char* label, char* buffer, size_t buffer_size, f32 x, f32 y, f32 w, f32 h, f32 corner_radius)
    {
        Context& ctx = Context::get();

        ParsedLabel parsed = parseLabel(label);
        u32 id = internal::hashString(parsed.id_part);

        Textbox* txtbx = nullptr;
        for (auto& txt : textboxes)
            if (txt.label == parsed.visible && txt.id == id) { txtbx = &txt; break; }
        if (!txtbx)
        {
            Textbox box{};
            box.label = parsed.visible;
            box.id = id;
            textboxes.push_back(box);
            txtbx = &textboxes.back();
        }

        // Adjust coords to top-left system if needed
        x -= (w / 2.0f);
        y -= (h / 2.0f);

        corner_radius = (corner_radius != -1.0f) ? corner_radius : ctx.style.corner_radius;

        const Rect rect = { x, y, w, h };
        bool hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, rect) && (ctx.current_window == nullptr || windowCanReceiveInput(ctx.current_window));

        // make sure that text selection graphic doesn't go outside the textbox itself
        Rect before_clip_rect = ctx.current_clip;
        bool before_clip = ctx.clip;

        ctx.clip = true;
        ctx.current_clip = rect;

        if (!ctx.input.mouse_down[0])
        {
            txtbx->dragging = false;
        }

        // Focus on click
        if (hovered && ctx.input.mouse_down[0] && (ctx.active_id == 0 || ctx.active_id == id))
        {
            if (!txtbx->dragging)
            {
                // Position cursor based on mouse click
                f32 click_x = ctx.input.mouse_pos.x - (x + 5.0f) + txtbx->scroll_x;

                //f32 click_x = ctx.input.mouse_pos.x + txtbx->scroll_x - (x + 5.0f); // 5px padding
                size_t pos = 0;
                f32 accum_x = 0;
                size_t len = strlen(buffer);

                for (; pos < len; pos++)
                {
                    accum_x += ctx.glyphs[(u8)buffer[pos]].xadvance;
                    if (accum_x > click_x)
                        break;
                }
                txtbx->cursor_pos = pos;
                txtbx->selection_start = txtbx->selection_end = pos;
            }

            txtbx->has_focus = true;

            // Position cursor based on mouse click
            f32 click_x = ctx.input.mouse_pos.x - (x + 5.0f) + txtbx->scroll_x;

            //f32 click_x = ctx.input.mouse_pos.x + txtbx->scroll_x - (x + 5.0f); // 5px padding
            size_t pos = 0;
            f32 accum_x = 0;
            size_t len = strlen(buffer);

            for (; pos < len; pos++)
            {
                accum_x += ctx.glyphs[(u8)buffer[pos]].xadvance;
                if (accum_x > click_x)
                    break;
            }
            txtbx->cursor_pos = pos;
            txtbx->selection_end = pos;
            txtbx->dragging = true;

            ctx.active_id = id;
        }
        else if (ctx.input.mouse_down[0] && !hovered)
        {
            if (!txtbx->dragging)
            {
                txtbx->has_focus = false;

                if (ctx.active_id == id)
                {
                    ctx.active_id = 0;
                }
            }
        }

        if (txtbx->dragging)
        {
            ctx.is_using_cursor = true;
        }

        // Only accept input if focused
        if (txtbx->has_focus)
        {
            size_t len = strlen(buffer);

            auto delete_selection = [&]() {
                if (txtbx->selection_start == txtbx->selection_end)
                    return false;
                size_t start = std::min(txtbx->selection_start, txtbx->selection_end);
                size_t end = std::max(txtbx->selection_start, txtbx->selection_end);
                memmove(buffer + start, buffer + end, len - end + 1);
                txtbx->cursor_pos = start;
                txtbx->selection_start = txtbx->selection_end = start;
                return true;
            };

            // Clipboard shortcuts
            if (ctx.input.key_ctrl)
            {
                if (ctx.input.key_a) // Select all
                {
                    txtbx->selection_start = 0;
                    txtbx->selection_end = len;
                    txtbx->cursor_pos = len;
                }
                if (ctx.input.key_c && txtbx->selection_start != txtbx->selection_end)
                {
                    size_t start = std::min(txtbx->selection_start, txtbx->selection_end);
                    size_t end = std::max(txtbx->selection_start, txtbx->selection_end);
                    internal::setClipboardText(std::string(buffer + start, end - start));
                }
                if (ctx.input.key_x && txtbx->selection_start != txtbx->selection_end)
                {
                    size_t start = std::min(txtbx->selection_start, txtbx->selection_end);
                    size_t end = std::max(txtbx->selection_start, txtbx->selection_end);
                    internal::setClipboardText(std::string(buffer + start, end - start));
                    delete_selection();
                }
                if (ctx.input.key_v)
                {
                    std::string paste = internal::getClipboardText();
                    if (!paste.empty())
                    {
                        delete_selection();
                        size_t paste_len = paste.size();
                        if (len + paste_len >= buffer_size)
                            paste_len = buffer_size - len - 1;
                        memmove(buffer + txtbx->cursor_pos + paste_len, buffer + txtbx->cursor_pos, len - txtbx->cursor_pos + 1);
                        memcpy(buffer + txtbx->cursor_pos, paste.c_str(), paste_len);
                        txtbx->cursor_pos += paste_len;
                        txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
                    }
                }
            }

            // Backspace
            if (ctx.input.key_backspace)
            {
                if (!delete_selection() && txtbx->cursor_pos > 0)
                {
                    memmove(buffer + txtbx->cursor_pos - 1, buffer + txtbx->cursor_pos, len - txtbx->cursor_pos + 1);
                    txtbx->cursor_pos--;
                    txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
                }
            }

            // Delete
            if (ctx.input.key_delete)
            {
                if (!delete_selection() && txtbx->cursor_pos < len)
                {
                    memmove(buffer + txtbx->cursor_pos, buffer + txtbx->cursor_pos + 1, len - txtbx->cursor_pos);
                    txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
                }
            }

            // Arrow keys
            if (ctx.input.key_left)
            {
                if (txtbx->cursor_pos > 0) txtbx->cursor_pos--;
                if (ctx.input.key_shift) txtbx->selection_end = txtbx->cursor_pos;
                else txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
            }
            if (ctx.input.key_right)
            {
                if (txtbx->cursor_pos < len) txtbx->cursor_pos++;
                if (ctx.input.key_shift) txtbx->selection_end = txtbx->cursor_pos;
                else txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
            }

            // Text input
            if (ctx.input.input_char >= 32 && ctx.input.input_char < 127)
            {
                delete_selection();
                if (len + 1 < buffer_size)
                {
                    memmove(buffer + txtbx->cursor_pos + 1, buffer + txtbx->cursor_pos, len - txtbx->cursor_pos + 1);
                    buffer[txtbx->cursor_pos] = ctx.input.input_char;
                    txtbx->cursor_pos++;
                    txtbx->selection_start = txtbx->selection_end = txtbx->cursor_pos;
                }
            }

            // Clamp cursor
            len = strlen(buffer);
            txtbx->cursor_pos = std::min(txtbx->cursor_pos, len);
            txtbx->selection_start = std::min(txtbx->selection_start, len);
            txtbx->selection_end = std::min(txtbx->selection_end, len);

            // Horizontal scrolling
            f32 caret_x = 0.0f;
            for (size_t i = 0; i < txtbx->cursor_pos; i++)
                caret_x += ctx.glyphs[(u8)buffer[i]].xadvance;

            if (caret_x - txtbx->scroll_x > w - 10)
                txtbx->scroll_x = caret_x - w + 10;
            else if (caret_x - txtbx->scroll_x < 0)
                txtbx->scroll_x = caret_x;

            if (txtbx->scroll_x < 0) txtbx->scroll_x = 0;

            // Draw background
            internal::renderRect(rect, Colors::TextboxFocused, corner_radius);

            // Draw selection
            if (txtbx->selection_start != txtbx->selection_end)
            {
                size_t sel_start = std::min(txtbx->selection_start, txtbx->selection_end);
                size_t sel_end = std::max(txtbx->selection_start, txtbx->selection_end);
                f32 sel_x = x + 5.0f - txtbx->scroll_x;
                for (size_t i = 0; i < sel_start; i++)
                    sel_x += ctx.glyphs[(u8)buffer[i]].xadvance;

                f32 sel_w = 0.0f;
                for (size_t i = sel_start; i < sel_end; i++)
                    sel_w += ctx.glyphs[(u8)buffer[i]].xadvance;

                internal::renderRect({ sel_x, y + 4.0f, sel_w, h - 8.0f }, Colors::TextboxSelection, 0.0f);
            }

            f32 padding = 4.0f; // left/right padding
            f32 max_width = w - padding * 2; // visible space inside textbox

            const char* full_text = buffer;
            f32 text_width = internal::getTextWidth(full_text);

            const char* render_start = full_text;

            // If the text is too wide, shift the start pointer so last part fits
            if (text_width > max_width) {
                f32 accumulated = 0.0f;
                const char* p = full_text + strlen(full_text);
                while (p > full_text) {
                    unsigned char c = *(p - 1);
                    if (c >= 32 && c < 128) {
                        f32 cw = ctx.glyphs[c].xadvance;
                        if (accumulated + cw > max_width) break;
                        accumulated += cw;
                    }
                    --p;
                }
                render_start = p;
            }

            internal::renderText(render_start, x + padding, y + ctx.text_size, Colors::Text);

            // Draw caret
            if (txtbx->has_focus && txtbx->caret_visible)
            {
                f32 caret_draw_x = x + 5.0f - txtbx->scroll_x;
                for (size_t i = 0; i < txtbx->cursor_pos; i++)
                    caret_draw_x += ctx.glyphs[(u8)buffer[i]].xadvance;

                internal::renderRect({ caret_draw_x, y + 4.0f, 1.0f, h - 8.0f }, Colors::Text, 0.0f);
            }

            f32 label_y = y - (h * 0.1f);
            f32 label_x = x + w * 0.5f - (w * 0.5f);

            internal::renderText(txtbx->label, label_x, label_y, Colors::Text);

            // restore state
            ctx.clip = before_clip;
            ctx.current_clip = before_clip_rect;

            return true;
        }
        else
        {
            // Inactive box
            internal::renderRect(rect, hovered ? Colors::TextboxHover : Colors::TextboxUnfocused, corner_radius);

            f32 label_y = y - (h * 0.1f);
            f32 label_x = x + w * 0.5f - (w * 0.5f);

            f32 padding = 4.0f; // left/right padding
            f32 max_width = w - padding * 2; // visible space inside textbox

            const char* full_text = buffer;
            f32 text_width = internal::getTextWidth(full_text);

            const char* render_start = full_text;

            // If the text is too wide, shift the start pointer so last part fits
            if (text_width > max_width) {
                f32 accumulated = 0.0f;
                const char* p = full_text + strlen(full_text);
                while (p > full_text) {
                    unsigned char c = *(p - 1);
                    if (c >= 32 && c < 128) {
                        f32 cw = ctx.glyphs[c].xadvance;
                        if (accumulated + cw > max_width) break;
                        accumulated += cw;
                    }
                    --p;
                }
                render_start = p;
            }

            internal::renderText(render_start, x + padding, y + ctx.text_size, Colors::Text);
            internal::renderText(txtbx->label, label_x, label_y, Colors::Text);

            // restore state
            ctx.clip = before_clip;
            ctx.current_clip = before_clip_rect;

            return false;
        }
    }

    void menuBar(f32 height_px)
    {
        Context& ctx = Context::get();
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_MENU;
        internal::renderRect({ 0.0f, 0.0f, ctx.screen_width, height_px }, Colors::MenuBar, 0.0f);
        ctx.current_menu_bar.height = height_px;
        ctx.current_menu_bar.total_size_x = 0.0f;
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
    }

    // TODO: current menu bar struct
    bool menuBarItem(const char* text, f32* x_offset)
    {
        Context& ctx = Context::get();
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_MENU;

        const f32 x_size = internal::getTextWidth(text) + ctx.style.menu_bar_margin;
        const u32 id = internal::hashString(text);
        Rect rect = { ctx.current_menu_bar.total_size_x + ctx.style.menu_bar_margin, 0.0f, x_size, ctx.current_menu_bar.height };
        const bool is_hovered = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, rect);

        // Handle interaction
        bool clicked = false;
        if (is_hovered && ctx.active_id == 0) // Only interact if nothing else is active
        {
            ctx.hot_id = id;
            if (ctx.input.mouse_down[0])
            {
                ctx.is_using_cursor = true;
                clicked = true;
                ctx.active_id = id;
            }
        }

        if (!ctx.input.mouse_down[0] && ctx.active_id == id)
        {
            ctx.active_id = 0;
        }

        // Render
        Colors::Color color = Colors::MenuBarItem;
        if (ctx.active_id == id)
            color = Colors::MenuBarItemClicked;
        else if (ctx.hot_id == id && is_hovered)
            color = Colors::MenuBarItemHover;

        f32 text_width = internal::getTextWidth(text);
        f32 text_height = s_context->text_size / 2.0f;

        f32 text_x = rect.x + (rect.w - text_width) * 0.5f;
        f32 text_y = rect.y + (rect.h + text_height) * 0.5f;

        internal::renderRect(rect, color, 0.0f);
        internal::renderText(text, text_x, text_y, Colors::Text);

        ctx.current_menu_bar.total_size_x += x_size;

        if (x_offset)
        {
            *x_offset = rect.x;
        }

        ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
        return clicked;
    }

    void textblock(const char* text, f32 x, f32 y)
    {
        Context& ctx = Context::get();

        // Adjust coords to top-left system if needed
        x -= (internal::getTextWidth(text) / 2.0f);
        y -= (ctx.text_size / 2.0f);

        internal::renderText(text, x, y, Colors::Text);
    }

    void bringCurrentWindowToFront()
    {
        Context& ctx = Context::get();

        for (auto& wdw : windows)
        {
            wdw.focused = false;
        }
        ctx.current_window->focused = true;
        
        std::vector<Window>::iterator it = std::find(windows.begin(), windows.end(), *ctx.current_window);

        // assuming current window is not null, that would be strange
        // reorder current window to the last one so it's drawn last
        Window tmp = *it;
        windows.erase(it);
        windows.push_back(tmp);
    }

    i32 menu(MenuItem* items, i32 count, f32 x, f32 y, i32 depth)
    {
        Context& ctx = Context::get();
        ctx.commandRecorder = XGUI_COMMAND_RECORDER_MENU;

        const f32 itemHeight = 22.0f;
        f32 menuWidth = 0.0f;

        for (i32 i = 0; i < count; ++i)
        {
            const MenuItem& item = items[i];
            f32 width = internal::getTextWidth(item.label);
            if (item.children)
            {
                width += 10.0f;
            }

            if (width > menuWidth)
            {
                menuWidth = width;
            }
        }

        i32 clickedIndex = -1;

        for (i32 i = 0; i < count; ++i)
        {
            MenuItem& item = items[i];
            Rect itemRect = { x, y + (i * itemHeight), menuWidth + 12.0f, itemHeight };

            ParsedLabel parsed = parseLabel(item.label);
            u32 id = internal::hashString(parsed.id_part);

            // Background
            bool hover = internal::isPointInRect(ctx.input.mouse_pos.x, ctx.input.mouse_pos.y, itemRect);
            if (hover && ctx.active_id == 0)
            {
                ctx.hot_id = id;
                internal::renderRect(itemRect, Colors::MenuHover, 0.0f);
            }
            else
            {
                internal::renderRect(itemRect, Colors::Menu, 0.0f);
            }

            internal::renderText(item.label, x + 6.0f, itemRect.y + itemHeight - 6.0f, Colors::White);

            if (item.children && item.child_count > 0)
            {
                renderArrowRight(x + menuWidth, itemRect.y + 6.0f, 10.0f, Colors::White);
            }

            if (item.children && item.is_open)
            {
                i32 sub = menu(item.children, item.child_count, x + menuWidth + 12.0f, itemRect.y, depth + 1);
                ctx.commandRecorder = XGUI_COMMAND_RECORDER_MENU;
                if (sub != -1)
                {
                    item.is_open = false;
                    ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
                    ctx.active_id = 0;
                    return (i32)((i << 8) | (sub != 0 ? sub /= 256 : sub));
                }
            }
            else if (hover)
            {
                for (i32 j = 0; j < count; ++j)
                {
                    items[j].is_open = false;   
                }

                if (item.children)
                {
                    item.is_open = true;
                    for (i32 i = 0; i < item.child_count; ++i) { item.children[i].is_open = false; }
                }
                else
                {
                    if (ctx.input.mouse_down[0])
                    {
                        ctx.is_using_cursor = true;
                        ctx.active_id = 0;
                        ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
                        return (i32)((i << 8) | 0);
                    }
                }
            }
        }

        ctx.commandRecorder = XGUI_COMMAND_RECORDER_DEFAULT;
        return clickedIndex;
    }

    void imageView(u32 gl_id, f32 x, f32 y, f32 w, f32 h, f32 u0, f32 v0, f32 u1, f32 v1)
    {
        // Adjust to top left coordinate sytem
        x -= (w / 2.0f);
        y -= (h / 2.0f);
        x = floor(x);
        y = floor(y);
        renderImageView(gl_id, x, y, w, h, u0, v0, u1, v1);
    }

    char* filePicker(const char* title, const char* filter)
    {
        return internal::openFileDialog(title, filter);
    }

    char* saveFilePicker(const char* title, const char* filter, const char* defaultExtension)
    {
        return internal::saveFileDialog(title, filter, defaultExtension);
    }

    namespace internal
    {
        f32 getTextWidth(const char* text)
        {
            Context& ctx = Context::get();
            f32 width = 0.0f;
            for (const char* p = text; *p; p++)
            {
                u8 c = static_cast<u8>(*p);
                if (c < 32 || c >= 128) continue;
                width += ctx.glyphs[c].xadvance;
            }
            return width;
        }

#ifdef _WIN32
#undef APIENTRY
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #include <commdlg.h> // file picker
        
        std::string xgui::internal::getClipboardText()
        {
            if (!OpenClipboard(nullptr)) return "";

            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData == nullptr)
            {
                CloseClipboard();
                return "";
            }

            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText == nullptr) {
                CloseClipboard();
                return "";
            }

            std::string text(pszText);
            GlobalUnlock(hData);
            CloseClipboard();

            return text;
        }

        void setClipboardText(const std::string& text)
        {
            if (!OpenClipboard(nullptr)) return;

            EmptyClipboard();
            HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
            if (hGlob != nullptr)
            {
                char* pBuf = static_cast<char*>(GlobalLock(hGlob));
                if (pBuf != nullptr)
                {
                    strcpy(pBuf, text.c_str());
                    GlobalUnlock(hGlob);
                    SetClipboardData(CF_TEXT, hGlob);
                }
            }
            CloseClipboard();
        }

        static char file_dialog_filename[MAX_PATH] = {};

        char* openFileDialog(const char* title, const char* filter)
        {
            OPENFILENAMEA ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = file_dialog_filename;
            ofn.nMaxFile = MAX_PATH;

            file_dialog_filename[0] = '\0';
            ofn.lpstrFilter = filter;

            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            ofn.lpstrTitle = title;

            if (GetOpenFileNameA(&ofn))
                return file_dialog_filename;

            return NULL;
        }

        char* saveFileDialog(const char* title, const char* filter, const char* defaultExtension)
        {
            OPENFILENAMEA ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = file_dialog_filename;
            ofn.nMaxFile = MAX_PATH;

            file_dialog_filename[0] = '\0';

            ofn.lpstrFilter = filter;
            ofn.lpstrTitle = title;
            ofn.lpstrDefExt = defaultExtension;

            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

            if (GetSaveFileNameA(&ofn))
                return file_dialog_filename;

            return nullptr;
        }
#endif 

        u32 hashString(const char* str)
        {
            // Simple hash function
            u32 hash = 2166136261u;
            while (*str)
            {
                hash ^= *str++;
                hash *= 16777619u;
            }
            return hash;
        }

        bool isPointInRect(f32 x, f32 y, const Rect& rect)
        {
            return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
        }

        void renderRect(const Rect& rect, const Colors::Color& color, f32 corner_radius)
        {
            xgui_render_command cmd{};
            cmd.rect = rect;
            cmd.color = color;
            cmd.submission_id = global_submission_counter++;
            cmd.corner_radius = corner_radius;
            cmd.render_type = XGUI_RENDER_TYPE_RECT;

            auto& ctx = Context::get();
            cmd.clip_rect = ctx.current_clip;
            cmd.use_clip = ctx.clip;

            pushCommand(cmd);
        }

        void renderText(const char* text, f32 x, f32 y, const Colors::Color& color)
        {
            xgui_render_command cmd{};
            cmd.x = x;
            cmd.y = y;
            cmd.submission_id = global_submission_counter++;
            cmd.text = std::string(text);
            cmd.color = color;
            cmd.render_type = XGUI_RENDER_TYPE_TEXT;

            auto& ctx = Context::get();
            cmd.clip_rect = ctx.current_clip;
            cmd.use_clip = ctx.clip;
            cmd.size = ctx.text_size;

            pushCommand(cmd);
        }
    }
}
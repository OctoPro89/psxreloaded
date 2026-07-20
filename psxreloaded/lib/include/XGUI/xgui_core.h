#ifndef XGUI_CORE_H
#define XGUI_CORE_H

#include "xgui.h"
#include <XGUI/xgui_font.h>
#include <XGUI/xgui_image.h>
#include <XGUI/xgui_colors.h>

#include <vector>
#include <string>
#include <functional>

namespace xgui
{
    // Forward declarations
    struct Context;
    struct Window;
    struct xgui_render_command;

    struct Rect { f32 x, y, w, h; };
    struct Vec2
    {
        f32 x, y;
        Vec2(f32 _x, f32 _y) : x(_x), y(_y) {}
        Vec2() : x(0.0f), y(0.0f) {}
    };

    // Style settings
    struct Style
    {
        f32 corner_radius = 4.0f;
        f32 menu_bar_margin = 8.0f;
        f32 menu_padding = 8.0f;
    };

    // Input state
    struct InputState
    {
        Vec2 mouse_pos;
        f32 mouse_wheel = 0.0f;
        bool mouse_down[3] = { false, false, false }; // Left, Right, Middle
        bool __mouse_clicked[3] = { false, false, false };
        char input_char = 0;
        bool key_enter = false;
        bool key_backspace = false;
        bool key_escape = false;
        bool key_ctrl = false;      // For clipboard shortcuts
        bool key_v = false;        // Paste
        bool key_c = false;        // Copy
        bool key_x = false;        // Cut
        bool key_a = false;        // Select All
        bool key_delete = false;
        bool key_left = false;     // Cursor movement
        bool key_right = false;
        bool key_home = false;
        bool key_end = false;
        bool key_shift = false;    // For selection
    };

    struct DockPoint
    {
        f32 x_percent, y_percent;
        f32 grow_percent_x, grow_percent_y;
        const char* dock_text;
    };

    // UI context (singleton)
    struct XAPI Context
    {
        Style style;
        InputState input;
        u32 hot_id = 0;
        u32 active_id = 0;
        Window* current_window = nullptr;

        std::vector<DockPoint> dock_points;

        // Rendering state
        u32 sdf_button_shader_program = 0;
        u32 sdf_text_shader_program = 0;
        u32 primitive_shader_program = 0;
        u32 image_shader_program = 0;

        u32 sdf_button_vao = 0;
        u32 sdf_button_vbo = 0;
        u32 sdf_button_ebo = 0;

        u32 sdf_text_vao = 0;
        u32 sdf_text_vbo = 0;
        u32 sdf_text_ebo = 0;

        u32 primitive_vao = 0;
        u32 primitive_vbo = 0;
        u32 primitive_ebo = 0;

        u32 image_vao = 0;
        u32 image_vbo = 0;
        u32 image_ebo = 0;

        // Matrix properties
        f32 screen_width = 800.0f;
        f32 screen_height = 600.0f;
        
        struct
        {
            bool values[4];
            i32 keys[4];

            inline bool get_value(i32 key) const
            {
                for (u8 i = 0; i < 4; ++i)
                {
                    if (keys[i] == key) { return values[i]; }
                }

                // this shouldn't happen
                __debugbreak();
                return false;
            }

            inline void set_value(i32 key, bool value)
            {
                for (u8 i = 0; i < 4; ++i)
                {
                    if (keys[i] == key) { values[i] = value; return; }
                }

                __debugbreak();

            }
        } projection_updated_map;

        u32 font_texture = 0;
        font::GlyphInfo glyphs[128];
        f32 text_size = 20.0f;

        u32 textbox_focus_id = 0;
        std::string textbox_buffer;
        size_t cursor_pos = 0;
        size_t selection_start = 0;
        bool is_using_cursor = false;

        bool clip = false;
        Rect current_clip{};

        struct
        {
            f32 height;
            f32 total_size_x;
        } current_menu_bar;

        // Initialize the context
        static bool init();
        static void shutdown();
        static Context& get();
        static inline void addDockPoint(f32 x_percent, f32 y_percent, const char* dock_text = "Dock")
        {
            get().dock_points.push_back({ x_percent, y_percent, 0.0f, 0.0f, dock_text });
        }
    private:
        Context() = default;
        ~Context() = default;
    };

    // Initialize the UI system
    XAPI bool init();

#ifdef XGUIBUILDDYNAMIC
    // Loads OpenGL functions in a DLL context, MUST be called before init if using XGUI.dll
    XAPI bool loadDylibGLFuncs();
#endif // XGUIBUILDDYNAMIC

    // Shutdown the UI system
    XAPI void shutdown();

    // Begin a new frame (call at start of frame)
    XAPI void beginFrame(const InputState& input);

    // End frame (call at end of frame)
    XAPI void endFrame();

    // Begin a new window
    XAPI void beginWindow(const char* title, f32 x, f32 y, f32 w, f32 h, bool draggable = true);

    // End current window
    XAPI void endWindow();

    // Begin a new popup window
    XAPI void beginPopup(const char* title, f32 w, f32 h, bool draggable = true);

    // End current popup window
    XAPI void endPopup();

    // Button control
    XAPI bool button(const char* label, f32 x, f32 y, f32 w = 100.0f, f32 h = 30.0f);

    // Checkbox control
    XAPI bool checkbox(const char* label, bool* value, f32 x, f32 y, f32 size, f32 corner_radius = -1.0f);

    // Radio button control, returns index of selected radio, -1 if none
    XAPI int radioButtons(const char** labels, bool* values, int count, f32 x, f32* y_positions, f32 size);

    // Slider control
    XAPI bool sliderFloat(const char* label, f32* value, f32 min, f32 max, f32 x, f32 y, f32 w, f32 h, f32 corner_radius = -1.0f);

    // Textbox control
    XAPI bool textbox(const char* label, char* buffer, size_t buffer_size, f32 x, f32 y, f32 w, f32 h, f32 corner_radius = -1.0f);

    // Menu bar control
    XAPI void menuBar(f32 height_px);

    // Menu bar item control
    XAPI bool menuBarItem(const char* text, f32* out_x_offset = nullptr);

    // Textblock control
    XAPI void textblock(const char* text, f32 x, f32 y);

    // Helpers
    XAPI void bringCurrentWindowToFront();

    struct MenuItem
    {
        const char* label;
        MenuItem* children;   // nullptr if no submenu
        i32 child_count = 0;
        bool is_open = false;
    };

    // Menu control
    XAPI i32 menu(MenuItem* items, i32 count, f32 x, f32 y, i32 depth = 0);

    // Render image
    XAPI void imageView(u32 gl_id, f32 x, f32 y, f32 w, f32 h);

    // Helper functions
    namespace internal
    {
        XAPI f32 getTextWidth(const char* text);

        XAPI std::string getClipboardText();
        XAPI void setClipboardText(const std::string& text);

        XAPI u32 hashString(const char* str);
        XAPI bool isPointInRect(f32 x, f32 y, const Rect& rect);

        XAPI void renderRect(const Rect& rect, const Color& color, f32 corner_radius = 0.0f);
        XAPI void renderText(const char* text, f32 x, f32 y, const Color& color);
    }

    struct Window
    {
        std::string title;
        u32 id; // from title
        Rect rect;
        Rect old_rect;
        bool docked;
        bool draggable;
        bool dragging = false;
        bool focused = false;
        Vec2 drag_offset;
        f32 scroll_y = 0.0f;
        f32 content_height = 0.0f;
        std::vector<xgui_render_command> commands;

        bool operator==(const Window& other) const {
            // simple ID check
            return this->id == other.id;
        }
    };

    // Used for making individual UI layouts
    struct XAPI UILayout
    {
        f32 start_x, start_y; // in pixels
        f32 width;            // in pixels
        f32 current_y;        // tracks where next element goes
        f32 spacing;          // space between elements

        inline void UIbegin(f32 x_percent, f32 y_percent, f32 width_percent, f32 spacing_px)
        {
            Context& ctx = Context::get();
            start_x = x_percent * ctx.screen_width;
            start_y = y_percent * ctx.screen_height;
            width = width_percent * ctx.screen_width;
            current_y = start_y;
            spacing = spacing_px;
        }

        inline bool UIbutton(const char* label, f32 height_px)
        {
            bool pressed = button(label, start_x, current_y, width, height_px);
            current_y += height_px + spacing;
            return pressed;
        }

        inline bool UIslider(const char* label, f32* value, f32 min, f32 max, f32 height_px, f32 corner_radius = -1.0f)
        {
            bool changed = sliderFloat(label, value, min, max, start_x, current_y, width, height_px, corner_radius);
            current_y += height_px + spacing;
            return changed;
        }

        inline void UIend() {}
    };

#define MAX_ROW_BUTTONS 12

    struct WindowedUILayout
    {
        f32 start_x, start_y; // inside window
        f32 client_x, client_y, client_w, client_h;
        f32 width;
        f32 current_y;
        f32 spacing;
        f32 current_x;
        f32 layout_origin_y;
        bool horizontal = false;
        Window* win;

        inline void UIbegin(f32 width_percent, f32 spacing_px)
        {
            Context& ctx = Context::get();
            win = ctx.current_window;
            if (!win) return; // Possibly assert here

            f32 title_bar_height = 22.0f; // match your window's title bar
            client_x = win->rect.x;
            client_y = win->rect.y + title_bar_height;
            client_w = win->rect.w;
            client_h = win->rect.h - title_bar_height;

            layout_origin_y = client_y + spacing_px * 2.0f;

            start_x = client_x + (client_w / 2.0f);
            start_y = layout_origin_y - win->scroll_y;
            width = width_percent * client_w;
            current_y = start_y;
            spacing = spacing_px;
        }

        struct RowButton
        {
            const char* label;
            f32 width;
            bool* out_pressed; // pointer to user bool
        };

        inline void UIrowButtons(const RowButton* buttons, u32 count, f32 height_px = 30.0f, f32 row_spacing = -1.0f)
        {
            Context::get().clip = true;

            if (!win || count == 0) return;

            if (count > MAX_ROW_BUTTONS) count = MAX_ROW_BUTTONS;

            f32 spacing_px = (row_spacing > 0.0f ? row_spacing : spacing);

            // Compute total row width
            f32 totalWidth = spacing_px * (count - 1);
            for (u32 i = 0; i < count; i++)
                totalWidth += buttons[i].width;

            // Left edge of first button (centered row)
            f32 x = start_x - (totalWidth * 0.5f);

            // Draw buttons
            for (u32 i = 0; i < count; i++)
            {
                bool pressed = button(buttons[i].label, x + buttons[i].width * 0.5f, current_y, buttons[i].width, height_px);

                if (buttons[i].out_pressed)
                    *buttons[i].out_pressed = pressed;

                x += buttons[i].width + spacing_px;
            }

            current_y += height_px + spacing;
            win->content_height = std::max(win->content_height, (current_y + win->scroll_y) - client_y);
        }

        inline bool UIbutton(const char* label, f32 height_px) 
        {
            Context::get().clip = true;

            if (horizontal)
            {
                bool pressed = button(label, current_x + (width * 0.5f), current_y, width * 0.3f, height_px);
                current_x += width * 0.32f;
                return pressed;
            }

            bool pressed = button(label, start_x, current_y, width, height_px);
            current_y += height_px + spacing;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
            return pressed;
        }

        inline void UItext(const char* text, f32 height_px)
        {
            Context::get().clip = true;

            internal::renderText(text, start_x - (width * 0.5f), current_y, Colors::Text);
            current_y += height_px + spacing;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
        }

        inline bool UIslider(const char* label, f32* value, f32 min, f32 max, f32 height_px)
        {
            Context::get().clip = true;

            bool changed = sliderFloat(label, value, min, max, start_x, current_y, width, height_px);
            current_y += height_px + spacing;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
            return changed;
        }

        inline bool UIcheckbox(const char* label, bool* value, f32 height_px)
        {
            Context::get().clip = true;

            bool changed = checkbox(label, value, start_x - (internal::getTextWidth(label) / 2.0f), current_y, height_px);
            current_y += height_px + spacing;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
            return changed;
        }

        inline bool UItextbox(const char* label, char* buffer, size_t buffer_size, f32 height_px)
        {
            Context::get().clip = true;

            bool changed = textbox(label, buffer, buffer_size, start_x, current_y, width, height_px);
            current_y += height_px + spacing;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
            return changed;
        }

        inline void UIimage(u32 gl_id, f32 width_px, f32 height_px, f32 xoff, f32 yoff)
        {
            Context::get().clip = true;

            imageView(gl_id, start_x + xoff, current_y + yoff, width_px, height_px);
            current_y += height_px + spacing + yoff;
            win->content_height = std::max(
                win->content_height,
                (current_y + win->scroll_y) - client_y
            );
        }

        inline void UIend() {}

        inline void UIbeginRow()
        {
            horizontal = true;
            current_x = start_x - (width * 0.5f);
        }

        inline void UIendRow()
        {
            horizontal = false;
            current_y += spacing + 30.0f;
        }
    };
}

#define XGUI_MENUBAR_PARENT(menubar_ret_val) ((selected >> 8) & 0xFF)
#define XGUI_MENUBAR_CHILD(menubar_ret_val) (selected & 0xFF)

#endif // XGUI_CORE_H
#ifndef XGUI_COLORS_H
#define XGUI_COLORS_H

#include <XGUI/xgui_common.h>

namespace xgui {
    namespace Colors {
        struct Color {
            f32 r, g, b, a;
        };

        // Global colors
        extern Color White;
        extern Color Text;

        // Button colors
        extern Color ButtonHover;
        extern Color ButtonClicked;
        extern Color Button;

        // Slider colors
        extern Color Slider;
        extern Color SliderHover;
        extern Color SliderClicked;
        extern Color SliderKnob;

        // Checkbox colors
        extern Color Checkbox;
        extern Color CheckboxHover;
        extern Color CheckboxClicked;

        // Textbox colors
        extern Color TextboxFocused;
        extern Color TextboxUnfocused;
        extern Color TextboxSelection;
        extern Color TextboxHover;

        // Menu bar colors
        extern Color MenuBar;
        extern Color MenuBarItem;
        extern Color MenuBarItemHover;
        extern Color MenuBarItemClicked;

        // Menu colors
        extern Color Menu;
        extern Color MenuHover;
        extern Color MenuClicked;
    }
}

#endif // XGUI_COLORS_H
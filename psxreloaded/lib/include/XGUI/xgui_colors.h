#ifndef XGUI_COLORS_H
#define XGUI_COLORS_H

#include <XGUI/xgui_common.h>

struct Color {
    f32 r, g, b, a;
};

namespace Colors {
    // Global colors
    static Color White{ 1.0f, 1.0f, 1.0f, 1.0f };
    static Color Text{ 1.0f, 1.0f, 1.0f, 1.0f };

    // Button colors
    static Color Button{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color ButtonHover{ 0.3f, 0.3f, 0.3f, 1.0f };
    static Color ButtonClicked{ 0.1f, 0.1f, 0.1f, 1.0f };

    // Slider colors
    static Color Slider{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color SliderHover{ 0.3f, 0.3f, 0.3f, 1.0f };
    static Color SliderClicked{ 0.1f, 0.1f, 0.1f, 1.0f };
    static Color SliderKnob{ 0.3f, 0.3f, 0.3f, 1.0f };

    // Checkbox colors
    static Color Checkbox{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color CheckboxHover{ 0.3f, 0.3f, 0.3f, 1.0f };
    static Color CheckboxClicked{ 0.1f, 0.1f, 0.1f, 1.0f };

    // Textbox colors
    static Color TextboxFocused{ 0.2f, 0.2f, 0.4f, 1.0f };
    static Color TextboxUnfocused{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color TextboxSelection{ 0.5f, 0.5f, 0.5f, 1.0f };
    static Color TextboxHover{ 0.3f, 0.3f, 0.3f, 1.0f };

    // Menu bar colors
    static Color MenuBar{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color MenuBarItem{ 0.2f, 0.2f, 0.2f, 1.0f };
    static Color MenuBarItemHover{ 0.3f, 0.3f, 0.3f, 1.0f };
    static Color MenuBarItemClicked{ 0.1f, 0.1f, 0.1f, 1.0f };
}

#endif // XGUI_COLORS_H
#pragma once

#include <XGUI/xgui_common.h>

namespace xgui
{
    namespace font
    {
        struct GlyphInfo {
            float u0, v0, u1, v1; // texture coords
            float xoff, yoff;     // offsets for positioning
            float xadvance;       // advance to next glyph
        };

        bool loadFontSDF(const char* filename, float pixelHeight, u32* outTexture, GlyphInfo glyphs[128]);
    }
}
#include <XGUI/xgui_font.h>

#define XTT_IMPLEMENTATION
#include <XGUI/xtruetype.h>

#include "../../platform/gl_loader.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace xgui
{
    namespace font
    {
        bool loadFontSDF(const char* filename, f32 pixelHeight,
            u32* outTexture, GlyphInfo glyphs[128])
        {
            FILE* f = fopen(filename, "rb");
            if (!f) return false;

            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            rewind(f);

            unsigned char* ttf_buffer = (unsigned char*)malloc(size);
            fread(ttf_buffer, size, 1, f);
            fclose(f);

            xtt_fontinfo font;
            if (!xtt_InitFont(&font, ttf_buffer, xtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
                free(ttf_buffer);
                return false;
            }

            // Allocate atlas memory
            const int atlasWidth = 512;
            const int atlasHeight = 512;
            unsigned char* atlasPixels = (unsigned char*)calloc(atlasWidth * atlasHeight, 1);

            // Bake SDF for ASCII 32..126
            xtt_bakedchar bakedChars[96];
            xtt_BakeFontBitmap(ttf_buffer, 0, pixelHeight, atlasPixels, atlasWidth, atlasHeight, 32, 96, bakedChars);

            // Upload to OpenGL
            glGenTextures(1, outTexture);
            glBindTexture(GL_TEXTURE_2D, *outTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasPixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Convert baked data to our format
            for (int i = 0; i < 96; i++) {
                auto& bc = bakedChars[i];
                GlyphInfo& g = glyphs[i + 32];
                g.u0 = bc.x0 / (f32)atlasWidth;
                g.v0 = bc.y0 / (f32)atlasHeight;
                g.u1 = bc.x1 / (f32)atlasWidth;
                g.v1 = bc.y1 / (f32)atlasHeight;
                g.xoff = bc.xoff;
                g.yoff = bc.yoff;
                g.xadvance = bc.xadvance;
            }

            free(atlasPixels);
            free(ttf_buffer);
            return true;
        }
    }
}
// TODO: integer attribs for UVs

#version 330 core
in vec3 v_color;
in vec2 v_uv; // OpenGL interpolates for us
flat in uvec2 v_extra;
flat in uvec4 v_extra2;

out vec4 FragColor;

// use usampler2D for uint data
uniform usampler2D u_vram;

const uint kVRAMWidthBytes = 2048u;

uint fetchVRAM16(uint x, uint y)
{
    return texelFetch(u_vram, ivec2(x, y), 0).r;
}

uint GPU_sampleTexture(uint render_type, uint u, uint v)
{
    // TODO: Texture window

    uint texpage = v_extra2.z;

    uint pageX = (texpage & 0xFu) * 64u;
    uint pageY = ((texpage >> 4u) & 1u) * 256u;

    uint clutX = v_extra.x;
    uint clutY = v_extra.y;

    uint sample = 0u;
    switch (render_type)
    {
    case 1u: // 4 bit
    {
        uint wordX = pageX + (u >> 2u);
        uint wordY = pageY + v;

        uint word = fetchVRAM16(wordX, wordY);

        uint shift = (u & 3u) * 4u;
        uint colorIndex = (word >> shift) & 0xFu;

        // sample color from CLUT
        sample = fetchVRAM16(clutX + colorIndex, clutY);

        break;
    }
    case 2u: // 8 bit
    {
        uint wordX = pageX + (u >> 1u);
        uint wordY = pageY + v;

        uint word = fetchVRAM16(wordX, wordY);

        uint colorIndex = ((u & 1u) == 0u) ? (word & 0xFFu) : ((word >> 8u) & 0xFFu);

        sample = fetchVRAM16(clutX + colorIndex, clutY);
        break;
    }
    case 3u: // 15 bit
    {
        sample = fetchVRAM16(pageX + u, pageY + v);
        break;
    }
    }

    return sample;
}

void main() {
    uint render_type = v_extra.x;
    uint flags = v_extra.y;
    
    uint b8g8r8 = v_extra2.w;

    if (render_type == 0u) { // NOT TEXTURED
        if (flags == 1u) { // GOURAUD SHADED
            FragColor = vec4(v_color, 1.0);
            return;
        }

        // FLAT SHADED
        float r = float(b8g8r8 & 0xFFu);
        float g = float((b8g8r8 >> 8u) & 0xFFu);
        float b = float((b8g8r8 >> 16u) & 0xFFu);
        FragColor = vec4(r, g, b, 1.0); 
        return;
    }

    // TODO: FULLY IMPLEMENT
    // TEXTURED
    // uint u = uint(floor(v_uv.x));
    // uint v = uint(floor(v_uv.y));

    uint u = uint(v_uv.x);
    uint v = uint(v_uv.y);

    uint texel = GPU_sampleTexture(render_type, u, v);

    if (texel == 0x0000u)
        discard;

    uint r5 =  texel        & 31u;
    uint g5 = (texel >> 5u) & 31u;
    uint b5 = (texel >> 10u) & 31u;

    vec3 color = vec3(r5, g5, b5) / 31.0;
    FragColor = vec4(color, 1.0);
}
// NOTE: vec4 "extra" leaves out some shader data, fix if needed!
// TODO: Really could use optimization including in shaders with color

#include "HardwareRenderer.h"
#include <platform/gl_loader.h>
#include <platform/platform.h>
#include <Host.h>
#include <psx/Bus.h>
#include <psx-utils/VRAMConvert.h>

static inline void unpackB8G8R8(u32 val, u8& r, u8& g, u8& b)
{
    r = (u8)(val & 0xff);
    g = (u8)((val >> 8) & 0xff);
    b = (u8)((val >> 16) & 0xff);
}

HardwareRenderer hw_renderer_singleton;

static const char* vs_src =
"#version 330 core\n"
"layout (location = 0) in vec2 a_pos;\n"
"layout (location = 1) in vec4 a_color;\n"
"layout (location = 2) in vec2 a_uv;\n"
"layout (location = 3) in vec4 a_extra;\n" // texpage, clut, etc
"\n"
"out vec4 v_color;\n"
"out vec2 v_uv;\n"
"out vec4 v_extra;\n"
"\n"
"uniform vec2 u_resolution;\n"
"\n"
"void main() {\n"
"    vec2 pos = a_pos;\n"
"\n"
"    // PS1 -> NDC (top-left origin)\n"
"    vec2 ndc = vec2(\n"
"        (pos.x / u_resolution.x) * 2.0 - 1.0,\n"
"        1.0 - (pos.y / u_resolution.y) * 2.0\n"
"    );\n"
"\n"
"gl_Position = vec4(ndc, 0.0, 1.0);\n"
"\n"
"    v_color = a_color;\n"
"    v_uv = a_uv;\n"
"    v_extra = a_extra;\n"
"}\n";

static const char* fs_src =
"#version 330 core\n"
"in vec4 v_color;\n"
"in vec2 v_uv;\n"
"in uvec4 v_extra;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"uniform sampler2D u_vram;\n"
"\n"
"void main() {\n"
"    uint render_type = v_extra.x;\n"
"    uint flags = v_extra.y;\n"
//"\n"
"    vec4 color = v_color;\n"
//"\n"
"    if (render_type == 0) {\n" // NOT TEXTURED
"        if (flags == 1) {\n" // GOURAUD SHADED
"            FragColor = vec4(color.xyz, 1.0);\n"
"            return;\n"
"        }\n"
"        float r = float(uint(color.w) & 0xff);\n"
"        float g = float((uint(color.w) >> 8) & 0xff);\n"
"        float b = float((uint(color.w) >> 16) & 0xff);\n"
"        FragColor = vec4(r, g, b, 1.0);\n" // NOT SHADED
"        return;\n"
"    }\n"
"ivec2 coord = ivec2(int(v_uv.x), int(v_uv.y));"
"vec4 texel = texelFetch(u_vram, coord, 0);"
"    FragColor = texel * vec4(texel.xyz, 1.0);\n"
"}\n";

static constexpr unsigned int kVRAMTextureWidthPixels = kVRAMWidth16bpp;

static u32 createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    s32 success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("Failed to compile vertex shader! | %s\n", infoLog);
    }

    // fragment shader
    u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("Failed to compile fragment shader! | %s\n", infoLog);
    }

    // link shaders
    u32 program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Failed to link shader program! | %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

HardwareRenderer::HardwareRenderer()
{
    // TODO:
}

void HardwareRenderer::Init()
{
    static constexpr unsigned int kVRAMTextureWidthPixels = kVRAMWidth16bpp;
    m_softwareVRAM.resize(kVRAMTextureWidthPixels * kVRAMHeightLines * sizeof(u32));
    m_tex = new Texture(kVRAMTextureWidthPixels, kVRAMHeightLines, "HW_VRAM");
    m_sampleTex = new Texture(kVRAMTextureWidthPixels, kVRAMHeightLines, "HW_VRAM_SAMPLE");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_program = createShaderProgram(vs_src, fs_src);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    const u32 MAX_VERTICES = 65536;
    m_vertices.resize(MAX_VERTICES);
    glBufferData(GL_ARRAY_BUFFER, sizeof(HardwareVertex) * MAX_VERTICES, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, x));

    glEnableVertexAttribArray(1); // color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, r));

    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, u));

    glEnableVertexAttribArray(3); // extra
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, render_type));

    glBindVertexArray(0);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glBindTexture(GL_TEXTURE_2D, m_tex->GetGLTexture());

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex->GetGLTexture(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer incomplete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

HardwareRenderer::~HardwareRenderer()
{
    // TODO:
    delete m_tex;
}

/*
u16 GPU::sampleTexture(unsigned int u, unsigned int v) const
{
    HP_DEBUG_ASSERT(u <= 0xff && v <= 0xff);

    // Respect texture window
    if (!m_textureWindowPrecomp.zero)
    {
        // Texcoord = (Texcoord AND (NOT (Mask * 8))) OR ((Offset AND Mask) * 8)
        // https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gp0e2h-texture-window-setting
        // Use precomputed terms to avoid repeated per-pixel calculations.
        u = (u & m_textureWindowPrecomp.uAND) | m_textureWindowPrecomp.uOR;
        v = (v & m_textureWindowPrecomp.vAND) | m_textureWindowPrecomp.vOR;
    }

    // Calculate texture pixel coordinates in VRAM

    u16 sample = 0;
    switch (m_gpustat.textureFormat)
    {
    case TextureFormat::k4BitPalette:
    {
        // V is expressed in 4-bit units
        u32 texelAddr = m_texturePageAddr + (v * kVRAMWidthBytes) + (u / 2); // 4 bpp = 0.5 bytes per pixel
        u8 colourIndex = m_vram[texelAddr];
        // low nibble for even u, high nibble for odd u
        if (u & 1)
            colourIndex = (colourIndex >> 4) & 0x0f;
        else
            colourIndex = colourIndex & 0x0f;

        // Sample colour from CLUT
        u32 colourAddr = m_clutAddr + (2 * colourIndex); // A1B5G5R5 16 bpp = 2 bytes per pixel
        sample = m_vram[colourAddr] | (m_vram[colourAddr + 1] << 8); // little-endian

        break;
    }
    case TextureFormat::k8BitPalette:
    {
        // V is expressed in 8-bit units
        u32 texelAddr = m_texturePageAddr + (v * kVRAMWidthBytes) + u; // 8 bpp = 1 byte per pixel
        u8 colourIndex = m_vram[texelAddr];

        // Sample colour from CLUT
        u32 colourAddr = m_clutAddr + (2 * colourIndex); // A1B5G5R5 16 bpp = 2 bytes per pixel
        sample = m_vram[colourAddr] | (m_vram[colourAddr + 1] << 8); // little-endian
        break;
    }
    case TextureFormat::kA1B5G5R5:
    case TextureFormat::kReserved: // Texture page colors setting 3 (reserved) is same as setting 2 (15bit).
    {
        // V is expressed in 16-bit units
        u32 addr = m_texturePageAddr + (v * kVRAMWidthBytes) + (2 * u); // 16 bpp = 2 bytes per pixel
        sample = m_vram[addr] | (m_vram[addr + 1] << 8); // little-endian
        break;
    }
    }

    return sample;
}
*/

void HardwareRenderer::PushTriangle(const InputVertex vertices[3], HardwareRenderer::RenderFlags drawFlags, TextureFormat textureType)
{
    const GPU& gpu = Host::GetBus().GetGPU();
    int offx = gpu.GetDisplayStartX();
    int offy = gpu.GetDisplayStartY();

    u8 render_type = 0;
    if (drawFlags.texture_mode)
    {
        switch (textureType)
        {
        case TextureFormat::k4BitPalette:
        {
            render_type = 1;
            break;
        }
        case TextureFormat::k8BitPalette:
        {
            render_type = 2;
            break;
        }
        case TextureFormat::kA1B5G5R5:
        case TextureFormat::kReserved:
            render_type = 3.0f;
            break;
        default:
            render_type = 3;
            break;
        }
    }

    u32 u0, v0, u1, v1, u2, v2;
    if (render_type != 0.0f)
    {
        u0 = vertices[0].u;
        v0 = vertices[0].v;
        u1 = vertices[1].u;
        v1 = vertices[1].v;
        u2 = vertices[2].u;
        v2 = vertices[2].v;
    }
    u32 uvs_x[3] = { u0, u1, u2 };
    u32 uvs_y[3] = { v0, v1, v2 };

    for (int i = 0; i < 3; i++) {
        HardwareVertex v{};

        v.x = (float)(vertices[i].x + offx);
        v.y = (float)(vertices[i].y + offy);

        u8 r, g, b;
        unpackB8G8R8(vertices[i].colourB8G8R8, r, g, b);

        v.r = (float)(r / 255.0f);
        v.g = (float)(g / 255.0f);
        v.b = (float)(b / 255.0f);
        v.w = (float)vertices[0].colourB8G8R8;
        
        if (render_type != 0.0f) {
            v.u = uvs_x[i];
            v.v = uvs_y[i];

            //v.clut_x = palette_x(&tex_info->palette);
            //v.clut_y = palette_y(&tex_info->palette);
            //v.texpage = tex_info->page;
        }
        else
        {
            v.u = v.v = 0.0f;
            v.texpage = 0.0f;
            v.clut_x = v.clut_y = 0.0f;
        }

        v.render_type = render_type;
        //v.render_type = 0.0f;
        v.flags = drawFlags.shading;

        m_vertices[m_vertexCount++] = v;
    }

    UpdateTexture(0, 0, 0, 0);
}

void HardwareRenderer::UpdateTexture(int x, int y, int w, int h)
{
    // TODO: optimize
    const GPU& gpu = Host::GetBus().GetGPU();
    glBindTexture(GL_TEXTURE_2D, m_tex->GetGLTexture());
    Rect srcRect{
        /*.x =*/ 0,
        /*.y =*/ 0,
        /*.w =*/ kVRAMWidth16bpp,
        /*.h =*/ kVRAMHeightLines
    };

    Rect dstRect{
        /*.x =*/ 0,
        /*.y =*/ 0,
        /*.w =*/ kVRAMTextureWidthPixels,
        /*.h =*/ kVRAMHeightLines
    };

    VRAMConvert::ConvertToR8G8B8A8_UNORM(gpu.GetVRAM(), srcRect, DisplayFormat::A1B5G5R5, (u32*)m_softwareVRAM.data(), dstRect, kVRAMTextureWidthPixels, kVRAMHeightLines);
    m_sampleTex->Upload(m_softwareVRAM.data()); // TODO: needs to be flipped
}

void HardwareRenderer::Render()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glViewport(0, 0, kVRAMTextureWidthPixels, kVRAMHeightLines);

    //glClearColor(0, 0, 0, 0);
    //glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_program);
    glDisable(GL_DEPTH_TEST); // psx has no depth buffer

    GLint res_loc = glGetUniformLocation(m_program, "u_resolution");
    float resw = (float)Host::GetBus().GetGPU().GetHorizontalResolution();
    float resh = (float)Host::GetBus().GetGPU().GetVerticalResolution();
    glUniform2f(res_loc, 1024.0f, 512.0f);

    // bind VRAM texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sampleTex->GetGLTexture());
    glUniform1i(glGetUniformLocation(m_program, "u_vram"), 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(HardwareVertex) * m_vertexCount, m_vertices.data());

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);

    glBindVertexArray(0);

    m_vertexCount = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // TODO: optimize
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);

    glBindTexture(GL_TEXTURE_2D, m_sampleTex->GetGLTexture());

    glCopyTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        0,
        0,
        1024,
        512);
}

u32 HardwareRenderer::GetTexture() const
{
	return m_tex->GetGLTexture();
}
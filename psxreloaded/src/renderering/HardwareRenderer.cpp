#include "HardwareRenderer.h"
#include <platform/gl_loader.h>
#include <platform/platform.h>
#include <Host.h>
#include <psx/Bus.h>

HardwareRenderer hw_renderer_singleton;

static const char* vs_src =
"#version 330 core\n"
"layout (location = 0) in vec2 a_pos;\n"
"layout (location = 1) in vec3 a_color;\n"
"layout (location = 2) in vec2 a_uv;\n"
"layout (location = 3) in vec4 a_extra;\n" // texpage, clut, etc
"\n"
"out vec3 v_color;\n"
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
"in vec3 v_color;\n"
"in vec2 v_uv;\n"
"in vec4 v_extra;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
//"uniform sampler2D u_vram;\n"
"\n"
"void main() {\n"
"    int render_type = int(v_extra.x);\n"
"\n"
"    vec3 color = v_color;\n"
"\n"
"    if (render_type == 0) {\n"
"        // SHADED\n"
"        FragColor = vec4(color, 1.0);\n"
"        return;\n"
"    }\n"
"\n"
"    // TEXTURED\n"
//"    vec2 uv = v_uv / 1024.0; // PS1 VRAM size assumption\n"
"\n"
//"    vec4 texel = texture(u_vram, uv);\n"
"\n"
//"    FragColor = texel * vec4(color, 1.0);\n"
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
    delete m_tex;
}

void HardwareRenderer::Init()
{
    m_tex = new Texture(kVRAMTextureWidthPixels, kVRAMHeightLines, "HW_VRAM");
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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, r));

    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, u));

    glEnableVertexAttribArray(3); // extra
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(HardwareVertex), (void*)offsetof(HardwareVertex, texpage));

    glBindVertexArray(0);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glBindTexture(GL_TEXTURE_2D, m_tex->GetGLTexture());

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_tex->GetGLTexture(),
        0);

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
}

void HardwareRenderer::PushTriangle(const float pos[3][2], const float** col, HardwareRenderer::RenderFlags draw_flags/*  pixel_render_t render_type */) {
    const GPU& gpu = Host::GetBus().GetGPU();
    int offx = gpu.GetDisplayStartX();
    int offy = gpu.GetDisplayStartY();

    for (int i = 0; i < 3; i++) {
        HardwareVertex v;

        v.x = (float)(pos[i][0] + offx);
        v.y = (float)(pos[i][1] + offy);

        v.r = 0.1f;//col->p[i].r16 / 255.0f;
        v.g = 0.3f;//col->p[i].g16 / 255.0f;
        v.b = 0.5f;//col->p[i].b16 / 255.0f;
        /*
        if (tex_info) {
            v.u = tex_info->uv_active.p[i].x;
            v.v = tex_info->uv_active.p[i].y;

            //v.clut_x = palette_x(&tex_info->palette);
            //v.clut_y = palette_y(&tex_info->palette);
            v.texpage = tex_info->page;
        }
        else {
        */
            v.u = v.v = 0.0f;
            v.texpage = 0.0f;
            v.clut_x = v.clut_y = 0.0f;
        //}

        v.render_type = (float)0.0f; // hardcoded to shaded for now
        v.flags = *(u32*)&draw_flags;

        m_vertices[m_vertexCount++] = v;
    }
}

void HardwareRenderer::Render()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glViewport(0, 0, kVRAMTextureWidthPixels, kVRAMHeightLines);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_program);
    glDisable(GL_DEPTH_TEST); // psx has no depth buffer

    GLint res_loc = glGetUniformLocation(m_program, "u_resolution");
    float resw = (float)Host::GetBus().GetGPU().GetHorizontalResolution();
    float resh = (float)Host::GetBus().GetGPU().GetVerticalResolution();
    glUniform2f(res_loc, resw, resh);

    // bind VRAM texture
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, rast->texture);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(HardwareVertex) * m_vertexCount, m_vertices.data());

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);

    glBindVertexArray(0);

    m_vertexCount = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

u32 HardwareRenderer::GetTexture() const
{
	return m_tex->GetGLTexture();
}
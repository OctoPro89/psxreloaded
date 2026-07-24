/*
    TODO: get locations ahead of time 
*/

#include <XGUI/impl/xgui_impl_opengl.h>
#include <XGUI/xgui_rendering_engine.h>

#ifdef XGUI_IMPL_OPENGL

#include "../../../platform/gl_loader.h"
#include "../../Filesystem.h"

namespace
{
    static u32 createShaderProgram(const char* vertexSource, const char* fragmentSource)
    {
        u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors
        i32 success;
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

    std::vector<f32> __vertices__;
    std::vector<u32> __indices__;

    static inline void updateProgramAndMatrix(xgui::Context& ctx, u32 program)
    {
        glUseProgram(program);

        // Update projection matrix if needed
        if (!ctx.projection_updated_map.get_value(program))
        {
            // Create orthographic projection matrix
            f32 projection[16] = {
                2.0f / ctx.screen_width, 0.0f,                   0.0f, 0.0f,
                0.0f,                  -2.0f / ctx.screen_height, 0.0f, 0.0f,
                0.0f,                  0.0f,                   -1.0f, 0.0f,
                -1.0f,                 1.0f,                   0.0f, 1.0f
            };

            glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection);
            ctx.projection_updated_map.set_value(program, true);
        }
    }
} // anonymous namespace

namespace xgui
{
    u64 global_submission_counter;

	namespace impl
	{
		namespace opengl
		{
			bool init_context_vars()
			{
                Context& ctx = Context::get();

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

                const char* vertexShaderSource = Filesystem::readFile("sdf_box.vert.glsl", NULL);
                const char* fragmentShaderSource = Filesystem::readFile("sdf_box.frag.glsl", NULL);

                if (!vertexShaderSource || !fragmentShaderSource) { return false; }

                ctx.sdf_button_shader_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

                free((void*)vertexShaderSource);
                free((void*)fragmentShaderSource);

                vertexShaderSource = Filesystem::readFile("sdf_text.vert.glsl", NULL);
                fragmentShaderSource = Filesystem::readFile("sdf_text.frag.glsl", NULL);

                if (!vertexShaderSource || !fragmentShaderSource) { return false; }

                ctx.sdf_text_shader_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

                free((void*)vertexShaderSource);
                free((void*)fragmentShaderSource);

                vertexShaderSource = Filesystem::readFile("primitive.vert.glsl", NULL);
                fragmentShaderSource = Filesystem::readFile("primitive.frag.glsl", NULL);

                if (!vertexShaderSource || !fragmentShaderSource) { return false; }

                ctx.primitive_shader_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

                free((void*)vertexShaderSource);
                free((void*)fragmentShaderSource);

                vertexShaderSource = Filesystem::readFile("image.vert.glsl", NULL);
                fragmentShaderSource = Filesystem::readFile("image.frag.glsl", NULL);

                if (!vertexShaderSource || !fragmentShaderSource) { return false; }

                ctx.image_shader_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

                free((void*)vertexShaderSource);
                free((void*)fragmentShaderSource);

                glGenVertexArrays(1, &ctx.sdf_button_vao);
                glGenBuffers(1, &ctx.sdf_button_vbo);
                glGenBuffers(1, &ctx.sdf_button_ebo);
                // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
                glBindVertexArray(ctx.sdf_button_vao);

                glBindBuffer(GL_ARRAY_BUFFER, ctx.sdf_button_vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.sdf_button_ebo);

                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
                glEnableVertexAttribArray(0);

                glGenVertexArrays(1, &ctx.sdf_text_vao);
                glGenBuffers(1, &ctx.sdf_text_vbo);
                glGenBuffers(1, &ctx.sdf_text_ebo);

                glBindVertexArray(ctx.sdf_text_vao);

                glBindBuffer(GL_ARRAY_BUFFER, ctx.sdf_text_vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.sdf_text_ebo);

                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);            // Position
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32))); // TexCoord
                glEnableVertexAttribArray(1);

                glGenVertexArrays(1, &ctx.primitive_vao);
                glGenBuffers(1, &ctx.primitive_vbo);
                glGenBuffers(1, &ctx.primitive_ebo);

                glBindVertexArray(ctx.primitive_vao);

                glBindBuffer(GL_ARRAY_BUFFER, ctx.primitive_vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.primitive_ebo);

                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
                glEnableVertexAttribArray(0);

                glBindVertexArray(0);

                glGenVertexArrays(1, &ctx.image_vao);
                glGenBuffers(1, &ctx.image_vbo);
                glGenBuffers(1, &ctx.image_ebo);

                glBindVertexArray(ctx.image_vao);
                glBindBuffer(GL_ARRAY_BUFFER, ctx.image_vbo);

                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, (void*)0); // pos
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, (void*)(sizeof(f32) * 2)); // uv
                glEnableVertexAttribArray(1);

                glBindVertexArray(0);

                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // setup key-value map
                ctx.projection_updated_map.keys[0] = ctx.image_shader_program;
                ctx.projection_updated_map.keys[1] = ctx.primitive_shader_program;
                ctx.projection_updated_map.keys[2] = ctx.sdf_button_shader_program;
                ctx.projection_updated_map.keys[3] = ctx.sdf_text_shader_program;

                return true;
			}

            void shutdown()
            {
                Context& ctx = Context::get();
                glDeleteProgram(ctx.sdf_button_shader_program);
            }

            void beginFrame()
            {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            /*
            void applyClip(const Context& ctx, const Rect& r)
            {
                glEnable(GL_SCISSOR_TEST);

                glScissor(
                    (int)r.x,
                    (int)(ctx.screen_height - (r.y + r.h)), // OpenGL Y flip
                    (int)r.w,
                    (int)r.h
                );
            }

            void disableClip()
            {
                glDisable(GL_SCISSOR_TEST);
            }
            */

            void renderRect(Context& ctx, xgui_render_command& cmd)
            {
                updateProgramAndMatrix(ctx, ctx.sdf_button_shader_program);

                // Bind shader and set uniforms
                glUniform4fv(glGetUniformLocation(ctx.sdf_button_shader_program, "color"), 1, &cmd.color.r);
                glUniform1f(glGetUniformLocation(ctx.sdf_button_shader_program, "cornerRadius"), cmd.corner_radius);
                glUniform2f(glGetUniformLocation(ctx.sdf_button_shader_program, "rectSize"), cmd.rect.w, cmd.rect.h);
                glUniform2f(glGetUniformLocation(ctx.sdf_button_shader_program, "rectPos"), cmd.rect.x, cmd.rect.y);
                glUniform1f(glGetUniformLocation(ctx.sdf_button_shader_program, "screenHeight"), ctx.screen_height);

                // Generate vertices based on actual rectangle dimensions
                f32 vertices[] = {
                    cmd.rect.x + cmd.rect.w, cmd.rect.y,            // top-right
                    cmd.rect.x + cmd.rect.w, cmd.rect.y + cmd.rect.h,   // bottom-right
                    cmd.rect.x,          cmd.rect.y + cmd.rect.h,   // bottom-left
                    cmd.rect.x,          cmd.rect.y,   // top-left
                };

                u32 indices[] = { 0, 1, 3, 1, 2, 3 };

                // Upload and draw
                glBindVertexArray(ctx.sdf_button_vao);
                glBindBuffer(GL_ARRAY_BUFFER, ctx.sdf_button_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.sdf_button_ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }

            void renderCheckmark(Context& ctx, xgui_render_command& cmd)
            {
                updateProgramAndMatrix(ctx, ctx.primitive_shader_program);

                // should not de-allocate memory
                __vertices__.clear();
                __indices__.clear();

                f32 thickness = cmd.size * 0.12f; // line thickness
                f32 x0 = cmd.x + cmd.size * 0.2f; // start left
                f32 y0 = cmd.y + cmd.size * 0.55f;

                f32 x1 = cmd.x + cmd.size * 0.4f; // middle join
                f32 y1 = cmd.y + cmd.size * 0.75f;

                f32 x2 = cmd.x + cmd.size * 0.8f; // end right
                f32 y2 = cmd.y + cmd.size * 0.25f;

                auto addSegment = [&](f32 sx, f32 sy, f32 ex, f32 ey, std::vector<f32>& verts, std::vector<u32>& inds, u32& idx)
                {
                    f32 dx = ex - sx;
                    f32 dy = ey - sy;
                    f32 len = sqrtf(dx * dx + dy * dy); // TODO: optimize out
                    f32 nx = -(dy / len) * thickness * 0.5f;
                    f32 ny = (dx / len) * thickness * 0.5f;

                    verts.insert(verts.end(), {
                        sx + nx, sy + ny, // top-left
                        ex + nx, ey + ny, // top-right
                        ex - nx, ey - ny, // bottom-right
                        sx - nx, sy - ny, // bottom-left
                    });

                    inds.insert(inds.end(), {
                        idx, idx + 1, idx + 2,
                        idx, idx + 2, idx + 3
                    });

                    idx += 4;
                };

                u32 index_offset = 0;

                // Left short stroke
                addSegment(x0, y0, x1, y1, __vertices__, __indices__, index_offset);
                // Right long stroke
                addSegment(x1, y1, x2, y2, __vertices__, __indices__, index_offset);

                // Upload and draw
                glUniform4fv(glGetUniformLocation(ctx.primitive_shader_program, "color"), 1, &cmd.color.r);

                glBindVertexArray(ctx.primitive_vao);
                glBindBuffer(GL_ARRAY_BUFFER, ctx.primitive_vbo);
                glBufferData(GL_ARRAY_BUFFER, __vertices__.size() * sizeof(f32), __vertices__.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.primitive_ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, __indices__.size() * sizeof(u32), __indices__.data(), GL_DYNAMIC_DRAW);

                glDrawElements(GL_TRIANGLES, (GLsizei)__indices__.size(), GL_UNSIGNED_INT, 0);

                glBindVertexArray(0);
            }

            void renderRightArrow(Context& ctx, xgui_render_command& cmd)
            {
                updateProgramAndMatrix(ctx, ctx.primitive_shader_program);

                f32 vertices[6] = {
                    cmd.x,       cmd.y,        // left top
                    cmd.x,       cmd.y + cmd.size,  // left bottom
                    cmd.x + cmd.size * 0.6f, cmd.y + cmd.size * 0.5f // right point
                };

                u32 indices[3] = {0, 1, 2};

                glUniform4fv(glGetUniformLocation(ctx.primitive_shader_program, "color"), 1, &cmd.color.r);

                glBindVertexArray(ctx.primitive_vao);
                glBindBuffer(GL_ARRAY_BUFFER, ctx.primitive_vbo);
                glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(f32), vertices, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.primitive_ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(u32), indices, GL_DYNAMIC_DRAW);

                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

                glBindVertexArray(0);
            }

            void renderText(Context& ctx, xgui_render_command& cmd)
            {
                updateProgramAndMatrix(ctx, ctx.sdf_text_shader_program);

                // should not de-allocate memory
                __vertices__.clear();
                __indices__.clear();
                
                glUniform4fv(glGetUniformLocation(ctx.sdf_text_shader_program, "color"), 1, &cmd.color.r);

                // Bind font texture
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, ctx.font_texture);
                glUniform1i(glGetUniformLocation(ctx.sdf_text_shader_program, "textAtlas"), 0);

                glBindVertexArray(ctx.sdf_text_vao);

                __vertices__.reserve(strlen(cmd.text.c_str()) * 6 * 5);
                __indices__.reserve(strlen(cmd.text.c_str()) * 6);

                f32 xpos = cmd.x;
                f32 ypos = cmd.y;

                u32 index_offset = 0;
                for (const char* p = cmd.text.c_str(); *p; p++)
                {
                    u8 c = static_cast<u8>(*p);
                    if (c < 32 || c >= 128)
                        continue;

                    font::GlyphInfo& g = ctx.glyphs[c];

                    /*
                    f32 x0 = xpos + g.xoff;
                    f32 y0 = ypos + g.yoff;
                    f32 w = (g.u1 - g.u0) * 512.0f; // atlas size hardcoded to 512
                    f32 h = (g.v1 - g.v0) * 512.0f;
                    */
                    const f32 scale = cmd.size / 20.0f;
                    f32 x0 = xpos + g.xoff * scale;
                    f32 y0 = ypos + g.yoff * scale;

                    f32 w = (g.u1 - g.u0) * 512.0f * scale;
                    f32 h = (g.v1 - g.v0) * 512.0f * scale;

                    // Positions: z=0.0
                    __vertices__.insert(__vertices__.end(), {
                        x0,     y0,      g.u0, g.v0, // top-left
                        x0 + w, y0,      g.u1, g.v0, // top-right
                        x0 + w, y0 + h,  g.u1, g.v1, // bottom-right
                        x0,     y0 + h,  g.u0, g.v1  // bottom-left
                        });

                    __indices__.insert(__indices__.end(), {
                        index_offset + 0, index_offset + 1, index_offset + 2,
                        index_offset + 0, index_offset + 2, index_offset + 3
                        });

                    index_offset += 4;
                    xpos += g.xadvance * scale; // account for text size
                    //xpos += g.xadvance; // move to next glyph
                }

                // Upload geometry
                glBindBuffer(GL_ARRAY_BUFFER, ctx.sdf_text_vbo);
                glBufferData(GL_ARRAY_BUFFER, __vertices__.size() * sizeof(f32), __vertices__.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.sdf_text_ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, __indices__.size() * sizeof(u32), __indices__.data(), GL_DYNAMIC_DRAW);

                // Draw all characters in one call
                glDrawElements(GL_TRIANGLES, (GLsizei)__indices__.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }

            void renderImageView(Context& ctx, xgui_render_command& cmd)
            {
                updateProgramAndMatrix(ctx, ctx.image_shader_program);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, cmd.gl_id);
                glUniform1i(glGetUniformLocation(ctx.image_shader_program, "tex"), 0);

                f32 vertices[] = {
                    cmd.x,         cmd.y,         cmd.u0, cmd.v0, // Top-left
                    cmd.x + cmd.w, cmd.y,         cmd.u1, cmd.v0, // Top-right
                    cmd.x + cmd.w, cmd.y + cmd.h, cmd.u1, cmd.v1, // Bottom-right
                    cmd.x,         cmd.y + cmd.h, cmd.u0, cmd.v1  // Bottom-left
                };

                u32 indices[] = {
                    0, 1, 2,
                    0, 2, 3
                };

                glBindVertexArray(ctx.image_vao);

                glBindBuffer(GL_ARRAY_BUFFER, ctx.image_vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.image_ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                glBindVertexArray(0);
            }

            void execCommands(std::vector<xgui_render_command>& render_commands)
            {
                Context& ctx = Context::get();

                size_t remaining_elements = render_commands.size();

                glDepthMask(GL_FALSE);
                for (size_t i = 0; i < remaining_elements; ++i)
                {
                    auto& cmd = render_commands[i];

                    if (cmd.use_clip)
                    {
                        applyClip(ctx, cmd.clip_rect);
                    }
                    else
                    {
                        disableClip();
                    }

                    switch (render_commands[i].render_type)
                    {
                        case XGUI_RENDER_TYPE_RECT:
                        {
                            renderRect(ctx, render_commands[i]);
                            break;
                        }
                        case XGUI_RENDER_TYPE_TEXT:
                        {
                            renderText(ctx, render_commands[i]);
                            break;
                        }
                        case XGUI_RENDER_TYPE_IMAGEVIEW:
                        {
                            renderImageView(ctx, render_commands[i]);
                            break;
                        }
                        case XGUI_RENDER_TYPE_CHECKMARK:
                        {
                            renderCheckmark(ctx, render_commands[i]);
                            break;
                        }
                        case XGUI_RENDER_TYPE_RARROW:
                        {
                            renderRightArrow(ctx, render_commands[i]);
                            break;
                        }
                    }
                }
                glDepthMask(GL_TRUE);
                disableClip();

                render_commands.clear();
                global_submission_counter = 0;
            }
		}
	}
}

#endif // XGUI_IMPL_OPENGL
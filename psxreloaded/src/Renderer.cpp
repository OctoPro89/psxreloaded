#include "Renderer.h"
#include "Texture.h"

#include <platform/gl_loader.h>
#include "xgui/Filesystem.h"

#include "core/Log.h"
#include "core/hp_assert.h"
#include "core/StringHelpers.h"
#include "core/Helpers.h" //HP_UNUSED

#include <stdint.h>

struct RendererState
{
	GLuint program = 0;
	GLuint vao = 0;

	GLint viewportLocation = -1;
	GLint rectLocation = -1;
	GLint uvLocation = -1;
	GLint textureLocation = -1;

	u32 width = 0;
	u32 height = 0;
};

static RendererState s_renderer;

struct ViewParams
{
	uint32_t viewportWidth;
	uint32_t viewportHeight;
};

struct RectParams
{
	int32_t x;
	int32_t y;
	uint32_t w;
	uint32_t h;
	float u0;
	float v0;
	float u1;
	float v1;
};

struct RectUniform
{
	GLint x;
	GLint y;

	GLint width;
	GLint height;
};

struct UVUniform
{
	GLfloat u0;
	GLfloat v0;

	GLfloat u1;
	GLfloat v1;
};

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

bool Renderer::Init()
{
	/*
	const char* vertexShaderSource = Filesystem::readFile("sdf_box.vert.glsl", NULL);
    const char* fragmentShaderSource = Filesystem::readFile("sdf_box.frag.glsl", NULL);

    if (!vertexShaderSource || !fragmentShaderSource) { return false; }

    ctx.sdf_button_shader_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    free((void*)vertexShaderSource);
    free((void*)fragmentShaderSource);
	*/
	const char* vertexShaderSource = Filesystem::readFile("rect.vert.glsl", NULL);
	const char* fragmentShaderSource = Filesystem::readFile("texture.frag.glsl", NULL);

	if (!vertexShaderSource || !fragmentShaderSource) { return false; }

	s_renderer.program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

	glGenVertexArrays(1, &s_renderer.vao);

	s_renderer.viewportLocation = glGetUniformLocation(s_renderer.program, "uViewport");
	s_renderer.rectLocation = glGetUniformLocation(s_renderer.program, "uRect");
	s_renderer.uvLocation = glGetUniformLocation(s_renderer.program, "uUV");
	s_renderer.textureLocation = glGetUniformLocation(s_renderer.program, "uTexture");

	return true;
}

void Renderer::Shutdown()
{

}

bool Renderer::Begin()
{
	glViewport(0, 0, s_renderer.width, s_renderer.height);

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(s_renderer.program);

	glBindVertexArray(s_renderer.vao);

	glUniform2ui(s_renderer.viewportLocation, s_renderer.width, s_renderer.height);
	
	return true;
}

void Renderer::End() {}

void Renderer::DrawTexture(Texture* texture, u32 dstGlTexture)
{
	ViewParams vp{ s_renderer.width, s_renderer.height };

	glUniform2ui(s_renderer.viewportLocation, vp.viewportWidth, vp.viewportHeight);

	RectParams rect{};
	rect.u0 = 0.0f;
	rect.v0 = 0.0f;
	rect.u1 = 1.0f;
	rect.v1 = 1.0f;

	glUniform4i(s_renderer.rectLocation, rect.x, rect.y, rect.w, rect.h);
	glUniform4f(s_renderer.uvLocation, rect.u0, rect.v0, rect.u1, rect.v1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture());

	glUniform1i(s_renderer.textureLocation, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::UpdateResolution(u32 width, u32 height)
{
	s_renderer.width = width;
	s_renderer.height = height;
}
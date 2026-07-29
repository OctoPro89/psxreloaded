#include "Texture.h"
#include "Renderer.h"
#include "core/hp_assert.h"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <platform/gl_loader.h>
#endif // __EMSCRIPTEN__

Texture::Texture(unsigned int width, unsigned int height, const char* name, bool vramTexture)
	: m_width(width)
	, m_height(height)
	, m_bytesPerPixel(4)
	, m_vramTexture(vramTexture)
{
	glGenTextures(1, &m_glTexture);
	glBindTexture(GL_TEXTURE_2D, m_glTexture);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, vramTexture ? GL_R16UI : GL_RGBA8, width, height, 0, vramTexture ? GL_RED_INTEGER : GL_RGBA, vramTexture ?  GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE, (void*)0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_glTexture);
}

void Texture::Upload(const void* data) const
{
	glBindTexture(GL_TEXTURE_2D, m_glTexture);
    if (m_vramTexture)
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            m_width,
            m_height,
            GL_RED_INTEGER,
            GL_UNSIGNED_SHORT,
            data);
    }
    else
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            m_width,
            m_height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data);
    }
	glBindTexture(GL_TEXTURE_2D, 0);
}

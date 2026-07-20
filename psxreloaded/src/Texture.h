#pragma once

#include <core/Types.h>

class Texture
{
public:

	static const unsigned int kBytesPerPixel = 4;

	Texture(unsigned int width, unsigned int height, const char* name);
	~Texture();

	void Upload(const void* data);

	unsigned int GetWidth() const { return m_width; }
	unsigned int GetHeight() const { return m_height; }
	u32 GetGLTexture() const { return m_glTexture; }

private:
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_bytesPerPixel;
	u32 m_glTexture;
};

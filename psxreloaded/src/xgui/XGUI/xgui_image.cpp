#define XI_IMAGE_IMPLEMENTATION
#include <XGUI/xgui_image.h>
#include "../../platform/gl_loader.h"

namespace xgui
{
	namespace image
	{
		bool XImage::loadFromFile(const char* file)
		{
			i32 x = 0, y = 0, _channels = 0;
			xi_uc* data = xi_load(file, &x, &y, &_channels, 0);
			if (!data)
			{
				gl_id = (u32)-1;
				return false;
			}

			width = x;
			height = y;
			channels = _channels;

			glGenTextures(1, &gl_id);
			glBindTexture(GL_TEXTURE_2D, gl_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			
			xi_image_free((void*)data);

			return true;
		}

		void XImage::dealloc()
		{
			glDeleteTextures(1, &gl_id);
			gl_id = (u32)-1;
		}
	}
}
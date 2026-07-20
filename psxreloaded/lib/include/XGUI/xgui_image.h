#ifndef XGUI_IMAGE_H
#define XGUI_IMAGE_H

#include <XGUI/xgui_common.h>
#include <XGUI/ximage.h>

namespace xgui
{
	namespace image
	{
		struct XAPI XImage
		{
			u32 width;
			u32 height;
			u32 channels;
			u32 gl_id;

			bool loadFromFile(const char* file);
			void dealloc();
			inline ~XImage() { if (gl_id != (u32)-1) { dealloc(); } }
		};
	}
}

#endif // XGUI_IMAGE_H
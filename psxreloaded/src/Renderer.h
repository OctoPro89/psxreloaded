#pragma once

#include "core/ClassHelpers.h"
#include <core/Types.h>

class Texture;

class Renderer
{
public:

	NON_INSTANTIABLE_STATIC_CLASS(Renderer);

	static bool Init();
	static void Shutdown();

	// If returns false then do not render
	[[nodiscard]] static bool Begin();
	static void End();

	static void DrawTexture(Texture* texture, u32 dstGlTexture);
	static void UpdateResolution(u32 width, u32 height);
};

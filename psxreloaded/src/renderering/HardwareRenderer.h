#pragma once

#ifdef EXPERIMENTAL_HW_RENDERER

#include <core/Types.h>
#include <vector>
#include <Texture.h>
#include <psx/GPU.h>

typedef struct {
	float x, y;
	float r, g, b;
	float u, v;

	u8 render_type;
	u8 flags;

	u32 clutX_halfwords;
	u32 clutY;
	u32 texpage;
	u32 B8G8R8;
} HardwareVertex;

struct InputVertex
{
	// Position
	int x = 0;
	int y = 0;

	// Colour
	u32 colourB8G8R8 = 0; // Store as 8-bit throughout pipeline to improve accuracy. Will be converted to 5 bit for write to VRAM display format.

	// Texture coords
	unsigned int u = 0; // 8-bit unsigned
	unsigned int v = 0;
};

class HardwareRenderer;

extern HardwareRenderer hw_renderer_singleton;

class HardwareRenderer
{
public:
	// NOTE: these are all boolean despite the type
	typedef struct {
		u8 shading;
		u8 texture_mode;
		u8 semi_transparency;
		u8 dithering;
	} RenderFlags;

	HardwareRenderer();
	void Init();
	~HardwareRenderer();

	void PushTriangle(const InputVertex vertices[3], HardwareRenderer::RenderFlags drawFlags, TextureFormat textureType, u32 clutX_halfwords, u32 clutY, u32 texpage);
	void UpdateTexture(int x, int y, int w, int h);
	void Render();
	u32 GetTexture() const;
	static HardwareRenderer* Get() { return &hw_renderer_singleton; }
private:
	u32 m_program;
	u32 m_vao;
	u32 m_vbo;
	u32 m_ebo;
	u32 m_fbo;
	Texture* m_tex;
	Texture* m_sampleTex;
	std::vector<HardwareVertex> m_vertices;
	std::vector<u16> m_softwareVRAM;
	u64 m_vertexCount;
};

#endif // EXPERIMENTAL_HW_RENDERER
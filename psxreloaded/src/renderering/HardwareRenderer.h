#pragma once

#include <core/Types.h>
#include <vector>
#include <Texture.h>

typedef struct {
	float x, y;
	float r, g, b;
	float u, v;

	float texpage;
	float clut_x, clut_y;

	float render_type;
	float flags;
} HardwareVertex;

class HardwareRenderer;

extern HardwareRenderer hw_renderer_singleton;

class HardwareRenderer
{
public:
#pragma pack(push)
	typedef struct {
		u32 texture_mode : 1;
		u32 semi_transparency : 1;
		u32 : 1;
		u32 shading : 1;
		u32 dithering : 1;
	} RenderFlags;
#pragma pack(pop)

	HardwareRenderer();
	void Init();
	~HardwareRenderer();

	void PushTriangle(const float pos[3][2], const float** col, HardwareRenderer::RenderFlags draw_flags/*  pixel_render_t render_type */);
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
	std::vector<HardwareVertex> m_vertices;
	u64 m_vertexCount;
};
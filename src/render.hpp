
#ifndef RENDER_HPP_INCLUDED
#define RENDER_HPP_INCLUDED

#include <gl.hpp>

#define VERT_ELEM_COUNT 8
#define QUAD_ELEM_COUNT (VERT_ELEM_COUNT * 6)
#define QUAD_LINES_ELEM_COUNT (VERT_ELEM_COUNT * 8)

//TODO: Automatically generate these structs for shaders!!
struct Shader {
	u32 id;

	u32 i_position;
	u32 i_tex_coord;
	u32 i_color;

	u32 transform;
	u32 color;
	u32 tex0;
	u32 tex0_dim;

	u32 pixelate_scale;
	u32 fade_amount;
};

enum RenderMode {
	RenderMode_triangles = GL_TRIANGLES,

	RenderMode_lines = GL_LINES,
	RenderMode_line_strip = GL_LINE_STRIP,
};

struct RenderBatch {
	Texture * tex;

	u32 v_len;
	f32 * v_arr;
	gl::VertexBuffer v_buf;
	u32 e;

	RenderMode mode;
};

struct RenderTransform {
	math::Vec2 pos;
	math::Vec2 offset;

	u32 projection_width;
	u32 projection_height;
};

struct RenderElement {
	math::Vec2 pos;
	math::Vec2 dim;
	math::Vec4 color;
	//TODO: Temp!!
	b32 scrollable;

	Asset * asset;
};

struct RenderGroup {
	u32 max_elem_count;
	u32 elem_count;
	RenderElement * elems;

	AssetState * assets;

	RenderTransform transform;
	math::Rec2 projection_bounds;
};

struct Font {
	RenderBatch * batch;

	u32 glyph_width;
	u32 glyph_height;
	u32 glyph_spacing;
};

enum FontLayoutAnchor {
	FontLayoutAnchor_top_left,
	FontLayoutAnchor_top_centre,
	FontLayoutAnchor_top_right,

	FontLayoutAnchor_bottom_left,
};

struct FontLayout {
	math::Vec2 align;
	math::Vec2 pos;
	FontLayoutAnchor anchor;

	//TODO: Remove this from layout??
	f32 scale;
};

struct RenderState {
	MemoryArena * arena;
	AssetState * assets;

	u32 back_buffer_width;
	u32 back_buffer_height;

	u32 screen_width;
	u32 screen_height;

	Shader basic_shader;
	Shader post_shader;

	gl::FrameBuffer frame_buffer;
	gl::VertexBuffer screen_quad_v_buf;

	gl::VertexBuffer quad_v_buf;
	gl::VertexBuffer scrollable_quad_v_buf;

	f32 pixelate_time;
	f32 fade_amount;

	Font * debug_font;
	
	RenderBatch * render_batch;
};

#endif
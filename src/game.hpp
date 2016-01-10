
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>

#define ANIMATION_FRAMES_PER_SEC 1

#define VERTS_PER_QUAD 30

enum TextureId {
	TextureId_dolly,
	TextureId_teacup,

	TextureId_bg_layer0,
	TextureId_bg_layer1,
	TextureId_bg_layer2,
	TextureId_bg_layer3,

	TextureId_debug,

	TextureId_count,
};

struct Shader {
	u32 id;
	u32 xform;
	u32 color;
	u32 tex0;
};

struct RenderBatch {
	//TODO: Batch should probably store texture directly!
	gl::Texture * tex;

	u32 v_len;
	f32 * v_arr;
	gl::VertexBuffer v_buf;
	u32 e;
};

//TODO: Formalise sprite animation!!
// struct SpriteAnimation {
// 	Sprite * sprites;
// 	f32 anim_time;
// };

struct Entity {
	math::Vec3 pos;
	math::Vec2 scale;
	f32 rot;

	math::Vec4 color;
	TextureId tex_id;
	gl::VertexBuffer * v_buf;

	//TODO: Temp!!
	b32 should_render;
};

struct Player {
	Entity * e;

	Sprite * sprite;
	f32 initial_x;
};

struct Teacup {
	Entity * e;

	b32 hit;
};

struct TeacupEmitter {
	math::Vec3 pos;
	math::Vec2 scale;
	f32 time_until_next_spawn;

	u32 entity_count;
	Teacup * entity_array[4];
};

enum ButtonId {
	ButtonId_left,
	ButtonId_right,
	ButtonId_up,
	ButtonId_down,

	//TODO: Temp!!
	ButtonId_mute,

	ButtonId_count,
};

struct Font {
	gl::Texture tex;

	//TODO: This is just a RenderBatch!!
	u32 v_len;
	f32 * v_arr;
	gl::VertexBuffer v_buf;
	u32 e;

	math::Mat4 projection_matrix;

	u32 glyph_width;
	u32 glyph_height;
	u32 glyph_spacing;

	f32 scale;
	math::Vec2 anchor;
	math::Vec2 pos;
};

struct GameMemory {
	size_t size;
	u8 * ptr;

	b32 initialized;
};

struct GameInput {
	u32 back_buffer_width;
	u32 back_buffer_height;

	b32 audio_supported;

	f32 delta_time;
	f32 total_time;

	math::Vec2 mouse_pos;
	math::Vec2 mouse_delta;

	u8 buttons[ButtonId_count];
};

struct GameState {
	MemoryPool memory_pool;

	AssetState asset_state;

	AudioState audio_state;
	AudioSource * music;

	Shader basic_shader;

	u32 entity_program;
	gl::VertexBuffer entity_v_buf;
	gl::VertexBuffer bg_v_buf;

	Font * font;
	Str * debug_str;
	Str * title_str;

	u32 ideal_window_width;
	u32 ideal_window_height;

	math::Mat4 view_matrix;
	math::Mat4 projection_matrix;

	gl::Texture textures[TextureId_count];

	RenderBatch * sprite_batch;

	u32 entity_count;
	Entity entity_array[64];

	Entity * bg_layers[4];

	Player player;
	TeacupEmitter teacup_emitter;

	Entity * pixel_art;

	f32 d_time;
	u32 score;
};

#endif
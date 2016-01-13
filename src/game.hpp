
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>

#define ANIMATION_FRAMES_PER_SEC 30
#define VERTS_PER_QUAD 30

struct Shader {
	u32 id;
	u32 xform;
	u32 color;
	u32 tex0;
};

struct RenderBatch {
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
	//TODO: Should scale simply be a multiplier??
	math::Vec2 scale;
	f32 rot;

	math::Vec4 color;
	f32 anim_time;

	AssetType asset_type;
	AssetId asset_id;
	u32 asset_index;

	gl::VertexBuffer * v_buf;
};

struct Player {
	Entity * e;

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

struct Camera {
	math::Vec2 pos;
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
	RenderBatch * batch;

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

	//TODO: Just rename this to assets!!
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

	math::Mat4 projection_matrix;

	u32 sprite_batch_count;
	RenderBatch ** sprite_batches;

	u32 entity_count;
	Entity entity_array[64];

	u32 bg_layer_count;
	Entity ** bg_layers;

	Player player;
	TeacupEmitter teacup_emitter;

	Entity * animated_entity;

	Camera camera;

	f32 d_time;
	u32 score;
};

#endif
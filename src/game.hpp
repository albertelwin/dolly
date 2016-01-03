
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <audio.hpp>
#include <debug.hpp>
#include <math.hpp>

#define TEXTURE_CHANNELS 4

enum TextureId {
	TextureId_dolly,
	TextureId_teacup,
	TextureId_background,

	TextureId_count,
};

struct Entity {
	math::Vec3 pos;
	math::Vec2 scale;
	f32 rot;

	TextureId tex_id;
};

struct TeacupEmitter {
	math::Vec3 pos;
	f32 time_until_next_spawn;
	
	u32 entity_count;
	Entity * entity_array[4];
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

struct TextBuffer {
	Str str;

	u32 vertex_array_length;
	f32 * vertex_array;

	gl::VertexBuffer vertex_buffer;
};

struct Font {
	u32 program;
	gl::Texture tex;

	u32 glyph_width;
	u32 glyph_height;
	u32 glyph_spacing;
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

	AudioState audio_state;
	AudioSource * music;

	u32 entity_program;
	gl::VertexBuffer entity_vertex_buffer;

	Font font;
	TextBuffer * text_buf;

	math::Mat4 view_matrix;
	math::Mat4 projection_matrix;

	gl::Texture textures[TextureId_count];

	u32 entity_count;
	Entity entity_array[32];

	Entity * background[2];
	Entity * player;

	TeacupEmitter teacup_emitter;

	f32 d_time;
	u32 score;
};

#endif
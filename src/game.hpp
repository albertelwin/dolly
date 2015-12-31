
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <audio.hpp>
#include <debug.hpp>
#include <math.hpp>

#define TEXTURE_CHANNELS 4

struct Entity {
	math::Vec2 pos;
	math::Vec2 scale;

	gl::Texture tex;
	f32 tex_offset;

	f32 scroll_velocity;
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

	u32 font_program;
	gl::Texture font_tex;

	TextBuffer * text_buf;

	math::Mat4 view_matrix;
	math::Mat4 projection_matrix;


	Entity entites[32];

	Entity * player;
	Entity * background;
};

#endif
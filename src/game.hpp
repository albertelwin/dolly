
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <math.hpp>

#define AUDIO_CHANNELS 2
#define AUDIO_CLIP_SAMPLES_PER_SECOND 48000
#define AUDIO_PADDING_SAMPLES 1

enum AudioClipId {
	AudioClipId_sin_440,
	AudioClipId_beep,
	AudioClipId_woosh,
	AudioClipId_music,

	AudioClipId_count,
};

struct AudioClip {
	u32 samples;
	i16 * sample_data;

	f32 length;
};

struct AudioVal64 {
	u32 int_part;
	f32 frc_part;
};

struct AudioSource {
	AudioClipId clip_id;
	AudioVal64 sample_pos;

	b32 loop;
	f32 pitch;
	math::Vec2 volume;

	math::Vec2 target_volume;
	math::Vec2 volume_delta;
	
	AudioSource * next;
	b32 is_free;
};

struct Texture {
	u32 width;
	u32 height;
	u32 id;
};

struct Entity {
	math::Vec2 pos;
	math::Vec2 scale;

	Texture tex;
	f32 tex_offset;

	f32 scroll_velocity;
};

struct GameMemory {
	size_t size;
	void * ptr;

	b32 initialized;
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

	u32 program_id;

	gl::VertexBuffer vertex_buffer;
	u32 texture;

	math::Mat4 view_matrix;
	math::Mat4 projection_matrix;

	AudioClip audio_clips[AudioClipId_count];
	AudioSource * audio_sources;
	AudioSource * audio_source_free_list;

	AudioSource * music;

	Entity entites[32];

	Entity * player;
	Entity * background;
};

#endif
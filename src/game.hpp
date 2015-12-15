
#ifndef NAMESPACE_GAME_INCLUDED
#define NAMESPACE_GAME_INCLUDED

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
	s16 * sample_data;

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

struct GameMemory {
	size_t size;
	void * ptr;

	b32 initialized;
};

struct GameInput {
	u32 back_buffer_width;
	u32 back_buffer_height;

	b32 audio_supported;

	f32 delta_time;
	f32 total_time;

	b32 num_keys[10];
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
};

#endif
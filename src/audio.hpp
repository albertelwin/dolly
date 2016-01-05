
#ifndef AUDIO_HPP_INCLUDED
#define AUDIO_HPP_INCLUDED

#include <math.hpp>

#define AUDIO_CHANNELS 2
#define AUDIO_CLIP_SAMPLES_PER_SECOND 48000
#define AUDIO_PADDING_SAMPLES 1

enum AudioClipId {
	AudioClipId_sin_440,
	AudioClipId_beep,
	AudioClipId_woosh,
	AudioClipId_pickup0,
	AudioClipId_pickup1,
	AudioClipId_pickup2,
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

struct AudioState {
	MemoryPool * memory_pool;

	AudioClip audio_clips[AudioClipId_count];
	AudioSource * audio_sources;
	AudioSource * audio_source_free_list;

	b32 supported;
	f32 master_volume;
};

#endif
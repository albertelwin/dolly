
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
	AudioClipId_pickup,
	AudioClipId_music,

	AudioClipId_count,
};

struct AudioClip {
	u32 samples;
	i16 * sample_data;

	f32 length;
};

struct AudioClipGroup {
	u32 index;
	u32 count;
};

struct AudioVal64 {
	u32 int_part;
	f32 frc_part;
};

struct AudioSource {
	AudioClip * clip;
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

	u32 clip_count;
	AudioClip * clips;
	AudioClipGroup clip_groups[AudioClipId_count];

	AudioSource * sources;
	AudioSource * source_free_list;

	b32 supported;
	f32 master_volume;
};

#endif
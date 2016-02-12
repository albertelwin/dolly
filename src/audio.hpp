
#ifndef AUDIO_HPP_INCLUDED
#define AUDIO_HPP_INCLUDED

#include <asset_format.hpp>
#include <asset.hpp>
#include <math.hpp>

struct AudioVal64 {
	u32 int_part;
	f32 frc_part;
};

enum AudioSourceFlags {
	AudioSourceFlags_loop = 0x1,
	AudioSourceFlags_free_on_end = 0x2,
	AudioSourceFlags_free_on_volume_end = 0x4,
};

struct AudioSource {
	AudioClip * clip;
	AudioVal64 sample_pos;

	u32 flags;

	f32 pitch;
	math::Vec2 volume;

	math::Vec2 target_volume;
	math::Vec2 volume_delta;
	
	AudioSource * next;
};

struct AudioState {
	MemoryArena * arena;
	AssetState * assets;

	AudioSource * sources;
	AudioSource * source_free_list;
	u32 debug_sources_to_free;
	u32 debug_sources_playing;

	b32 supported;
	f32 master_volume;
};

#endif
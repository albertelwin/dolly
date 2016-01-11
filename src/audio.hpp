
#ifndef AUDIO_HPP_INCLUDED
#define AUDIO_HPP_INCLUDED

#include <asset_format.hpp>
#include <asset.hpp>
#include <math.hpp>

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
	AssetState * assets;

	AudioSource * sources;
	AudioSource * source_free_list;

	b32 supported;
	f32 master_volume;
};

#endif
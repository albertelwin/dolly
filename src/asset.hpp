
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

struct Sprite {
	math::Vec2 dim;
	math::Vec2 tex_coords[2];
};

struct AudioClip {
	u32 samples;
	i16 * sample_data;
};

//TOOD: Asset groups!!
struct AudioClipGroup {
	u32 index;
	u32 count;
};

struct AssetState {
	MemoryPool * memory_pool;

	gl::Texture textures[TextureId_count];
	Sprite sprites[SpriteId_count];

	u32 clip_count;
	AudioClip * clips;
	AudioClipGroup clip_groups[AudioClipId_count];
};

#endif
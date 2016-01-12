
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

//TODO: Turn this into a X macro!!
enum AssetType {
	AssetType_texture,
	AssetType_sprite,
	AssetType_audio_clip,

	AssetType_count,
};

struct Asset {
	AssetType type;

	union {
		gl::Texture texture;
		Sprite sprite;
		AudioClip audio_clip;
	};
};

struct AssetGroup {
	u32 index;
	u32 count;
};

struct AssetState {
	MemoryPool * memory_pool;

	u32 asset_count;
	Asset * assets;
	AssetGroup asset_groups[AssetId_count];
};

#endif
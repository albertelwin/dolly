
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

typedef gl::Texture Texture;

struct Sprite {
	u32 atlas_index;
	math::Vec2 dim;
	math::Vec2 tex_coords[2];
	math::Vec2 offset;
};

struct AudioClip {
	u32 samples;
	i16 * sample_data;
};

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X	\
	X(texture, Texture)				\
	X(sprite, Sprite)				\
	X(audio_clip, AudioClip)

#define X(NAME, STRUCT) AssetType_##NAME,
	ASSET_TYPE_NAME_STRUCT_X
#undef X

	AssetType_count,
};

struct Asset {
	AssetType type;

	union {
#define X(NAME, STRUCT) STRUCT NAME;
	ASSET_TYPE_NAME_STRUCT_X
#undef X
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

#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

struct Texture {
	math::Vec2 dim;
	math::Vec2 offset;

	//TODO: Should textures/sprites be in union like this??
	union {
		struct { 
			u32 gl_id; 
		};

		//NOTE: Sprite
		struct {
			math::Vec2 tex_coords[2];
			u32 atlas_index;
		};
	};
};

struct AudioClip {
	u32 samples;
	i16 * sample_data;
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
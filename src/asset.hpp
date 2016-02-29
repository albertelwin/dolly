
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

struct AssetPackZipFile {
	char * file_name;
	char * archive_name;
};

#if DEV_ENABLED
static char const * debug_global_asset_file_names[] = {
	"/dev/placement0.png",
	"/dev/placement1.png",
	"/dev/placement2.png",
	"/dev/placement3.png",
	"/dev/placement4.png",
	"/dev/placement5.png",
	"/dev/placement6.png",
	"/dev/placement7.png",
	"/dev/placement8.png",
	"/dev/placement9.png",
};
#endif

struct Texture {
	math::Vec2 dim;
	//TODO: Too many things named offset, align instead??
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

struct AssetRef {
	AssetId id;
	u32 index;
};

struct AssetGroup {
	u32 index;
	u32 count;
};

struct AssetState {
	MemoryArena * arena;

	u32 asset_count;
	Asset assets[2048];
	AssetGroup asset_groups[AssetId_count];

	f32 debug_load_time;
};

#endif
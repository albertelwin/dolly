
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

enum AssetFileType {
	AssetFileType_pak,
	AssetFileType_one,
};
struct AssetFile {
	char * file_name;

	AssetFileType type;
	union {
		char * archive_name;
		AssetId asset_id;
	};
};

inline AssetFile asset_file_one(char * file_name, AssetId asset_id) {
	AssetFile asset_file = {};
	asset_file.file_name = file_name;
	asset_file.type = AssetFileType_one;
	asset_file.asset_id = asset_id;
	return asset_file;
}

inline AssetFile asset_file_pak(char * file_name, char * archive_name) {
	AssetFile asset_file = {};
	asset_file.file_name = file_name;
	asset_file.type = AssetFileType_pak;
	asset_file.archive_name = archive_name;
	return asset_file;
}

#if DEV_ENABLED
static AssetFile global_dev_asset_files[] = {
	asset_file_one((char *)"/dev/placement0.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement1.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement2.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement3.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement4.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement5.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement6.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement7.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement8.png", AssetId_debug_lower_map),
	asset_file_one((char *)"/dev/placement9.png", AssetId_debug_lower_map),

	asset_file_one((char *)"/dev/placement_space0.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space1.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space2.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space3.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space4.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space5.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space6.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space7.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space8.png", AssetId_debug_upper_map),
	asset_file_one((char *)"/dev/placement_space9.png", AssetId_debug_upper_map),
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

	u32 last_loaded_file_index;
	u32 loaded_file_count;

	u32 asset_count;
	Asset assets[2048];
	AssetGroup asset_groups[AssetId_count];

	f32 debug_load_time;
	u32 debug_total_size;
};

#endif
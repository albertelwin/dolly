
#include <asset.hpp>

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MINIZ_NO_TIME
#include <miniz.c>

Asset * get_asset(AssetState * assets, AssetId id, u32 index) {
	Asset * asset = 0;

	AssetGroup * group = assets->asset_groups + id;
	if(group->count && index < group->count) {
		asset = assets->assets + group->index + index;
	}

	return asset;
}

#define X(NAME, STRUCT)														\
STRUCT * get_##NAME##_asset(AssetState * assets, AssetId id, u32 index) {	\
	STRUCT * NAME = 0;														\
																			\
	Asset * asset = get_asset(assets, id, index);							\
	if(asset) {																\
		ASSERT(asset->type == AssetType_##NAME);							\
		NAME = &asset->NAME;												\
	}																		\
																			\
	return NAME;															\
}
	ASSET_TYPE_NAME_STRUCT_X
#undef X

u32 get_asset_count(AssetState * assets, AssetId id) {
	AssetGroup * group = assets->asset_groups + id;
	return group->count;
}

Asset * push_asset(AssetState * assets, AssetId id, AssetType type) {
	u32 index = assets->asset_count++;

	AssetGroup * group = assets->asset_groups + id;
	if(!group->count) {
		ASSERT(group->count == 0);
		group->index = index;
	}
	else {
		//NOTE: Asset variations must be added sequentially!!
		ASSERT((index - group->index) == group->count);
	}

	group->count++;

	Asset * asset = assets->assets + index;
	asset->type = type;
	return asset;
}

void load_assets(AssetState * assets, MemoryArena * arena) {
	DEBUG_TIME_BLOCK();

	assets->arena = arena;

	MemoryPtr file_buf;
	file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap("asset.zip", "asset.pak", &file_buf.size, 0);
	u8 * file_ptr = file_buf.ptr;

	std::printf("LOG: Uncompressed asset footprint: %uKB\n", file_buf.size / 1024);

	AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
	file_ptr += sizeof(AssetPackHeader);

	u32 total_asset_count = pack->asset_count;

#if DEV_ENABLED
	total_asset_count += ARRAY_COUNT(debug_global_asset_file_names);
#endif

	assets->assets = PUSH_ARRAY(assets->arena, Asset, total_asset_count);
 
	for(u32 i = 0; i < pack->asset_count; i++) {
		AssetInfo * asset_info = (AssetInfo *)file_ptr;
		file_ptr += sizeof(AssetInfo);

		switch(asset_info->type) {
			case AssetType_texture: {
				TextureInfo * info = &asset_info->texture;

				i32 filter = GL_LINEAR;
				if(info->sampling == TextureSampling_point) {
					filter = GL_NEAREST;
				}
				else {
					ASSERT(info->sampling == TextureSampling_bilinear);
				}

				gl::Texture gl_tex = gl::create_texture(file_ptr, info->width, info->height, GL_RGBA, filter, GL_CLAMP_TO_EDGE);

				Asset * asset = push_asset(assets, asset_info->id, AssetType_texture);
				asset->texture.dim = math::vec2(info->width, info->height);
				asset->texture.offset = math::vec2(0.0f);
				asset->texture.gl_id = gl_tex.id;

				file_ptr += info->width * info->height * TEXTURE_CHANNELS;

				break;
			}

			case AssetType_sprite: {
				SpriteInfo * info = &asset_info->sprite;

				Asset * asset = push_asset(assets, asset_info->id, AssetType_sprite);
				asset->sprite.dim = math::vec2(info->width, info->height);
				asset->sprite.offset = info->offset;
				asset->sprite.tex_coords[0] = info->tex_coords[0];
				asset->sprite.tex_coords[1] = info->tex_coords[1];
				asset->sprite.atlas_index = info->atlas_index;

				break;
			}

			case AssetType_audio_clip: {
				AudioClipInfo * info = (AudioClipInfo *)&asset_info->audio_clip;

				Asset * asset = push_asset(assets, asset_info->id, AssetType_audio_clip);
				asset->audio_clip.samples = info->samples;
				asset->audio_clip.sample_data = (i16 *)file_ptr;

				file_ptr += info->size;

				break;
			}

			case AssetType_placement_map: {
				PlacementMapInfo * info = (PlacementMapInfo *)&asset_info->placement_map;

				Asset * asset = push_asset(assets, asset_info->id, AssetType_placement_map);
				asset->placement_map.count = info->count;
				asset->placement_map.placements = (Placement *)file_ptr;

				file_ptr += info->count * sizeof(Placement);

				break;
			}

			INVALID_CASE();
		}
	}

#if DEV_ENABLED
	for(u32 i = 0; i < ARRAY_COUNT(debug_global_asset_file_names); i++) {
		char const * file_name = debug_global_asset_file_names[i];

		i32 width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
		if(img_data) {
			ASSERT(channels == 3 || channels == 4);
			ASSERT(height = PLACEMENT_HEIGHT);

			Asset * asset = push_asset(assets, AssetId_debug_placement, AssetType_placement_map);
			asset->placement_map.count = (u32)width;
			asset->placement_map.placements = ALLOC_ARRAY(Placement, (u32)width);

			for(u32 x = 0; x < (u32)width; x++) {
				Placement * placement = asset->placement_map.placements + x;

				for(u32 y = 0; y < PLACEMENT_HEIGHT; y++) {
					u32 i = (y * (u32)width + x) * channels;
					ColorRGB8 color = color_rgb8(img_data[i + 0], img_data[i + 1], img_data[i + 2]);

					u32 id = AssetId_null;
					for(u32 ii = 0; ii < ARRAY_COUNT(asset_id_color_table); ii++) {
						AssetIdColorRGB8 id_color = asset_id_color_table[ii];
						if(colors_are_equal(color, id_color.color)) {
							id = id_color.id;
							break;
						}
					}

					placement->ids[y] = id;
				}
			}

			stbi_image_free(img_data);			
		}
	}
#endif
 
	// free(file_buf.ptr);
}
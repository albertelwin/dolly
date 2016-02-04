
#include <asset.hpp>

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
	assets->arena = arena;

	MemoryPtr file_buf;
	file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap("asset.zip", "asset.pak", &file_buf.size, 0);
	u8 * file_ptr = file_buf.ptr;

	AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
	file_ptr += sizeof(AssetPackHeader);

	assets->assets = PUSH_ARRAY(assets->arena, Asset, pack->asset_count);
 
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
				asset->audio_clip.sample_data = (i16 *)(info + 1);

				file_ptr += info->size;

				break;
			}

			default: {
				ASSERT(!"Invalid asset type!!");
				break;
			}
		}

	}
 
	// free(file_buf.ptr);
}
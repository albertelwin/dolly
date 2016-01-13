
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

void load_assets(AssetState * assets, MemoryPool * pool) {
	assets->memory_pool = pool;

	MemoryPtr file_buf; 
	file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap("asset.zip", "asset.pak", &file_buf.size, 0);
	u8 * file_ptr = file_buf.ptr;

	AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
	file_ptr += sizeof(AssetPackHeader);

	assets->assets = PUSH_ARRAY(assets->memory_pool, Asset, pack->asset_count);
 
	for(u32 i = 0; i < pack->texture_count; i++) {
		TextureInfo * tex_info = (TextureInfo *)file_ptr;

		SpriteInfo * sprite_info_arr = (SpriteInfo *)(tex_info + 1);
		for(u32 i = 0; i < tex_info->sub_tex_count; i++) {
			SpriteInfo * info = sprite_info_arr + i;

			Asset * asset = push_asset(assets, info->id, AssetType_sprite);
			asset->sprite.dim = info->dim;
			asset->sprite.tex_coords[0] = info->tex_coords[0];
			asset->sprite.tex_coords[1] = info->tex_coords[1];
		}		

		u32 tex_size = tex_info->width * tex_info->height * TEXTURE_CHANNELS;
		u8 * tex_ptr = (u8 *)(sprite_info_arr + tex_info->sub_tex_count);

		i32 filter = GL_LINEAR;
		if(tex_info->sampling == TextureSampling_point) {
			filter = GL_NEAREST;
		}
		else {
			ASSERT(tex_info->sampling == TextureSampling_bilinear);
		}

		Asset * asset = push_asset(assets, tex_info->id, AssetType_texture);
		asset->texture = gl::create_texture(tex_ptr, tex_info->width, tex_info->height, GL_RGBA, filter, GL_CLAMP_TO_EDGE);

		file_ptr = tex_ptr + tex_size;
	}

	for(u32 i = 0; i < pack->clip_count; i++) {
		AudioClipInfo * info = (AudioClipInfo *)file_ptr;

		Asset * asset = push_asset(assets, info->id, AssetType_audio_clip);
		asset->audio_clip.samples = info->samples;
		asset->audio_clip.sample_data = (i16 *)(info + 1);

		file_ptr += sizeof(AudioClipInfo) + info->size;
	}

	// free(file_buf.ptr);
}
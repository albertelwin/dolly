
#include <asset.hpp>

AudioClip * get_audio_clip(AssetState * assets, AudioClipId clip_id, u32 index) {
	AudioClip * clip = 0;

	AudioClipGroup * clip_group = assets->clip_groups + clip_id;
	if(clip_group->index) {
		u32 clip_index = clip_group->index + index;
		clip = assets->clips + clip_index;
	}

	return clip;
}

u32 get_audio_clip_count(AssetState * assets, AudioClipId clip_id) {
	AudioClipGroup * clip_group = assets->clip_groups + clip_id;
	return clip_group->count;
}

void load_assets(AssetState * assets, MemoryPool * pool) {
	assets->memory_pool = pool;

	FileBuffer file_buf = read_file_to_memory("asset.pak");
	u8 * file_ptr = file_buf.ptr;

	AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
	file_ptr += sizeof(AssetPackHeader);

	for(u32 i = 0; i < pack->tex_count; i++) {
		TextureInfo * tex_info = (TextureInfo *)file_ptr;

		SpriteInfo * sprite_info_arr = (SpriteInfo *)(tex_info + 1);
		for(u32 i = 0; i < tex_info->sub_tex_count; i++) {
			SpriteInfo * asset = sprite_info_arr + i;

			Sprite * sprite = assets->sprites + asset->id;
			sprite->dim = asset->dim;
			sprite->tex_coords[0] = asset->tex_coords[0];
			sprite->tex_coords[1] = asset->tex_coords[1];
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

		assets->textures[tex_info->id] = gl::create_texture(tex_ptr, tex_info->width, tex_info->height, GL_RGBA, filter, GL_CLAMP_TO_EDGE);

		file_ptr = tex_ptr + tex_size;
	}

	std::printf("LOG: %u\n", pack->clip_count);

	assets->clip_count = 0;
	assets->clips = PUSH_ARRAY(assets->memory_pool, AudioClip, pack->clip_count);
	for(u32 i = 0; i < pack->clip_count; i++) {
		AudioClipInfo * info = (AudioClipInfo *)file_ptr;

		AudioClipGroup * clip_group = assets->clip_groups + info->id;

		u32 clip_index = assets->clip_count++;
		if(!clip_group->index) {
			ASSERT(clip_group->count == 0);
			clip_group->index = clip_index;
		}
		else {
			//NOTE: Clip variations must be added sequentially!!
			ASSERT((clip_index - clip_group->index) == clip_group->count);
		}

		AudioClip * clip = assets->clips + clip_index;
		clip->samples = info->samples;
		clip->sample_data = (i16 *)(info + 1);

		clip_group->count++;

		file_ptr += sizeof(AudioClipInfo) + info->size;
	}

	// free(file_buf.ptr);
}
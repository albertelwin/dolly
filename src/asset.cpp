
#include <asset.hpp>

#define ZIP_LOCAL_FILE_HEADER_SIGNATURE 0x04034b50
#define ZIP_LZMA_COMPRESSION_METHOD 14
#define ZIP_LZMA_MIN_DICTIONARY_SIZE (1 << 12)

#pragma pack(push, 1)
struct ZipLocalFileHeader {
	u32 signature;
	u16 version_to_extract;
	u16 flags;
	u16 compression_method;
	u16 last_modified_time;
	u16 last_modified_date;
	u32 crc_32;
	u32 compressed_size;
	u32 uncompressed_size;
	u16 file_name_length;
	u16 extra_field_length;
};

struct ZipLzmaHeader {
	u16 version;
	u16 properties_size;
	u8 properties_byte;
	u32 dictionary_size;
};
#pragma pack(pop)

MemoryPtr load_and_decompress_zip_file(char const * file_name) {
	MemoryPtr file_ptr = read_file_to_memory(file_name);
	ASSERT(file_ptr.ptr);

	ZipLocalFileHeader * local_file_header = (ZipLocalFileHeader *)file_ptr.ptr;
	ASSERT(local_file_header->signature = ZIP_LOCAL_FILE_HEADER_SIGNATURE);
	ASSERT(local_file_header->compression_method == ZIP_LZMA_COMPRESSION_METHOD);

	Str local_file_name = str_fixed_size((char *)(local_file_header + 1), local_file_header->file_name_length);
	ASSERT(str_equal(&local_file_name, "asset.pak"));

	std::printf("LOG: __ZIP_LOCAL_FILE_HEADER_______\n");
	std::printf("LOG:  - signature 0x%08x\n", local_file_header->signature);
	std::printf("LOG:  - version_to_extract %u\n", local_file_header->version_to_extract);
	std::printf("LOG:  - flags %u\n", local_file_header->flags);
	std::printf("LOG:  - compression_method %u\n", local_file_header->compression_method);
	std::printf("LOG:  - last_modified_time %u\n", local_file_header->last_modified_time);
	std::printf("LOG:  - last_modified_date %u\n", local_file_header->last_modified_date);
	std::printf("LOG:  - crc_32 %u\n", local_file_header->crc_32);
	std::printf("LOG:  - compressed_size %u\n", local_file_header->compressed_size);
	std::printf("LOG:  - uncompressed_size %u\n", local_file_header->uncompressed_size);
	std::printf("LOG:  - file_name_length %u\n", local_file_header->file_name_length);
	std::printf("LOG:  - extra_field_length %u\n", local_file_header->extra_field_length);
	std::printf("LOG:\n");

	u32 local_file_header_size = sizeof(ZipLocalFileHeader) + local_file_header->file_name_length + local_file_header->extra_field_length * sizeof(u16) * 2;
	u8 * compressed_data = file_ptr.ptr + local_file_header_size;

	ZipLzmaHeader * lzma_header = (ZipLzmaHeader *)compressed_data;
	ASSERT(lzma_header->properties_size == 5);

	u32 lzma_literal_context_bits = lzma_header->properties_byte % 9;
	u32 lzma_literal_pos_bits = (lzma_header->properties_byte / 9) % 5;
	u32 lzma_pos_bits = (lzma_header->properties_byte / 9) / 5;

	ASSERT(lzma_literal_context_bits <= 8);
	ASSERT(lzma_literal_pos_bits <= 4);
	ASSERT(lzma_pos_bits <= 4);

	u32 lzma_dictionary_size = lzma_header->dictionary_size;
	if(lzma_dictionary_size < ZIP_LZMA_MIN_DICTIONARY_SIZE) {
		lzma_dictionary_size = ZIP_LZMA_MIN_DICTIONARY_SIZE;
	}

	//TODO: Allocate output buffer!!

	u32 lzma_probability_arr_size = 4096 + 1536 * (1 << (lzma_literal_context_bits + lzma_literal_pos_bits));
	u16 * lzma_probability_arr = ALLOC_MEMORY(u16, lzma_probability_arr_size);

	std::printf("LOG: __ZIP_LZMA_HEADER_____________\n");
	std::printf("LOG:  - version %u\n", lzma_header->version);
	std::printf("LOG:  - dictionary_size: %u\n", lzma_dictionary_size);
	std::printf("LOG:  - lzma_literal_context_bits: %u\n", lzma_literal_context_bits);
	std::printf("LOG:  - lzma_literal_pos_bits: %u\n", lzma_literal_pos_bits);
	std::printf("LOG:  - lzma_pos_bits: %u\n", lzma_pos_bits);
	std::printf("LOG:  - lzma_probability_arr_size: %u\n", lzma_probability_arr_size);

	FREE_MEMORY(lzma_probability_arr);

	return {};
}

AudioClip * get_audio_clip(AssetState * assets, AudioClipId clip_id, u32 index) {
	AudioClip * clip = 0;

	AudioClipGroup * clip_group = assets->clip_groups + clip_id;
	if(clip_group->count) {
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

	load_and_decompress_zip_file("asset.zip");

	MemoryPtr file_buf = read_file_to_memory("asset.pak");
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

	assets->clip_count = pack->clip_count;
	assets->clips = PUSH_ARRAY(assets->memory_pool, AudioClip, pack->clip_count);

	for(u32 i = 0; i < assets->clip_count; i++) {
		AudioClipInfo * info = (AudioClipInfo *)file_ptr;

		AudioClipGroup * clip_group = assets->clip_groups + info->id;
		if(!clip_group->index) {
			ASSERT(clip_group->count == 0);
			clip_group->index = i;
		}
		else {
			//NOTE: Clip variations must be added sequentially!!
			ASSERT((i - clip_group->index) == clip_group->count);
		}

		AudioClip * clip = assets->clips + i;
		clip->samples = info->samples;
		clip->sample_data = (i16 *)(info + 1);

		clip_group->count++;

		file_ptr += sizeof(AudioClipInfo) + info->size;
	}

	// free(file_buf.ptr);
}
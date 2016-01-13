
#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sys.hpp>

#include <asset_format.hpp>

#define PACK_RIFF_CODE(x, y, z, w) ((u32)(x) << 0) | ((u32)(y) << 8) | ((u32)(z) << 16) | ((u32)(w) << 24)
enum RiffCode {
	RiffCode_RIFF = PACK_RIFF_CODE('R', 'I', 'F', 'F'),
	RiffCode_WAVE = PACK_RIFF_CODE('W', 'A', 'V', 'E'),
	RiffCode_fmt = PACK_RIFF_CODE('f', 'm', 't', ' '),
	RiffCode_data = PACK_RIFF_CODE('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)
struct WavHeader {
	u32 riff_id;
	u32 size;
	u32 wave_id;
};

struct WavChunkHeader {
	u32 id;
	u32 size;
};

struct WavFormat {
	u16 tag;
	u16 channels;
	u32 samples_per_second;
	u32 avg_bytes_per_second;
	u16 block_align;
	u16 bits_per_sample;
	u16 extension_size;
	u16 valid_bits_per_sample;
	u32 channel_mask;
	u8 guid[16];
};
#pragma pack(pop)

struct Texture {
	AssetId id;

	u32 width;
	u32 height;
	TextureSampling sampling;

	u32 size;
	u8 * ptr;

	u32 x;
	u32 y;
	u32 safe_y;
};

struct TexCoord {
	u32 u;
	u32 v;
};

struct AssetFile {
	AssetId id;
	char * name;
};

struct AudioClip {
	AssetId id;

	u32 samples;

	u32 size;
	i16 * ptr;
};

struct TextureAtlas {
	Texture tex;

	u32 sprite_count;
	SpriteInfo sprites[128];
};

struct AssetPacker {
	AssetPackHeader header;

	u32 atlas_count;
	TextureAtlas atlases[64];
};

Texture load_texture(char const * file_name, AssetId id, TextureSampling sampling = TextureSampling_bilinear) {
	stbi_set_flip_vertically_on_load(true);

	i32 width, height, channels;
	u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
	ASSERT(channels == TEXTURE_CHANNELS);

	Texture tex = {};
	tex.id = id;
	tex.width = (u32)width;
	tex.height = (u32)height;	
	tex.sampling = sampling;

	//NOTE: Premultiplied alpha!!
	f32 r_255 = 1.0f / 255.0f;
	for(u32 y = 0, i = 0; y < tex.height; y++) {
		for(u32 x = 0; x < tex.width; x++, i += 4) {
			f32 a = (f32)img_data[i + 3] * r_255;

			f32 r = (f32)img_data[i + 0] * r_255 * a;
			f32 g = (f32)img_data[i + 1] * r_255 * a;
			f32 b = (f32)img_data[i + 2] * r_255 * a;

			img_data[i + 0] = (u8)(r * 255.0f);
			img_data[i + 1] = (u8)(g * 255.0f);
			img_data[i + 2] = (u8)(b * 255.0f);
		}	
	}

	tex.size = tex.width * tex.height * TEXTURE_CHANNELS;
	tex.ptr = img_data;

	return tex;
}

Texture allocate_texture(u32 width, u32 height, AssetId id, TextureSampling sampling = TextureSampling_bilinear) {
	Texture tex = {};
	tex.id = id;

	tex.width = width;
	tex.height = height;
	tex.sampling = sampling;

	tex.size = tex.width * tex.height * TEXTURE_CHANNELS;
	tex.ptr = ALLOC_ARRAY(u8, tex.size);

	tex.x = TEXTURE_PADDING_PIXELS;
	tex.y = tex.safe_y = TEXTURE_PADDING_PIXELS;

	for(u32 y = 0, i = 0; y < tex.height; y++) {
		for(u32 x = 0; x < tex.width; x++, i += 4) {
			tex.ptr[i + 0] = 0;
			tex.ptr[i + 1] = 0;
			tex.ptr[i + 2] = 0;
			tex.ptr[i + 3] = 0;
		}
	}

	return tex;
}

struct Blit { u32 u; u32 v; u32 width; u32 height; i32 min_x; i32 min_y; };
Blit blit_texture(Texture * dst, Texture * src) {
	u32 min_x = U32_MAX;
	u32 min_y = U32_MAX;

	u32 max_x = 0;
	u32 max_y = 0;

	for(u32 y = 0, i = 0; y < src->height; y++) {
		for(u32 x = 0; x < src->width; x++, i += 4) {
			u8 a = src->ptr[i + 3];
			if(a) {
				if(x < min_x) {
					min_x = x;
				}

				if(y < min_y) {
					min_y = y;
				}

				if(x > max_x) {
					max_x = x;
				}

				if(y > max_y) {
					max_y = y;
				}
			}
		}
	}

	u32 width = (max_x + 1) - min_x;
	u32 height = (max_y + 1) - min_y;
	ASSERT(width > 0 && height > 0);

	u32 pad = TEXTURE_PADDING_PIXELS;
	u32 pad_2 = pad * 2;

	ASSERT((width + pad_2) <= dst->width);

	if((dst->x + width + pad) > dst->width) {
		dst->x = pad;
		dst->y = dst->safe_y;
	}

	ASSERT((dst->y + height + pad) <= dst->height);

	for(u32 y = 0; y < height; y++) {
		for(u32 x = 0; x < width; x++) {
			u32 d_i = ((y + dst->y) * dst->width + x + dst->x) * TEXTURE_CHANNELS;
			u32 s_i = ((y + min_y) * src->width + (x + min_x)) * TEXTURE_CHANNELS;

			dst->ptr[d_i + 0] = src->ptr[s_i + 0];
			dst->ptr[d_i + 1] = src->ptr[s_i + 1];
			dst->ptr[d_i + 2] = src->ptr[s_i + 2];
			dst->ptr[d_i + 3] = src->ptr[s_i + 3];
		}
	}

	Blit result = {};
	result.u = dst->x - pad;
	result.v = dst->y - pad;
	result.width = width + pad_2;
	result.height = height + pad_2;
	result.min_x = (i32)min_x - 1;
	result.min_y = (i32)min_y - 1;

	u32 y = dst->y + height + pad_2;
	if(y > dst->safe_y) {
		dst->safe_y = y;
	}

	dst->x += width + pad_2;
	ASSERT(dst->x <= dst->width);
	if(dst->x >= dst->width) {
		dst->x = pad;
		dst->y = dst->safe_y;
	}

	return result;
}

AudioClip load_audio_clip(char const * file_name, AssetId id) {
	AudioClip clip = {};
	clip.id = id;

	MemoryPtr file_buffer = read_file_to_memory(file_name);

	WavHeader * wav_header = (WavHeader *)file_buffer.ptr;
	ASSERT(wav_header->riff_id == RiffCode_RIFF);
	ASSERT(wav_header->wave_id == RiffCode_WAVE);

	WavChunkHeader * wav_format_header = (WavChunkHeader *)((u8 *)wav_header + sizeof(WavHeader));
	ASSERT(wav_format_header->id == RiffCode_fmt);

	WavFormat * wav_format = (WavFormat *)((u8 *)wav_format_header + sizeof(WavChunkHeader));
	ASSERT(wav_format->channels <= AUDIO_CHANNELS);
	ASSERT(wav_format->samples_per_second == AUDIO_CLIP_SAMPLES_PER_SECOND);
	ASSERT((wav_format->bits_per_sample / 8) == sizeof(i16));

	WavChunkHeader * wav_data_header = (WavChunkHeader *)((u8 *)wav_format_header + sizeof(WavChunkHeader) + wav_format_header->size);
	ASSERT(wav_data_header->id == RiffCode_data);

	i16 * wav_data = (i16 *)((u8 *)wav_data_header + sizeof(WavChunkHeader));

	clip.samples = wav_data_header->size / (wav_format->channels * sizeof(i16));
	ASSERT(clip.samples > 0);
	u32 samples_with_padding = clip.samples + AUDIO_PADDING_SAMPLES;

	clip.size = samples_with_padding * sizeof(i16) * AUDIO_CHANNELS; 
	clip.ptr = ALLOC_MEMORY(i16, clip.size);
	if(wav_format->channels == 2) {
		for(u32 i = 0; i < samples_with_padding * 2; i++) {
			clip.ptr[i] = wav_data[i % (clip.samples * 2)];
		}
	}
	else {
		for(u32 i = 0, sample_index = 0; i < samples_with_padding; i++) {
			i16 sample = wav_data[i % clip.samples];
			clip.ptr[sample_index++] = sample;
			clip.ptr[sample_index++] = sample;
		}
	}

	FREE_MEMORY(file_buffer.ptr);

	return clip;
}

void push_packed_texture(AssetPacker * packer, AssetFile * files, u32 file_count) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));

	u32 atlas_index = packer->atlas_count++;
	TextureAtlas * atlas = packer->atlases + atlas_index;
	atlas->tex = allocate_texture(512, 512, AssetId_atlas);

	f32 r_tex_size = 1.0f / (f32)atlas->tex.width;

	for(u32 i = 0; i < file_count; i++) {
		AssetFile * file = files + i;

		Texture tex = load_texture(file->name, AssetId_null);
		math::Vec2 tex_dim = math::vec2(tex.width, tex.height);

		Blit blit = blit_texture(&atlas->tex, &tex);
		math::Vec2 blit_dim = math::vec2(blit.width, blit.height);

		math::Rec2 sub_rec = math::rec2_min_dim(math::vec2(blit.min_x, blit.min_y), blit_dim);

		SpriteInfo * sprite = atlas->sprites + atlas->sprite_count++;
		sprite->id = file->id;
		sprite->atlas_index = atlas_index;
		sprite->dim = blit_dim;
		sprite->tex_coords[0] = math::vec2(blit.u, blit.v) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(blit.u + blit.width, blit.v + blit.height) * r_tex_size;
		sprite->offset = math::rec_centre(sub_rec) - tex_dim * 0.5f;
	}

	packer->header.texture_count++;
	packer->header.sprite_count += atlas->sprite_count;
}

void push_sprite_sheet(AssetPacker * packer, char const * file_name, AssetId sprite_id, u32 sprite_width, u32 sprite_height, u32 sprite_count) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));

	u32 atlas_index = packer->atlas_count++;
	TextureAtlas * atlas = packer->atlases + atlas_index;
	atlas->tex = load_texture("sprite_sheet.png", AssetId_atlas);

	u32 tex_size = atlas->tex.width;
	f32 r_tex_size = 1.0f / tex_size;

	math::Vec2 sprite_dim = math::vec2(sprite_width, sprite_height);
	u32 sprites_in_row = atlas->tex.width / sprite_width;
	u32 sprites_in_col = atlas->tex.height / sprite_height;

	for(u32 y = 0; y < sprites_in_col; y++) {
		for(u32 x = 0; x < sprites_in_row; x++) {
			ASSERT(atlas->sprite_count < ARRAY_COUNT(atlas->sprites));

			SpriteInfo * sprite = atlas->sprites + atlas->sprite_count++;
			sprite->id = sprite_id;
			sprite->atlas_index = atlas_index;
			sprite->dim = sprite_dim;

			u32 u = x * sprite_width;
			u32 v = y * sprite_height;
			sprite->tex_coords[0] = math::vec2(u, tex_size - (v + sprite_height)) * r_tex_size;
			sprite->tex_coords[1] = math::vec2(u + sprite_width, tex_size - v) * r_tex_size;

			sprite->offset = math::vec2(0.0f, 0.0f);
		}
	}

	atlas->sprite_count = sprite_count;

	packer->header.texture_count++;
	packer->header.sprite_count += atlas->sprite_count;
}

int main() {
	AssetPacker asset_packer = {};
	AssetPackHeader * asset_pack = &asset_packer.header;

	AssetFile sprite_files[] = {
		{ AssetId_dolly, "dolly.png" },
		{ AssetId_dolly, "dolly0.png" },
		{ AssetId_dolly, "dolly1.png" },
		{ AssetId_dolly, "dolly2.png" },
		{ AssetId_dolly, "dolly3.png" },
		{ AssetId_dolly, "dolly4.png" },
		{ AssetId_dolly, "dolly5.png" },
		{ AssetId_dolly, "dolly6.png" },
		{ AssetId_dolly, "dolly7.png" },

		{ AssetId_teacup, "teacup.png" },
	};

	push_packed_texture(&asset_packer, sprite_files, ARRAY_COUNT(sprite_files));
	push_sprite_sheet(&asset_packer, "sprite_sheet.png", AssetId_animation, 56, 156, 24);

	Texture reg_tex_array[] = {
		load_texture("font.png", AssetId_font, TextureSampling_point),

		load_texture("bg_layer0.png", AssetId_bg_layer),
		load_texture("bg_layer1.png", AssetId_bg_layer),
		load_texture("bg_layer2.png", AssetId_bg_layer),
		load_texture("bg_layer3.png", AssetId_bg_layer),
	};

	asset_pack->texture_count += ARRAY_COUNT(reg_tex_array);

	AudioClip clips[] = {
		load_audio_clip("sin_440.wav", AssetId_sin_440),
		load_audio_clip("woosh.wav", AssetId_woosh),
		load_audio_clip("beep.wav", AssetId_beep),
		load_audio_clip("pickup0.wav", AssetId_pickup),
		load_audio_clip("pickup1.wav", AssetId_pickup),
		load_audio_clip("pickup2.wav", AssetId_pickup),
		load_audio_clip("explosion0.wav", AssetId_explosion),
		load_audio_clip("explosion1.wav", AssetId_explosion),
		load_audio_clip("music.wav", AssetId_music),
	};

	asset_pack->clip_count += ARRAY_COUNT(clips);
	asset_pack->asset_count = asset_pack->texture_count + asset_pack->sprite_count + asset_pack->clip_count;

	std::FILE * file_ptr = std::fopen("asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(asset_pack, sizeof(AssetPackHeader), 1, file_ptr);

	for(u32 i = 0; i < asset_packer.atlas_count; i++) {
		TextureAtlas * atlas = asset_packer.atlases + i;
		Texture * tex = &atlas->tex;

		TextureInfo info = {};
		info.id = tex->id;
		info.width = tex->width;
		info.height = tex->height;
		info.sampling = tex->sampling;
		info.sub_tex_count = atlas->sprite_count;

		std::fwrite(&info, sizeof(TextureInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

	for(u32 i = 0; i < ARRAY_COUNT(reg_tex_array); i++) {
		Texture * tex = reg_tex_array + i;

		TextureInfo info = {};
		info.id = tex->id;
		info.width = tex->width;
		info.height = tex->height;
		info.sampling = tex->sampling;
		info.sub_tex_count = 0;

		std::fwrite(&info, sizeof(TextureInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

	for(u32 i = 0; i < asset_packer.atlas_count; i++) {
		TextureAtlas * atlas = asset_packer.atlases + i;
		std::fwrite(atlas->sprites, sizeof(SpriteInfo), atlas->sprite_count, file_ptr);
	}

	for(u32 i = 0; i < ARRAY_COUNT(clips); i++) {
		AudioClip * clip = clips + i;

		AudioClipInfo info = {};
		info.id = clip->id;
		info.samples = clip->samples;
		info.size = clip->size;

		std::fwrite(&info, sizeof(AudioClipInfo), 1, file_ptr);
		std::fwrite(clip->ptr, clip->size, 1, file_ptr);
	}

	std::fclose(file_ptr);

	return 0;
}
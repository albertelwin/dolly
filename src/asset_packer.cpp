
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
	TextureId id;

	u32 width;
	u32 height;

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

struct LoadReq {
	char * file_name;
	SpriteId id;
};

struct LoadReqArray {
	u32 count;
	LoadReq array[128];
};

struct AudioClip {
	AudioClipId id;

	u32 samples;

	u32 size;
	i16 * ptr;
};

void push_req(LoadReqArray * reqs, char * file_name, SpriteId id) {
	ASSERT(reqs->count < (ARRAY_COUNT(reqs->array) - 1));

	LoadReq * req = reqs->array + reqs->count++;
	req->file_name = file_name;
	req->id = id;
}

Texture load_texture(char const * file_name, TextureId id, TextureSampling filter = TextureSampling_bilinear) {
	stbi_set_flip_vertically_on_load(true);

	i32 width, height, channels;
	u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
	ASSERT(channels == TEXTURE_CHANNELS);

	Texture tex = {};
	tex.id = id;
	tex.width = (u32)width;
	tex.height = (u32)height;	

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

Texture allocate_texture(u32 width, u32 height) {
	Texture tex = {};
	tex.width = width;
	tex.height = height;
	tex.size = tex.width * tex.height * TEXTURE_CHANNELS;
	tex.ptr = ALLOC_ARRAY(u8, tex.size);

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

struct Blit { u32 u; u32 v; u32 width; u32 height; };
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

	u32 width = max_x - min_x;
	u32 height = max_y - min_y;
	ASSERT(width > 0 && height > 0);

	if((dst->x + width) > dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	ASSERT((dst->y + height <= dst->height));

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
	result.u = dst->x;
	result.v = dst->y;
	result.width = width;
	result.height = height;

	//NOTE: 1px padding for bilinear filter!!
	u32 pad = 1;

	u32 y = dst->y + height + pad;
	if(y > dst->safe_y) {
		dst->safe_y = y;
	}

	dst->x += width + pad;
	ASSERT(dst->x <= dst->width);
	if(dst->x >= dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	return result;
}

AudioClip load_audio_clip(char const * file_name, AudioClipId id) {
	AudioClip clip = {};
	clip.id = id;

	FileBuffer file_buffer = read_file_to_memory(file_name);

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
	u32 samples_with_padding = clip.samples + AUDIO_PADDING_SAMPLES;

	clip.size = samples_with_padding * sizeof(i16) * AUDIO_CHANNELS; 
	clip.ptr = ALLOC_ARRAY(i16, samples_with_padding * AUDIO_CHANNELS);
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

	free(file_buffer.ptr);

	return clip;
}

int main() {
	AssetPackHeader asset_pack = {};
	asset_pack.tex_count = 1;
	asset_pack.clip_count = 0;

	LoadReqArray reqs = {};

	push_req(&reqs, "dolly0.png", SpriteId_null);
	push_req(&reqs, "dolly1.png", SpriteId_null);
	push_req(&reqs, "dolly2.png", SpriteId_null);
	push_req(&reqs, "dolly3.png", SpriteId_null);
	push_req(&reqs, "dolly4.png", SpriteId_null);
	push_req(&reqs, "dolly5.png", SpriteId_null);
	push_req(&reqs, "dolly6.png", SpriteId_null);
	push_req(&reqs, "dolly7.png", SpriteId_null);

	push_req(&reqs, "dolly.png", SpriteId_dolly);
	push_req(&reqs, "teacup.png", SpriteId_teacup);

	TextureInfo tex_info = {};
	tex_info.id = TextureId_atlas;
	tex_info.width = 512;
	tex_info.height = 512;
	tex_info.sub_tex_count = reqs.count;

	Texture atlas = allocate_texture(tex_info.width, tex_info.height);

	SpriteInfo * sprites = ALLOC_ARRAY(SpriteInfo, reqs.count);
	for(u32 i = 0; i < reqs.count; i++) {
		LoadReq * req = reqs.array + i;

		Texture tex_ = load_texture(req->file_name, TextureId_null);
		Texture * tex = &tex_;

		Blit blit = blit_texture(&atlas, tex);

		u32 tex_size = tex_info.width;
		f32 r_tex_size = 1.0f / (f32)tex_size;

		SpriteInfo * sprite = sprites + i;
		sprite->id = req->id;
		sprite->dim = math::vec2(blit.width, blit.height);
		sprite->tex_coords[0] = math::vec2(blit.u, blit.v) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(blit.u + blit.width, blit.v + blit.height) * r_tex_size;
	}

	Texture reg_tex_array[] = {
		load_texture("font.png", TextureId_font, TextureSampling_point),

		load_texture("bg_layer0.png", TextureId_bg_layer0),
		load_texture("bg_layer1.png", TextureId_bg_layer1),
		load_texture("bg_layer2.png", TextureId_bg_layer2),
		load_texture("bg_layer3.png", TextureId_bg_layer3),
	};

	asset_pack.tex_count += ARRAY_COUNT(reg_tex_array);

	AudioClip clips[] = {
		load_audio_clip("sin_440.wav", AudioClipId_sin_440),
		load_audio_clip("woosh.wav", AudioClipId_woosh),
		load_audio_clip("beep.wav", AudioClipId_beep),
		load_audio_clip("pickup0.wav", AudioClipId_pickup),
		load_audio_clip("pickup1.wav", AudioClipId_pickup),
		load_audio_clip("pickup2.wav", AudioClipId_pickup),
		load_audio_clip("explosion0.wav", AudioClipId_explosion),
		load_audio_clip("explosion1.wav", AudioClipId_explosion),
		load_audio_clip("music.wav", AudioClipId_music),
	};

	asset_pack.clip_count += ARRAY_COUNT(clips);

	std::FILE * file_ptr = std::fopen("asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&asset_pack, sizeof(AssetPackHeader), 1, file_ptr);
	std::fwrite(&tex_info, sizeof(TextureInfo), 1, file_ptr);
	std::fwrite(sprites, sizeof(SpriteInfo), reqs.count, file_ptr);
	std::fwrite(atlas.ptr, atlas.size, 1, file_ptr);

	for(u32 i = 0; i < ARRAY_COUNT(reg_tex_array); i++) {
		Texture * tex = reg_tex_array + i;

		TextureInfo info = {};
		info.id = tex->id;
		info.width = tex->width;
		info.height = tex->height;
		info.sub_tex_count = 0;

		std::fwrite(&info, sizeof(TextureInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
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
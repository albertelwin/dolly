
#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <sys.hpp>

#include <asset_format.hpp>

#define PACK_AUDIO_ASSETS 0

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

struct AudioClip {
	AssetId id;

	u32 samples;

	u32 size;
	i16 * ptr;
};

struct TextureAtlas {
	Texture tex;

	//TODO: These can just be part of the texture struct and we'll alloc them!!
	u32 sprite_count;
	AssetInfo sprites[256];
};

struct TileMapAsset {
	TileMap map;
	AssetId id;
};

struct FontAsset {
	Font font;
	AssetId id;
};

struct AssetFile {
	char * name;
	AssetId id;
};

struct AssetPacker {
	u32 atlas_count;
	TextureAtlas atlases[32];

	u32 font_count;
	FontAsset fonts[256];

	u32 texture_count;
	Texture textures[256];

	u32 audio_clip_count;
	AudioClip audio_clips[256];

	u32 tile_map_count;
	TileMapAsset tile_maps[256];
};

u8 * load_image_from_file(char const * file_name, i32 * width, i32 * height, i32 * channels) {
	stbi_set_flip_vertically_on_load(true);

	u8 * img_data = stbi_load(file_name, width, height, channels, 0);
	if(!img_data) {
		std::printf("ERROR: Could not find %s!!\n", file_name);
		ASSERT(!"Texture not found!");
	}

	return img_data;
}

TileMapAsset load_tile_map(char const * file_name, AssetId asset_id) {
	i32 width, height, channels;
	u8 * img_data = load_image_from_file(file_name, &width, &height, &channels);
	ASSERT(channels == 3 || channels == 4);
	ASSERT(height == TILE_MAP_HEIGHT);

	TileMapAsset map_asset = {};
	map_asset.id = asset_id;
	map_asset.map.width = (u32)width;
	map_asset.map.tiles = ALLOC_ARRAY(Tiles, width);

	for(u32 x = 0; x < (u32)width; x++) {
		Tiles * tiles = map_asset.map.tiles + x;

		for(u32 y = 0; y < TILE_MAP_HEIGHT; y++) {
			u32 i = (y * (u32)width + x) * channels;
			ColorRGB8 color = color_rgb8(img_data[i + 0], img_data[i + 1], img_data[i + 2]);

			TileId id = TileId_null;
			for(u32 ii = 0; ii < ARRAY_COUNT(tile_id_color_table); ii++) {
				TileIdColorRGB8 id_color = tile_id_color_table[ii];
				if(colors_are_equal(color, id_color.color)) {
					id = id_color.id;
					break;
				}
			}

			tiles->ids[y] = id;
		}
	}

	return map_asset;
}

Texture load_texture(char const * file_name, AssetId id, TextureSampling sampling = TextureSampling_bilinear) {
	i32 width, height, channels;
	u8 * img_data = load_image_from_file(file_name, &width, &height, &channels);

	Texture tex = {};
	tex.id = id;
	tex.width = (u32)width;
	tex.height = (u32)height;
	tex.sampling = sampling;

	if(channels == 4) {
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
	}
	else {
		ASSERT(channels == 3);

		u8 * img_data_ = ALLOC_MEMORY(u8, tex.width * tex.height * TEXTURE_CHANNELS);

		for(u32 y = 0, i = 0; y < tex.height; y++) {
			for(u32 x = 0; x < tex.width; x++, i++) {
				u32 i3 = i * 3;
				u32 i4 = i * 4;

				img_data_[i4 + 0] = img_data[i3 + 0];
				img_data_[i4 + 1] = img_data[i3 + 1];
				img_data_[i4 + 2] = img_data[i3 + 2];
				img_data_[i4 + 3] = 255;
			}
		}

		stbi_image_free(img_data);
		img_data = img_data_;
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
	ASSERT(dst->x <= (dst->width + pad));
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
	// if(!file_buffer.ptr) {
	// 	std::printf("ERROR: Could not find %s!!\n", file_name);
	// 	ASSERT(!"Audio clip not found!");
	// }

	WavHeader * wav_header = (WavHeader *)file_buffer.ptr;
	ASSERT(wav_header->riff_id == RiffCode_RIFF);
	ASSERT(wav_header->wave_id == RiffCode_WAVE);

	WavChunkHeader * wav_format_header = (WavChunkHeader *)((u8 *)wav_header + sizeof(WavHeader));
	ASSERT(wav_format_header->id == RiffCode_fmt);

	WavFormat * wav_format = (WavFormat *)((u8 *)wav_format_header + sizeof(WavChunkHeader));
	ASSERT(wav_format->channels <= AUDIO_CHANNELS);
	ASSERT(wav_format->samples_per_second == AUDIO_SAMPLE_RATE);
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

void push_sprite(TextureAtlas * atlas, AssetId asset_id, u32 atlas_index, Blit * blit, math::Vec2 sprite_dim) {
	ASSERT(atlas->sprite_count < ARRAY_COUNT(atlas->sprites));
	AssetInfo * info = atlas->sprites + atlas->sprite_count++;
	info->id = asset_id;
	info->type = AssetType_sprite;

	math::Vec2 r_atlas_dim = math::vec2(1.0f / (f32)atlas->tex.width, 1.0f / (f32)atlas->tex.height);
	math::Rec2 blit_rec = math::rec2_min_dim(math::vec2(blit->min_x, blit->min_y), math::vec2(blit->width, blit->height));

	SpriteInfo * sprite = &info->sprite;
	sprite->atlas_index = atlas_index;
	sprite->width = blit->width;
	sprite->height = blit->height;
	sprite->offset = math::rec_pos(blit_rec) - sprite_dim * 0.5f;
	sprite->tex_coords[0] = math::vec2(blit->u, blit->v) * r_atlas_dim;
	sprite->tex_coords[1] = math::vec2(blit->u + blit->width, blit->v + blit->height) * r_atlas_dim;
}

void push_font(AssetPacker * packer, char const * file_name, AssetId font_id, f32 pixel_height) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));
	ASSERT(packer->font_count < ARRAY_COUNT(packer->fonts));

	FontAsset * font_asset = packer->fonts + packer->font_count++;
	font_asset->id = font_id;

	stbtt_fontinfo ttf_info;
	MemoryPtr ttf_file = read_file_to_memory(file_name);
	ASSERT(ttf_file.ptr);
	stbtt_InitFont(&ttf_info, ttf_file.ptr, stbtt_GetFontOffsetForIndex(ttf_file.ptr, 0));

	f32 scale_factor = stbtt_ScaleForPixelHeight(&ttf_info, pixel_height);

	i32 ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&ttf_info, &ascent, &descent, &line_gap);
	// std::printf("LOG: %f\n", line_gap * scale_factor);

	i32 whitespace_advance;
	stbtt_GetCodepointHMetrics(&ttf_info, ' ', &whitespace_advance, 0);

	Font * font = &font_asset->font;
	font->glyphs = ALLOC_ARRAY(FontGlyph, FONT_GLYPH_COUNT);
	font->ascent = ascent * scale_factor;
	font->descent = descent * scale_factor;
	font->whitespace_advance = whitespace_advance * scale_factor;
	font->atlas_index = packer->atlas_count++;
	font->glyph_id = font_asset->id + 1;

	TextureAtlas * atlas = packer->atlases + font->atlas_index;
	atlas->tex = allocate_texture(384, 384, AssetId_atlas);

	f32 r_tex_size = 1.0f / (f32)atlas->tex.width;

	//TODO: Collapse this!!
	for(char code_point = FONT_FIRST_CHAR; code_point < FONT_ONE_PAST_LAST_CHAR; code_point++) {
		FontGlyph * glyph = font->glyphs + get_font_glyph_index((char)code_point);

		i32 bitmap_width, bitmap_height;
		i32 x_offset, y_offset;
		u8 * bitmap_data = stbtt_GetCodepointBitmap(&ttf_info, 0, scale_factor, code_point, &bitmap_width, &bitmap_height, &x_offset, &y_offset);
		ASSERT(bitmap_data != 0);

		i32 advance, left_side_bearing;
		stbtt_GetCodepointHMetrics(&ttf_info, code_point, &advance, &left_side_bearing);

		glyph->advance = advance * scale_factor;

		Texture tex = {};
		tex.width = (u32)bitmap_width;
		tex.height = (u32)bitmap_height;
		tex.sampling = TextureSampling_bilinear;
		tex.size = tex.width * tex.height * TEXTURE_CHANNELS;
		tex.ptr = ALLOC_ARRAY(u8, tex.size);

		for(u32 y = 0, i = 0; y < tex.height; y++) {
			for(u32 x = 0; x < tex.width; x++, i += TEXTURE_CHANNELS) {
				u8 a = bitmap_data[((tex.height - 1) - y) * tex.width + x];

				tex.ptr[i + 0] = a;
				tex.ptr[i + 1] = a;
				tex.ptr[i + 2] = a;
				tex.ptr[i + 3] = a;
			}
		}

		math::Vec2 tex_dim = math::vec2(tex.width, tex.height);

		Blit blit = blit_texture(&atlas->tex, &tex);

		ASSERT(atlas->sprite_count < ARRAY_COUNT(atlas->sprites));
		AssetInfo * info = atlas->sprites + atlas->sprite_count++;
		info->id = (AssetId)font->glyph_id;
		info->type = AssetType_sprite;

		SpriteInfo * sprite = &info->sprite;
		sprite->atlas_index = font->atlas_index;
		sprite->width = blit.width;
		sprite->height = blit.height;
		sprite->offset = math::vec2(tex_dim.x * 0.5f + x_offset, -tex_dim.y * 0.5f - y_offset);
		sprite->tex_coords[0] = math::vec2(blit.u, blit.v) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(blit.u + blit.width, blit.v + blit.height) * r_tex_size;

		stbtt_FreeBitmap(bitmap_data, 0);
		FREE_MEMORY(tex.ptr);
	}

	FREE_MEMORY(ttf_file.ptr);
}

void push_texture(AssetPacker * packer, char * file_name, AssetId asset_id, TextureSampling sampling = TextureSampling_bilinear) {
	ASSERT(packer->texture_count < ARRAY_COUNT(packer->textures));

	Texture * texture = packer->textures + packer->texture_count++;
	*texture = load_texture(file_name, asset_id, sampling);
}

void push_audio_clip(AssetPacker * packer, char * file_name, AssetId asset_id) {
	ASSERT(packer->audio_clip_count < ARRAY_COUNT(packer->audio_clips));

	AudioClip * audio_clip = packer->audio_clips + packer->audio_clip_count++;
	*audio_clip = load_audio_clip(file_name, asset_id);
}

void push_tile_map(AssetPacker * packer, char * file_name, AssetId asset_id) {
	ASSERT(packer->tile_map_count < ARRAY_COUNT(packer->tile_maps));

	TileMapAsset * tile_map = packer->tile_maps + packer->tile_map_count++;
	*tile_map = load_tile_map(file_name, asset_id);
}

void begin_packed_texture(AssetPacker * packer, TextureSampling sampling = TextureSampling_bilinear) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));

	u32 atlas_index = packer->atlas_count++;
	TextureAtlas * atlas = packer->atlases + atlas_index;
	atlas->tex = allocate_texture(1536, 1536, AssetId_atlas, sampling);
}

void pack_sprite(AssetPacker * packer, char * file_name, AssetId asset_id) {
	ASSERT(packer->atlas_count);

	u32 atlas_index = packer->atlas_count - 1;
	TextureAtlas * atlas = packer->atlases + atlas_index;

	Texture blit_tex = load_texture(file_name, AssetId_null);
	Blit blit = blit_texture(&atlas->tex, &blit_tex);

	push_sprite(atlas, asset_id, atlas_index, &blit, math::vec2(blit_tex.width, blit_tex.height));
}

void pack_sprite_sheet(AssetPacker * packer, char const * file_name, AssetId sprite_id, u32 sprite_width, u32 sprite_height, u32 max_sprites_to_pack = U32_MAX) {
	ASSERT(packer->atlas_count);

	u32 atlas_index = packer->atlas_count - 1;
	TextureAtlas * atlas = packer->atlases + atlas_index;

	Texture source_tex = load_texture(file_name, AssetId_null);

	math::Vec2 sprite_dim = math::vec2(sprite_width, sprite_height);
	u32 sprites_in_row = source_tex.width / sprite_width;
	u32 sprites_in_col = source_tex.height / sprite_height;

	u32 sprites_to_pack = sprites_in_row * sprites_in_col;
	if(sprites_to_pack > max_sprites_to_pack) {
		sprites_to_pack = max_sprites_to_pack;
	}

	Texture blit_tex = {};
	blit_tex.width = (u32)sprite_width;
	blit_tex.height = (u32)sprite_height;
	blit_tex.sampling = TextureSampling_bilinear;
	blit_tex.size = blit_tex.width * blit_tex.height * TEXTURE_CHANNELS;
	blit_tex.ptr = ALLOC_ARRAY(u8, blit_tex.size);

	for(u32 i = 0; i < sprites_to_pack; i++) {
		u32 y = i / sprites_in_row;
		u32 x = i % sprites_in_row;

		u32 u = x * sprite_width;
		u32 v = (sprites_in_col - (y + 1)) * sprite_height;

		for(u32 yy = 0, ii = 0; yy < sprite_height; yy++) {
			for(u32 xx = 0; xx < sprite_width; xx++, ii += 4) {
				u32 kk = ((v + yy) * source_tex.width + (u + xx)) * 4;

				blit_tex.ptr[ii + 0] = source_tex.ptr[kk + 0];
				blit_tex.ptr[ii + 1] = source_tex.ptr[kk + 1];
				blit_tex.ptr[ii + 2] = source_tex.ptr[kk + 2];
				blit_tex.ptr[ii + 3] = source_tex.ptr[kk + 3];
			}
		}

		Blit blit = blit_texture(&atlas->tex, &blit_tex);
		push_sprite(atlas, sprite_id, atlas_index, &blit, math::vec2(blit_tex.width, blit_tex.height));
	}

	FREE_MEMORY(blit_tex.ptr);
}

void write_out_asset_pack(AssetPacker * packer, char * file_name) {
	std::FILE * file_ptr = std::fopen(file_name, "wb");
	ASSERT(file_ptr != 0);

	AssetPackHeader header = {};
	header.asset_count += packer->atlas_count;
	for(u32 i = 0; i < packer->atlas_count; i++) {
		header.asset_count += packer->atlases[i].sprite_count;
	}
	header.asset_count += packer->texture_count;
	header.asset_count += packer->audio_clip_count;
	header.asset_count += packer->tile_map_count;
	header.asset_count += packer->font_count;
	std::fwrite(&header, sizeof(AssetPackHeader), 1, file_ptr);

	for(u32 i = 0; i < packer->atlas_count; i++) {
		TextureAtlas * atlas = packer->atlases + i;
		Texture * tex = &atlas->tex;

		AssetInfo info = {};
		info.id = tex->id;
		info.type = AssetType_texture;
		info.texture.width = tex->width;
		info.texture.height = tex->height;
		info.texture.offset = math::vec2(0.0f);
		info.texture.sampling = tex->sampling;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

	for(u32 i = 0; i < packer->atlas_count; i++) {
		TextureAtlas * atlas = packer->atlases + i;
		std::fwrite(atlas->sprites, sizeof(AssetInfo), atlas->sprite_count, file_ptr);
	}

	for(u32 i = 0; i < packer->texture_count; i++) {
		Texture * tex = packer->textures + i;

		AssetInfo info = {};
		info.id = tex->id;
		info.type = AssetType_texture;
		info.texture.width = tex->width;
		info.texture.height = tex->height;
		info.texture.sampling = tex->sampling;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

#if PACK_AUDIO_ASSETS
	for(u32 i = 0; i < ARRAY_COUNT(clips); i++) {
		AudioClip * clip = clips + i;

		AssetInfo info = {};
		info.id = clip->id;
		info.type = AssetType_audio_clip;
		info.audio_clip.samples = clip->samples;
		info.audio_clip.size = clip->size;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(clip->ptr, clip->size, 1, file_ptr);
	}
#endif

	for(u32 i = 0; i < packer->tile_map_count; i++) {
		TileMapAsset * map_asset = packer->tile_maps + i;

		AssetInfo info = {};
		info.id = map_asset->id;
		info.type = AssetType_tile_map;
		info.tile_map.width = map_asset->map.width;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(map_asset->map.tiles, sizeof(Tiles), map_asset->map.width, file_ptr);
	}

	for(u32 i = 0; i < packer->font_count; i++) {
		FontAsset * font_asset = packer->fonts + i;
		Font * font = &font_asset->font;

		AssetInfo info = {};
		info.id = font_asset->id;
		info.type = AssetType_font;
		info.font.glyph_id = font->glyph_id;
		info.font.ascent = font->ascent;
		info.font.descent = font->descent;
		info.font.whitespace_advance = font->whitespace_advance;
		info.font.atlas_index = font->atlas_index;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(font->glyphs, sizeof(FontGlyph), FONT_GLYPH_COUNT, file_ptr);
	}

	std::fclose(file_ptr);
	ZERO_STRUCT(packer);
}

int main() {
	AssetPacker * packer = ALLOC_STRUCT(AssetPacker);
	ZERO_STRUCT(packer);

	{
		push_texture(packer, "white.png", AssetId_white);
		push_texture(packer, "load_body.png", AssetId_load_background);

		write_out_asset_pack(packer, "preload.pak");
	}

	{
		char tmp_buf[256];
		Str tmp_str = str_fixed_size(tmp_buf, ARRAY_COUNT(tmp_buf));
		for(u32 i = 0; i < 27; i++) {
			str_clear(&tmp_str);
			str_print(&tmp_str, "placement%u.png", i);
			push_tile_map(packer, tmp_str.ptr, AssetId_lower_map);
		}

		push_tile_map(packer, "placement_space0.png", AssetId_upper_map),

		push_tile_map(packer, "tutorial.png", AssetId_tutorial_map),

		write_out_asset_pack(packer, "map.pak");
	}

	{
		push_texture(packer, "menu_background.png", AssetId_menu_background);
		push_texture(packer, "score_background.png", AssetId_score_background);

		push_texture(packer, "dundee_background.png", AssetId_background);
		push_texture(packer, "edinburgh_background.png", AssetId_background);
		push_texture(packer, "highlands_background.png", AssetId_background);
		push_texture(packer, "ocean_background.png", AssetId_background);

		push_texture(packer, "dundee_layer0.png", AssetId_scene_dundee);
		push_texture(packer, "dundee_layer1.png", AssetId_scene_dundee);
		push_texture(packer, "dundee_layer2.png", AssetId_scene_dundee);
		push_texture(packer, "dundee_layer3.png", AssetId_scene_dundee);

		push_texture(packer, "edinburgh_layer0.png", AssetId_scene_edinburgh);
		push_texture(packer, "edinburgh_layer1.png", AssetId_scene_edinburgh);
		push_texture(packer, "edinburgh_layer2.png", AssetId_scene_edinburgh);
		push_texture(packer, "edinburgh_layer3.png", AssetId_scene_edinburgh);

		push_texture(packer, "highlands_layer0.png", AssetId_scene_highlands);
		push_texture(packer, "highlands_layer1.png", AssetId_scene_highlands);
		push_texture(packer, "highlands_layer2.png", AssetId_scene_highlands);
		push_texture(packer, "highlands_layer3.png", AssetId_scene_highlands);

		push_texture(packer, "ocean_layer0.png", AssetId_scene_forth);
		push_texture(packer, "ocean_layer1.png", AssetId_scene_forth);
		push_texture(packer, "empty_layer.png", AssetId_scene_forth);
		push_texture(packer, "ocean_layer3.png", AssetId_scene_forth);

		push_texture(packer, "space_layer0.png", AssetId_scene_space);
		push_texture(packer, "space_layer1.png", AssetId_scene_space);
		push_texture(packer, "space_layer2.png", AssetId_scene_space);
		push_texture(packer, "empty_layer.png", AssetId_scene_space);

		push_texture(packer, "clouds.png", AssetId_clouds);

		write_out_asset_pack(packer, "texture.pak");
	}

#if 0
	{
		push_audio_clip(packer, "wav/pickup0.wav", AssetId_pickup),
		push_audio_clip(packer, "wav/pickup1.wav", AssetId_pickup),
		push_audio_clip(packer, "wav/pickup2.wav", AssetId_pickup),
		push_audio_clip(packer, "wav/bang0.wav", AssetId_bang),
		push_audio_clip(packer, "wav/bang1.wav", AssetId_bang),
		push_audio_clip(packer, "wav/baa0.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa1.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa2.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa3.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa4.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa5.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa6.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa7.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa8.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa9.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa10.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa11.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa12.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa13.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa14.wav", AssetId_baa),
		push_audio_clip(packer, "wav/baa15.wav", AssetId_baa),
		push_audio_clip(packer, "wav/special.wav", AssetId_special),

		push_audio_clip(packer, "wav/click_yes.wav", AssetId_click_yes),
		push_audio_clip(packer, "wav/click_no.wav", AssetId_click_no),

		push_audio_clip(packer, "wav/rocket_sfx.wav", AssetId_rocket_sfx),

		push_audio_clip(packer, "wav/menu_music.wav", AssetId_menu_music),
		push_audio_clip(packer, "wav/game_music.wav", AssetId_game_music),
		push_audio_clip(packer, "wav/space_music.wav", AssetId_space_music),

		write_out_asset_pack(packer, "audio.pak");
	}
#endif

	//TODO: Spread atlases across multiple files??
	{
		push_font(packer, "pragmata_pro.ttf", AssetId_pragmata_pro, 16.0f),
		push_font(packer, "munro.ttf", AssetId_munro, 30.0f),
		push_font(packer, "munro.ttf", AssetId_munro_large, 60.0f),

		begin_packed_texture(packer);

		pack_sprite(packer, "sun.png", AssetId_sun);
		pack_sprite(packer, "rocket.png", AssetId_rocket);

		pack_sprite_sheet(packer, "atom_smasher_4fer.png", AssetId_atom_smasher_4fer, 96, 240, 18);
		pack_sprite_sheet(packer, "atom_smasher_3fer.png", AssetId_atom_smasher_3fer, 96, 192, 18);
		pack_sprite_sheet(packer, "atom_smasher_2fer.png", AssetId_atom_smasher_2fer, 96, 144, 16);
		pack_sprite_sheet(packer, "atom_smasher_1fer.png", AssetId_atom_smasher_1fer, 96, 96, 22);

		pack_sprite(packer, "circle.png", AssetId_circle);
		pack_sprite_sheet(packer, "glow.png", AssetId_glow, 96, 96);
		pack_sprite_sheet(packer, "shield.png", AssetId_shield, 96, 96, 30);

		pack_sprite_sheet(packer, "dolly_idle.png", AssetId_dolly_idle, 48, 48);
		pack_sprite_sheet(packer, "dolly_up.png", AssetId_dolly_up, 48, 48);
		pack_sprite_sheet(packer, "dolly_down.png", AssetId_dolly_down, 48, 48);
		pack_sprite_sheet(packer, "dolly_hit.png", AssetId_dolly_hit, 48, 48);
		pack_sprite_sheet(packer, "dolly_fall.png", AssetId_dolly_fall, 48, 48);

		pack_sprite(packer, "dolly_space_idle.png", AssetId_dolly_space_idle);
		pack_sprite(packer, "dolly_space_up.png", AssetId_dolly_space_up);
		pack_sprite(packer, "dolly_space_down.png", AssetId_dolly_space_down);

		pack_sprite_sheet(packer, "speed_up.png", AssetId_goggles, 48, 48);

		pack_sprite(packer, "label_clone.png", AssetId_label_clone);
		pack_sprite(packer, "clone.png", AssetId_clone);
		pack_sprite(packer, "clone_blink.png", AssetId_clone_blink);
		pack_sprite_sheet(packer, "clone_space.png", AssetId_clone_space, 48, 48);

#define X(NAME) pack_sprite(packer, "collect_" #NAME ".png", AssetId_collect_##NAME);
		ASSET_ID_COLLECT_X
#undef X

		pack_sprite(packer, "concord.png", AssetId_concord);

		begin_packed_texture(packer, TextureSampling_point);

		pack_sprite(packer, "rocket_large0.png", AssetId_rocket_large);
		pack_sprite(packer, "rocket_large1.png", AssetId_rocket_large);
		pack_sprite(packer, "rocket_large2.png", AssetId_rocket_large);

		pack_sprite(packer, "about_body.png", AssetId_about_body);
		pack_sprite(packer, "about_title.png", AssetId_about_title);

		pack_sprite(packer, "btn_play_off.png", AssetId_btn_play);
		pack_sprite(packer, "btn_play_on.png", AssetId_btn_play);
		pack_sprite(packer, "btn_about_off.png", AssetId_btn_about);
		pack_sprite(packer, "btn_about_on.png", AssetId_btn_about);
		pack_sprite(packer, "btn_menu_off.png", AssetId_btn_back);
		pack_sprite(packer, "btn_menu_on.png", AssetId_btn_back);
		pack_sprite(packer, "btn_baa_off.png", AssetId_btn_baa);
		pack_sprite(packer, "btn_baa_on.png", AssetId_btn_baa);
		pack_sprite(packer, "btn_replay_off.png", AssetId_btn_replay);
		pack_sprite(packer, "btn_replay_on.png", AssetId_btn_replay);
		pack_sprite(packer, "btn_up_off.png", AssetId_btn_up);
		pack_sprite(packer, "btn_up_on.png", AssetId_btn_up);
		pack_sprite(packer, "btn_down_off.png", AssetId_btn_down);
		pack_sprite(packer, "btn_down_on.png", AssetId_btn_down);
		pack_sprite(packer, "btn_next_off.png", AssetId_btn_skip);
		pack_sprite(packer, "btn_next_on.png", AssetId_btn_skip);

		pack_sprite(packer, "score_clone.png", AssetId_score_clone);

		pack_sprite_sheet(packer, "intro0.png", AssetId_intro0, 75, 75);
		pack_sprite_sheet(packer, "intro1.png", AssetId_intro1, 75, 75);
		pack_sprite_sheet(packer, "intro2.png", AssetId_intro2, 75, 75);
		pack_sprite_sheet(packer, "intro3.png", AssetId_intro3, 75, 75);
		pack_sprite_sheet(packer, "intro4.png", AssetId_intro4, 75, 75, 61);
		pack_sprite_sheet(packer, "intro5.png", AssetId_intro5, 75, 75);

		pack_sprite(packer, "intro4_background0.png", AssetId_intro4_background);
		pack_sprite(packer, "intro4_background1.png", AssetId_intro4_background);

#define X(NAME) pack_sprite(packer, "score_" #NAME ".png", AssetId_score_##NAME);
		ASSET_ID_COLLECT_X
#undef X

		write_out_asset_pack(packer, "atlas.pak");
	}

	return 0;
}
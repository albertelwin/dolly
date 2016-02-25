
#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

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
	AssetInfo sprites[128];
};

struct TileMapAsset {
	TileMap map;
	AssetId id;
};

struct FontAsset {
	Font font;
	AssetId id;
};

struct AssetPacker {
	AssetPackHeader header;

	u32 atlas_count;
	TextureAtlas atlases[64];
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
	atlas->tex = allocate_texture(2048, 2048, AssetId_atlas);

	f32 r_tex_size = 1.0f / (f32)atlas->tex.width;

	for(u32 i = 0; i < file_count; i++) {
		AssetFile * file = files + i;

		Texture tex = load_texture(file->name, AssetId_null);
		math::Vec2 tex_dim = math::vec2(tex.width, tex.height);

		Blit blit = blit_texture(&atlas->tex, &tex);
		math::Rec2 blit_rec = math::rec2_min_dim(math::vec2(blit.min_x, blit.min_y), math::vec2(blit.width, blit.height));

		ASSERT(atlas->sprite_count < ARRAY_COUNT(atlas->sprites));
		AssetInfo * info = atlas->sprites + atlas->sprite_count++;
		info->id = file->id;
		info->type = AssetType_sprite;

		SpriteInfo * sprite = &info->sprite;
		sprite->atlas_index = atlas_index;
		sprite->width = blit.width;
		sprite->height = blit.height;
		sprite->offset = math::rec_pos(blit_rec) - tex_dim * 0.5f;
		sprite->tex_coords[0] = math::vec2(blit.u, blit.v) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(blit.u + blit.width, blit.v + blit.height) * r_tex_size;
	}

	packer->header.asset_count++;
	packer->header.asset_count += atlas->sprite_count;
}

void push_sprite_sheet(AssetPacker * packer, char const * file_name, AssetId sprite_id, u32 sprite_width, u32 sprite_height, u32 sprite_count) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));

	u32 atlas_index = packer->atlas_count++;
	TextureAtlas * atlas = packer->atlases + atlas_index;
	atlas->tex = load_texture(file_name, AssetId_atlas);

	u32 tex_width = atlas->tex.width;
	u32 tex_height = atlas->tex.height;

	math::Vec2 r_tex_dim = math::vec2(1.0f / (f32)tex_width, 1.0f / (f32)tex_height);

	math::Vec2 sprite_dim = math::vec2(sprite_width, sprite_height);
	u32 sprites_in_row = tex_width / sprite_width;
	u32 sprites_in_col = tex_height / sprite_height;

	for(u32 y = 0; y < sprites_in_col; y++) {
		for(u32 x = 0; x < sprites_in_row; x++) {
			u32 u = x * sprite_width;
			u32 v = y * sprite_height;

			ASSERT(atlas->sprite_count < ARRAY_COUNT(atlas->sprites));
			AssetInfo * info = atlas->sprites + atlas->sprite_count++;
			info->id = sprite_id;
			info->type = AssetType_sprite;

			SpriteInfo * sprite = &info->sprite;
			sprite->atlas_index = atlas_index;
			sprite->width = sprite_width;
			sprite->height = sprite_height;
			sprite->offset = math::vec2(0.0f, 0.0f);
			sprite->tex_coords[0] = math::vec2(u, tex_height - (v + sprite_height)) * r_tex_dim;
			sprite->tex_coords[1] = math::vec2(u + sprite_width, tex_height - v) * r_tex_dim;
		}
	}

	atlas->sprite_count = sprite_count;

	packer->header.asset_count++;
	packer->header.asset_count += atlas->sprite_count;
}

FontAsset load_font(AssetPacker * packer, char const * file_name, AssetId font_id, f32 pixel_height) {
	ASSERT(packer->atlas_count < ARRAY_COUNT(packer->atlases));

	FontAsset font_asset = {};
	font_asset.id = font_id;

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

	Font * font = &font_asset.font;
	font->glyphs = ALLOC_ARRAY(FontGlyph, FONT_GLYPH_COUNT);
	font->ascent = ascent * scale_factor;
	font->descent = descent * scale_factor;
	font->whitespace_advance = whitespace_advance * scale_factor;
	font->atlas_index = packer->atlas_count++;
	font->glyph_id = font_asset.id + 1;

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

	packer->header.asset_count++;
	packer->header.asset_count += atlas->sprite_count;

	FREE_MEMORY(ttf_file.ptr);

	return font_asset;
}

#define PACK_EVERYTHING 1

int main() {
	//TODO: Automatically sync these up!!
	ASSERT(ASSET_GROUP_COUNT(collect) == ASSET_GROUP_COUNT(menu_collect));

	AssetPacker packer = {};

#if PACK_EVERYTHING
	FontAsset fonts[] = {
		load_font(&packer, "pragmata_pro.ttf", AssetId_pragmata_pro, 20.0f),
		load_font(&packer, "supersrc.ttf", AssetId_supersrc, 30.0f),
	};

	packer.header.asset_count += ARRAY_COUNT(fonts);
#endif

	AssetFile sprite_files[] = {
		{ AssetId_dolly_idle, "dolly_idle.png" },
		{ AssetId_dolly_up, "dolly_up.png" },
		{ AssetId_dolly_down, "dolly_down.png" },

		{ AssetId_dolly_space_idle, "dolly_space_idle.png" },
		{ AssetId_dolly_space_up, "dolly_space_up.png" },
		{ AssetId_dolly_space_down, "dolly_space_down.png" },

		{ AssetId_rocket, "rocket.png" },
		{ AssetId_rocket_large, "rocket_large0.png" },
		{ AssetId_rocket_large, "rocket_large1.png" },
		{ AssetId_rocket_large, "rocket_large2.png" },
		{ AssetId_goggles, "speed_up.png" },
		{ AssetId_shield, "shield.png" },
		{ AssetId_clone, "clone.png" },
		{ AssetId_clone_space, "clone_space.png" },
		{ AssetId_clock, "clock_.png" },

#define X(NAME) { AssetId_collect_##NAME, "collect_" #NAME ".png" },
		ASSET_ID_COLLECT_X
#undef X

#define X(NAME) { AssetId_menu_collect_##NAME, "menu_collect_" #NAME ".png" },
		ASSET_ID_COLLECT_X
#undef X

		{ AssetId_concord, "concord0.png" },
		{ AssetId_concord, "concord1.png" },
	};
	push_packed_texture(&packer, sprite_files, ARRAY_COUNT(sprite_files));

	AssetFile ui_sprite_files[] = {
		{ AssetId_btn_play, "btn_play0.png" },
		{ AssetId_btn_play, "btn_play1.png" },
		{ AssetId_btn_about, "btn_about0.png" },
		{ AssetId_btn_about, "btn_about1.png" },
		{ AssetId_btn_back, "btn_back0.png" },
		{ AssetId_btn_back, "btn_back1.png" },
		{ AssetId_btn_baa, "btn_baa0.png" },
		{ AssetId_btn_baa, "btn_baa1.png" },
		{ AssetId_btn_replay, "btn_replay0.png" },
		{ AssetId_btn_replay, "btn_replay1.png" },
		{ AssetId_btn_up, "btn_up0.png" },
		{ AssetId_btn_up, "btn_up1.png" },
		{ AssetId_btn_down, "btn_down0.png" },
		{ AssetId_btn_down, "btn_down1.png" },
		{ AssetId_btn_skip, "btn_skip0.png" },
		{ AssetId_btn_skip, "btn_skip1.png" },

		{ AssetId_icon_clone, "icon_clone.png" },
		{ AssetId_icon_clock, "clock_.png" },

		{ AssetId_intro, "intro0.png" },
		{ AssetId_intro, "intro1.png" },
		{ AssetId_intro, "intro2.png" },
		{ AssetId_intro, "intro3.png" },

		{ AssetId_sun, "sun.png" },
	};
	push_packed_texture(&packer, ui_sprite_files, ARRAY_COUNT(ui_sprite_files));

	push_sprite_sheet(&packer, "dolly_fall.png", AssetId_dolly_fall, 64, 64, 4);
	push_sprite_sheet(&packer, "atom_smasher.png", AssetId_atom_smasher, 128, 128, 19);

	Texture reg_tex_array[] = {
		load_texture("white.png", AssetId_white),

		load_texture("menu_background.png", AssetId_menu_background),
		load_texture("menu_about.png", AssetId_menu_background),

		load_texture("intro_background.png", AssetId_intro_background),

		load_texture("city_background.png", AssetId_background),
		load_texture("highlands_background.png", AssetId_background),
		load_texture("ocean_background_.png", AssetId_background),

		load_texture("city_layer0.png", AssetId_scene_city),
		load_texture("city_layer1.png", AssetId_scene_city),
		load_texture("city_layer2.png", AssetId_scene_city),
		load_texture("city_layer3.png", AssetId_scene_city),

		load_texture("highlands_layer0.png", AssetId_scene_highlands),
		load_texture("highlands_layer1.png", AssetId_scene_highlands),
		load_texture("highlands_layer2.png", AssetId_scene_highlands),
		load_texture("highlands_layer3.png", AssetId_scene_highlands),

		load_texture("ocean_layer0.png", AssetId_scene_ocean),
		load_texture("ocean_layer1.png", AssetId_scene_ocean),
		load_texture("empty_layer.png", AssetId_scene_ocean),
		load_texture("empty_layer.png", AssetId_scene_ocean),

		load_texture("space_layer0.png", AssetId_scene_space),
		load_texture("space_layer1.png", AssetId_scene_space),
		load_texture("space_layer2.png", AssetId_scene_space),
		load_texture("empty_layer.png", AssetId_scene_space),

		load_texture("clouds.png", AssetId_clouds),
	};

	packer.header.asset_count += ARRAY_COUNT(reg_tex_array);

#if PACK_EVERYTHING
	AudioClip clips[] = {
		load_audio_clip("pickup0.wav", AssetId_pickup),
		load_audio_clip("pickup1.wav", AssetId_pickup),
		load_audio_clip("pickup2.wav", AssetId_pickup),
		load_audio_clip("bang0.wav", AssetId_bang),
		load_audio_clip("bang1.wav", AssetId_bang),
		load_audio_clip("baa0.wav", AssetId_baa),
		load_audio_clip("baa1.wav", AssetId_baa),
		load_audio_clip("baa2.wav", AssetId_baa),
		load_audio_clip("baa3.wav", AssetId_baa),
		load_audio_clip("baa4.wav", AssetId_baa),
		load_audio_clip("baa5.wav", AssetId_baa),
		load_audio_clip("baa6.wav", AssetId_baa),
		load_audio_clip("baa7.wav", AssetId_baa),
		load_audio_clip("baa8.wav", AssetId_baa),
		load_audio_clip("baa9.wav", AssetId_baa),
		load_audio_clip("baa10.wav", AssetId_baa),
		load_audio_clip("baa11.wav", AssetId_baa),
		load_audio_clip("baa12.wav", AssetId_baa),
		load_audio_clip("baa13.wav", AssetId_baa),
		load_audio_clip("baa14.wav", AssetId_baa),
		load_audio_clip("baa15.wav", AssetId_baa),
		load_audio_clip("special.wav", AssetId_special),

		load_audio_clip("click_yes.wav", AssetId_click_yes),
		load_audio_clip("click_no.wav", AssetId_click_no),

		load_audio_clip("tick_tock.wav", AssetId_tick_tock),
		load_audio_clip("rocket_sfx.wav", AssetId_rocket_sfx),

		load_audio_clip("game_music.wav", AssetId_game_music),
		load_audio_clip("space_music.wav", AssetId_space_music),
	};

	packer.header.asset_count += ARRAY_COUNT(clips);
#endif

#if PACK_EVERYTHING
	TileMapAsset tile_maps[] = {
		load_tile_map("tile_map0.png", AssetId_tile_map),
		load_tile_map("tile_map1.png", AssetId_tile_map),

		load_tile_map("placement0.png", AssetId_lower_map),
		load_tile_map("placement1.png", AssetId_lower_map),
		load_tile_map("placement2.png", AssetId_lower_map),
		load_tile_map("placement3.png", AssetId_lower_map),

		load_tile_map("placement_space0.png", AssetId_upper_map),

		load_tile_map("tutorial.png", AssetId_tutorial_map),
	};

	packer.header.asset_count += ARRAY_COUNT(tile_maps);
#endif

	std::FILE * file_ptr = std::fopen("asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&packer.header, sizeof(AssetPackHeader), 1, file_ptr);

	for(u32 i = 0; i < packer.atlas_count; i++) {
		TextureAtlas * atlas = packer.atlases + i;
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

	for(u32 i = 0; i < packer.atlas_count; i++) {
		TextureAtlas * atlas = packer.atlases + i;
		std::fwrite(atlas->sprites, sizeof(AssetInfo), atlas->sprite_count, file_ptr);
	}

	for(u32 i = 0; i < ARRAY_COUNT(reg_tex_array); i++) {
		Texture * tex = reg_tex_array + i;

		AssetInfo info = {};
		info.id = tex->id;
		info.type = AssetType_texture;
		info.texture.width = tex->width;
		info.texture.height = tex->height;
		info.texture.sampling = tex->sampling;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

#if PACK_EVERYTHING
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

	for(u32 i = 0; i < ARRAY_COUNT(tile_maps); i++) {
		TileMapAsset * map_asset = tile_maps + i;

		AssetInfo info = {};
		info.id = map_asset->id;
		info.type = AssetType_tile_map;
		info.tile_map.width = map_asset->map.width;

		std::fwrite(&info, sizeof(AssetInfo), 1, file_ptr);
		std::fwrite(map_asset->map.tiles, sizeof(Tiles), map_asset->map.width, file_ptr);
	}

	for(u32 i = 0; i < ARRAY_COUNT(fonts); i++) {
		FontAsset * font_asset = fonts + i;
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
#endif

	std::fclose(file_ptr);

	return 0;
}
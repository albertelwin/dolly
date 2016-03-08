
#include <asset.hpp>

#if DEV_ENABLED
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#define MINIZ_NO_TIME
#include <miniz.c>

#define STB_VORBIS_NO_PUSHDATA_API
#include <stb_vorbis.c>

AssetRef asset_ref(AssetId id, u32 index = 0) {
	AssetRef ref;
	ref.id = id;
	ref.index = index;
	return ref;
}

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
	ASSERT(assets->asset_count < ARRAY_COUNT(assets->assets));

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

void process_asset_file(AssetState * assets, AssetFile asset_file) {
	//TODO: Pull this out!!
	if(asset_file.type == AssetFileType_pak) {
		MemoryPtr file_buf;
		//TODO: Can we just get the first archive??
		file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap(asset_file.file_name, asset_file.archive_name, &file_buf.size, 0);
		u8 * file_ptr = file_buf.ptr;

		assets->debug_total_size += file_buf.size;

		AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
		file_ptr += sizeof(AssetPackHeader);
	 
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

				case AssetType_tile_map: {
					TileMapInfo * info = &asset_info->tile_map;

					Asset * asset = push_asset(assets, asset_info->id, AssetType_tile_map);
					asset->tile_map.width = info->width;
					asset->tile_map.tiles = (Tiles *)file_ptr;

					file_ptr += info->width * sizeof(Tiles);

					break;
				}

				case AssetType_font: {
					FontInfo * info = &asset_info->font;

					Asset * asset = push_asset(assets, asset_info->id, AssetType_font);
					asset->font.glyphs = (FontGlyph *)file_ptr;
					asset->font.glyph_id = info->glyph_id;
					asset->font.ascent = info->ascent;
					asset->font.descent = info->descent;
					asset->font.whitespace_advance = info->whitespace_advance;
					asset->font.atlas_index = info->atlas_index;

					file_ptr += sizeof(FontGlyph) * FONT_GLYPH_COUNT;

					break;
				}

				INVALID_CASE();
			}
		}
	}
	else {
		ASSERT(asset_file.type == AssetFileType_one);

		i32 channels, sample_rate;
		i16 * samples;
		i32 sample_count = stb_vorbis_decode_filename(asset_file.file_name, &channels, &sample_rate, &samples);
		ASSERT(channels == AUDIO_CHANNELS);
		ASSERT(sample_rate == AUDIO_SAMPLE_RATE)

		Asset * asset = push_asset(assets, asset_file.asset_id, AssetType_audio_clip);
		//TODO: Need to add the padding sample to the front of the source audio clip!!
		asset->audio_clip.samples = (u32)(sample_count - AUDIO_PADDING_SAMPLES);
		asset->audio_clip.sample_data = samples;

		assets->debug_total_size += (u32)sample_count * channels * sizeof(i16);
	}
}

void load_assets(AssetState * assets, MemoryArena * arena) {
	DEBUG_TIME_BLOCK();
	
	assets->arena = arena;

	process_asset_file(assets, asset_file_pak((char *)"pak/preload.zip", (char *)"preload.pak"));

#if DEV_ENABLED
	for(u32 i = 0; i < ARRAY_COUNT(global_dev_asset_files); i++) {
		AssetFile asset_file = global_dev_asset_files[i];
		ASSERT(asset_file.type == AssetFileType_one);

		i32 width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		u8 * img_data = stbi_load(asset_file.file_name, &width, &height, &channels, 0);
		if(img_data) {
			ASSERT(channels == 3 || channels == 4);
			ASSERT(height = TILE_MAP_HEIGHT);

			Asset * asset = push_asset(assets, asset_file.asset_id, AssetType_tile_map);
			asset->tile_map.width = (u32)width;
			asset->tile_map.tiles = ALLOC_ARRAY(Tiles, (u32)width);

			for(u32 x = 0; x < (u32)width; x++) {
				Tiles * tiles = asset->tile_map.tiles + x;

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

			stbi_image_free(img_data);
		}
	}
#endif 
}

b32 process_next_asset_file(AssetState * assets) {
	DEBUG_TIME_BLOCK();

	b32 loaded = false;

	AssetFile asset_files[] = {
		asset_file_one((char *)"audio/pickup0.ogg", AssetId_pickup),
		asset_file_one((char *)"audio/pickup1.ogg", AssetId_pickup),
		asset_file_one((char *)"audio/pickup2.ogg", AssetId_pickup),
		asset_file_one((char *)"audio/bang0.ogg", AssetId_bang),
		asset_file_one((char *)"audio/bang1.ogg", AssetId_bang),
		asset_file_one((char *)"audio/baa0.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa1.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa2.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa3.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa4.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa5.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa6.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa7.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa8.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa9.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa10.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa11.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa12.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa13.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa14.ogg", AssetId_baa),
		asset_file_one((char *)"audio/baa15.ogg", AssetId_baa),
		asset_file_one((char *)"audio/special.ogg", AssetId_special),
		asset_file_one((char *)"audio/shield_loop.ogg", AssetId_shield_loop),
		asset_file_one((char *)"audio/pickup0.ogg", AssetId_unlock),
		asset_file_one((char *)"audio/pickup1.ogg", AssetId_unlock),

		asset_file_one((char *)"audio/move_up.ogg", AssetId_move_up),
		asset_file_one((char *)"audio/move_down.ogg", AssetId_move_down),
		asset_file_one((char *)"audio/falling.ogg", AssetId_falling),

		asset_file_one((char *)"audio/click_yes.ogg", AssetId_click_yes),
		asset_file_one((char *)"audio/click_no.ogg", AssetId_click_no),

		asset_file_one((char *)"audio/rocket_sfx.ogg", AssetId_rocket_sfx),

		asset_file_one((char *)"audio/pixelate.ogg", AssetId_pixelate),

		asset_file_one((char *)"audio/menu_music.ogg", AssetId_menu_music),
		asset_file_one((char *)"audio/intro_music.ogg", AssetId_intro_music),
		asset_file_one((char *)"audio/game_music.ogg", AssetId_game_music),
		asset_file_one((char *)"audio/space_music.ogg", AssetId_space_music),

		asset_file_pak((char *)"pak/map.zip", (char *)"map.pak"),
		asset_file_pak((char *)"pak/texture.zip", (char *)"texture.pak"),
		asset_file_pak((char *)"pak/atlas.zip", (char *)"atlas.pak"),
	};

	ASSERT(assets->last_loaded_file_index < ARRAY_COUNT(asset_files));
	assets->loaded_file_count = ARRAY_COUNT(asset_files);

	f64 begin_load_timestamp = emscripten_get_now();

	AssetFile asset_file = asset_files[assets->last_loaded_file_index++];
	process_asset_file(assets, asset_file);

	f32 asset_load_time = (f32)(emscripten_get_now() - begin_load_timestamp);
	assets->debug_load_time += asset_load_time;
	// std::printf("LOG: %s -> %f\n", asset_file.file_name, asset_load_time);

	if(assets->last_loaded_file_index >= ARRAY_COUNT(asset_files)) {
		loaded = true;
	}

	return loaded;
}

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

void load_assets(AssetState * assets, MemoryArena * arena) {
	DEBUG_TIME_BLOCK();

	f64 begin_timestamp = emscripten_get_now();

	assets->arena = arena;

	{
		f64 begin_load_timestamp = emscripten_get_now();

		AssetFile audio_asset_files[] = {
			{ (char *)"audio/pickup0.ogg", AssetId_pickup },
			{ (char *)"audio/pickup1.ogg", AssetId_pickup },
			{ (char *)"audio/pickup2.ogg", AssetId_pickup },
			{ (char *)"audio/bang0.ogg", AssetId_bang },
			{ (char *)"audio/bang1.ogg", AssetId_bang },
			{ (char *)"audio/baa0.ogg", AssetId_baa },
			{ (char *)"audio/baa1.ogg", AssetId_baa },
			{ (char *)"audio/baa2.ogg", AssetId_baa },
			{ (char *)"audio/baa3.ogg", AssetId_baa },
			{ (char *)"audio/baa4.ogg", AssetId_baa },
			{ (char *)"audio/baa5.ogg", AssetId_baa },
			{ (char *)"audio/baa6.ogg", AssetId_baa },
			{ (char *)"audio/baa7.ogg", AssetId_baa },
			{ (char *)"audio/baa8.ogg", AssetId_baa },
			{ (char *)"audio/baa9.ogg", AssetId_baa },
			{ (char *)"audio/baa10.ogg", AssetId_baa },
			{ (char *)"audio/baa11.ogg", AssetId_baa },
			{ (char *)"audio/baa12.ogg", AssetId_baa },
			{ (char *)"audio/baa13.ogg", AssetId_baa },
			{ (char *)"audio/baa14.ogg", AssetId_baa },
			{ (char *)"audio/baa15.ogg", AssetId_baa },
			{ (char *)"audio/special.ogg", AssetId_special },

			{ (char *)"audio/click_yes.ogg", AssetId_click_yes },
			{ (char *)"audio/click_no.ogg", AssetId_click_no },

			{ (char *)"audio/rocket_sfx.ogg", AssetId_rocket_sfx },

			{ (char *)"audio/menu_music.ogg", AssetId_menu_music },
			{ (char *)"audio/game_music.ogg", AssetId_game_music },
			{ (char *)"audio/space_music.ogg", AssetId_space_music },
		};

		for(u32 i = 0; i < ARRAY_COUNT(audio_asset_files); i++) {
			AssetFile * asset_file = audio_asset_files + i;

			i32 channels, sample_rate;
			i16 * samples;
			i32 sample_count = stb_vorbis_decode_filename(asset_file->name, &channels, &sample_rate, &samples);
			ASSERT(channels == AUDIO_CHANNELS);

			Asset * asset = push_asset(assets, asset_file->id, AssetType_audio_clip);
			//TODO: Need to add the padding sample to the front of the source audio clip!!
			asset->audio_clip.samples = (u32)(sample_count - AUDIO_PADDING_SAMPLES);
			asset->audio_clip.sample_data = samples;
		}

		std::printf("LOG: %s -> %f\n", "pak/audio.zip", (f32)(emscripten_get_now() - begin_load_timestamp));
	}

	AssetPackZipFile pack_files[] = {
		{ (char *)"pak/map.zip", (char *)"map.pak" },
		{ (char *)"pak/texture.zip", (char *)"texture.pak" },
		{ (char *)"pak/atlas.zip", (char *)"atlas.pak" },
	};

	for(u32 i = 0; i < ARRAY_COUNT(pack_files); i++) {
		AssetPackZipFile * pack_file = pack_files + i;

		f64 begin_load_timestamp = emscripten_get_now();

		MemoryPtr file_buf;
		file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap(pack_file->file_name, pack_file->archive_name, &file_buf.size, 0);
		u8 * file_ptr = file_buf.ptr;

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

		std::printf("LOG: %s -> %f\n", pack_file->file_name, (f32)(emscripten_get_now() - begin_load_timestamp));
	}

#if DEV_ENABLED
	for(u32 i = 0; i < ARRAY_COUNT(debug_global_asset_file_names); i++) {
		char const * file_name = debug_global_asset_file_names[i];

		i32 width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
		if(img_data) {
			ASSERT(channels == 3 || channels == 4);
			ASSERT(height = TILE_MAP_HEIGHT);

			Asset * asset = push_asset(assets, AssetId_debug_tile_map, AssetType_tile_map);
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
 
	assets->debug_load_time = (f32)(emscripten_get_now() - begin_timestamp);
	std::printf("LOG: asset_load_time: %f\n", assets->debug_load_time);
}
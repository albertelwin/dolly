
#ifndef ASSET_FORMAT_HPP_INCLUDED
#define ASSET_FORMAT_HPP_INCLUDED

#include <math.hpp>

#define TEXTURE_CHANNELS 4
#define TEXTURE_PADDING_PIXELS 1

enum TextureSampling {
	TextureSampling_point,
	TextureSampling_bilinear,

	TextureSampling_count,
};

#define AUDIO_CLIP_SAMPLES_PER_SECOND 44100
#define AUDIO_PADDING_SAMPLES 1
#define AUDIO_CHANNELS 2

#define TILE_MAP_HEIGHT 37

enum TileId {
	TileId_bad,
	TileId_time,
	TileId_clone,
	TileId_collect,
	TileId_concord,
	TileId_rocket,

	TileId_count,
	TileId_null = TileId_count,
};

struct ColorRGB8 {
	u8 r;
	u8 g;
	u8 b;
};

struct TileIdColorRGB8 {
	TileId id;
	ColorRGB8 color;
};

inline ColorRGB8 color_rgb8(u8 r, u8 g, u8 b) {
	ColorRGB8 color = {};
	color.r = r;
	color.g = g;
	color.b = b;
	return color;
}

inline b32 colors_are_equal(ColorRGB8 color0, ColorRGB8 color1) {
	return color0.r == color1.r && color0.g == color1.g && color0.b == color1.b;
}

static TileIdColorRGB8 tile_id_color_table[] = {
	{ TileId_bad, color_rgb8(255, 0, 0) },
	{ TileId_time, color_rgb8(0, 255, 0) },
	{ TileId_clone, color_rgb8(255, 255, 255) },
	{ TileId_collect, color_rgb8(0, 0, 255) },
	{ TileId_concord, color_rgb8(255, 255, 0) },
	{ TileId_rocket, color_rgb8(0, 255, 255) },
};

#pragma pack(push, 1)
struct Tiles {
	TileId ids[TILE_MAP_HEIGHT];
};

struct TileMap {
	u32 width;
	Tiles * tiles;
};
#pragma pack(pop)

#define ASSET_GROUP_COUNT(name) (AssetId_one_past_last_##name - AssetId_first_##name)

enum AssetId {
	AssetId_null,

	//NOTE: Textures
	AssetId_white,

	AssetId_font,
	AssetId_debug_font,

	AssetId_menu_background,

	AssetId_background,

	//TODO: First lower id??
	AssetId_city,
	AssetId_highlands,
	AssetId_ocean,
	AssetId_space,

	AssetId_clouds,

	AssetId_atlas,

	//NOTE: Sprites
	AssetId_dolly_idle,
	AssetId_dolly_up,
	AssetId_dolly_down,
	AssetId_dolly_fall,

	AssetId_dolly_space_idle,
	AssetId_dolly_space_up,
	AssetId_dolly_space_down,

	AssetId_clone,
	AssetId_clone_space,
	AssetId_clock,
	AssetId_rocket,
	AssetId_rocket_large,
	AssetId_shield,

	AssetId_glitched_telly,

	//TODO: Should this be one_before_first??
	AssetId_first_collect,
	AssetId_collect_chair = AssetId_first_collect,
	AssetId_collect_cup,
	AssetId_collect_mobile,
	AssetId_collect_necklace,
	AssetId_collect_shoes,
	AssetId_one_past_last_collect,

	//TODO: Figure out a way to combine these with the collect ids!!
	AssetId_first_display,
	AssetId_display_chair = AssetId_first_display,
	AssetId_display_cup,
	AssetId_display_mobile,
	AssetId_display_necklace,
	AssetId_display_shoes,
	AssetId_one_past_last_display,

	AssetId_btn_play,
	AssetId_btn_about,
	AssetId_btn_back,
	AssetId_btn_baa,
	AssetId_btn_replay,
	AssetId_btn_up,
	AssetId_btn_down,

	AssetId_icon_clone,
	AssetId_icon_clock,

	AssetId_intro,

	//NOTE: Audio
	// AssetId_sin_440,
	AssetId_pickup,
	AssetId_bang,
	AssetId_baa,
	AssetId_special,

	AssetId_click_yes,
	AssetId_click_no,

	AssetId_tick_tock,
	AssetId_rocket_sfx,
	
	AssetId_game_music,
	AssetId_space_music,

	//NOTE: Tile maps
	AssetId_tile_map,
	AssetId_debug_tile_map,

	AssetId_lower_map,
	AssetId_upper_map,
	AssetId_tutorial_map,

	AssetId_count,
};

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X	\
	X(texture, Texture)				\
	X(sprite, Texture)				\
	X(audio_clip, AudioClip)		\
	X(tile_map, TileMap)

#define X(NAME, STRUCT) AssetType_##NAME,
	ASSET_TYPE_NAME_STRUCT_X
#undef X

	AssetType_count,
};

#pragma pack(push, 1)
struct AssetPackHeader {
	u32 asset_count;
};

struct TextureInfo {
	u32 width;
	u32 height;
	math::Vec2 offset;

	TextureSampling sampling;
};

struct SpriteInfo {
	u32 width;
	u32 height;
	math::Vec2 offset;

	u32 atlas_index;
	math::Vec2 tex_coords[2];
};

struct AudioClipInfo {
	u32 samples;
	u32 size;
};

struct TileMapInfo {
	u32 width;
};

struct AssetInfo {
	AssetId id;
	AssetType type;

	union {
		TextureInfo texture;
		SpriteInfo sprite;
		AudioClipInfo audio_clip;
		TileMapInfo tile_map;
	};
};
#pragma pack(pop)

#endif

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

#define PLACEMENT_WIDTH 32
#define PLACEMENT_HEIGHT 18

#pragma pack(push, 1)
struct Placement {
	u32 ids[PLACEMENT_HEIGHT];
};

struct PlacementMap {
	u32 count;
	Placement * placements;
};
#pragma pack(pop)

#define ASSET_GROUP_COUNT(name) (AssetId_one_past_last_##name - AssetId_first_##name)

enum AssetId {
	AssetId_null,

	//NOTE: Textures
	AssetId_white,

	AssetId_font,

	AssetId_menu_background,

	AssetId_city,
	AssetId_space,
	AssetId_mountains,

	AssetId_atlas,

	//NOTE: Sprites
	AssetId_dolly,
	AssetId_rocket,
	AssetId_large_rocket,
	AssetId_boots,
	AssetId_car,
	AssetId_shield,

	AssetId_first_collectable,
	AssetId_collectable_blob = AssetId_first_collectable,
	AssetId_collectable_clock,
	AssetId_collectable_diamond,
	AssetId_collectable_flower,
	AssetId_collectable_heart,
	AssetId_collectable_paw,
	AssetId_collectable_smiley,
	AssetId_collectable_speech,
	AssetId_collectable_speed_up,
	AssetId_collectable_telly,
	AssetId_one_past_last_collectable,

	//TODO: Figure out a way to combine these with the collectable ids!!
	AssetId_first_display,
	AssetId_display_blob = AssetId_first_display,
	AssetId_display_clock,
	AssetId_display_diamond,
	AssetId_display_flower,
	AssetId_display_heart,
	AssetId_display_paw,
	AssetId_display_smiley,
	AssetId_display_speech,
	AssetId_display_speed_up,
	AssetId_display_telly,
	AssetId_one_past_last_display,

	AssetId_score_background,

	AssetId_menu_btn_credits,
	AssetId_menu_btn_play,
	AssetId_menu_btn_score,
	AssetId_menu_btn_back,

	AssetId_score_btn_menu,
	AssetId_score_btn_replay,

	AssetId_intro,

	AssetId_maze_top,
	AssetId_maze_bottom,

	//NOTE: Audio
	// AssetId_sin_440,
	AssetId_pickup,
	AssetId_explosion,
	AssetId_baa,
	AssetId_tick_tock,
	AssetId_game_music,
	AssetId_death_music,

	//NOTE: Placements
	AssetId_placement,
	AssetId_debug_placement,

	AssetId_count,
};

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X	\
	X(texture, Texture)				\
	X(sprite, Texture)				\
	X(audio_clip, AudioClip)		\
	X(placement_map, PlacementMap)

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

struct PlacementMapInfo {
	u32 count;
};

struct AssetInfo {
	AssetId id;
	AssetType type;

	union {
		TextureInfo texture;
		SpriteInfo sprite;
		AudioClipInfo audio_clip;
		PlacementMapInfo placement_map;
	};
};
#pragma pack(pop)

struct ColorRGB8 {
	u8 r;
	u8 g;
	u8 b;
};

struct AssetIdColorRGB8 {
	AssetId id;
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

static AssetIdColorRGB8 asset_id_color_table[] = {
	{ AssetId_collectable_telly, color_rgb8(255, 0, 0) },
	{ AssetId_collectable_clock, color_rgb8(0, 255, 0) },
	{ AssetId_collectable_speed_up, color_rgb8(0, 0, 255) },
};

#endif
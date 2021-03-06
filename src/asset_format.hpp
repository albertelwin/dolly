
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

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_PADDING_SAMPLES 1
#define AUDIO_CHANNELS 2

#define TILE_MAP_HEIGHT 37

enum TileId {
	TileId_bad_1fer,
	TileId_bad_2fer,
	TileId_bad_3fer,
	TileId_bad_4fer,

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
	{ TileId_bad_1fer, color_rgb8(255, 255, 0) },
	{ TileId_bad_2fer, color_rgb8(255, 128, 0) },
	{ TileId_bad_3fer, color_rgb8(255, 0, 0) },
	{ TileId_bad_4fer, color_rgb8(255, 0, 255) },

	{ TileId_clone, color_rgb8(255, 255, 255) },
	{ TileId_collect, color_rgb8(0, 0, 255) },

	{ TileId_concord, color_rgb8(0, 255, 0) },
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

#define FONT_FIRST_CHAR '!'
#define FONT_ONE_PAST_LAST_CHAR ('~' + 1)
#define FONT_GLYPH_COUNT (FONT_ONE_PAST_LAST_CHAR - FONT_FIRST_CHAR)

inline u32 get_font_glyph_index(char char_) {
	u32 index = (u32)char_ - FONT_FIRST_CHAR;
	ASSERT(index < FONT_GLYPH_COUNT);
	return index;
}

#pragma pack(push, 1)
struct FontGlyph {
	f32 advance;
};

struct Font {
	FontGlyph * glyphs;
	u32 glyph_id;

	f32 ascent;
	f32 descent;
	f32 whitespace_advance;

	//TODO: Do we really need this??
	u32 atlas_index;
};
#pragma pack(pop)

inline f32 get_font_line_height(Font * font) {
	return font->ascent - font->descent;
}

#define BEGIN_ASSET_GROUP(name) AssetId_one_before_first_##name
#define END_ASSET_GROUP(name) AssetId_one_past_last_##name

#define ADD_FONT_ASSET_ID(name) AssetId_##name, AssetId_##name##_glyph

enum AssetId {
	AssetId_null,

	//NOTE: Textures
	AssetId_white,

	AssetId_load_background,
	AssetId_menu_background,
	AssetId_score_background,

	AssetId_background,

	BEGIN_ASSET_GROUP(lower_scene),
	AssetId_scene_dundee,
	AssetId_scene_edinburgh,
	AssetId_scene_highlands,
	AssetId_scene_forth,
	END_ASSET_GROUP(lower_scene),

	AssetId_scene_space,

	AssetId_clouds,

	AssetId_atlas,

	//NOTE: Sprites
	AssetId_dolly_idle,
	AssetId_dolly_up,
	AssetId_dolly_down,
	AssetId_dolly_hit,
	AssetId_dolly_fall,

	AssetId_dolly_space_idle,
	AssetId_dolly_space_up,
	AssetId_dolly_space_down,

	AssetId_clone,
	AssetId_clone_blink,
	AssetId_clone_space,
	AssetId_rocket,
	AssetId_rocket_large,
	AssetId_goggles,
	AssetId_concord,
	AssetId_circle,
	AssetId_shield,
	AssetId_glow,

	AssetId_label_clone,
	AssetId_score_clone,

	BEGIN_ASSET_GROUP(atom_smasher),
	AssetId_atom_smasher_1fer,
	AssetId_atom_smasher_2fer,
	AssetId_atom_smasher_3fer,
	AssetId_atom_smasher_4fer,
	END_ASSET_GROUP(atom_smasher),

	BEGIN_ASSET_GROUP(intro),
	AssetId_intro0,
	AssetId_intro1,
	AssetId_intro2,
	AssetId_intro3,
	AssetId_intro4,
	AssetId_intro5,
	END_ASSET_GROUP(intro),

	AssetId_intro4_background,

	AssetId_sun,

#define ASSET_ID_COLLECT_X \
	X(bike) \
	X(chair) \
	X(computer) \
	X(cup) \
	X(guitar) \
	X(lion) \
	X(mini) \
	X(mobile) \
	X(necklace) \
	X(shoes) \

	BEGIN_ASSET_GROUP(collect),
#define X(NAME) AssetId_collect_##NAME,
	ASSET_ID_COLLECT_X
#undef X
	END_ASSET_GROUP(collect),

	BEGIN_ASSET_GROUP(score),
#define X(NAME) AssetId_score_##NAME,
	ASSET_ID_COLLECT_X
#undef X
	END_ASSET_GROUP(score),

	AssetId_about_title,
	AssetId_about_body,

	AssetId_btn_play,
	AssetId_btn_about,
	AssetId_btn_back,
	AssetId_btn_baa,
	AssetId_btn_replay,
	AssetId_btn_up,
	AssetId_btn_down,
	AssetId_btn_skip,

	//NOTE: Audio
	AssetId_pickup,
	AssetId_bang,
	AssetId_baa,
	AssetId_special,
	AssetId_shield_loop,
	AssetId_tally,

	AssetId_move_up,
	AssetId_move_down,

	AssetId_click_yes,
	AssetId_click_no,

	AssetId_rocket_sfx,

	AssetId_pixelate,

	AssetId_menu_music,
	AssetId_intro_music,
	AssetId_game_music,
	AssetId_space_music,

	//NOTE: Tile maps
	AssetId_debug_lower_map,
	AssetId_debug_upper_map,

	AssetId_lower_map,
	AssetId_upper_map,
	AssetId_tutorial_map,

	//NOTE: Fonts
	ADD_FONT_ASSET_ID(pragmata_pro),
	ADD_FONT_ASSET_ID(munro),
	ADD_FONT_ASSET_ID(munro_large),

	AssetId_count,
};

#define ASSET_FIRST_GROUP_ID(name) ((AssetId)(AssetId_one_before_first_##name + 1))
#define ASSET_LAST_GROUP_ID(name) ((AssetId)(AssetId_one_past_last_##name - 1))
#define ASSET_GROUP_COUNT(name) (AssetId_one_past_last_##name - (AssetId_one_before_first_##name + 1))

#define ASSET_IN_GROUP(name, id) ((id) >= ASSET_FIRST_GROUP_ID(name) && (id) <= ASSET_LAST_GROUP_ID(name))

#define ASSET_ID_TO_GROUP_INDEX(name, id) (id - (AssetId_one_before_first_##name + 1))
#define ASSET_GROUP_INDEX_TO_ID(name, index) ((AssetId)(AssetId_one_before_first_##name + 1 + index))

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X \
	X(texture, Texture) \
	X(sprite, Texture) \
	X(audio_clip, AudioClip) \
	X(tile_map, TileMap) \
	X(font, Font) \

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

struct FontInfo {
	u32 glyph_id;

	f32 ascent;
	f32 descent;
	f32 whitespace_advance;

	u32 atlas_index;
};

struct AssetInfo {
	AssetId id;
	AssetType type;

	union {
		TextureInfo texture;
		SpriteInfo sprite;
		AudioClipInfo audio_clip;
		TileMapInfo tile_map;
		FontInfo font;
	};
};
#pragma pack(pop)

#endif
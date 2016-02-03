
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

#define ASSET_GROUP_COUNT(name) (AssetId_one_past_last_##name - AssetId_first_##name)

enum AssetId {
	AssetId_null,

	//NOTE: Textures
	AssetId_white,

	AssetId_font,

	AssetId_city,
	AssetId_mountains,
	AssetId_space,

	AssetId_intro,

	AssetId_atlas,

	//NOTE: Sprites
	AssetId_dolly,
	AssetId_clock,
	AssetId_telly,
	AssetId_rocket,
	AssetId_large_rocket,
	AssetId_car,
	AssetId_shield,
	AssetId_speed_up,

	AssetId_first_collectable,
	AssetId_collectable_blob = AssetId_first_collectable,
	AssetId_collectable_diamond,
	AssetId_collectable_flower,
	AssetId_collectable_heart,
	AssetId_collectable_paw,
	AssetId_collectable_speech,
	AssetId_collectable_spot,
	AssetId_one_past_last_collectable,

	AssetId_menu_credits,
	AssetId_menu_play,
	AssetId_menu_score,

	//NOTE: Audio
	// AssetId_sin_440,
	AssetId_pickup,
	AssetId_explosion,
	AssetId_baa,
	AssetId_tick_tock,
	AssetId_music,
	AssetId_death_music,

	AssetId_count,
};

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X	\
	X(texture, Texture)				\
	X(sprite, Texture)				\
	X(audio_clip, AudioClip)

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

struct AssetInfo {
	AssetId id;
	AssetType type;

	union {
		TextureInfo texture;
		SpriteInfo sprite;
		AudioClipInfo audio_clip;
	};
};

#pragma pack(pop)

#endif

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

#define AUDIO_CLIP_SAMPLES_PER_SECOND 48000
#define AUDIO_PADDING_SAMPLES 1
#define AUDIO_CHANNELS 2

enum AssetId {
	AssetId_null,

	//NOTE: Textures!!
	AssetId_white,

	AssetId_font,

	AssetId_city,
	AssetId_space,
	AssetId_clouds,

	AssetId_atlas,

	//NOTE: Sprites!!
	AssetId_dolly,
	AssetId_teacup,
	AssetId_telly,
	AssetId_smiley,
	AssetId_rocket,
	AssetId_large_rocket,

	//NOTE: Audio!!
	AssetId_sin_440,
	AssetId_beep,
	AssetId_woosh,
	AssetId_pickup,
	AssetId_explosion,
	AssetId_music,

	AssetId_count,
};

enum AssetType {
#define ASSET_TYPE_NAME_STRUCT_X	\
	X(texture, Texture)				\
	X(sprite, Sprite)				\
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

	TextureSampling sampling;
	u32 sub_tex_count;
};

struct SpriteInfo {
	//TODO: Share this info!!
	u32 atlas_index;
	math::Vec2 dim;
	math::Vec2 tex_coords[2];
	math::Vec2 offset;
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

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
	AssetId_font,
	AssetId_bg_layer,

	AssetId_atlas,

	//NOTE: Sprites!!
	AssetId_dolly,
	AssetId_teacup,

	//NOTE: Audio!!
	AssetId_sin_440,
	AssetId_beep,
	AssetId_woosh,
	AssetId_pickup,
	AssetId_explosion,
	AssetId_music,

	AssetId_count,
};

#pragma pack(push, 1)
struct AssetPackHeader {
	u32 asset_count;

	//TODO: Remove these!!
	u32 texture_count;
	u32 sprite_count;
	u32 clip_count;
};

struct TextureInfo {
	AssetId id;

	u32 width;
	u32 height;
	TextureSampling sampling;

	u32 sub_tex_count;
};

struct SpriteInfo {
	AssetId id;

	math::Vec2 dim;
	math::Vec2 tex_coords[2];
};

struct AudioClipInfo {
	AssetId id;

	u32 samples;
	u32 size;
};
#pragma pack(pop)

#endif
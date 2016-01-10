
#ifndef ASSET_FORMAT_HPP_INCLUDED
#define ASSET_FORMAT_HPP_INCLUDED

#include <math.hpp>

#define TEXTURE_CHANNELS 4

enum TextureId {
	TextureId_null,

	TextureId_font,

	//TODO: Texture groups!!
	TextureId_bg_layer0,
	TextureId_bg_layer1,
	TextureId_bg_layer2,
	TextureId_bg_layer3,

	TextureId_atlas,

	TextureId_count,
};

enum TextureSampling {
	TextureSampling_point,
	TextureSampling_bilinear,

	TextureSampling_count,
};

enum SpriteId {
	//TODO: Temp!!
	SpriteId_null,

	SpriteId_dolly,
	SpriteId_teacup,

	SpriteId_count,
};

enum AudioClipId {
	AudioClipId_sin_440,
	AudioClipId_beep,
	AudioClipId_woosh,
	AudioClipId_pickup,
	AudioClipId_explosion,
	AudioClipId_music,

	AudioClipId_count,
};

#pragma pack(push, 1)
struct AssetPackHeader {
	u32 tex_count;
	u32 clip_count;
};

struct TextureAsset {
	TextureId id;
	TextureSampling sampling;

	u32 width;
	u32 height;
	u32 sub_tex_count;
};

struct SpriteAsset {
	SpriteId id;

	math::Vec2 dim;
	math::Vec2 tex_coords[2];
};

struct AudioClipAsset {

};
#pragma pack(pop)

#endif
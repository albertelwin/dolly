
#ifndef ASSET_FORMAT_HPP_INCLUDED
#define ASSET_FORMAT_HPP_INCLUDED

#include <math.hpp>

#define TEXTURE_CHANNELS 4

#pragma pack(push, 1)
struct AssetPackHeader {
	u32 tex_count;
};

struct TextureAsset {
	u32 width;
	u32 height;
	u32 sub_tex_count;
};

struct SubTextureAsset {
	math::Vec2 size;
	//TODO: Store just one texture coordinate??
	math::Vec2 tex_coords[2];
};
#pragma pack(pop)

#endif
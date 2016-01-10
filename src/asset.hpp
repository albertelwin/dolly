
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

struct TextureAtlas {
	gl::Texture tex;
	u32 sub_tex_count;
};

struct Sprite {
	math::Vec2 size;
	//TODO: Just store one tex coord!!
	math::Vec2 tex_coords[2];
};

struct AssetState {
	MemoryPool * memory_pool;

	TextureAtlas tex_atlas;
	Sprite sprites[SpriteId_count];
};

#endif
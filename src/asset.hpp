
#ifndef ASSET_HPP_INCLUDED
#define ASSET_HPP_INCLUDED

#include <asset_format.hpp>

struct Sprite {
	math::Vec2 dim;
	math::Vec2 tex_coords[2];
};

struct AssetState {
	MemoryPool * memory_pool;

	gl::Texture textures[TextureId_count];
	Sprite sprites[SpriteId_count];
};

#endif
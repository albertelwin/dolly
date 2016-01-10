
#include <asset.hpp>

void load_assets(AssetState * asset_state, MemoryPool * pool) {
	asset_state->memory_pool = pool;

	FileBuffer file_buf = read_file_to_memory("asset.pak");
	u8 * file_ptr = file_buf.ptr;

	AssetPackHeader * pack = (AssetPackHeader *)file_ptr;
	file_ptr += sizeof(AssetPackHeader);

	for(u32 i = 0; i < pack->tex_count; i++) {
		TextureAsset * tex_asset = (TextureAsset *)file_ptr;

		SpriteAsset * sprite_assets = (SpriteAsset *)(tex_asset + 1);
		for(u32 i = 0; i < tex_asset->sub_tex_count; i++) {
			SpriteAsset * asset = sprite_assets + i;

			Sprite * sprite = asset_state->sprites + asset->id;
			sprite->dim = asset->dim;
			sprite->tex_coords[0] = asset->tex_coords[0];
			sprite->tex_coords[1] = asset->tex_coords[1];
		}		

		u32 tex_size = tex_asset->width * tex_asset->height * TEXTURE_CHANNELS;
		u8 * tex_ptr = (u8 *)(sprite_assets + tex_asset->sub_tex_count);

		i32 filter = GL_LINEAR;
		if(tex_asset->sampling == TextureSampling_point) {
			filter = GL_NEAREST;
		}
		else {
			ASSERT(tex_asset->sampling == TextureSampling_bilinear);
		}

		asset_state->textures[tex_asset->id] = gl::create_texture(tex_ptr, tex_asset->width, tex_asset->height, GL_RGBA, filter, GL_CLAMP_TO_EDGE);

		file_ptr = tex_ptr + tex_size;
	}

	free(file_buf.ptr);
}
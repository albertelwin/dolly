
#include <asset.hpp>

void load_assets(AssetState * asset_state, MemoryPool * pool) {
	asset_state->memory_pool = pool;

	std::FILE * file_ptr = std::fopen("asset.pak", "rb");
	ASSERT(file_ptr != 0);

	std::fseek(file_ptr, 0, SEEK_END);
	size_t file_size = (size_t)std::ftell(file_ptr);
	std::rewind(file_ptr);

	u8 * file_buf = (u8 *)malloc(file_size);
	size_t read_result = std::fread(file_buf, 1, file_size, file_ptr);
	ASSERT(read_result == file_size);

	AssetPackHeader * pack = (AssetPackHeader *)file_buf;

	TextureAsset * tex_asset = (TextureAsset *)(pack + 1);
	SubTextureAsset * sub_tex_assets = (SubTextureAsset *)(tex_asset + 1);
	u8 * tex_mem = (u8 *)(sub_tex_assets + tex_asset->sub_tex_count);

	TextureAtlas * atlas = &asset_state->tex_atlas;
	atlas->tex = gl::create_texture(tex_mem, tex_asset->width, tex_asset->height, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);
	atlas->sub_tex_count = tex_asset->sub_tex_count;
	ASSERT(atlas->tex.id);

	asset_state->sprites = PUSH_ARRAY(asset_state->memory_pool, Sprite, tex_asset->sub_tex_count);
	for(u32 i = 0; i < tex_asset->sub_tex_count; i++) {
		SubTextureAsset * sub_tex = sub_tex_assets + i;

		Sprite * sprite = asset_state->sprites + i;
		sprite->size = sub_tex->size;
		sprite->tex_coords[0] = sub_tex->tex_coords[0];
		sprite->tex_coords[1] = sub_tex->tex_coords[1];		
	}

	std::fclose(file_ptr);
	free(file_buf);
}
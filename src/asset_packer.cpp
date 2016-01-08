
#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sys.hpp>

#include <asset_format.hpp>

struct TextureAtlas {
	u32 width;
	u32 height;

	u32 size;
	u8 * mem;

	u32 x;
	u32 y;
	u32 safe_y;
};

struct Texture {
	u32 width;
	u32 height;
	u8 * mem;
};

Texture load_texture(char const * file_name) {
	i32 width, height, channels;
	u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
	ASSERT(channels == TEXTURE_CHANNELS);

	//NOTE: Premultiplied alpha!!
	f32 r_255 = 1.0f / 255.0f;
	for(u32 y = 0, i = 0; y < (u32)height; y++) {
		for(u32 x = 0; x < (u32)width; x++, i += 4) {
			f32 a = (f32)img_data[i + 3] * r_255;

			f32 r = (f32)img_data[i + 0] * r_255 * a;
			f32 g = (f32)img_data[i + 1] * r_255 * a;
			f32 b = (f32)img_data[i + 2] * r_255 * a;

			img_data[i + 0] = (u8)(r * 255.0f);
			img_data[i + 1] = (u8)(g * 255.0f);
			img_data[i + 2] = (u8)(b * 255.0f);
		}	
	}

	Texture tex;
	tex.width = (u32)width;
	tex.height = (u32)height;
	tex.mem = img_data;
	return tex;
}

void copy_sub_tex_into_tex(TextureAtlas * atlas, Texture * tex) {
	//TODO: Make sure there's enough width remaining on this row!!
	for(u32 y = 0; y < tex->height; y++) {
		for(u32 x = 0; x < tex->width; x++) {
			u32 i = (((atlas->height - 1) - (y + atlas->y)) * atlas->width + x + atlas->x) * TEXTURE_CHANNELS;
			u32 k = (y * tex->width + x) * TEXTURE_CHANNELS;

			atlas->mem[i + 0] = tex->mem[k + 0];
			atlas->mem[i + 1] = tex->mem[k + 1];
			atlas->mem[i + 2] = tex->mem[k + 2];
			atlas->mem[i + 3] = tex->mem[k + 3];
		}
	}

	u32 y = atlas->y + tex->height;
	if(y > atlas->safe_y) {
		atlas->safe_y = y;
	}

	atlas->x += tex->width;
	if(atlas->x >= atlas->width) {
		atlas->x = 0;
		atlas->y = atlas->safe_y;
	}
}

int main() {
	Texture tex_arr[] = {
		load_texture("../dat/dolly0.png"),
		load_texture("../dat/dolly1.png"),
		load_texture("../dat/dolly2.png"),
		load_texture("../dat/dolly3.png"),
		load_texture("../dat/dolly4.png"),
		load_texture("../dat/dolly5.png"),
		load_texture("../dat/dolly6.png"),
		load_texture("../dat/dolly7.png"),
	};

	AssetPackHeader asset_pack = {};
	asset_pack.tex_count = 1;

	TextureAsset tex_asset = {};
	tex_asset.width = 1024;
	tex_asset.height = 1024;
	tex_asset.sub_tex_count = ARRAY_COUNT(tex_arr);

	//TODO: Generate sub tex from actual blit!!
	SubTextureAsset sub_tex_assets[ARRAY_COUNT(tex_arr)];
	for(u32 i = 0; i < ARRAY_COUNT(tex_arr); i++) {
		SubTextureAsset * sub_tex = sub_tex_assets + i;
		Texture * tex = tex_arr + i;

		sub_tex->size = math::vec2(tex->width, tex->height);

		u32 tex_size = tex_asset.width;
		f32 r_tex_size = 1.0f / (f32)tex_size;
		u32 sub_tex_per_row = tex_size / tex->width;

		u32 u = (i % sub_tex_per_row) * tex->width;
		u32 v = (i / sub_tex_per_row) * tex->height;

		sub_tex->tex_coords[0] = math::vec2(u, tex_size - (v + tex->height)) * r_tex_size;
		sub_tex->tex_coords[1] = math::vec2(u + tex->width, tex_size - v) * r_tex_size;
	}

	TextureAtlas atlas = {};
	atlas.width = tex_asset.width;
	atlas.height = tex_asset.height;
	atlas.size = atlas.width * atlas.height * TEXTURE_CHANNELS;
	atlas.mem = (u8 *)malloc(atlas.size);
	for(u32 y = 0, i = 0; y < atlas.height; y++) {
		for(u32 x = 0; x < atlas.width; x++, i += 4) {
			// u8 v = 255;
			u8 v = 0;

			atlas.mem[i + 0] = v;
			atlas.mem[i + 1] = 0;
			atlas.mem[i + 2] = 0;
			atlas.mem[i + 3] = v;
		}
	}

	for(u32 i = 0; i < ARRAY_COUNT(tex_arr); i++) {
		Texture * tex = tex_arr + i;
		copy_sub_tex_into_tex(&atlas, tex);
	}

	std::FILE * file_ptr = std::fopen("../dat/asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&asset_pack, sizeof(AssetPackHeader), 1, file_ptr);
	std::fwrite(&tex_asset, sizeof(TextureAsset), 1, file_ptr);
	std::fwrite(&sub_tex_assets, sizeof(SubTextureAsset), ARRAY_COUNT(sub_tex_assets), file_ptr);
	std::fwrite(atlas.mem, atlas.size, 1, file_ptr);
	std::fclose(file_ptr);

	return 0;
}

#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sys.hpp>

#include <asset_format.hpp>

struct Texture {
	u32 size;
	u8 * ptr;

	u32 width;
	u32 height;

	u32 x;
	u32 y;
	u32 safe_y;
};

struct TexCoord {
	u32 u;
	u32 v;
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

	Texture tex = {};
	tex.width = (u32)width;
	tex.height = (u32)height;
	tex.size = tex.width * tex.height * TEXTURE_CHANNELS;
	tex.ptr = img_data;
	return tex;
}

Texture allocate_texture(u32 width, u32 height) {
	Texture tex = {};
	tex.size = width * height * TEXTURE_CHANNELS;
	tex.ptr = (u8 *)malloc(tex.size);
	tex.width = width;
	tex.height = height;

	for(u32 y = 0, i = 0; y < height; y++) {
		for(u32 x = 0; x < width; x++, i += 4) {
			tex.ptr[i + 0] = 0;
			tex.ptr[i + 1] = 0;
			tex.ptr[i + 2] = 0;
			tex.ptr[i + 3] = 255;
		}
	}

	return tex;
}

TexCoord blit_texture(Texture * dst, Texture * src) {
	if((dst->x + src->width) > dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	ASSERT((dst->y + src->height <= dst->height));

	//TODO: Remove empty space from src texture!!

	for(u32 y = 0; y < src->height; y++) {
		for(u32 x = 0; x < src->width; x++) {
			u32 i = (((dst->height - 1) - (y + dst->y)) * dst->width + x + dst->x) * TEXTURE_CHANNELS;
			u32 k = (y * src->width + x) * TEXTURE_CHANNELS;

			dst->ptr[i + 0] = src->ptr[k + 0];
			dst->ptr[i + 1] = src->ptr[k + 1];
			dst->ptr[i + 2] = src->ptr[k + 2];
			dst->ptr[i + 3] = src->ptr[k + 3];
		}
	}

	TexCoord tex_coord;
	tex_coord.u = dst->x;
	tex_coord.v = dst->y;

	//NOTE: 1px padding for bilinear filter!!
	u32 pad = 1;

	u32 y = dst->y + src->height + pad;
	if(y > dst->safe_y) {
		dst->safe_y = y;
	}

	dst->x += src->width + pad;
	ASSERT(dst->x <= dst->width);
	if(dst->x >= dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	return tex_coord;
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

		load_texture("../dat/dolly.png"),
		load_texture("../dat/teacup.png"),
	};

	AssetPackHeader asset_pack = {};
	asset_pack.tex_count = 1;

	TextureAsset tex_asset = {};
	tex_asset.width = 512;
	tex_asset.height = 512;
	tex_asset.sub_tex_count = ARRAY_COUNT(tex_arr);

	Texture atlas = allocate_texture(tex_asset.width, tex_asset.height);

	SubTextureAsset sub_tex_assets[ARRAY_COUNT(tex_arr)];
	for(u32 i = 0; i < ARRAY_COUNT(tex_arr); i++) {
		Texture * tex = tex_arr + i;
		TexCoord tex_coord = blit_texture(&atlas, tex);

		SubTextureAsset * sub_tex = sub_tex_assets + i;
		sub_tex->size = math::vec2(tex->width, tex->height);

		u32 tex_size = tex_asset.width;
		f32 r_tex_size = 1.0f / (f32)tex_size;
		u32 sub_tex_per_row = tex_size / tex->width;

		u32 u = tex_coord.u;
		u32 v = tex_coord.v;

		sub_tex->tex_coords[0] = math::vec2(u, tex_size - (v + tex->height)) * r_tex_size;
		sub_tex->tex_coords[1] = math::vec2(u + tex->width, tex_size - v) * r_tex_size;
	}

	std::FILE * file_ptr = std::fopen("../dat/asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&asset_pack, sizeof(AssetPackHeader), 1, file_ptr);
	std::fwrite(&tex_asset, sizeof(TextureAsset), 1, file_ptr);
	std::fwrite(&sub_tex_assets, sizeof(SubTextureAsset), ARRAY_COUNT(sub_tex_assets), file_ptr);
	std::fwrite(atlas.ptr, atlas.size, 1, file_ptr);
	std::fclose(file_ptr);

	return 0;
}

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

struct LoadReq {
	char * file_name;
	SpriteId id;
};

struct LoadReqArray {
	u32 count;
	LoadReq array[128];
};

void push_req(LoadReqArray * reqs, char * file_name, SpriteId id) {
	ASSERT(reqs->count < (ARRAY_COUNT(reqs->array) - 1));

	LoadReq * req = reqs->array + reqs->count++;
	req->file_name = file_name;
	req->id = id;
}

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
	AssetPackHeader asset_pack = {};
	asset_pack.tex_count = 1;

	LoadReqArray reqs = {};

	push_req(&reqs, "../dat/dolly0.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly1.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly2.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly3.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly4.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly5.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly6.png", SpriteId_null);
	push_req(&reqs, "../dat/dolly7.png", SpriteId_null);

	push_req(&reqs, "../dat/dolly.png", SpriteId_dolly);
	push_req(&reqs, "../dat/teacup.png", SpriteId_teacup);

	TextureAsset tex_asset = {};
	tex_asset.width = 512;
	tex_asset.height = 512;
	tex_asset.sub_tex_count = reqs.count;

	Texture atlas = allocate_texture(tex_asset.width, tex_asset.height);

	SpriteAsset * sprites = (SpriteAsset *)malloc(reqs.count);
	for(u32 i = 0; i < reqs.count; i++) {
		LoadReq * req = reqs.array + i;

		Texture tex_ = load_texture(req->file_name);
		Texture * tex = &tex_;

		TexCoord tex_coord = blit_texture(&atlas, tex);

		u32 tex_size = tex_asset.width;
		f32 r_tex_size = 1.0f / (f32)tex_size;

		u32 u = tex_coord.u;
		u32 v = tex_coord.v;

		SpriteAsset * sprite = sprites + i;
		sprite->id = req->id;
		sprite->dim = math::vec2(tex->width, tex->height);
		sprite->tex_coords[0] = math::vec2(u, tex_size - (v + tex->height)) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(u + tex->width, tex_size - v) * r_tex_size;
	}

	std::FILE * file_ptr = std::fopen("../dat/asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&asset_pack, sizeof(AssetPackHeader), 1, file_ptr);
	std::fwrite(&tex_asset, sizeof(TextureAsset), 1, file_ptr);
	std::fwrite(sprites, sizeof(SpriteAsset), reqs.count, file_ptr);
	std::fwrite(atlas.ptr, atlas.size, 1, file_ptr);
	std::fclose(file_ptr);

	return 0;
}
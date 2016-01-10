
#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sys.hpp>

#include <asset_format.hpp>

struct Texture {
	TextureId id;

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

Texture load_texture(char const * file_name, TextureId id, TextureSampling filter = TextureSampling_bilinear) {
	stbi_set_flip_vertically_on_load(true);

	i32 width, height, channels;
	u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
	ASSERT(channels == TEXTURE_CHANNELS);

	Texture tex = {};
	tex.id = id;
	tex.width = (u32)width;
	tex.height = (u32)height;	

	//NOTE: Premultiplied alpha!!
	f32 r_255 = 1.0f / 255.0f;
	for(u32 y = 0, i = 0; y < tex.height; y++) {
		for(u32 x = 0; x < tex.width; x++, i += 4) {
			f32 a = (f32)img_data[i + 3] * r_255;

			f32 r = (f32)img_data[i + 0] * r_255 * a;
			f32 g = (f32)img_data[i + 1] * r_255 * a;
			f32 b = (f32)img_data[i + 2] * r_255 * a;

			img_data[i + 0] = (u8)(r * 255.0f);
			img_data[i + 1] = (u8)(g * 255.0f);
			img_data[i + 2] = (u8)(b * 255.0f);
		}	
	}

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
			tex.ptr[i + 3] = 0;
		}
	}

	return tex;
}

struct Blit { u32 u; u32 v; u32 width; u32 height; };
Blit blit_texture(Texture * dst, Texture * src) {
	u32 min_x = U32_MAX;
	u32 min_y = U32_MAX;

	u32 max_x = 0;
	u32 max_y = 0;

	for(u32 y = 0, i = 0; y < src->height; y++) {
		for(u32 x = 0; x < src->width; x++, i += 4) {
			u8 a = src->ptr[i + 3];
			if(a) {
				if(x < min_x) {
					min_x = x;
				}

				if(y < min_y) {
					min_y = y;
				}

				if(x > max_x) {
					max_x = x;
				}

				if(y > max_y) {
					max_y = y;
				}
			}
		}
	}

	u32 width = max_x - min_x;
	u32 height = max_y - min_y;
	ASSERT(width > 0 && height > 0);

	if((dst->x + width) > dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	ASSERT((dst->y + height <= dst->height));

	for(u32 y = 0; y < height; y++) {
		for(u32 x = 0; x < width; x++) {
			u32 d_i = ((y + dst->y) * dst->width + x + dst->x) * TEXTURE_CHANNELS;
			u32 s_i = ((y + min_y) * src->width + (x + min_x)) * TEXTURE_CHANNELS;

			dst->ptr[d_i + 0] = src->ptr[s_i + 0];
			dst->ptr[d_i + 1] = src->ptr[s_i + 1];
			dst->ptr[d_i + 2] = src->ptr[s_i + 2];
			dst->ptr[d_i + 3] = src->ptr[s_i + 3];
		}
	}

	Blit result = {};
	result.u = dst->x;
	result.v = dst->y;
	result.width = width;
	result.height = height;

	//NOTE: 1px padding for bilinear filter!!
	u32 pad = 1;

	u32 y = dst->y + height + pad;
	if(y > dst->safe_y) {
		dst->safe_y = y;
	}

	dst->x += width + pad;
	ASSERT(dst->x <= dst->width);
	if(dst->x >= dst->width) {
		dst->x = 0;
		dst->y = dst->safe_y;
	}

	return result;
}

int main() {
	AssetPackHeader asset_pack = {};
	asset_pack.tex_count = 1;

	LoadReqArray reqs = {};

	push_req(&reqs, "dolly0.png", SpriteId_null);
	push_req(&reqs, "dolly1.png", SpriteId_null);
	push_req(&reqs, "dolly2.png", SpriteId_null);
	push_req(&reqs, "dolly3.png", SpriteId_null);
	push_req(&reqs, "dolly4.png", SpriteId_null);
	push_req(&reqs, "dolly5.png", SpriteId_null);
	push_req(&reqs, "dolly6.png", SpriteId_null);
	push_req(&reqs, "dolly7.png", SpriteId_null);

	push_req(&reqs, "dolly.png", SpriteId_dolly);
	push_req(&reqs, "teacup.png", SpriteId_teacup);

	TextureAsset tex_asset = {};
	tex_asset.id = TextureId_atlas;
	tex_asset.width = 512;
	tex_asset.height = 512;
	tex_asset.sub_tex_count = reqs.count;

	Texture atlas = allocate_texture(tex_asset.width, tex_asset.height);

	SpriteAsset * sprites = (SpriteAsset *)malloc(sizeof(SpriteAsset) * reqs.count);
	for(u32 i = 0; i < reqs.count; i++) {
		LoadReq * req = reqs.array + i;

		Texture tex_ = load_texture(req->file_name, TextureId_null);
		Texture * tex = &tex_;

		Blit blit = blit_texture(&atlas, tex);

		u32 tex_size = tex_asset.width;
		f32 r_tex_size = 1.0f / (f32)tex_size;

		SpriteAsset * sprite = sprites + i;
		sprite->id = req->id;
		sprite->dim = math::vec2(blit.width, blit.height);
		sprite->tex_coords[0] = math::vec2(blit.u, blit.v) * r_tex_size;
		sprite->tex_coords[1] = math::vec2(blit.u + blit.width, blit.v + blit.height) * r_tex_size;
	}

	Texture reg_tex_array[] = {
		load_texture("font.png", TextureId_font, TextureSampling_point),

		load_texture("bg_layer0.png", TextureId_bg_layer0),
		load_texture("bg_layer1.png", TextureId_bg_layer1),
		load_texture("bg_layer2.png", TextureId_bg_layer2),
		load_texture("bg_layer3.png", TextureId_bg_layer3),
	};

	asset_pack.tex_count += ARRAY_COUNT(reg_tex_array);

	std::FILE * file_ptr = std::fopen("asset.pak", "wb");
	ASSERT(file_ptr != 0);

	std::fwrite(&asset_pack, sizeof(AssetPackHeader), 1, file_ptr);
	std::fwrite(&tex_asset, sizeof(TextureAsset), 1, file_ptr);
	std::fwrite(sprites, sizeof(SpriteAsset), reqs.count, file_ptr);
	std::fwrite(atlas.ptr, atlas.size, 1, file_ptr);

	for(u32 i = 0; i < ARRAY_COUNT(reg_tex_array); i++) {
		Texture * tex = reg_tex_array + i;

		TextureAsset header = {};
		header.id = tex->id;
		header.width = tex->width;
		header.height = tex->height;
		header.sub_tex_count = 0;

		std::fwrite(&header, sizeof(TextureAsset), 1, file_ptr);
		std::fwrite(tex->ptr, tex->size, 1, file_ptr);
	}

	std::fclose(file_ptr);

	return 0;
}

#include <render.hpp>

#include <basic.vert>
#include <basic.frag>
#include <screen_quad.vert>
#include <post_filter.frag>

math::Vec2 project_pos(Camera * camera, math::Vec3 pos) {
	math::Vec2 projected_pos = pos.xy - camera->pos;
	projected_pos *= 1.0f / (pos.z + 1.0f);
	return projected_pos + camera->offset;
}

math::Vec3 unproject_pos(Camera * camera, math::Vec2 pos, f32 z) {
	math::Vec3 unprojected_pos = math::vec3(pos - camera->offset, z);
	unprojected_pos.xy *= (z + 1.0f);
	unprojected_pos.xy += camera->pos;
	return unprojected_pos;
}

RenderBatch * allocate_render_batch(MemoryArena * arena, Texture * tex, u32 v_len, RenderMode render_mode = RenderMode_triangles) {
	RenderBatch * batch = PUSH_STRUCT(arena, RenderBatch);

	batch->tex = tex;

	batch->v_len = v_len;
	batch->v_arr = PUSH_ARRAY(arena, f32, batch->v_len);
	batch->v_buf = gl::create_vertex_buffer(batch->v_arr, batch->v_len, VERT_ELEM_COUNT, GL_DYNAMIC_DRAW);
	batch->e = 0;

	batch->mode = render_mode;

	return batch;
}

Font * allocate_font(RenderState * render_state, Texture * tex, u32 v_len) {
	Font * font = PUSH_STRUCT(render_state->arena, Font);

	font->batch = allocate_render_batch(render_state->arena, tex, v_len);

	font->projection_matrix = math::orthographic_projection((f32)render_state->back_buffer_width, (f32)render_state->back_buffer_height);
	font->glyph_width = 3;
	font->glyph_height = 5;
	font->glyph_spacing = 1;

	font->scale = 2.0f;
	// font->scale = 4.0f;
	font->anchor.x = -(f32)render_state->back_buffer_width * 0.5f + font->glyph_spacing * font->scale;
	font->anchor.y = (f32)render_state->back_buffer_height * 0.5f - (font->glyph_height + font->glyph_spacing) * font->scale;
	font->pos = font->anchor;

	return font;
}

void push_quad(RenderBatch * batch, math::Vec2 pos0, math::Vec2 pos1, math::Vec2 uv0, math::Vec2 uv1, math::Vec4 color) {
	ASSERT(batch->e <= (batch->v_len - QUAD_ELEM_COUNT));
	f32 * v = batch->v_arr;

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
}

void push_quad_lines(RenderBatch * batch, math::Rec2 * rec, math::Vec4 color) {
	ASSERT(batch->e <= (batch->v_len - QUAD_ELEM_COUNT));
	f32 * v = batch->v_arr;

	math::Vec2 pos0 = rec->min;
	math::Vec2 pos1 = rec->max;

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
	v[batch->e++] =  pos1.x; v[batch->e++] =  pos0.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos0.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
	v[batch->e++] =  pos0.x; v[batch->e++] =  pos1.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos1.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =    0.0f; v[batch->e++] =    0.0f;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	
}

void push_sprite(RenderBatch * batch, Texture * sprite, math::Vec2 pos, math::Vec2 dim, math::Vec4 color) {
	math::Vec2 pos0 = pos - dim * 0.5f;
	math::Vec2 pos1 = pos + dim * 0.5f;

	math::Vec2 uv0 = sprite->tex_coords[0];
	math::Vec2 uv1 = sprite->tex_coords[1];

	push_quad(batch, pos0, pos1, uv0, uv1, color);
}

void push_str(Font * font, Str * str) {
	DEBUG_TIME_BLOCK();

	RenderBatch * batch = font->batch;

	f32 glyph_width = font->glyph_width * font->scale;
	f32 glyph_height = font->glyph_height * font->scale;
	f32 glyph_spacing = font->glyph_spacing * font->scale;

	math::Vec2 tex_dim = batch->tex->dim;
	math::Vec2 r_tex_dim = math::vec2(1.0f / tex_dim.x, 1.0f / tex_dim.y);

	//TODO: Put these in the font struct??
	u32 glyph_first_char = 24;
	u32 glyphs_per_row = 12;

	math::Vec4 color = math::vec4(1.0f);

	for(u32 i = 0; i < str->len; i++) {
		char char_ = str->ptr[i];
		if(char_ == '\n') {
			font->pos.x = font->anchor.x;
			font->pos.y -= (glyph_height + glyph_spacing);
		}
		else {
			math::Vec2 pos0 = font->pos;
			math::Vec2 pos1 = math::vec2(pos0.x + glyph_width, pos0.y + glyph_height);

			font->pos.x += (glyph_width + glyph_spacing);
			if(char_ != ' ') {
				u32 u = ((char_ - glyph_first_char) % glyphs_per_row) * (font->glyph_width + font->glyph_spacing * 2) + font->glyph_spacing;
				u32 v = ((char_ - glyph_first_char) / glyphs_per_row) * (font->glyph_height + font->glyph_spacing * 2) + font->glyph_spacing;

				math::Vec2 uv0 = math::vec2(u, (u32)tex_dim.y - (v + font->glyph_height)) * r_tex_dim;
				math::Vec2 uv1 = math::vec2(u + font->glyph_width, (u32)tex_dim.y - v) * r_tex_dim;

				push_quad(batch, pos0, pos1, uv0, uv1, color);
			}
		}
	}
}

void push_c_str(Font * font, char const * c_str) {
	Str str;
	str.max_len = str.len = c_str_len(c_str);
	str.ptr = (char *)c_str;
	push_str(font, &str);
}

void render_v_buf(gl::VertexBuffer * v_buf, RenderMode render_mode, Shader * shader, math::Mat4 * transform, Texture * tex0, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	glUseProgram(shader->id);

	glUniformMatrix4fv(shader->transform, 1, GL_FALSE, transform->v);
	glUniform4f(shader->color, color.r, color.g, color.b, color.a);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0->gl_id);
	glUniform1i(shader->tex0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, v_buf->id);

	u32 stride = v_buf->vert_size * sizeof(f32);

	glVertexAttribPointer(shader->i_position, 2, GL_FLOAT, 0, stride, 0);
	glEnableVertexAttribArray(shader->i_position);

	glVertexAttribPointer(shader->i_tex_coord, 2, GL_FLOAT, 0, stride, (void *)(2 * sizeof(f32)));
	glEnableVertexAttribArray(shader->i_tex_coord);

	glVertexAttribPointer(shader->i_color, 4, GL_FLOAT, 0, stride, (void *)(4 * sizeof(f32)));
	glEnableVertexAttribArray(shader->i_color);

	glDrawArrays(render_mode, 0, v_buf->vert_count);
}

void render_and_clear_render_batch(RenderBatch * batch, Shader * shader, math::Mat4 * transform) {
	DEBUG_TIME_BLOCK();

	if(batch->e > 0) {
		for(u32 i = batch->e; i < batch->v_len; i++) {
			batch->v_arr[i] = 0.0f;
		}

		glBindBuffer(GL_ARRAY_BUFFER, batch->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch->v_buf.size_in_bytes, batch->v_arr);

		render_v_buf(&batch->v_buf, batch->mode, shader, transform, batch->tex);

		batch->e = 0;
	}
}

void load_render(RenderState * render_state, MemoryArena * arena, AssetState * assets, u32 back_buffer_width, u32 back_buffer_height) {
	render_state->arena = arena;
	render_state->assets = assets;

	render_state->back_buffer_width = back_buffer_width;
	render_state->back_buffer_height = back_buffer_height;

	//TODO: Adjust these to actual aspect ratio?
	render_state->screen_width = 1280;
	render_state->screen_height = 720;
	render_state->letterboxed_height = (f32)render_state->screen_width;
	render_state->projection = math::orthographic_projection((f32)render_state->screen_width, (f32)render_state->screen_height);

	Shader * basic_shader = &render_state->basic_shader;
	u32 basic_vert = gl::compile_shader_from_source(BASIC_VERT_SRC, GL_VERTEX_SHADER);
	u32 basic_frag = gl::compile_shader_from_source(BASIC_FRAG_SRC, GL_FRAGMENT_SHADER);
	basic_shader->id = gl::link_shader_program(basic_vert, basic_frag);
	basic_shader->i_position = glGetAttribLocation(basic_shader->id, "i_position");
	basic_shader->i_tex_coord = glGetAttribLocation(basic_shader->id, "i_tex_coord");
	basic_shader->i_color = glGetAttribLocation(basic_shader->id, "i_color");
	basic_shader->transform = glGetUniformLocation(basic_shader->id, "transform");
	basic_shader->color = glGetUniformLocation(basic_shader->id, "color");
	basic_shader->tex0 = glGetUniformLocation(basic_shader->id, "tex0");

	Shader * post_shader = &render_state->post_shader;
	u32 post_vert = gl::compile_shader_from_source(SCREEN_QUAD_VERT_SRC, GL_VERTEX_SHADER);
	u32 post_frag = gl::compile_shader_from_source(POST_FILTER_FRAG_SRC, GL_FRAGMENT_SHADER);
	post_shader->id = gl::link_shader_program(post_vert, post_frag);
	post_shader->i_position = glGetAttribLocation(post_shader->id, "i_position");
	post_shader->tex0 = glGetUniformLocation(post_shader->id, "tex0");
	post_shader->tex0_dim = glGetUniformLocation(post_shader->id, "tex0_dim");
	post_shader->pixelate_scale = glGetUniformLocation(post_shader->id, "pixelate_scale");
	post_shader->fade_amount = glGetUniformLocation(post_shader->id, "fade_amount");

	render_state->frame_buffer = gl::create_frame_buffer(back_buffer_width, back_buffer_height, false);

	f32 screen_quad_verts[] = {
		-1.0f,-1.0f,
		 1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f,-1.0f,
		 1.0f,-1.0f,
		 1.0f, 1.0f,
	};

	render_state->screen_quad_v_buf = gl::create_vertex_buffer(screen_quad_verts, ARRAY_COUNT(screen_quad_verts), 2, GL_STATIC_DRAW);

	f32 quad_verts[] = {
		-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	render_state->quad_v_buf = gl::create_vertex_buffer(quad_verts, ARRAY_COUNT(quad_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

	f32 scrollable_quad_verts[] = {
		-1.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-1.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-1.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

		-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

		 0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 1.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	render_state->scrollable_quad_v_buf = gl::create_vertex_buffer(scrollable_quad_verts, ARRAY_COUNT(scrollable_quad_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

	render_state->debug_font = allocate_font(render_state, get_texture_asset(assets, AssetId_font, 0), 65536);

	render_state->debug_batch = allocate_render_batch(render_state->arena, get_texture_asset(assets, AssetId_white, 0), 65536, RenderMode_lines);
	render_state->debug_render_entity_bounds = false;

	render_state->sprite_batch_count = get_asset_count(assets, AssetId_atlas);
	render_state->sprite_batches = PUSH_ARRAY(render_state->arena, RenderBatch *, render_state->sprite_batch_count);
	for(u32 i = 0; i < render_state->sprite_batch_count; i++) {
		u32 batch_size = QUAD_ELEM_COUNT * 128;
		render_state->sprite_batches[i] = allocate_render_batch(render_state->arena, get_texture_asset(assets, AssetId_atlas, i), batch_size);
	}

	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void begin_render(RenderState * render_state) {
	glBindFramebuffer(GL_FRAMEBUFFER, render_state->frame_buffer.id);
	glViewport(0, 0, render_state->frame_buffer.width, render_state->frame_buffer.height);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	
}

void end_render(RenderState * render_state) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, render_state->back_buffer_width, render_state->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	Shader * basic_shader = &render_state->basic_shader;
	Shader * post_shader = &render_state->post_shader;

	{
		gl::VertexBuffer * v_buf = &render_state->screen_quad_v_buf;

		glUseProgram(post_shader->id);

		glUniform2f(post_shader->tex0_dim, (f32)render_state->frame_buffer.width, (f32)render_state->frame_buffer.height);

		f32 pixelate_time = render_state->pixelate_time;
		f32 pixelate_scale = math::frac(pixelate_time);
		if((u32)pixelate_time & 1) {
			pixelate_scale = 1.0f - pixelate_scale;
		}

		pixelate_scale = 1.0f / math::pow(2.0f, (pixelate_scale * 8.0f));
		glUniform1f(post_shader->pixelate_scale, pixelate_scale);

		f32 fade_amount = math::clamp01(render_state->fade_amount);
		glUniform1f(post_shader->fade_amount, fade_amount);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, render_state->frame_buffer.texture_id);
		glUniform1i(post_shader->tex0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, v_buf->id);

		u32 stride = v_buf->vert_size * sizeof(f32);

		glVertexAttribPointer(post_shader->i_position, 2, GL_FLOAT, 0, stride, 0);
		glEnableVertexAttribArray(post_shader->i_position);

		glDrawArrays(GL_TRIANGLES, 0, v_buf->vert_count);		
	}

	{
		Texture * white_tex = get_texture_asset(render_state->assets, AssetId_white, 0);

		math::Vec2 dim = math::vec2(render_state->screen_width, render_state->screen_height);
		math::Mat4 scale = math::scale(dim.x, dim.y, 1.0f);

		f32 letterbox_pixels = (render_state->screen_height - render_state->letterboxed_height) * 0.5f;

		math::Mat4 transform0 = render_state->projection * math::translate(0.0f, dim.y - letterbox_pixels, 0.0f) * scale;
		render_v_buf(&render_state->quad_v_buf, RenderMode_triangles, basic_shader, &transform0, white_tex, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		math::Mat4 transform1 = render_state->projection * math::translate(0.0f,-dim.y + letterbox_pixels, 0.0f) * scale;
		render_v_buf(&render_state->quad_v_buf, RenderMode_triangles, basic_shader, &transform1, white_tex, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	render_and_clear_render_batch(render_state->debug_font->batch, basic_shader, &render_state->debug_font->projection_matrix);

	glDisable(GL_BLEND);
}

//TODO: Remove entities from renderer??
void render_entities(RenderState * render_state, Entity * entities, u32 entity_count) {
	DEBUG_TIME_BLOCK();

	math::Rec2 screen_bounds = math::rec2_pos_dim(math::vec2(0.0f), math::vec2((f32)render_state->screen_width, render_state->letterboxed_height));

	u32 null_batch_index = U32_MAX;
	u32 current_batch_index = null_batch_index;

	Shader * basic_shader = &render_state->basic_shader;

	//TODO: Sorting!!
	for(u32 i = 0; i < entity_count; i++) {
		Entity * entity = entities + i;

		Asset * asset = get_asset(render_state->assets, entity->asset_id, entity->asset_index);
		ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

		math::Vec2 pos = project_pos(&render_state->camera, entity->pos + math::vec3(asset->texture.offset, 0.0f));
		math::Vec2 dim = asset->texture.dim * entity->scale;

		math::Vec4 color = entity->color;
		color.rgb *= color.a;
		
		if(entity->asset_type == AssetType_texture) {
			Texture * tex = &asset->texture;
			gl::VertexBuffer * v_buf = entity->scrollable ? &render_state->scrollable_quad_v_buf : &render_state->quad_v_buf;

			//TODO: Remove epsilon!!
			f32 epsilon = 0.1f;
			if(math::rec_overlap(screen_bounds, math::rec2_pos_dim(pos, dim * 1.0f + epsilon))) {
				if(current_batch_index != null_batch_index) {
					render_and_clear_render_batch(render_state->sprite_batches[current_batch_index], basic_shader, &render_state->projection);
					current_batch_index = null_batch_index;
				}

				math::Mat4 transform = render_state->projection * math::translate(pos.x, pos.y, 0.0f) * math::scale(dim.x, dim.y, 1.0f);
				render_v_buf(v_buf, RenderMode_triangles, basic_shader, &transform, tex, color);
			}
		}
		else {
			Texture * sprite = &asset->sprite;
			ASSERT(sprite->atlas_index < render_state->sprite_batch_count);

			if(math::rec_overlap(screen_bounds, math::rec2_pos_dim(pos, dim))) {
				if(current_batch_index != null_batch_index) {
					RenderBatch * current_batch = render_state->sprite_batches[current_batch_index];
					if(current_batch_index != sprite->atlas_index || current_batch->e >= current_batch->v_len) {
						render_and_clear_render_batch(render_state->sprite_batches[current_batch_index], basic_shader, &render_state->projection);
					}
				}

				current_batch_index = sprite->atlas_index;

				RenderBatch * batch = render_state->sprite_batches[sprite->atlas_index];
				push_sprite(batch, sprite, pos, dim, color);
			}
		}
	}
	
	for(u32 i = 0; i < render_state->sprite_batch_count; i++) {
		render_and_clear_render_batch(render_state->sprite_batches[i], basic_shader, &render_state->projection);
	}

	if(render_state->debug_render_entity_bounds) {
		for(u32 i = 0; i < entity_count; i++) {
			Entity * entity = entities + i;

			// math::Rec2 bounds = get_entity_render_bounds(assets, entity);
			math::Rec2 bounds = math::rec_scale(entity->collider, entity->scale);
			math::Vec2 pos = project_pos(&render_state->camera, entity->pos + math::vec3(math::rec_pos(bounds), 0.0f));
			bounds = math::rec2_pos_dim(pos, math::rec_dim(bounds));

			push_quad_lines(render_state->debug_batch, &bounds, math::vec4(1.0f));
		}

		render_and_clear_render_batch(render_state->debug_batch, basic_shader, &render_state->projection);
	}
}
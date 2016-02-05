
#include <render.hpp>

#include <basic.vert>
#include <basic.frag>
#include <screen_quad.vert>
#include <post_filter.frag>

RenderTransform create_render_transform(u32 projection_width, u32 projection_height, math::Vec2 pos = math::vec2(0.0f), math::Vec2 offset = math::vec2(0.0f)) {
	RenderTransform transform = {};
	transform.pos = pos;
	transform.offset = offset;
	transform.projection_width = projection_width;
	transform.projection_height = projection_height;
	return transform;
}

math::Vec2 project_pos(RenderTransform * transform, math::Vec3 pos) {
	math::Vec2 projected_pos = pos.xy - transform->pos;
	projected_pos *= 1.0f / (pos.z + 1.0f);
	return projected_pos + transform->offset;
}

math::Vec3 unproject_pos(RenderTransform * transform, math::Vec2 pos, f32 z) {
	math::Vec3 unprojected_pos = math::vec3(pos - transform->offset, z);
	unprojected_pos.xy *= (z + 1.0f);
	unprojected_pos.xy += transform->pos;
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

Font * allocate_font(RenderState * render_state, u32 v_len, u32 projection_width, u32 projection_height) {
	Font * font = PUSH_STRUCT(render_state->arena, Font);

	font->batch = allocate_render_batch(render_state->arena, get_texture_asset(render_state->assets, AssetId_font, 0), v_len);

	font->glyph_width = 3;
	font->glyph_height = 5;
	font->glyph_spacing = 1;

	return font;
}

FontLayout create_font_layout(Font * font, math::Vec2 dim, f32 scale, FontLayoutAnchor anchor, math::Vec2 offset = math::vec2(0.0f)) {
	FontLayout layout = {};
	layout.scale = scale;

	switch(anchor) {
		case FontLayoutAnchor_top_left: {
			layout.anchor.x = -dim.x * 0.5f + font->glyph_spacing * scale;
			layout.anchor.y = dim.y * 0.5f - (font->glyph_height + font->glyph_spacing) * scale;

			break;
		}

		case FontLayoutAnchor_top_centre: {
			layout.anchor.x = 0.0f;
			layout.anchor.y = dim.y * 0.5f - (font->glyph_height + font->glyph_spacing) * scale;

			break;
		}

		case FontLayoutAnchor_bottom_left: {
			layout.anchor.x = -dim.x * 0.5f + font->glyph_spacing * scale;
			layout.anchor.y = -dim.y * 0.5f + font->glyph_spacing * scale;

			break;
		}

		INVALID_CASE();
	}

	layout.anchor += offset;
	layout.pos = layout.anchor;
	return layout;
}

void push_quad(RenderBatch * batch, math::Vec2 pos0, math::Vec2 pos1, math::Vec2 uv0, math::Vec2 uv1, math::Vec4 color) {
	ASSERT(batch->v_len >= QUAD_ELEM_COUNT);
	ASSERT(batch->e <= (batch->v_len - QUAD_ELEM_COUNT));
	f32 * v = batch->v_arr;

	color.rgb *= color.a;

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
	ASSERT(batch->v_len >= QUAD_LINES_ELEM_COUNT);
	ASSERT(batch->e <= (batch->v_len - QUAD_LINES_ELEM_COUNT));
	f32 * v = batch->v_arr;

	color.rgb *= color.a;

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

void push_str(Font * font, FontLayout * layout, Str * str) {
	DEBUG_TIME_BLOCK();

	RenderBatch * batch = font->batch;

	f32 glyph_width = font->glyph_width * layout->scale;
	f32 glyph_height = font->glyph_height * layout->scale;
	f32 glyph_spacing = font->glyph_spacing * layout->scale;

	math::Vec2 tex_dim = batch->tex->dim;
	math::Vec2 r_tex_dim = math::vec2(1.0f / tex_dim.x, 1.0f / tex_dim.y);

	//TODO: Put these in the font struct??
	u32 glyph_first_char = 24;
	u32 glyphs_per_row = 12;

	math::Vec4 color = math::vec4(1.0f);

	for(u32 i = 0; i < str->len; i++) {
		char char_ = str->ptr[i];
		if(char_ == '\n') {
			layout->pos.x = layout->anchor.x;
			layout->pos.y -= (glyph_height + glyph_spacing);
		}
		else {
			math::Vec2 pos0 = layout->pos;
			math::Vec2 pos1 = math::vec2(pos0.x + glyph_width, pos0.y + glyph_height);

			layout->pos.x += (glyph_width + glyph_spacing);
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

void push_c_str(Font * font, FontLayout * layout, char const * c_str) {
	Str str = str_from_c_str(c_str);
	push_str(font, layout, &str);
}

f32 get_str_render_width(Font * font, f32 scale, Str * str) {
	f32 width = 0.0f;

	for(u32 i = 0; i < str->len; i++) {
		char char_ = str->ptr[i];
		//TODO: Support multi-line strings??
		ASSERT(char_ != '\n');

		width += (font->glyph_width + font->glyph_spacing) * scale;
	}

	if(width) {
		width -= font->glyph_spacing * scale;
	}

	return width;
}

void render_v_buf(gl::VertexBuffer * v_buf, RenderMode render_mode, Shader * shader, math::Mat4 * transform, Texture * tex0, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	glUseProgram(shader->id);

	glUniformMatrix4fv(shader->transform, 1, GL_FALSE, transform->v);

	color.rgb *= color.a;
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
		glBindBuffer(GL_ARRAY_BUFFER, batch->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch->v_buf.size_in_bytes, batch->v_arr);
		//TODO: This is a bit ugly!!
		batch->v_buf.vert_count = batch->e / VERT_ELEM_COUNT;

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

	render_state->debug_font = allocate_font(render_state, 65536, render_state->back_buffer_width, render_state->back_buffer_height);
	render_state->debug_render_entity_bounds = false;

	render_state->render_batch = allocate_render_batch(render_state->arena, get_texture_asset(assets, AssetId_white, 0), QUAD_ELEM_COUNT * 512);

	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void render_font(RenderState * render_state, Font * font, RenderTransform * render_transform) {
	math::Mat4 projection = math::orthographic_projection((f32)render_transform->projection_width, (f32)render_transform->projection_height);
	render_and_clear_render_batch(font->batch, &render_state->basic_shader, &projection);
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

	RenderTransform font_render_transform = create_render_transform(render_state->back_buffer_width, render_state->back_buffer_height);
	render_font(render_state, render_state->debug_font, &font_render_transform);

	glDisable(GL_BLEND);
}

//TODO: Remove entities from renderer??
void render_entities(RenderState * render_state, RenderTransform * render_transform, Entity * entities, u32 entity_count) {
	DEBUG_TIME_BLOCK();

	math::Rec2 screen_bounds = math::rec2_pos_dim(math::vec2(0.0f), math::vec2((f32)render_state->screen_width, (f32)render_state->screen_height));
	math::Mat4 projection = math::orthographic_projection((f32)render_transform->projection_width, (f32)render_transform->projection_height);

	u32 null_atlas_index = U32_MAX;
	u32 current_atlas_index = null_atlas_index;
	RenderBatch * render_batch = render_state->render_batch;
	ASSERT(!render_batch->e);
	render_batch->mode = RenderMode_triangles;

	Shader * basic_shader = &render_state->basic_shader;

	//TODO: Sorting!!
	for(u32 i = 0; i < entity_count; i++) {
		Entity * entity = entities + i;

		Asset * asset = get_asset(render_state->assets, entity->asset_id, entity->asset_index);
		ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

		math::Vec2 pos = project_pos(render_transform, entity->pos + math::vec3(asset->texture.offset, 0.0f));
		math::Vec2 dim = asset->texture.dim * entity->scale;

		math::Vec4 color = entity->color;
		if(color.a > 0.0f) {
			if(entity->asset_type == AssetType_texture) {
				Texture * tex = &asset->texture;
				gl::VertexBuffer * v_buf = entity->scrollable ? &render_state->scrollable_quad_v_buf : &render_state->quad_v_buf;

				//TODO: Remove epsilon!!
				f32 epsilon = 0.1f;
				if(math::rec_overlap(screen_bounds, math::rec2_pos_dim(pos, dim * 1.0f + epsilon))) {
					if(current_atlas_index != null_atlas_index) {
						render_and_clear_render_batch(render_batch, basic_shader, &projection);
						current_atlas_index = null_atlas_index;
					}

					math::Mat4 transform = projection * math::translate(pos.x, pos.y, 0.0f) * math::scale(dim.x, dim.y, 1.0f);
					render_v_buf(v_buf, RenderMode_triangles, basic_shader, &transform, tex, color);
				}
			}
			else {
				Texture * sprite = &asset->sprite;

				if(math::rec_overlap(screen_bounds, math::rec2_pos_dim(pos, dim))) {
					u32 elems_remaining = render_batch->v_len - render_batch->e;
					if(current_atlas_index != sprite->atlas_index || elems_remaining < QUAD_ELEM_COUNT) {
						render_and_clear_render_batch(render_state->render_batch, basic_shader, &projection);

						current_atlas_index = sprite->atlas_index;
						render_batch->tex = get_texture_asset(render_state->assets, AssetId_atlas, sprite->atlas_index);
					}

					push_sprite(render_batch, sprite, pos, dim, color);
				}
			}			
		}
	}

	if(current_atlas_index != null_atlas_index) {
		render_and_clear_render_batch(render_batch, basic_shader, &projection);
	}
	
	if(render_state->debug_render_entity_bounds) {
		render_batch->tex = get_texture_asset(render_state->assets, AssetId_white, 0);
		render_batch->mode = RenderMode_lines;

		for(u32 i = 0; i < entity_count; i++) {
			Entity * entity = entities + i;

			// math::Rec2 bounds = get_entity_render_bounds(assets, entity);
			math::Rec2 bounds = math::rec_scale(entity->collider, entity->scale);
			math::Vec2 pos = project_pos(render_transform, entity->pos + math::vec3(math::rec_pos(bounds), 0.0f));
			bounds = math::rec2_pos_dim(pos, math::rec_dim(bounds));

			u32 elems_remaining = render_batch->v_len - render_batch->e;
			if(elems_remaining < QUAD_LINES_ELEM_COUNT) {
				render_and_clear_render_batch(render_batch, basic_shader, &projection);
			}

			push_quad_lines(render_batch, &bounds, math::vec4(1.0f));
		}

		render_and_clear_render_batch(render_batch, basic_shader, &projection);
	}
}
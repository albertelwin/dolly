
#include <render.hpp>

#include <basic.vert>
#include <basic.frag>
#include <screen_quad.vert>
#include <post_filter.frag>

math::Vec4 color_255(u32 r, u32 g, u32 b) {
	f32 r_255 = 1.0f / 255.0f;
	return math::vec4((f32)r * r_255, (f32)g * r_255, (f32)b * r_255, 1.0f);
}

RenderTransform create_render_transform(u32 projection_width, u32 projection_height, math::Vec2 pos = math::vec2(0.0f), math::Vec2 offset = math::vec2(0.0f)) {
	RenderTransform transform = {};
	transform.pos = pos;
	transform.offset = offset;
	transform.projection_width = projection_width;
	transform.projection_height = projection_height;
	return transform;
}

math::Vec2 project_pos(math::Vec3 pos) {
	math::Vec2 projected_pos = pos.xy;
	projected_pos *= 1.0f / (pos.z + 1.0f);
	return projected_pos;
}

math::Vec3 unproject_pos(math::Vec2 pos, f32 z) {
	math::Vec3 unprojected_pos = math::vec3(pos, z);
	unprojected_pos.xy *= (z * 1.0f);
	return unprojected_pos;
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

FontLayout create_font_layout(Font * font, math::Vec2 dim, f32 scale, FontLayoutAnchor anchor, math::Vec2 offset = math::vec2(0.0f), b32 pixel_align = true) {
	FontLayout layout = {};
	layout.anchor = anchor;
	layout.scale = scale;
	layout.pixel_align = pixel_align;

	f32 line_height = (font->ascent - font->descent) * scale;

	switch(anchor) {
		case FontLayoutAnchor_top_left: {
			layout.align.x = -dim.x * 0.5f;
			layout.align.y = dim.y * 0.5f - line_height;

			break;
		}

		case FontLayoutAnchor_top_centre: {
			layout.align.x = 0.0f;
			layout.align.y = dim.y * 0.5f - line_height;

			break;
		}

		case FontLayoutAnchor_top_right: {
			layout.align.x = dim.x * 0.5f;
			layout.align.y = dim.y * 0.5f - line_height;

			break;
		}

		case FontLayoutAnchor_bottom_left: {
			layout.align.x = -dim.x * 0.5f;
			layout.align.y = -dim.y * 0.5f;

			break;
		}

		INVALID_CASE();
	}

	layout.align += offset;
	layout.pos = layout.align;
	return layout;
}

void push_quad_to_batch(RenderBatch * batch, math::Vec2 pos0, math::Vec2 pos1, math::Vec2 uv0, math::Vec2 uv1, math::Vec4 color) {
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

void push_rotated_quad_to_batch(RenderBatch * batch, math::Vec2 pos0, math::Vec2 pos1, math::Vec2 pos2, math::Vec2 pos3, math::Vec2 uv0, math::Vec2 uv1, math::Vec4 color) {
	ASSERT(batch->v_len >= QUAD_ELEM_COUNT);
	ASSERT(batch->e <= (batch->v_len - QUAD_ELEM_COUNT));
	f32 * v = batch->v_arr;

	color.rgb *= color.a;
	
	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos2.x; v[batch->e++] =  pos2.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos0.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv0.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos3.x; v[batch->e++] =  pos3.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;
}

void push_quad_lines_to_batch(RenderBatch * batch, math::Rec2 * rec, math::Vec4 color) {
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

void push_sprite_to_batch(RenderBatch * batch, Texture * sprite, math::Vec2 pos, math::Vec2 dim, f32 angle, math::Vec4 color) {
	math::Vec2 x_axis = math::vec2(math::cos(angle), math::sin(angle));
	math::Vec2 y_axis = math::perp(x_axis);

	x_axis *= dim.x * 0.5f;
	y_axis *= dim.y * 0.5f;

	math::Vec2 pos0 = pos - x_axis - y_axis;
	math::Vec2 pos1 = pos + x_axis + y_axis;
	math::Vec2 pos2 = pos - x_axis + y_axis;
	math::Vec2 pos3 = pos + x_axis - y_axis;

	math::Vec2 uv0 = sprite->tex_coords[0];
	math::Vec2 uv1 = sprite->tex_coords[1];

	push_rotated_quad_to_batch(batch, pos0, pos1, pos2, pos3, uv0, uv1, color);
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

	render_state->render_batch = allocate_render_batch(render_state->arena, get_texture_asset(assets, AssetId_white, 0), QUAD_ELEM_COUNT * 512);

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

	glDisable(GL_BLEND);
}

RenderGroup * allocate_render_group(RenderState * render_state, MemoryArena * arena, u32 projection_width, u32 projection_height, u32 max_elem_count = 256) {
	RenderGroup * render_group = PUSH_STRUCT(arena, RenderGroup);

	render_group->max_elem_count = max_elem_count;
	render_group->elem_count = 0;
	render_group->elems = PUSH_ARRAY(arena, RenderElement, render_group->max_elem_count);

	render_group->assets = render_state->assets;

	render_group->transform = create_render_transform(projection_width, projection_height);
	render_group->projection_bounds = math::rec2_pos_dim(math::vec2(0.0f), math::vec2(projection_width, projection_height));

	return render_group;
}

RenderElement * push_render_elem(RenderGroup * render_group, Asset * asset, math::Vec3 pos, math::Vec2 dim, f32 angle, math::Vec4 color, b32 scrollable = false) {
	ASSERT(render_group);
	ASSERT(asset);
	ASSERT(render_group->elem_count < render_group->max_elem_count);

	math::Vec2 pos2 = project_pos(&render_group->transform, pos);

	//TODO: Need to do an AABB test now that we have rotation!!
	b32 culled = true;
	if(color.a > 0.0f) {
		if(asset->type == AssetType_texture) {
			//TODO: Remove epsilon!!
			f32 epsilon = 0.1f;
			if(math::rec_overlap(render_group->projection_bounds, math::rec2_pos_dim(pos2, dim * (1.0f + epsilon)))) {
				culled = false;
			}			
		}
		else {
			if(math::rec_overlap(render_group->projection_bounds, math::rec2_pos_dim(pos2, dim))) {
				culled = false;
			}
		}
	}

	RenderElement * elem = 0;
	if(!culled) {
		elem = render_group->elems + render_group->elem_count++;
		elem->pos = pos2;
		elem->dim = dim;
		elem->color = color;
		elem->angle = angle;
		elem->scrollable = scrollable;
		elem->asset = asset;
	}

	return elem;
}

void push_colored_quad(RenderGroup * render_group, math::Vec3 pos, math::Vec2 dim, f32 angle, math::Vec4 color) {
	ASSERT(render_group);

	push_render_elem(render_group, get_asset(render_group->assets, AssetId_white, 0), pos, dim, angle, color);
}

void push_textured_quad(RenderGroup * render_group, AssetRef ref, math::Vec3 pos = math::vec3(0.0f), math::Vec2 scale = math::vec2(1.0f), f32 angle = 0.0f, math::Vec4 color = math::vec4(1.0f), b32 scrollable = false) {
	ASSERT(render_group);

	Asset * asset = get_asset(render_group->assets, ref.id, ref.index);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);
	RenderElement * elem = push_render_elem(render_group, asset, pos + math::vec3(asset->texture.offset, 0.0f), asset->texture.dim * scale, angle, color, scrollable);
}

f32 get_str_width_to_new_line(AssetState * assets, Font * font, f32 scale, char * str, u32 len) {
	f32 width = 0.0f;

	for(u32 i = 0; i < len; i++) {
		char char_ = str[i];
		if(char_ != '\n') {
			if(char_ != ' ') {
				u32 glyph_index = get_font_glyph_index(char_);
				width += font->glyphs[glyph_index].advance;
			}
			else {
				width += font->whitespace_advance;
			}
		}
		else {
			break;
		}
	}

	return width * scale;
}

void push_str_to_render_group(RenderGroup * render_group, Font * font, FontLayout * layout, Str * str, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	ASSERT(render_group);

	f32 line_height = (font->ascent + font->descent) * layout->scale;
	//TODO: Can we get this from the font??
	f32 line_gap = line_height * 0.75f;

	math::Vec2 offset = math::vec2(0.0f);
	b32 new_line = true;

	for(u32 i = 0; i < str->len; i++) {
		char char_ = str->ptr[i];

		if(new_line) {
			if(layout->anchor == FontLayoutAnchor_top_centre) {
				offset.x = -get_str_width_to_new_line(render_group->assets, font, layout->scale, str->ptr + i, str->len - i) * 0.5f;
			}
			else if(layout->anchor == FontLayoutAnchor_top_right) {
				offset.x = -get_str_width_to_new_line(render_group->assets, font, layout->scale, str->ptr + i, str->len - i);
			}

			new_line = false;
		}

		if(char_ == '\n') {
			layout->pos.x = layout->align.x;
			layout->pos.y -= (line_height + line_gap);

			new_line = true;
		}
		else {
			if(char_ != ' ') {
				u32 glyph_index = get_font_glyph_index(char_);

				Asset * asset = get_asset(render_group->assets, (AssetId)font->glyph_id, glyph_index);
				Texture * sprite = &asset->sprite;

				math::Vec2 align = layout->pos + offset;
				//TODO: We need something that works universally!!
				if(layout->pixel_align) {
					align.x = (i32)align.x;
					align.y = (i32)align.y;					
				}

				push_render_elem(render_group, asset, math::vec3(align + sprite->offset * layout->scale, 0.0f), sprite->dim * layout->scale, 0.0f, color);

				layout->pos.x += font->glyphs[glyph_index].advance * layout->scale;
			}
			else {
				layout->pos.x += font->whitespace_advance * layout->scale;
			}
		}
	} 
}

void push_c_str_to_render_group(RenderGroup * render_group, Font * font, FontLayout * layout, char const * c_str, math::Vec4 color = math::vec4(1.0f)) {
	Str str = str_from_c_str(c_str);
	push_str_to_render_group(render_group, font, layout, &str, color);
}

void render_and_clear_render_group(RenderState * render_state, RenderGroup * render_group) {
	ASSERT(render_group);
	DEBUG_TIME_BLOCK();

	RenderTransform * render_transform = &render_group->transform;
	math::Mat4 projection = math::orthographic_projection((f32)render_transform->projection_width, (f32)render_transform->projection_height);

	u32 null_atlas_index = U32_MAX;
	u32 current_atlas_index = null_atlas_index;
	RenderBatch * render_batch = render_state->render_batch;
	ASSERT(!render_batch->e);
	render_batch->mode = RenderMode_triangles;

	Shader * basic_shader = &render_state->basic_shader;

	for(u32 i = 0; i < render_group->elem_count; i++) {
		RenderElement * elem = render_group->elems + i;
		Asset * asset = elem->asset;

		if(asset->type == AssetType_texture) {
			Texture * tex = &asset->texture;

			if(current_atlas_index != null_atlas_index) {
				render_and_clear_render_batch(render_batch, basic_shader, &projection);
				current_atlas_index = null_atlas_index;
			}

			//TODO: This is total overkill!!
			math::Mat4 transform = projection * math::translate(elem->pos.x, elem->pos.y, 0.0f) * math::rotate_around_z(elem->angle) * math::scale(elem->dim.x, elem->dim.y, 1.0f);
			gl::VertexBuffer * v_buf = elem->scrollable ? &render_state->scrollable_quad_v_buf : &render_state->quad_v_buf;
			render_v_buf(v_buf, RenderMode_triangles, basic_shader, &transform, tex, elem->color);
		}
		else {
			Texture * sprite = &asset->sprite;

			u32 elems_remaining = render_batch->v_len - render_batch->e;
			if(current_atlas_index != sprite->atlas_index || elems_remaining < QUAD_ELEM_COUNT) {
				render_and_clear_render_batch(render_batch, basic_shader, &projection);

				current_atlas_index = sprite->atlas_index;
				render_batch->tex = get_texture_asset(render_state->assets, AssetId_atlas, sprite->atlas_index);
			}

			push_sprite_to_batch(render_batch, sprite, elem->pos, elem->dim, elem->angle, elem->color);
		}
	}

	if(current_atlas_index != null_atlas_index) {
		render_and_clear_render_batch(render_batch, basic_shader, &projection);
	}

	render_group->elem_count = 0;
}
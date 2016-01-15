
#include <game.hpp>

#include <asset.cpp>
#include <audio.cpp>
#include <math.cpp>

Entity * push_entity(GameState * game_state, AssetId asset_id, u32 asset_index, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(game_state->entity_count < ARRAY_COUNT(game_state->entity_array));

	//TODO: Should entities only be sprites??
	Asset * asset = get_asset(&game_state->asset_state, asset_id, asset_index);
	ASSERT(asset);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

	Entity * entity = game_state->entity_array + game_state->entity_count++;
	entity->pos = pos;
	entity->scale = 1.0f;
	entity->color = math::vec4(1.0f);

	entity->asset_type = asset->type;
	entity->asset_id = asset_id;
	entity->asset_index = asset_index;
	entity->v_buf = &game_state->entity_v_buf;

	entity->d_pos = math::vec2(0.0f);
	entity->speed = math::vec2(500.0f);
	entity->damp = 0.1f;
	entity->use_gravity = false;

	entity->anim_time = 0.0f;
	entity->initial_x = pos.x;
	entity->hit = false;

	return entity;
}

RenderBatch * allocate_render_batch(MemoryPool * pool, gl::Texture * tex, u32 v_len, RenderMode render_mode = RenderMode_triangles) {
	RenderBatch * batch = PUSH_STRUCT(pool, RenderBatch);

	batch->tex = tex;

	batch->v_len = v_len;
	batch->v_arr = PUSH_ARRAY(pool, f32, batch->v_len);
	batch->v_buf = gl::create_vertex_buffer(batch->v_arr, batch->v_len, VERT_ELEM_COUNT, GL_DYNAMIC_DRAW);
	batch->e = 0;

	batch->mode = render_mode;

	return batch;
}

void render_quad(RenderBatch * batch, math::Vec2 pos0, math::Vec2 pos1, math::Vec2 uv0, math::Vec2 uv1, math::Vec4 color) {
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

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	
}

void render_quad_lines(RenderBatch * batch, math::Rec2 * rec, math::Vec4 color) {
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

void render_sprite(RenderBatch * batch, Sprite * sprite, math::Vec2 pos, math::Vec2 dim, math::Vec4 color) {
	math::Vec2 pos0 = pos - dim * 0.5f;
	math::Vec2 pos1 = pos + dim * 0.5f;

	math::Vec2 uv0 = sprite->tex_coords[0];
	math::Vec2 uv1 = sprite->tex_coords[1];

	render_quad(batch, pos0, pos1, uv0, uv1, color);
}

void render_v_buf(gl::VertexBuffer * v_buf, RenderMode render_mode, Shader * shader, math::Mat4 * xform, gl::Texture * tex0, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	glUseProgram(shader->id);

	glUniformMatrix4fv(shader->xform, 1, GL_FALSE, xform->v);
	glUniform4f(shader->color, color.r, color.g, color.b, color.a);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0->id);
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

void render_and_clear_render_batch(RenderBatch * batch, Shader * shader, math::Mat4 * xform) {
	DEBUG_TIME_BLOCK();

	if(batch->e > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, batch->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch->v_buf.size_in_bytes, batch->v_arr);

		render_v_buf(&batch->v_buf, batch->mode, shader, xform, batch->tex);

		zero_memory((u8 *)batch->v_arr, batch->v_len * sizeof(f32));
		batch->e = 0;		
	}
}
 
Font * allocate_font(MemoryPool * pool, gl::Texture * tex, u32 v_len, u32 back_buffer_width, u32 back_buffer_height) {
	Font * font = PUSH_STRUCT(pool, Font);

	font->batch = allocate_render_batch(pool, tex, v_len);

	font->projection_matrix = math::orthographic_projection((f32)back_buffer_width, (f32)back_buffer_height);
	font->glyph_width = 3;
	font->glyph_height = 5;
	font->glyph_spacing = 1;

	font->scale = 2.0f;
	font->anchor.x = -(f32)back_buffer_width * 0.5f + font->glyph_spacing * font->scale;
	font->anchor.y = (f32)back_buffer_height * 0.5f - (font->glyph_height + font->glyph_spacing) * font->scale;
	font->pos = font->anchor;

	return font;
}

Str * allocate_str(MemoryPool * pool, u32 max_len) {
	Str * str = PUSH_STRUCT(pool, Str);
	str->max_len = max_len;
	str->len = 0;
	str->ptr = PUSH_ARRAY(pool, char, max_len);	
	return str;
}

void render_str(Font * font, Str * str) {
	DEBUG_TIME_BLOCK();

	RenderBatch * batch = font->batch;

	f32 glyph_width = font->glyph_width * font->scale;
	f32 glyph_height = font->glyph_height * font->scale;
	f32 glyph_spacing = font->glyph_spacing * font->scale;

	u32 tex_size = batch->tex->width;
	f32 r_tex_size = 1.0f / (f32)tex_size;

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

				math::Vec2 uv0 = math::vec2(u, tex_size - (v + font->glyph_height)) * r_tex_size;
				math::Vec2 uv1 = math::vec2(u + font->glyph_width, tex_size - v) * r_tex_size;

				render_quad(batch, pos0, pos1, uv0, uv1, color);
			}
		}
	}
}

void render_c_str(Font * font, char const * c_str) {
	Str str;
	str.max_len = str.len = c_str_len(c_str);
	str.ptr = (char *)c_str;
	render_str(font, &str);
}

math::Vec2 project_pos(Camera * camera, math::Vec3 pos) {
	math::Vec2 projected_pos = pos.xy - camera->pos;
	projected_pos *= 1.0f / (pos.z + 1.0f);
	return projected_pos;
}

math::Vec3 unproject_pos(Camera * camera, math::Vec2 pos, f32 z) {
	math::Vec3 unprojected_pos = math::vec3(pos, z);
	unprojected_pos.xy *= (z + 1.0f);
	unprojected_pos.xy += camera->pos;
	return unprojected_pos;
}

math::Rec2 get_entity_bounds(AssetState * assets, Entity * entity) {
	math::Vec2 pos = entity->pos.xy;
	math::Vec2 dim;
	if(entity->asset_type == AssetType_texture) {
		Texture * texture = get_texture_asset(assets, entity->asset_id, entity->asset_index);
		dim = math::vec2(texture->width, texture->height);
	}
	else {
		Sprite * sprite = get_sprite_asset(assets, entity->asset_id, entity->asset_index);
		pos += sprite->offset;
		dim = sprite->dim;
	}

	return math::rec2_centre_dim(pos, dim * entity->scale);
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;
	AssetState * assets = &game_state->asset_state;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_pool = create_memory_pool((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

		load_assets(assets, &game_state->memory_pool);

		AudioState * audio_state = &game_state->audio_state;
		load_audio(audio_state, &game_state->memory_pool, assets, game_input->audio_supported);
		audio_state->master_volume = 0.0f;

		game_state->music = play_audio_clip(audio_state, AssetId_music, true);

		Shader * basic_shader = &game_state->basic_shader;
		u32 basic_vert = gl::compile_shader_from_source(BASIC_VERT_SRC, GL_VERTEX_SHADER);
		u32 basic_frag = gl::compile_shader_from_source(BASIC_FRAG_SRC, GL_FRAGMENT_SHADER);
		basic_shader->id = gl::link_shader_program(basic_vert, basic_frag);
		basic_shader->i_position = glGetAttribLocation(basic_shader->id, "i_position");
		basic_shader->i_tex_coord = glGetAttribLocation(basic_shader->id, "i_tex_coord");
		basic_shader->i_color = glGetAttribLocation(basic_shader->id, "i_color");
		basic_shader->xform = glGetUniformLocation(basic_shader->id, "xform");
		basic_shader->color = glGetUniformLocation(basic_shader->id, "color");
		basic_shader->tex0 = glGetUniformLocation(basic_shader->id, "tex0");

		f32 quad_verts[] = {
			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};

		game_state->entity_v_buf = gl::create_vertex_buffer(quad_verts, ARRAY_COUNT(quad_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

		f32 bg_verts[] = {
			-1.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-1.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-1.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,

			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,

			 0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 1.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};

		game_state->bg_v_buf = gl::create_vertex_buffer(bg_verts, ARRAY_COUNT(bg_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

		gl::Texture * font_tex = get_texture_asset(assets, AssetId_font, 0);
		game_state->font = allocate_font(&game_state->memory_pool, font_tex, 65536, game_input->back_buffer_width, game_input->back_buffer_height);
		game_state->debug_str = allocate_str(&game_state->memory_pool, 1024);
		game_state->title_str = allocate_str(&game_state->memory_pool, 128);

		//TODO: Adjust these to actual aspect ratio?
		game_state->ideal_window_width = 1280;
		game_state->ideal_window_height = 720;

		game_state->projection_matrix = math::orthographic_projection((f32)game_state->ideal_window_width, (f32)game_state->ideal_window_height);

		game_state->debug_batch = allocate_render_batch(&game_state->memory_pool, get_texture_asset(assets, AssetId_white, 0), 65536, RenderMode_lines);
		game_state->debug_render_entity_bounds = false;

		game_state->sprite_batch_count = get_asset_count(assets, AssetId_atlas);
		game_state->sprite_batches = PUSH_ARRAY(&game_state->memory_pool, RenderBatch *, game_state->sprite_batch_count);
		for(u32 i = 0; i < game_state->sprite_batch_count; i++) {
			u32 batch_size = QUAD_ELEM_COUNT * 32;
			game_state->sprite_batches[i] = allocate_render_batch(&game_state->memory_pool, get_texture_asset(assets, AssetId_atlas, i), batch_size);
		}

		game_state->entity_gravity = math::vec2(0.0f, -10000.0f);

		game_state->bg_layer_count = get_asset_count(assets, AssetId_background_layer);
		game_state->bg_layers = PUSH_ARRAY(&game_state->memory_pool, Entity *, game_state->bg_layer_count);
		for(u32 i = 0; i < game_state->bg_layer_count - 1; i++) {
			Entity * bg_layer = push_entity(game_state, AssetId_background_layer, i);

			bg_layer->pos.z = 2.5f * (game_state->bg_layer_count - i);
			bg_layer->v_buf = &game_state->bg_v_buf;

			game_state->bg_layers[i] = bg_layer;
		}

		game_state->player = push_entity(game_state, AssetId_dolly, 0, math::vec3((f32)game_state->ideal_window_width * -0.5f * (2.0f / 3.0f), 0.0f, 0.0f));
		game_state->player->speed = math::vec2(50.0f, 6000.0f);
		game_state->player->damp = 0.15f;
		game_state->player->initial_x = game_state->player->pos.x;

		EntityEmitter * emitter = &game_state->entity_emitter;
		Sprite * emitter_sprite = get_sprite_asset(assets, AssetId_teacup, 0);
		emitter->pos = math::vec3(game_state->ideal_window_width * 0.75f, 0.0f, 0.0f);
		emitter->time_until_next_spawn = 0.0f;
		emitter->entity_count = 0;
		for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
			emitter->entity_array[i] = push_entity(game_state, AssetId_teacup, 0, emitter->pos);
		}

		// push_entity(game_state, AssetId_atlas, 1);

		//TODO: Adding foreground layer at the end until we have sorting!!
		{
			u32 i = game_state->bg_layer_count - 1;
			Entity * bg_layer = push_entity(game_state, AssetId_background_layer, i);

			bg_layer->pos.z = -0.5f;
			bg_layer->v_buf = &game_state->bg_v_buf;
			game_state->bg_layers[i] = bg_layer;
		}

		game_state->camera.pos = math::vec2(0.0f, game_state->player->pos.y);

		game_state->d_time = 1.0f;
		game_state->pitch = 1.0f;
		game_state->score = 0;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		// game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;

		game_state->debug_render_entity_bounds = !game_state->debug_render_entity_bounds;
	}

	f32 dd_time = 0.0f;
	if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
		dd_time -= 4.0f * game_input->delta_time;
	}

	if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
		dd_time += 4.0f * game_input->delta_time;
	}

	game_state->d_time += dd_time;
	//TODO: Disable this to go backwards in time!!
	game_state->d_time = math::max(game_state->d_time, 0.0f);

	if(game_state->d_time < 1.0f) {
		game_state->pitch = game_state->d_time;
	}
	else {
		game_state->pitch = 1.0f + (game_state->d_time - 1.0f) * 0.02f;
	}

	change_pitch(game_state->music, game_state->pitch);
	
	f32 adjusted_dt = game_state->d_time * game_input->delta_time;

	f32 half_screen_width = (f32)game_state->ideal_window_width * 0.5f;
	f32 half_screen_height = (f32)game_state->ideal_window_height * 0.5f;

	Entity * player = game_state->player;
	math::Vec2 player_dd_pos = math::vec2(0.0f);

	if(game_input->buttons[ButtonId_up] & KEY_DOWN) {
		player_dd_pos.y += player->speed.y;
	}

	if(game_input->buttons[ButtonId_down] & KEY_DOWN) {
		player_dd_pos.y -= player->speed.y;
	}

	if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
		player_dd_pos.x -= player->speed.x;
	}

	if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
		player_dd_pos.x += player->speed.x;
	}

	// if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		// player->use_gravity = !player->use_gravity;
	// }

	if(player->use_gravity) {
		player_dd_pos += game_state->entity_gravity;
	}

	player->d_pos += player_dd_pos * game_input->delta_time;
	player->d_pos *= (1.0f - player->damp);

	player->pos.xy += player->d_pos * game_input->delta_time;

	f32 max_dist_x = 20.0f;
	player->pos.x = math::clamp(player->pos.x, player->initial_x - max_dist_x, player->initial_x + max_dist_x);

	EntityEmitter * emitter = &game_state->entity_emitter;
	emitter->time_until_next_spawn -= adjusted_dt;
	if(emitter->time_until_next_spawn <= 0.0f) {
		if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
			Entity * entity = emitter->entity_array[emitter->entity_count++];

			entity->pos = math::vec3(emitter->pos.x, emitter->pos.y + (math::rand_f32() * 2.0f - 1.0f) * 250.0f, 0.0f);
			entity->scale = 1.0f;
			entity->color = math::vec4(1.0f);

			AssetId asset_id = math::rand_f32() > 0.5f ? AssetId_smiley : AssetId_telly;
			Asset * asset = get_asset(assets, asset_id, 0);
			ASSERT(asset);
			ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

			entity->asset_type = asset->type;
			entity->asset_id = asset_id;
			entity->asset_index = 0;

			entity->hit = false;
		}

		emitter->time_until_next_spawn = 0.5f;
	}

	math::Rec2 player_bounds = get_entity_bounds(assets, player);

	for(u32 i = 0; i < emitter->entity_count; i++) {
		Entity * entity = emitter->entity_array[i];
		b32 destroy = false;

		math::Rec2 bounds = get_entity_bounds(assets, entity);

		if(!entity->hit) {
			f32 dd_pos = -500.0f * adjusted_dt;
			entity->pos.x += dd_pos;
			if(entity->pos.x < -(half_screen_width + math::rec_dim(bounds).x * 0.5f)) {
				destroy = true;
			}
		}
		else {
			f32 dd_pos = 500.0f * adjusted_dt;
			entity->pos.y += dd_pos;

			entity->scale -= 4.0f * adjusted_dt;
			if(entity->scale < 0.0f) {
				destroy = true;
			}
		}

		entity->anim_time += adjusted_dt * ANIMATION_FRAMES_PER_SEC;;
		entity->asset_index = (u32)entity->anim_time % get_asset_count(assets, entity->asset_id);

		//TODO: When should this check happen??
		if(rec_overlap(player_bounds, bounds) && !entity->hit) {
			entity->hit = true;
			entity->scale = 2.0f;

			//TODO: Should there be a helper function for this??
			AssetId clip_id = AssetId_pickup;
			AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
			AudioSource * source = play_audio_clip(&game_state->audio_state, clip);
			change_pitch(source, math::lerp(0.9f, 1.1f, math::rand_f32()));

			player->asset_index++;
			if(player->asset_index >= get_asset_count(assets, player->asset_id)) {
				player->asset_index = 0;
			}

			game_state->score++;
		}

		if(destroy) {
			entity->pos = emitter->pos;

			u32 swap_index = emitter->entity_count - 1;
			emitter->entity_array[i] = emitter->entity_array[swap_index];
			emitter->entity_array[swap_index] = entity;
			emitter->entity_count--;
			i--;
		}
	}

	for(u32 i = 0; i < game_state->bg_layer_count; i++) {
		Entity * bg_layer = game_state->bg_layers[i];

		bg_layer->pos.x -= 500.0f * adjusted_dt;

		math::Vec2 screen_pos = project_pos(&game_state->camera, bg_layer->pos);
		math::Rec2 bounds = get_entity_bounds(assets, bg_layer);
		f32 width = math::rec_dim(bounds).x;

		if(screen_pos.x < -(half_screen_width + width * 0.5f)) {
			screen_pos.x += width * 2.0f;
			bg_layer->pos = unproject_pos(&game_state->camera, screen_pos, bg_layer->pos.z);
		}
	}

#if 1
	f32 actual_over_ideal_height = (f32)game_input->back_buffer_height / (f32)game_state->ideal_window_height;
	u32 letterboxing_pixels = (u32)(math::max(game_state->d_time - 1.0f, 0.0f) * 10.0f * actual_over_ideal_height);

	u32 min_viewport_height = game_input->back_buffer_height / 3;
	u32 viewport_height;
	if(letterboxing_pixels * 2 < game_input->back_buffer_height - min_viewport_height) {
		viewport_height = game_input->back_buffer_height - letterboxing_pixels * 2;
	}
	else {
		viewport_height = min_viewport_height;
		letterboxing_pixels = (game_input->back_buffer_height - min_viewport_height) / 2;
	}
#else
	u32 letterboxing_pixels = 0;
	u32 viewport_height = game_input->back_buffer_height;
#endif

	u32 ideal_window_height = (u32)((f32)game_state->ideal_window_height * (f32)viewport_height / (f32)game_input->back_buffer_height);
	game_state->projection_matrix = math::orthographic_projection((f32)game_state->ideal_window_width, (f32)ideal_window_height);

	glViewport(0, letterboxing_pixels, game_input->back_buffer_width, viewport_height);
	glClear(GL_COLOR_BUFFER_BIT);

	Shader * shader = &game_state->basic_shader;
	
	{
		Texture * tex = get_texture_asset(assets, AssetId_background, 0);
		ASSERT(tex);

		math::Vec2 dim = math::vec2(tex->width, tex->height);
		math::Mat4 xform = game_state->projection_matrix * math::scale(dim.x, dim.y, 1.0f);

		render_v_buf(&game_state->entity_v_buf, RenderMode_triangles, shader, &xform, tex);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	{
		DEBUG_TIME_BLOCK();

		Camera * camera = &game_state->camera;
		camera->pos.x = 0.0f;
		camera->pos.y = math::max(player->pos.y, 0.0f);
		camera->offset = (math::rand_vec2() * 2.0f - 1.0f) * math::max(game_state->d_time - 2.0f, 0.0f);

		u32 null_batch_index = game_state->sprite_batch_count;
		u32 current_batch_index = null_batch_index;

		//TODO: Sorting!!
		for(u32 i = 0; i < game_state->entity_count; i++) {
			Entity * entity = game_state->entity_array + i;

			math::Vec2 pos = project_pos(camera, entity->pos) + camera->offset;

			if(entity->asset_type == AssetType_texture) {
				gl::Texture * tex = get_texture_asset(assets, entity->asset_id, entity->asset_index);

				if(current_batch_index != null_batch_index) {
					render_and_clear_render_batch(game_state->sprite_batches[current_batch_index], shader, &game_state->projection_matrix);
					current_batch_index = null_batch_index;
				}

				math::Vec2 dim = math::vec2(tex->width, tex->height) * entity->scale;
				math::Mat4 xform = game_state->projection_matrix * math::translate(pos.x, pos.y, 0.0f) * math::scale(dim.x, dim.y, 1.0f);

				render_v_buf(entity->v_buf, RenderMode_triangles, shader, &xform, tex, entity->color);
			}
			else {
				Sprite * sprite = get_sprite_asset(assets, entity->asset_id, entity->asset_index);
				ASSERT(sprite->atlas_index < game_state->sprite_batch_count);

				if(current_batch_index != sprite->atlas_index && current_batch_index != null_batch_index) {
					render_and_clear_render_batch(game_state->sprite_batches[current_batch_index], shader, &game_state->projection_matrix);
				}

				current_batch_index = sprite->atlas_index;

				RenderBatch * batch = game_state->sprite_batches[sprite->atlas_index];
				render_sprite(batch, sprite, pos + sprite->offset, sprite->dim * entity->scale, entity->color);
			}
		}
		
		for(u32 i = 0; i < game_state->sprite_batch_count; i++) {
			render_and_clear_render_batch(game_state->sprite_batches[i], shader, &game_state->projection_matrix);
		}

		if(game_state->debug_render_entity_bounds) {
			for(u32 i = 0; i < game_state->entity_count; i++) {
				Entity * entity = game_state->entity_array + i;

				math::Vec2 pos = project_pos(camera, entity->pos) + camera->offset;
				math::Vec2 dim;

				//TODO: Compress this down!!
				if(entity->asset_type == AssetType_texture) {
					Texture * tex = get_texture_asset(assets, entity->asset_id, entity->asset_index);
					dim = math::vec2(tex->width, tex->height) * entity->scale;
				}
				else {
					Sprite * sprite = get_sprite_asset(assets, entity->asset_id, entity->asset_index);
					pos += sprite->offset;
					dim = sprite->dim * entity->scale;
				}

				math::Rec2 rec = math::rec2_centre_dim(pos, dim);

				render_quad_lines(game_state->debug_batch, &rec, math::vec4(1.0f));
			}

			render_and_clear_render_batch(game_state->debug_batch, shader, &game_state->projection_matrix);
		}
	}

	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);

	{
		DEBUG_TIME_BLOCK();

		Font * font = game_state->font;
		font->pos = font->anchor;

		RenderBatch * batch = font->batch;

		str_clear(game_state->title_str);
		str_print(game_state->title_str, "DOLLY DOLLY DOLLY DAYS!\nDT: %f | D_TIME: %f | D_POS %f | SCORE: %u\n\n", game_input->delta_time, game_state->d_time, game_state->player->d_pos.y, game_state->score);
		render_str(font, game_state->title_str);

		render_str(font, game_state->debug_str);

		render_and_clear_render_batch(batch, shader, &font->projection_matrix);
	}

	glDisable(GL_BLEND);
}

#if 0
static u32 global_sample_index = 0;

void game_sample(GameMemory * game_memory, i16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
	f32 tone_hz = 440.0f;

	for(u32 i = 0, sample_index = 0; i < samples_to_write; i++) {
		f32 sample_time = (f32)global_sample_index++ / (f32)samples_per_second;

		f32 sample_f32 = math::sin(sample_time * math::TAU * tone_hz);
		i16 sample = (i16)(sample_f32 * (sample_f32 > 0.0f ? 32767.0f : 32768.0f));

		sample_memory_ptr[sample_index++] = sample;
		sample_memory_ptr[sample_index++] = sample;
	}
}
#else
void game_sample(GameMemory * game_memory, i16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
	DEBUG_TIME_BLOCK();

	for(u32 i = 0, sample_index = 0; i < samples_to_write; i++) {
		sample_memory_ptr[sample_index++] = 0;
		sample_memory_ptr[sample_index++] = 0;
	}

	if(game_memory->initialized) {
		GameState * game_state = (GameState *)game_memory->ptr;
		audio_output_samples(&game_state->audio_state, sample_memory_ptr, samples_to_write, samples_per_second);
	}
}
#endif

DebugBlockProfile debug_block_profiles[__COUNTER__];

void debug_game_tick(GameMemory * game_memory, GameInput * game_input) {
	if(game_memory->initialized) {
		GameState * game_state = (GameState *)game_memory->ptr;

		Str * str = game_state->debug_str;
		str_clear(str);

		for(u32 i = 0; i < ARRAY_COUNT(debug_block_profiles); i++) {
			DebugBlockProfile * profile = debug_block_profiles + i;

			if(profile->func) {
				str_print(str, "%s[%u]: time: %fms | hits: %u\n", profile->func, profile->id, profile->ms, profile->hits);
			}

			profile->ms = 0.0;
			profile->hits = 0;
		}
	}
}


#include <game.hpp>

#include <asset.cpp>
#include <audio.cpp>
#include <math.cpp>

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

math::Rec2 get_asset_bounds(AssetState * assets, AssetId asset_id, u32 asset_index) {
	math::Vec2 pos = math::vec2(0.0f);
	math::Vec2 dim;

	Asset * asset = get_asset(assets, asset_id, asset_index);
	if(asset->type == AssetType_texture) {
		dim = math::vec2(asset->texture.width, asset->texture.height);
	}
	else {
		ASSERT(asset->type == AssetType_sprite);
		Sprite * sprite = &asset->sprite;

		pos += sprite->offset;
		dim = sprite->dim;
	}

	return math::rec2_pos_dim(pos, dim);
}

Entity * push_entity(GameState * game_state, AssetId asset_id, u32 asset_index, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(game_state->entity_count < ARRAY_COUNT(game_state->entity_array));

	//TODO: Should entities only be sprites??
	Asset * asset = get_asset(&game_state->assets, asset_id, asset_index);
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
	entity->collider = get_asset_bounds(&game_state->assets, entity->asset_id, entity->asset_index);

	entity->anim_time = 0.0f;
	entity->initial_x = pos.x;
	entity->hit = false;

	return entity;
}

math::Rec2 get_entity_render_bounds(AssetState * assets, Entity * entity, Camera * camera) {
	math::Rec2 bounds = get_asset_bounds(assets, entity->asset_id, entity->asset_index);

	//TODO: Should the sprite offset be projected??
	math::Vec2 pos = project_pos(camera, entity->pos) + math::rec_pos(bounds);
	math::Vec2 dim = math::rec_dim(bounds) * entity->scale;

	return math::rec2_pos_dim(pos, dim);
}

math::Rec2 get_entity_collider_bounds(Entity * entity) {
	return math::rec_scale(math::rec_offset(entity->collider, entity->pos.xy), entity->scale);
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

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos0.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv0.y;
	v[batch->e++] = color.r; v[batch->e++] = color.b; v[batch->e++] = color.g; v[batch->e++] = color.a;	

	v[batch->e++] =  pos1.x; v[batch->e++] =  pos1.y; v[batch->e++] =   uv1.x; v[batch->e++] =   uv1.y;
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

void render_v_buf(gl::VertexBuffer * v_buf, RenderMode render_mode, Shader * shader, math::Mat4 * transform, gl::Texture * tex0, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	glUseProgram(shader->id);

	glUniformMatrix4fv(shader->transform, 1, GL_FALSE, transform->v);
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

void render_and_clear_render_batch(RenderBatch * batch, Shader * shader, math::Mat4 * transform) {
	DEBUG_TIME_BLOCK();

	if(batch->e > 0) {
		for(u32 i = batch->e; i < batch->v_len; i++) {
			batch->v_arr[i] = 0.0f;
		}

		glBindBuffer(GL_ARRAY_BUFFER, batch->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch->v_buf.size_in_bytes, batch->v_arr);

		render_v_buf(&batch->v_buf, batch->mode, shader, transform, batch->tex);

		// zero_memory((u8 *)batch->v_arr, batch->v_len * sizeof(f32));
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

	// font->scale = 2.0f;
	font->scale = 4.0f;
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

void push_player_clone(Player * player) {
	if(player->clone_count < ARRAY_COUNT(player->clones)) {
		player->clone_count++;
	}
}

void pop_player_clones(Player * player, u32 count) {
	while(count && player->clone_count > 0) {
		u32 clone_index = --player->clone_count;
		Entity * entity = player->clones[clone_index];
		entity->pos = ENTITY_NULL_POS;
		count--;
	}
}

void begin_rocket_sequence(GameState * game_state) {
	RocketSequence * seq = &game_state->rocket_seq;

	if(!seq->playing) {
		seq->playing = true;
		seq->time_ = 0.0f;

		math::Rec2 render_bounds = get_entity_render_bounds(&game_state->assets, seq->rocket, &game_state->camera);
		f32 height = math::rec_dim(render_bounds).y;

		Entity * rocket = seq->rocket;
		rocket->pos = game_state->player.e->pos;
		rocket->pos.y -= (game_state->ideal_window_height * 0.5f + height * 0.5f);
		rocket->d_pos = math::vec2(0.0f);
		rocket->speed = math::vec2(0.0f, 8000.0f);
		rocket->damp = 4.0f;
	}
}

void play_rocket_sequence(GameState * game_state, f32 dt) {
	RocketSequence * seq = &game_state->rocket_seq;

	if(game_state->rocket_seq.playing) {
		Entity * rocket = seq->rocket;
		Player * player = &game_state->player;

		rocket->d_pos += rocket->speed * dt;
		rocket->d_pos *= (1.0f - rocket->damp * dt);

		math::Vec2 d_pos = rocket->d_pos * dt;
		math::Vec2 new_pos = rocket->pos.xy + d_pos;

		if(rocket->pos.y < game_state->location_y_offset) {
			if(new_pos.y < game_state->location_y_offset) {
				math::Rec2 player_bounds = get_entity_collider_bounds(player->e);
				math::Rec2 collider_bounds = get_entity_collider_bounds(rocket);
				if(math::rec_overlap(collider_bounds, player_bounds)) {
					player->e->d_pos = math::vec2(0.0f);
					player->e->pos.xy += d_pos;
					player->allow_input = false;
				}

				game_state->camera.offset = (math::rand_vec2() * 2.0f - 1.0f) * 10.0f;
			}
			else {
				player->e->pos.y = game_state->location_y_offset;
				player->allow_input = true;

				game_state->current_location = LocationId_space;
				game_state->entity_emitter.pos.y = game_state->location_y_offset;
				game_state->entity_emitter.time_until_next_spawn = 0.0f;
			}
		}

		rocket->pos.xy = new_pos;
		seq->time_ += dt;

		f32 seq_max_time = 20.0f;
		if(seq->time_ > seq_max_time) {
			f32 new_pixelate_time = game_state->pixelate_time + dt * 1.5f;
			if(game_state->pixelate_time < 1.0f && new_pixelate_time >= 1.0f) {
				rocket->pos = ENTITY_NULL_POS;

				game_state->d_time = 1.0f;
				player->e->d_pos = math::vec2(0.0f);
				player->e->pos.xy = math::vec2(player->e->initial_x, 0.0f);
				player->allow_input = true;

				game_state->current_location = LocationId_city;
				game_state->entity_emitter.pos.y = 0.0f;
				game_state->entity_emitter.time_until_next_spawn = 0.0f;
			}

			game_state->pixelate_time = new_pixelate_time;
			if(game_state->pixelate_time >= 2.0f) {
				game_state->pixelate_time = 0.0f;

				seq->playing = false;
				seq->time_ = 0.0f;
			}			
		}
	}
}

b32 move_entity(GameState * game_state, Entity * entity, f32 dt, b32 loop = false) {
	b32 off_screen = false;

	//TODO: Move by player speed!!
	entity->pos.x -= 500.0f * game_state->d_time * dt;

	math::Rec2 bounds = get_entity_render_bounds(&game_state->assets, entity, &game_state->camera);
	math::Vec2 screen_pos = math::rec_pos(bounds);
	f32 width = math::rec_dim(bounds).x;

	if(screen_pos.x < -(game_state->ideal_window_width * 0.5f + width * 0.5f)) {
		off_screen = true;

		//TODO: Loop arbitary sized entities??
		if(loop) {
			screen_pos.x += width * 2.0f;
			entity->pos = unproject_pos(&game_state->camera, screen_pos, entity->pos.z);
		}
	}

	return off_screen;
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;
	AssetState * assets = &game_state->assets;

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
		basic_shader->transform = glGetUniformLocation(basic_shader->id, "transform");
		basic_shader->color = glGetUniformLocation(basic_shader->id, "color");
		basic_shader->tex0 = glGetUniformLocation(basic_shader->id, "tex0");

		Shader * post_shader = &game_state->post_shader;
		u32 post_vert = gl::compile_shader_from_source(SCREEN_QUAD_VERT_SRC, GL_VERTEX_SHADER);
		u32 post_frag = gl::compile_shader_from_source(POST_FILTER_FRAG_SRC, GL_FRAGMENT_SHADER);
		post_shader->id = gl::link_shader_program(post_vert, post_frag);
		post_shader->i_position = glGetAttribLocation(post_shader->id, "i_position");
		post_shader->tex0 = glGetUniformLocation(post_shader->id, "tex0");
		post_shader->tex0_dim = glGetUniformLocation(post_shader->id, "tex0_dim");
		post_shader->pixelate_scale = glGetUniformLocation(post_shader->id, "pixelate_scale");
		post_shader->fade_amount = glGetUniformLocation(post_shader->id, "fade_amount");

		game_state->frame_buffer = gl::create_frame_buffer(game_input->back_buffer_width, game_input->back_buffer_height, false);

		f32 screen_quad_verts[] = {
			-1.0f,-1.0f,
			 1.0f, 1.0f,
			-1.0f, 1.0f,
			-1.0f,-1.0f,
			 1.0f,-1.0f,
			 1.0f, 1.0f,
		};

		game_state->screen_quad_v_buf = gl::create_vertex_buffer(screen_quad_verts, ARRAY_COUNT(screen_quad_verts), 2, GL_STATIC_DRAW);

		f32 quad_verts[] = {
			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};

		game_state->entity_v_buf = gl::create_vertex_buffer(quad_verts, ARRAY_COUNT(quad_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

		f32 bg_verts[] = {
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

		game_state->bg_v_buf = gl::create_vertex_buffer(bg_verts, ARRAY_COUNT(bg_verts), VERT_ELEM_COUNT, GL_STATIC_DRAW);

		gl::Texture * font_tex = get_texture_asset(assets, AssetId_font, 0);
		game_state->font = allocate_font(&game_state->memory_pool, font_tex, 65536, game_input->back_buffer_width, game_input->back_buffer_height);
		game_state->debug_str = allocate_str(&game_state->memory_pool, 1024);
		game_state->title_str = allocate_str(&game_state->memory_pool, 128);

		//TODO: Adjust these to actual aspect ratio?
		game_state->ideal_window_width = 1280;
		game_state->ideal_window_height = 720;

		game_state->debug_batch = allocate_render_batch(&game_state->memory_pool, get_texture_asset(assets, AssetId_white, 0), 65536, RenderMode_lines);
		game_state->debug_render_entity_bounds = false;

		game_state->sprite_batch_count = get_asset_count(assets, AssetId_atlas);
		game_state->sprite_batches = PUSH_ARRAY(&game_state->memory_pool, RenderBatch *, game_state->sprite_batch_count);
		for(u32 i = 0; i < game_state->sprite_batch_count; i++) {
			u32 batch_size = QUAD_ELEM_COUNT * 128;
			game_state->sprite_batches[i] = allocate_render_batch(&game_state->memory_pool, get_texture_asset(assets, AssetId_atlas, i), batch_size);
		}

		ENTITY_NULL_POS = math::vec3(F32_MAX, F32_MAX, 0.0f);
		game_state->entity_gravity = math::vec2(0.0f, -8000.0f);

		game_state->current_location = LocationId_city;

		f32 location_z_offsets[PARALLAX_LAYER_COUNT] = {
			 10.0f,
			 10.0f,
			  7.5f,
			  5.0f,
			 -0.5f,
		};

		f32 location_max_z = location_z_offsets[0];
		game_state->location_y_offset = (f32)game_state->ideal_window_height / (1.0f / (location_max_z + 1.0f));
		game_state->ground_height = -(f32)game_state->ideal_window_height * 0.5f;

		AssetId location_layer_ids[LocationId_count];
		location_layer_ids[LocationId_city] = AssetId_city;
		location_layer_ids[LocationId_space] = AssetId_space;
		for(u32 i = 0; i < LocationId_count; i++) {
			Location * location = game_state->locations + i;
			AssetId asset_id = location_layer_ids[i];

			location->min_y = game_state->location_y_offset * i;
			location->max_y = location->min_y + game_state->location_y_offset * 0.5f;

			for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers) - 1; layer_index++) {
				Entity * entity = push_entity(game_state, asset_id, layer_index);

				entity->pos.y = game_state->location_y_offset * i;
				entity->pos.z = location_z_offsets[layer_index];
				entity->v_buf = &game_state->bg_v_buf;

				location->layers[layer_index] = entity;
			}
		}

		//TODO: Collapse this!!
		{
			//TODO: Better probability generation!!
			AssetId emitter_types[] = {
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,
				AssetId_smiley,

				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,
				AssetId_telly,

				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,
				AssetId_dolly,

				AssetId_rocket,
			};

			game_state->locations[LocationId_city].emitter_type_count = ARRAY_COUNT(emitter_types);
			game_state->locations[LocationId_city].emitter_types = PUSH_COPY_ARRAY(&game_state->memory_pool, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
		}

		{
			AssetId emitter_types[] = {
				AssetId_smiley,
				AssetId_dolly,
				AssetId_dolly,
			};

			game_state->locations[LocationId_space].emitter_type_count = ARRAY_COUNT(emitter_types);
			game_state->locations[LocationId_space].emitter_types = PUSH_COPY_ARRAY(&game_state->memory_pool, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
		}

		Player * player = &game_state->player;

		player->clone_index = 0;
		player->clone_count = 0;
		player->clone_offset = math::vec2(-5.0f, 0.0f);
		for(u32 i = 0; i < ARRAY_COUNT(player->clones); i++) {
			u32 asset_index = (i % (get_asset_count(&game_state->assets, AssetId_dolly) - 1)) + 1;
			player->clones[i] = push_entity(game_state, AssetId_dolly, asset_index, ENTITY_NULL_POS);
		}

		player->e = push_entity(game_state, AssetId_dolly, 0, math::vec3((f32)game_state->ideal_window_width * -0.25f, 0.0f, 0.0f));
		player->e->speed = math::vec2(50.0f, 6000.0f);
		player->e->damp = 9.0f;
		player->e->initial_x = player->e->pos.x;
		player->grounded = false;
		player->running = false;
		player->allow_input = true;

		EntityEmitter * emitter = &game_state->entity_emitter;
		Sprite * emitter_sprite = get_sprite_asset(assets, AssetId_teacup, 0);
		emitter->pos = math::vec3(game_state->ideal_window_width * 0.75f, 0.0f, 0.0f);
		emitter->time_until_next_spawn = 0.0f;
		emitter->entity_count = 0;
		for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
			emitter->entity_array[i] = push_entity(game_state, AssetId_teacup, 0, emitter->pos);
		}

		RocketSequence * seq = &game_state->rocket_seq;
		seq->rocket = push_entity(game_state, AssetId_large_rocket, 0, ENTITY_NULL_POS);
		seq->rocket->collider = math::rec_offset(seq->rocket->collider, math::vec2(0.0f, -math::rec_dim(seq->rocket->collider).y * 0.5f));
		seq->playing = false;
		seq->time_ = 0.0f;

		//TODO: Adding foreground layer at the end until we have sorting!!
		for(u32 i = 0; i < LocationId_count; i++) {
			Location * location = game_state->locations + i;
			AssetId asset_id = location_layer_ids[i];

			u32 layer_index = ARRAY_COUNT(location->layers) - 1;

			Entity * entity = push_entity(game_state, asset_id, layer_index);
			entity->pos.y = game_state->location_y_offset * i;
			entity->pos.z = location_z_offsets[layer_index];
			entity->v_buf = &game_state->bg_v_buf;

			location->layers[layer_index] = entity;
		}

#if 0
		game_state->clouds = push_entity(game_state, AssetId_clouds, 0);
		game_state->clouds->pos = math::vec3(0.0f, game_state->location_y_offset * 0.5f, -0.5f);
		game_state->clouds->v_buf = &game_state->bg_v_buf;
#endif

		game_state->camera.pos = math::vec2(0.0f, game_state->player.e->pos.y);
		game_state->camera.letterboxed_height = (f32)game_state->ideal_window_height;
		game_state->pixelate_time = 0.0f;

		game_state->d_time = 1.0f;
		game_state->pitch = 1.0f;
		game_state->score = 0;
		game_state->distance = 0.0f;

		glEnable(GL_CULL_FACE);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	Player * player = &game_state->player;

	if(game_input->buttons[ButtonId_debug] & KEY_PRESSED) {
		game_state->debug_render_entity_bounds = !game_state->debug_render_entity_bounds;
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		// game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;
		begin_rocket_sequence(game_state);
	}

	f32 dd_time = 0.0f;
	if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
		dd_time -= 4.0f * game_input->delta_time;
	}

	if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
		dd_time += 4.0f * game_input->delta_time;
	}

#if 0
	game_state->d_time = 0.5f + (player->clone_count * 0.1f);
#else
	game_state->d_time += dd_time;
	//TODO: Disable this to go backwards in time!!
	game_state->d_time = math::max(game_state->d_time, 0.0f);
#endif

	if(game_state->d_time < 1.0f) {
		game_state->pitch = game_state->d_time;
	}
	else {
		game_state->pitch = 1.0f + (game_state->d_time - 1.0f) * 0.02f;
	}

	change_pitch(game_state->music, game_state->pitch);

#if 1
	f32 adjusted_dt = game_state->d_time * game_input->delta_time;
#else
	f32 adjusted_dt = game_input->delta_time;
#endif

	game_state->distance += adjusted_dt;

	math::Vec2 half_window_dim = math::vec2(game_state->ideal_window_width, game_state->ideal_window_height) * 0.5f;

	for(i32 i = player->clone_count - 1; i >= 0; i--) {
		Entity * entity = player->clones[i];

		if(i == 0) {
			entity->chain_pos = player->e->pos.xy + player->clone_offset * 4.0f;
		}
		else {
			Entity * next_entity = player->clones[i - 1];
			entity->chain_pos = next_entity->chain_pos + player->clone_offset;
		}

		entity->pos.xy = entity->chain_pos;
		entity->scale = 0.75f;

		// f32 amplitude = math::max(50.0f / math::max(game_state->d_time, 1.0f), 0.0f);
		f32 amplitude = 50.0f;
		entity->pos.y += math::sin((game_input->total_time * 0.25f + i * (3.0f / 13.0f)) * math::TAU) * amplitude;
	}

	math::Vec2 player_dd_pos = math::vec2(0.0f);

#if 0
	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		if(player->running) {
			player->running = false;
			player->grounded = false;

			player->e->damp = 9.0f;
			player->e->use_gravity = false;
			player->e->d_pos = math::vec2(0.0f);
		}
		else {
			player->running = true;
			player->grounded = false;

			player->e->damp = 0.0f;
			player->e->use_gravity = true;
			player->e->d_pos = math::vec2(0.0f);
		}
	}
#endif

	if(player->allow_input) {
		if(!player->running) {
			if(game_input->buttons[ButtonId_up] & KEY_DOWN) {
				player_dd_pos.y += player->e->speed.y;
			}

			if(game_input->buttons[ButtonId_down] & KEY_DOWN) {
				player_dd_pos.y -= player->e->speed.y;
			}

			if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
				player_dd_pos.x -= player->e->speed.x;
			}

			if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
				player_dd_pos.x += player->e->speed.x;
			}			
		}
		else {
			if(player->grounded) {
				if(game_input->buttons[ButtonId_up] & KEY_PRESSED) {
					player_dd_pos.y += player->e->speed.y * 15.0f;
				}
			}
		}
	}

	if(player->e->use_gravity) {
		player_dd_pos += game_state->entity_gravity;
	}

	player->e->d_pos += player_dd_pos * game_input->delta_time;
	player->e->d_pos *= (1.0f - player->e->damp * game_input->delta_time);

	player->e->pos.xy += player->e->d_pos * game_input->delta_time;

	if(player->running) {
		f32 ground_height = game_state->ground_height + math::rec_dim(get_entity_collider_bounds(player->e)).y * 0.5f;
		if(player->e->pos.y <= ground_height) {
			player->e->pos.y = ground_height;
			player->e->d_pos.y = 0.0f;
			player->grounded = true;
		}
		else {
			player->grounded = false;
		}
	}

	f32 max_dist_x = 20.0f;
	player->e->pos.x = math::clamp(player->e->pos.x, player->e->initial_x - max_dist_x, player->e->initial_x + max_dist_x);

	Camera * camera = &game_state->camera;
	camera->offset = (math::rand_vec2() * 2.0f - 1.0f) * math::max(game_state->d_time - 2.0f, 0.0f);

	if(game_state->rocket_seq.playing) {
		play_rocket_sequence(game_state, game_input->delta_time);
	}

	Location * current_location = game_state->locations + game_state->current_location;

#if 1
	camera->letterboxed_height = game_state->ideal_window_height - math::max(game_state->d_time - 1.0f, 0.0f) * 20.0f;
	camera->letterboxed_height = math::max(camera->letterboxed_height, game_state->ideal_window_height / 3.0f);
#endif
	camera->pos.x = 0.0f;
	camera->pos.y = math::max(player->e->pos.y, 0.0f);
	if(player->allow_input) {
		camera->pos.y = math::clamp(camera->pos.y, current_location->min_y, current_location->max_y);
	}

	math::Rec2 player_bounds = get_entity_collider_bounds(player->e);

	EntityEmitter * emitter = &game_state->entity_emitter;
	emitter->time_until_next_spawn -= adjusted_dt;
	if(emitter->time_until_next_spawn <= 0.0f) {
		if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
			Entity * entity = emitter->entity_array[emitter->entity_count++];

			entity->pos = emitter->pos;
			entity->pos.y += (math::rand_f32() * 2.0f - 1.0f) * 250.0f;;
			entity->scale = 1.0f;
			entity->color = math::vec4(1.0f);

			u32 type_index = (u32)(math::rand_f32() * current_location->emitter_type_count);
			ASSERT(type_index < current_location->emitter_type_count);
			AssetId asset_id = current_location->emitter_types[type_index];

			Asset * asset = get_asset(assets, asset_id, 0);
			ASSERT(asset);
			ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

			entity->asset_type = asset->type;
			entity->asset_id = asset_id;
			entity->asset_index = 0;

			entity->collider = get_asset_bounds(assets, entity->asset_id, entity->asset_index);
			if(entity->asset_id == AssetId_telly) {
				entity->collider = math::rec_scale(entity->collider, 0.5f);
			}

			entity->hit = false;
		}

		emitter->time_until_next_spawn = 0.5f;
	}

	for(u32 i = 0; i < emitter->entity_count; i++) {
		Entity * entity = emitter->entity_array[i];
		b32 destroy = false;

		if(!entity->hit) {
			if(move_entity(game_state, entity, game_input->delta_time)) {
				destroy = true;
			}
		}
		else {
			f32 dd_pos = 100.0f * adjusted_dt;
			entity->pos.y += dd_pos;

			entity->color.a -= 2.0f * adjusted_dt;
			if(entity->color.a < 0.0f) {
				entity->color.a = 0.0f;
				destroy = true;
			}
		}

		entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
		entity->asset_index = (u32)entity->anim_time % get_asset_count(assets, entity->asset_id);

		math::Rec2 bounds = get_entity_collider_bounds(entity);
		//TODO: When should this check happen??
		if(rec_overlap(player_bounds, bounds) && !entity->hit) {
			entity->hit = true;
			entity->scale = 2.0f;

			//TODO: Should there be a helper function for this??
			AssetId clip_id = AssetId_pickup;
			AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
			AudioSource * source = play_audio_clip(&game_state->audio_state, clip);
			change_pitch(source, math::lerp(0.9f, 1.1f, math::rand_f32()));

			switch(entity->asset_id) {
				case AssetId_dolly: {
					push_player_clone(player);
					break;
				}

				case AssetId_telly: {
					pop_player_clones(player, 5);
					break;
				}

				case AssetId_rocket: {
					begin_rocket_sequence(game_state);
					break;
				}

				default: {
					game_state->score++;
					break;
				}
			}
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

	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = game_state->locations + i;
		for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers); layer_index++) {
			move_entity(game_state, location->layers[layer_index], game_input->delta_time, true);
		}
	}

	if(game_state->clouds) {
		move_entity(game_state, game_state->clouds, game_input->delta_time, true);
	}

	math::Mat4 projection_matrix = math::orthographic_projection((f32)game_state->ideal_window_width, (f32)game_state->ideal_window_height);

	glBindFramebuffer(GL_FRAMEBUFFER, game_state->frame_buffer.id);
	glViewport(0, 0, game_state->frame_buffer.width, game_state->frame_buffer.height);
	glClear(GL_COLOR_BUFFER_BIT);

	Shader * basic_shader = &game_state->basic_shader;
	Shader * post_shader = &game_state->post_shader;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	{
		DEBUG_TIME_BLOCK();

		math::Rec2 camera_bounds = math::rec2_pos_dim(math::vec2(0.0f), math::vec2((f32)game_state->ideal_window_width, camera->letterboxed_height));

		u32 null_batch_index = U32_MAX;
		u32 current_batch_index = null_batch_index;

		//TODO: Sorting!!
		for(u32 i = 0; i < game_state->entity_count; i++) {
			Entity * entity = game_state->entity_array + i;

			math::Rec2 bounds = math::rec_offset(get_entity_render_bounds(assets, entity, camera), camera->offset);
			math::Vec2 pos = math::rec_pos(bounds);
			math::Vec2 dim = math::rec_dim(bounds);

			math::Vec4 color = entity->color;
			color.rgb *= color.a;
			
			if(entity->asset_type == AssetType_texture) {
				gl::Texture * tex = get_texture_asset(assets, entity->asset_id, entity->asset_index);

#if 1
				//TODO: Remove epsilon!!
				f32 epsilon = 0.1f;
				if(math::rec_overlap(camera_bounds, math::rec_scale(bounds, 1.0f + epsilon))) {
					if(current_batch_index != null_batch_index) {
						render_and_clear_render_batch(game_state->sprite_batches[current_batch_index], basic_shader, &projection_matrix);
						current_batch_index = null_batch_index;
					}

					math::Mat4 transform = projection_matrix * math::translate(pos.x, pos.y, 0.0f) * math::scale(dim.x, dim.y, 1.0f);
					render_v_buf(entity->v_buf, RenderMode_triangles, basic_shader, &transform, tex, color);					
				}
#endif
			}
			else {
				Sprite * sprite = get_sprite_asset(assets, entity->asset_id, entity->asset_index);
				ASSERT(sprite->atlas_index < game_state->sprite_batch_count);

				if(math::rec_overlap(camera_bounds, bounds)) {
					if(current_batch_index != sprite->atlas_index && current_batch_index != null_batch_index) {
						render_and_clear_render_batch(game_state->sprite_batches[current_batch_index], basic_shader, &projection_matrix);
					}

					current_batch_index = sprite->atlas_index;

					RenderBatch * batch = game_state->sprite_batches[sprite->atlas_index];
					render_sprite(batch, sprite, pos, dim, color);					
				}
			}
		}
		
		for(u32 i = 0; i < game_state->sprite_batch_count; i++) {
			render_and_clear_render_batch(game_state->sprite_batches[i], basic_shader, &projection_matrix);
		}

		if(game_state->debug_render_entity_bounds) {
			for(u32 i = 0; i < game_state->entity_count; i++) {
				Entity * entity = game_state->entity_array + i;

				// math::Rec2 bounds = math::rec_offset(get_entity_render_bounds(assets, entity, camera), camera->offset);
				math::Rec2 bounds = math::rec_offset(math::rec_scale(entity->collider, entity->scale), project_pos(camera, entity->pos) + camera->offset);
				render_quad_lines(game_state->debug_batch, &bounds, math::vec4(1.0f));
			}

			render_and_clear_render_batch(game_state->debug_batch, basic_shader, &projection_matrix);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	{
		gl::VertexBuffer * v_buf = &game_state->screen_quad_v_buf;

		glUseProgram(post_shader->id);

		glUniform2f(post_shader->tex0_dim, (f32)game_state->frame_buffer.width, (f32)game_state->frame_buffer.height);

		f32 pixelate_time = game_state->pixelate_time;
		f32 pixelate_scale = math::frac(pixelate_time);
		if((u32)pixelate_time & 1) {
			pixelate_scale = 1.0f - pixelate_scale;
		}

		pixelate_scale = 1.0f / math::pow(2.0f, (pixelate_scale * 8.0f));
		glUniform1f(post_shader->pixelate_scale, pixelate_scale);

		f32 fade_amount = math::sin(game_input->total_time) * 0.5f + 0.5f;
		glUniform1f(post_shader->fade_amount, 0.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, game_state->frame_buffer.texture_id);
		glUniform1i(post_shader->tex0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, v_buf->id);

		u32 stride = v_buf->vert_size * sizeof(f32);

		glVertexAttribPointer(post_shader->i_position, 2, GL_FLOAT, 0, stride, 0);
		glEnableVertexAttribArray(post_shader->i_position);

		glDrawArrays(GL_TRIANGLES, 0, v_buf->vert_count);
	}

	{
		Texture * tex = get_texture_asset(assets, AssetId_white, 0);

		math::Vec2 dim = math::vec2(game_state->ideal_window_width, game_state->ideal_window_height);
		math::Mat4 scale = math::scale(dim.x, dim.y, 1.0f);

		f32 letterbox_pixels = (game_state->ideal_window_height - game_state->camera.letterboxed_height) * 0.5f;

		math::Mat4 transform0 = projection_matrix * math::translate(0.0f, dim.y - letterbox_pixels, 0.0f) * scale;
		render_v_buf(&game_state->entity_v_buf, RenderMode_triangles, basic_shader, &transform0, tex, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		math::Mat4 transform1 = projection_matrix * math::translate(0.0f,-dim.y + letterbox_pixels, 0.0f) * scale;
		render_v_buf(&game_state->entity_v_buf, RenderMode_triangles, basic_shader, &transform1, tex, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);

	{
		DEBUG_TIME_BLOCK();

		Font * font = game_state->font;
		font->pos = font->anchor;

		RenderBatch * batch = font->batch;

		str_clear(game_state->title_str);
		// str_print(game_state->title_str, "DOLLY DOLLY DOLLY DAYS!\nDT: %f | D_TIME: %f\n", game_input->delta_time, game_state->d_time);
		str_print(game_state->title_str, "SPEED: %f | SCORE: %u | CLONES: %u\n", game_state->d_time, game_state->score, player->clone_count);
		render_str(font, game_state->title_str);

#if 0
		render_str(font, game_state->debug_str);
#endif

		render_and_clear_render_batch(batch, basic_shader, &font->projection_matrix);

		glDisable(GL_BLEND);
	}

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

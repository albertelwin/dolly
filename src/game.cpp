
#include <game.hpp>

#include <audio.cpp>
#include <math.cpp>

gl::Texture load_texture_from_file(char const * file_name, i32 filter_mode = GL_LINEAR) {
	stbi_set_flip_vertically_on_load(true);

	i32 width, height, channels;
	u8 * img_data = stbi_load(file_name, &width, &height, &channels, 0);
	ASSERT(channels == TEXTURE_CHANNELS);

	gl::Texture tex = gl::create_texture(img_data, width, height, GL_RGBA, filter_mode, GL_CLAMP_TO_EDGE);
	ASSERT(tex.id);

	stbi_image_free(img_data);

	return tex;
}

Entity * push_entity(GameState * game_state, TextureId tex_id, math::Vec3 pos) {
	ASSERT(game_state->entity_count < ARRAY_COUNT(game_state->entity_array));

	gl::Texture * tex = &game_state->textures[tex_id];

	Entity * entity = game_state->entity_array + game_state->entity_count++;
	//TODO: Should this be here??
	entity->pos = pos;
	entity->scale = math::vec2(tex->width, tex->height);
	entity->rot = 0.0f;
	entity->color = math::vec4(1.0f);
	entity->tex_id = tex_id;
	entity->v_buf = &game_state->entity_v_buf;
	return entity;
}

void render_v_buf(gl::VertexBuffer * v_buf, Shader * shader, math::Mat4 * xform, gl::Texture * tex0, math::Vec4 color = math::vec4(1.0f)) {
	DEBUG_TIME_BLOCK();

	glUseProgram(shader->id);

	glUniformMatrix4fv(shader->xform, 1, GL_FALSE, xform->v);
	glUniform4f(shader->color, color.r, color.g, color.b, color.a);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0->id);
	glUniform1i(shader->tex0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, v_buf->id);

	u32 stride = v_buf->vert_size * sizeof(f32);

	glVertexAttribPointer(0, 3, GL_FLOAT, 0, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, 0, stride, (void *)(3 * sizeof(f32)));
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, v_buf->vert_count);
}

Font * allocate_font(MemoryPool * pool, u32 v_len, u32 back_buffer_width, u32 back_buffer_height) {
	Font * font = PUSH_STRUCT(pool, Font);

	font->tex = load_texture_from_file("font_transparent.png", GL_NEAREST);

	font->v_len = v_len;
	font->v_arr = PUSH_ARRAY(pool, f32, font->v_len);
	zero_memory((u8 *)font->v_arr, font->v_len * sizeof(f32));

	font->e = 0;

	font->v_buf = gl::create_vertex_buffer(font->v_arr, font->v_len, 5, GL_DYNAMIC_DRAW);

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

	f32 * v_arr = font->v_arr;

	f32 glyph_width = font->glyph_width * font->scale;
	f32 glyph_height = font->glyph_height * font->scale;
	f32 glyph_spacing = font->glyph_spacing * font->scale;

	u32 tex_size = font->tex.width;
	f32 r_tex_size = 1.0f / (f32)tex_size;

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

			u32 u = ((char_ - 24) % 12) * (font->glyph_width + font->glyph_spacing * 2) + font->glyph_spacing;
			u32 v = ((char_ - 24) / 12) * (font->glyph_height + font->glyph_spacing * 2) + font->glyph_spacing;

			math::Vec2 uv0 = math::vec2(u, tex_size - (v + font->glyph_height)) * r_tex_size;
			math::Vec2 uv1 = math::vec2(u + font->glyph_width, tex_size - v) * r_tex_size;

			v_arr[font->e++] = pos0.x; v_arr[font->e++] = pos0.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv0.x; v_arr[font->e++] = uv0.y;
			v_arr[font->e++] = pos1.x; v_arr[font->e++] = pos1.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv1.x; v_arr[font->e++] = uv1.y;
			v_arr[font->e++] = pos0.x; v_arr[font->e++] = pos1.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv0.x; v_arr[font->e++] = uv1.y;
			v_arr[font->e++] = pos0.x; v_arr[font->e++] = pos0.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv0.x; v_arr[font->e++] = uv0.y;
			v_arr[font->e++] = pos1.x; v_arr[font->e++] = pos1.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv1.x; v_arr[font->e++] = uv1.y;
			v_arr[font->e++] = pos1.x; v_arr[font->e++] = pos0.y; v_arr[font->e++] = 0.0f; v_arr[font->e++] = uv1.x; v_arr[font->e++] = uv0.y;
		}
	}
}

void render_c_str(Font * font, char const * c_str) {
	Str str;
	str.max_len = str.len = c_str_len(c_str);
	str.ptr = (char *)c_str;
	render_str(font, &str);
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_pool = create_memory_pool((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

		AudioState * audio_state = &game_state->audio_state;
		load_audio(audio_state, &game_state->memory_pool, game_input->audio_supported);
		audio_state->master_volume = 0.0f;

		game_state->music = play_audio_clip(audio_state, AudioClipId_music, true);

		Shader * basic_shader = &game_state->basic_shader;
		u32 basic_vert = gl::compile_shader_from_source(BASIC_VERT_SRC, GL_VERTEX_SHADER);
		u32 basic_frag = gl::compile_shader_from_source(BASIC_FRAG_SRC, GL_FRAGMENT_SHADER);
		basic_shader->id = gl::link_shader_program(basic_vert, basic_frag);
		basic_shader->xform = glGetUniformLocation(basic_shader->id, "xform");
		basic_shader->color = glGetUniformLocation(basic_shader->id, "color");
		basic_shader->tex0 = glGetUniformLocation(basic_shader->id, "tex0");

		f32 quad_verts[] = {
			-0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
		};

		game_state->entity_v_buf = gl::create_vertex_buffer(quad_verts, ARRAY_COUNT(quad_verts), 5, GL_STATIC_DRAW);

		f32 bg_verts[] = {
			-1.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			-1.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			-1.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 1.0f, 0.0f,

			-0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,

			 0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 1.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			 0.5f,-0.5f, 0.0f, 0.0f, 0.0f,
			 1.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			 1.5f,-0.5f, 0.0f, 1.0f, 0.0f,
		};

		game_state->bg_v_buf = gl::create_vertex_buffer(bg_verts, ARRAY_COUNT(bg_verts), 5, GL_STATIC_DRAW);

		game_state->font = allocate_font(&game_state->memory_pool, 65536, game_input->back_buffer_width, game_input->back_buffer_height);
		game_state->debug_str = allocate_str(&game_state->memory_pool, 1024);
		game_state->title_str = allocate_str(&game_state->memory_pool, 128);

		math::Vec3 camera_forward = math::VEC3_FORWARD;
		math::Vec3 camera_pos = camera_forward * 1.0f;
		game_state->view_matrix = math::look_at(camera_pos, camera_pos - camera_forward, math::VEC3_UP);

		//TODO: Adjust these to actual aspect ratio?
		game_state->ideal_window_width = 1280;
		game_state->ideal_window_height = 720;

		game_state->projection_matrix = math::orthographic_projection((f32)game_state->ideal_window_width, (f32)game_state->ideal_window_height);

		game_state->textures[TextureId_background] = load_texture_from_file("background.png", GL_LINEAR);
		game_state->textures[TextureId_dolly] = load_texture_from_file("dolly.png", GL_NEAREST);
		game_state->textures[TextureId_teacup] = load_texture_from_file("teacup.png", GL_NEAREST);

		game_state->background = push_entity(game_state, TextureId_background, math::vec3(0.0f));
		game_state->background->v_buf = &game_state->bg_v_buf;

		game_state->player = push_entity(game_state, TextureId_dolly, math::vec3((f32)game_state->ideal_window_width * -0.5f * (2.0f / 3.0f), 0.0f, 0.0f));

		TeacupEmitter * emitter = &game_state->teacup_emitter;
		gl::Texture * emitter_tex = &game_state->textures[TextureId_teacup];
		emitter->pos = math::vec3(game_state->ideal_window_width, 0.0f, 0.0f);
		emitter->time_until_next_spawn = 0.0f;
		emitter->scale = math::vec2(emitter_tex->width, emitter_tex->height);
		emitter->entity_count = 0;
		for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
			emitter->entity_array[i] = push_entity(game_state, TextureId_teacup, emitter->pos);
		}

		{
			SpriteBatch * batch = &game_state->sprite_batch;

			batch->tex = load_texture_from_file("sprite_sheet.png", GL_NEAREST);

			batch->v_len = VERTS_PER_QUAD * 256;
			batch->v_arr = PUSH_ARRAY(&game_state->memory_pool, f32, batch->v_len);
			batch->v_buf = gl::create_vertex_buffer(batch->v_arr, batch->v_len, 5, GL_DYNAMIC_DRAW);
			batch->e = 0;

			batch->tex_size = batch->tex.width;
			batch->sprite_width = 64;
			batch->sprite_height = 64;

			for(u32 i = 0; i < ARRAY_COUNT(game_state->sprites); i++) {
				Sprite * sprite = game_state->sprites + i;

				sprite->pos = math::vec3(0.0f);
				sprite->frame = 0;
				sprite->frame_time = 10000.0f;
			}

			game_state->sprite = &game_state->sprites[0];
		}

		game_state->d_time = 1.0f;
		game_state->score = 0;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	Entity * player = game_state->player;
	//TODO: World units!!
	f32 player_accel = 500.0f * game_input->delta_time;

	if(game_input->buttons[ButtonId_up] & KEY_DOWN) {
		player->pos.y += player_accel;
	}

	if(game_input->buttons[ButtonId_down] & KEY_DOWN) {
		player->pos.y -= player_accel;
	}

	game_state->sprite->pos = player->pos;

	f32 dd_time = 0.5f * game_input->delta_time;
	if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
		game_state->d_time -= dd_time;
		if(game_state->d_time < 0.0f) {
			game_state->d_time = 0.0f;
		}
	}

	if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
		game_state->d_time += dd_time;
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;
	}

	f32 adjusted_dt = game_state->d_time * game_input->delta_time;

	f32 half_screen_width = (f32)game_state->ideal_window_width * 0.5f;
	f32 half_screen_height = (f32)game_state->ideal_window_height * 0.5f;

	TeacupEmitter * emitter = &game_state->teacup_emitter;
	emitter->time_until_next_spawn -= adjusted_dt;
	if(emitter->time_until_next_spawn <= 0.0f) {
		if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
			Entity * teacup = emitter->entity_array[emitter->entity_count++];
			
			f32 y = (math::rand_f32() * 2.0f - 1.0f) * 300.0f;

			teacup->pos = math::vec3(emitter->pos.x, y, 0.0f);
			teacup->scale = emitter->scale;
			teacup->color = math::vec4(1.0f);
			teacup->hit = false;
		}

		emitter->time_until_next_spawn = 1.0f;
	}

	math::Rec2 player_rec = math::rec2_centre_size(player->pos.xy, player->scale);

	for(u32 i = 0; i < emitter->entity_count; i++) {
		Entity * teacup = emitter->entity_array[i];
		b32 destroy = false;

		if(!teacup->hit) {
			f32 dd_pos = -500.0f * adjusted_dt;
			teacup->pos.x += dd_pos;
			if(teacup->pos.x < -half_screen_width) {
				destroy = true;
			}
		}
		else {
			f32 dd_pos = 500.0f * adjusted_dt;
			teacup->pos.y += dd_pos;

			teacup->scale -= emitter->scale * 4.0f * adjusted_dt;

			if(teacup->pos.y > half_screen_height || teacup->scale.x < 0.0f || teacup->scale.y < 0.0f) {
				destroy = true;
			}
		}

		//TODO: When should this check happen??
		math::Rec2 rec = math::rec2_centre_size(teacup->pos.xy, teacup->scale);
		if(rec_overlap(player_rec, rec) && !teacup->hit) {
			teacup->hit = true;
			teacup->scale = emitter->scale * 2.0f;

			Sprite * sprite = &game_state->sprites[i + 1];
			sprite->pos = teacup->pos;
			sprite->frame = 0;
			sprite->frame_time = 0.0f;

			//TODO: Should there be a helper function for this??
			AudioClipId clip_id = AudioClipId_pickup;
			AudioClip * clip = get_audio_clip(&game_state->audio_state, clip_id, math::rand_i32() % get_audio_clip_count(&game_state->audio_state, clip_id));
			AudioSource * source = play_audio_clip(&game_state->audio_state, clip);
			change_pitch(source, math::lerp(0.9f, 1.1f, math::rand_f32()));

			game_state->score++;
		}

		if(destroy) {
			teacup->pos = emitter->pos;
			teacup->scale = emitter->scale;

			u32 swap_index = emitter->entity_count - 1;
			emitter->entity_array[i] = emitter->entity_array[swap_index];
			emitter->entity_array[swap_index] = teacup;
			emitter->entity_count--;
			i--;
		}
	}

	Entity * background = game_state->background;
	background->pos.x += -200.0f * adjusted_dt;
	if(background->pos.x < -(half_screen_width + background->scale.x * 0.5f)) {
		background->pos.x += background->scale.x * 2.0f;
	}

	{
		DEBUG_TIME_BLOCK();

		SpriteBatch * batch = &game_state->sprite_batch;

		zero_memory((u8 *)batch->v_arr, batch->v_len * sizeof(f32));
		batch->e = 0;

		u32 frames = 25;

		u32 tex_size = batch->tex_size;
		f32 r_tex_size = 1.0f / (f32)tex_size;

		u32 sprite_width = batch->sprite_width;
		u32 sprite_height = batch->sprite_height;

		u32 sprites_per_row = tex_size / sprite_width;

		for(u32 i = 0; i < ARRAY_COUNT(game_state->sprites); i++) {
			Sprite * sprite = game_state->sprites + i;

			sprite->frame_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
			sprite->frame = (u32)sprite->frame_time;

			if(sprite->frame <= frames) {
				math::Vec2 pos0 = sprite->pos.xy - math::vec2(sprite_width, sprite_height) * 0.5f;
				math::Vec2 pos1 = sprite->pos.xy + math::vec2(sprite_width, sprite_height) * 0.5f;
				f32 z = sprite->pos.z;
				
				u32 u = (sprite->frame % sprites_per_row) * sprite_width;
				u32 v = (sprite->frame / sprites_per_row) * sprite_height;

				math::Vec2 uv0 = math::vec2(u, tex_size - (v + sprite_height)) * r_tex_size;
				math::Vec2 uv1 = math::vec2(u + sprite_width, tex_size - v) * r_tex_size;

				f32 * v_arr = batch->v_arr;
				v_arr[batch->e++] = pos0.x; v_arr[batch->e++] = pos0.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv0.x; v_arr[batch->e++] = uv0.y;
				v_arr[batch->e++] = pos1.x; v_arr[batch->e++] = pos1.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv1.x; v_arr[batch->e++] = uv1.y;
				v_arr[batch->e++] = pos0.x; v_arr[batch->e++] = pos1.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv0.x; v_arr[batch->e++] = uv1.y;
				v_arr[batch->e++] = pos0.x; v_arr[batch->e++] = pos0.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv0.x; v_arr[batch->e++] = uv0.y;
				v_arr[batch->e++] = pos1.x; v_arr[batch->e++] = pos1.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv1.x; v_arr[batch->e++] = uv1.y;
				v_arr[batch->e++] = pos1.x; v_arr[batch->e++] = pos0.y; v_arr[batch->e++] = z; v_arr[batch->e++] = uv1.x; v_arr[batch->e++] = uv0.y;
			}
		}		
	}

	change_pitch(game_state->music, game_state->d_time);

	math::Mat4 view_projection_matrix = game_state->projection_matrix * game_state->view_matrix;
	
	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//TODO: Sorting!!
	glUseProgram(game_state->entity_program);
	for(u32 i = 0; i < game_state->entity_count; i++) {
		Entity * entity = game_state->entity_array + i;
		gl::Texture * tex = game_state->textures + entity->tex_id;

		math::Mat4 xform = view_projection_matrix * math::translate(entity->pos.x, entity->pos.y, entity->pos.z) * math::scale(entity->scale.x, entity->scale.y, 1.0f) * math::rotate_around_z(entity->rot);

		render_v_buf(entity->v_buf, &game_state->basic_shader, &xform, tex, entity->color);
	}

	{
		DEBUG_TIME_BLOCK();

		SpriteBatch * batch = &game_state->sprite_batch;

		glBindBuffer(GL_ARRAY_BUFFER, batch->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch->v_buf.size_in_bytes, batch->v_arr);

		render_v_buf(&batch->v_buf, &game_state->basic_shader, &view_projection_matrix, &batch->tex);
	}

	{
		DEBUG_TIME_BLOCK();

		Font * font = game_state->font;

		zero_memory((u8 *)font->v_arr, font->v_len * sizeof(f32));
		font->e = 0;

		font->pos = font->anchor;

		render_c_str(font, "HELLO WORLD!\n\n");

		str_clear(game_state->title_str);
		str_print(game_state->title_str, "DOLLY DOLLY DOLLY DAYS!\nDT: %f | D_TIME: %f | SCORE: %u\n\n", game_input->delta_time, game_state->d_time, game_state->score);
		render_str(font, game_state->title_str);

		render_str(font, game_state->debug_str);

		glBindBuffer(GL_ARRAY_BUFFER, font->v_buf.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, font->v_buf.size_in_bytes, font->v_arr);
		render_v_buf(&font->v_buf, &game_state->basic_shader, &font->projection_matrix, &font->tex);
	}
}

#if 0
static u32 global_sample_index = 0;

void game_sample(GameMemory * game_memory, i16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
	f32 tone_hz = 440.0f;

	for(u32 i = 0, sample_index = 0; i < samples_to_write; i++) {
		f32 sample_time = (f32)global_sample_index++ / (f32)samples_per_second;

		f32 sample_f32 = intrin::sin(sample_time * math::TAU * tone_hz);
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

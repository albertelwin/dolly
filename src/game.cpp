
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

void render_entity(GameState * game_state, Entity * entity, math::Mat4 const & view_projection_matrix) {
	glUseProgram(game_state->entity_program);

	math::Mat4 xform = view_projection_matrix * math::translate(entity->pos.x, entity->pos.y, 0.0f) * math::scale(entity->scale.x, entity->scale.y, 1.0f);

	u32 xform_id = glGetUniformLocation(game_state->entity_program, "xform");
	glUniformMatrix4fv(xform_id, 1, GL_FALSE, xform.v);

	glUniform1f(glGetUniformLocation(game_state->entity_program, "tex_offset"), entity->tex_offset);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, entity->tex.id);
	glUniform1i(glGetUniformLocation(game_state->entity_program, "tex0"), 0);

	glBindBuffer(GL_ARRAY_BUFFER, game_state->entity_vertex_buffer.id);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, game_state->entity_vertex_buffer.vert_count);
}

TextBuffer * allocate_text_buffer(MemoryPool * pool, u32 max_len, u32 vert_size) {
	TextBuffer * buf = PUSH_STRUCT(pool, TextBuffer);

	buf->str.max_len = max_len;
	buf->str.len = 0;
	buf->str.ptr = PUSH_ARRAY(pool, char, max_len);

	u32 verts_per_char = 6;
	buf->vertex_array_length = max_len * verts_per_char * vert_size;
	buf->vertex_array = PUSH_ARRAY(pool, f32, buf->vertex_array_length);
	zero_memory((u8 *)buf->vertex_array, buf->vertex_array_length * sizeof(f32));

	buf->vertex_buffer = gl::create_vertex_buffer(buf->vertex_array, buf->vertex_array_length, vert_size, GL_STATIC_DRAW);

	return buf;
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_pool = create_memory_pool((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

		AudioState * audio_state = &game_state->audio_state;
		load_audio(audio_state, &game_state->memory_pool, game_input->audio_supported);
		game_state->music = play_audio_clip(audio_state, AudioClipId_music, true);

		u32 basic_vert = gl::compile_shader_from_source(BASIC_VERT_SRC, GL_VERTEX_SHADER);
		u32 basic_frag = gl::compile_shader_from_source(BASIC_FRAG_SRC, GL_FRAGMENT_SHADER);
		game_state->entity_program = gl::link_shader_program(basic_vert, basic_frag);

		f32 quad_verts[] = {
			-0.5f,-0.5f, 0.0f,
			 0.5f, 0.5f, 0.0f,
			-0.5f, 0.5f, 0.0f,
			-0.5f,-0.5f, 0.0f,
			 0.5f, 0.5f, 0.0f,
			 0.5f,-0.5f, 0.0f,
		};

		game_state->entity_vertex_buffer = gl::create_vertex_buffer(quad_verts, ARRAY_COUNT(quad_verts), 3, GL_STATIC_DRAW);

		Font * font = &game_state->font;
		u32 font_vert = gl::compile_shader_from_source(FONT_VERT_SRC, GL_VERTEX_SHADER);
		u32 font_frag = gl::compile_shader_from_source(FONT_FRAG_SRC, GL_FRAGMENT_SHADER);
		font->program = gl::link_shader_program(font_vert, font_frag);
		font->tex = load_texture_from_file("font.png", GL_NEAREST);
		font->glyph_width = 3;
		font->glyph_height = 5;
		font->glyph_spacing = 1;

		game_state->text_buf = allocate_text_buffer(&game_state->memory_pool, 1024, 5);

		math::Vec3 camera_forward = math::VEC3_FORWARD;
		math::Vec3 camera_pos = camera_forward * 1.0f;
		game_state->view_matrix = math::look_at(camera_pos, camera_pos - camera_forward, math::VEC3_UP);

		f32 aspect_ratio = (f32)game_input->back_buffer_width / (f32)game_input->back_buffer_height;
		game_state->projection_matrix = math::orthographic_projection((f32)game_input->back_buffer_width, (f32)game_input->back_buffer_height);

		game_state->player = PUSH_STRUCT(&game_state->memory_pool, Entity);
		game_state->player->pos = math::vec2(-426.67f, 0.0f);
		game_state->player->tex = load_texture_from_file("dolly.png");
		game_state->player->scale = math::vec2(game_state->player->tex.width, game_state->player->tex.height);

		game_state->background = PUSH_STRUCT(&game_state->memory_pool, Entity);
		game_state->background->pos = math::vec2(0.0f);
		game_state->background->tex = load_texture_from_file("background.png");
		game_state->background->scale = math::vec2(game_state->background->tex.width, game_state->background->tex.height);
		game_state->background->scroll_velocity = 1.0f;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	Entity * player = game_state->player;
	f32 player_accel = 1000.0f * game_input->delta_time;

	if(game_input->buttons[ButtonId_up] & KEY_DOWN) {
		player->pos.y += player_accel;
	}

	if(game_input->buttons[ButtonId_down] & KEY_DOWN) {
		player->pos.y -= player_accel;
	}

	Entity * background = game_state->background;
	f32 scroll_accel = 0.5f * game_input->delta_time;

	if(game_input->buttons[ButtonId_left] & KEY_DOWN) {
		background->scroll_velocity -= scroll_accel;
		if(background->scroll_velocity < 0.0f) {
			background->scroll_velocity = 0.0f;
		}
	}

	if(game_input->buttons[ButtonId_right] & KEY_DOWN) {
		background->scroll_velocity += scroll_accel;
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;
	}

	background->tex_offset += background->scroll_velocity * game_input->delta_time;
	background->tex_offset = math::frac(background->tex_offset);

	change_pitch(game_state->music, background->scroll_velocity);

	{
		DEBUG_TIME_BLOCK();

		Font * font = &game_state->font;
		Str * str = &game_state->text_buf->str;

		f32 glyph_scale = 2.0f;
		f32 glyph_width = font->glyph_width * glyph_scale;
		f32 glyph_height = font->glyph_height * glyph_scale;
		f32 glyph_spacing = font->glyph_spacing * glyph_scale;

		u32 tex_size = font->tex.width;

		f32 * verts = game_state->text_buf->vertex_array;
		u32 vert_size = game_state->text_buf->vertex_buffer.vert_size;

		zero_memory((u8 *)verts, game_state->text_buf->vertex_array_length * sizeof(f32));

		f32 anchor_x = -(f32)game_input->back_buffer_width * 0.5f + glyph_spacing;
		f32 anchor_y = game_input->back_buffer_height * 0.5f - (glyph_height + glyph_spacing);

		math::Vec2 pos = math::vec2(anchor_x, anchor_y);

		for(u32 i = 0, e = 0; i < str->len; i++) {
			char char_ = str->ptr[i];
			if(char_ == '\n') {
				pos.x = anchor_x;
				pos.y -= (glyph_height + glyph_spacing);
			}
			else {
				math::Vec2 pos0 = pos;
				math::Vec2 pos1 = math::vec2(pos0.x + glyph_width, pos0.y + glyph_height);

				pos.x += (glyph_width + glyph_spacing);

				u32 u = ((char_ - 24) % 12) * (font->glyph_width + font->glyph_spacing * 2) + font->glyph_spacing;
				u32 v = ((char_ - 24) / 12) * (font->glyph_height + font->glyph_spacing * 2) + font->glyph_spacing;

				math::Vec2 uv0 = math::vec2(u, tex_size - (v + font->glyph_height)) / (f32)tex_size;
				math::Vec2 uv1 = math::vec2(u + font->glyph_width, tex_size - v) / (f32)tex_size;

				verts[e++] = pos0.x; verts[e++] = pos0.y; verts[e++] = 0.0f; verts[e++] = uv0.x; verts[e++] = uv0.y;
				verts[e++] = pos1.x; verts[e++] = pos1.y; verts[e++] = 0.0f; verts[e++] = uv1.x; verts[e++] = uv1.y;
				verts[e++] = pos0.x; verts[e++] = pos1.y; verts[e++] = 0.0f; verts[e++] = uv0.x; verts[e++] = uv1.y;
				verts[e++] = pos0.x; verts[e++] = pos0.y; verts[e++] = 0.0f; verts[e++] = uv0.x; verts[e++] = uv0.y;
				verts[e++] = pos1.x; verts[e++] = pos1.y; verts[e++] = 0.0f; verts[e++] = uv1.x; verts[e++] = uv1.y;
				verts[e++] = pos1.x; verts[e++] = pos0.y; verts[e++] = 0.0f; verts[e++] = uv1.x; verts[e++] = uv0.y;
			}
		}
	}

	math::Mat4 view_projection_matrix = game_state->projection_matrix * game_state->view_matrix;
	
	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(game_state->entity_program);

	render_entity(game_state, game_state->background, view_projection_matrix);
	render_entity(game_state, game_state->player, view_projection_matrix);

	{
		DEBUG_TIME_BLOCK();

		Font * font = &game_state->font;
		glUseProgram(font->program);

		math::Mat4 xform = view_projection_matrix;

		u32 xform_id = glGetUniformLocation(font->program, "xform");
		glUniformMatrix4fv(xform_id, 1, GL_FALSE, xform.v);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, font->tex.id);
		glUniform1i(glGetUniformLocation(font->program, "tex0"), 0);

		glBindBuffer(GL_ARRAY_BUFFER, game_state->text_buf->vertex_buffer.id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, game_state->text_buf->vertex_buffer.size_in_bytes, game_state->text_buf->vertex_array);

		u32 stride = game_state->text_buf->vertex_buffer.vert_size * sizeof(f32);

		glVertexAttribPointer(0, 3, GL_FLOAT, 0, stride, 0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, 0, stride, (void *)(3 * sizeof(f32)));
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLES, 0, game_state->text_buf->vertex_buffer.vert_count);		
	}

	glDisable(GL_BLEND);
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

		Str * str = &game_state->text_buf->str;

		str_clear(str);
		str_print(str, "DOLLY DOLLY DOLLY DOLLY DAYS!\nDT: %f | PITCH: %f\n\n", game_input->delta_time, game_state->background->scroll_velocity);

		for(u32 i = 0; i < ARRAY_COUNT(debug_block_profiles); i++) {
			DebugBlockProfile * profile = debug_block_profiles + i;

			str_print(str, "%s[%u]: time: %fms | hits: %u\n", profile->func, profile->id, profile->ms, profile->hits);

			profile->ms = 0.0;
			profile->hits = 0;
		}
	}
}

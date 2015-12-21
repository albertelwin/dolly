
#include <cmath>
#include <cstring>

#define PACK_RIFF_CODE(x, y, z, w) ((u32)(x) << 0) | ((u32)(y) << 8) | ((u32)(z) << 16) | ((u32)(w) << 24)
enum RiffCode {
	RiffCode_RIFF = PACK_RIFF_CODE('R', 'I', 'F', 'F'),
	RiffCode_WAVE = PACK_RIFF_CODE('W', 'A', 'V', 'E'),
	RiffCode_fmt = PACK_RIFF_CODE('f', 'm', 't', ' '),
	RiffCode_data = PACK_RIFF_CODE('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)
struct WavHeader {
	u32 riff_id;
	u32 size;
	u32 wave_id;
};

struct WavChunkHeader {
	u32 id;
	u32 size;
};

struct WavFormat {
	u16 tag;
	u16 channels;
	u32 samples_per_second;
	u32 avg_bytes_per_second;
	u16 block_align;
	u16 bits_per_sample;
	u16 extension_size;
	u16 valid_bits_per_sample;
	u32 channel_mask;
	u8 guid[16];
};

struct BmpHeader {
	u16 file_type;
	u32 file_size;
	u16 reserved0_;
	u16 reserved1_;
	u32 bitmap_offset;

	u32 size;
	i32 width;
	i32 height;
	u16 planes;
	u16 bits_per_pixel;
	u32 compression;
	u32 size_of_bitmap;
	i32 horz_res;
	i32 vert_res;
	u32 colors_used;
	u32 colors_important;
};
#pragma pack(pop)

u8 * read_file_to_memory(char const * file_name) {
	std::FILE * file_ptr = std::fopen(file_name, "rb");
	ASSERT(file_ptr != 0);

	std::fseek(file_ptr, 0, SEEK_END);
	long file_size = std::ftell(file_ptr);
	std::rewind(file_ptr);

	u8 * file_buffer = new u8[file_size];
	size_t read_result = std::fread(file_buffer, 1, file_size, file_ptr);
	ASSERT(read_result == file_size);

	std::fclose(file_ptr);

	return file_buffer;
}

void load_audio_clip_from_file(GameState * game_state, AudioClip * clip, char const * file_name) {
	u8 * file_buffer = read_file_to_memory(file_name);

	WavHeader * wav_header = (WavHeader *)file_buffer;
	ASSERT(wav_header->riff_id == RiffCode_RIFF);
	ASSERT(wav_header->wave_id == RiffCode_WAVE);

	WavChunkHeader * wav_format_header = (WavChunkHeader *)((u8 *)wav_header + sizeof(WavHeader));
	ASSERT(wav_format_header->id == RiffCode_fmt);

	WavFormat * wav_format = (WavFormat *)((u8 *)wav_format_header + sizeof(WavChunkHeader));
	ASSERT(wav_format->channels <= AUDIO_CHANNELS);
	ASSERT(wav_format->samples_per_second == AUDIO_CLIP_SAMPLES_PER_SECOND);
	ASSERT((wav_format->bits_per_sample / 8) == sizeof(i16));

	WavChunkHeader * wav_data_header = (WavChunkHeader *)((u8 *)wav_format_header + sizeof(WavChunkHeader) + wav_format_header->size);
	ASSERT(wav_data_header->id == RiffCode_data);

	i16 * wav_data = (i16 *)((u8 *)wav_data_header + sizeof(WavChunkHeader));

	clip->samples = wav_data_header->size / (wav_format->channels * sizeof(i16));
	u32 samples_with_padding = clip->samples + AUDIO_PADDING_SAMPLES;
	//TODO: Should all clips be forced stereo??
	clip->sample_data = PUSH_ARRAY(&game_state->memory_pool, i16, samples_with_padding * 2);
	if(wav_format->channels == 2) {
		for(u32 i = 0; i < samples_with_padding * 2; i++) {
			clip->sample_data[i] = wav_data[i % (clip->samples * 2)];
		}
	}
	else {
		for(u32 i = 0, sample_index = 0; i < samples_with_padding; i++) {
			i16 sample = wav_data[i % clip->samples];
			clip->sample_data[sample_index++] = sample;
			clip->sample_data[sample_index++] = sample;
		}
	}

	clip->length = (f32)clip->samples / (f32)wav_format->samples_per_second;

	delete[] file_buffer;
}

AudioVal64 audio_val64(f32 val_f32) {
	AudioVal64 val;
	val.int_part = (u32)val_f32;
	val.frc_part = val_f32 - val.int_part;
	return val;
}

AudioSource * play_audio_clip(GameState * game_state, AudioClipId clip_id, b32 loop = false) {
	AudioSource * source = 0;
	//TODO: If the clip isn't loaded don't play it??
	AudioClip * clip = game_state->audio_clips + clip_id;
	if(clip) {
		if(!game_state->audio_source_free_list) {
			game_state->audio_source_free_list = PUSH_STRUCT(&game_state->memory_pool, AudioSource);
			game_state->audio_source_free_list->next = 0;
		}

		source = game_state->audio_source_free_list;
		game_state->audio_source_free_list = source->next;

		source->next = game_state->audio_sources;
		game_state->audio_sources = source;

		source->clip_id = clip_id;
		source->sample_pos = audio_val64(0.0f);
		source->loop = loop;
		source->pitch = 1.0f;
		source->volume = math::vec2(1.0f);
		source->target_volume = source->volume;
		source->volume_delta = math::vec2(0.0f);
	}

	return source;
}

void change_volume(AudioSource * source, math::Vec2 volume, f32 time_) {
	if(source) {
		math::Vec2 clamped_volume = math::clamp01(volume);
		if(time_ <= 0.0f) {
			source->volume = clamped_volume;
			source->volume_delta = math::vec2(0.0f);
		}
		else {
			source->target_volume = clamped_volume;
			source->volume_delta = (source->target_volume - source->volume) * (1.0f / time_);
		}		
	}
}

void change_pitch(AudioSource * source, f32 pitch) {
	if(source) {
		//TODO: Pick appropriate min/max values!!
		source->pitch = math::clamp(pitch, 0.001f, 10.0f);		
	}
}

Texture load_texture_from_file(char const * file_name, i32 filter_mode = GL_LINEAR) {
	u8 * file_buffer = read_file_to_memory(file_name);

	BmpHeader * bmp_header = (BmpHeader *)file_buffer;
	ASSERT(bmp_header->file_type == 0x4D42);

	u8 * bmp_data = (u8 *)(file_buffer + bmp_header->bitmap_offset);
	for(u32 i = 0; i < bmp_header->width * bmp_header->height; i++) {
		u8 a = bmp_data[i * 4 + 0];
		u8 b = bmp_data[i * 4 + 1];
		u8 g = bmp_data[i * 4 + 2];
		u8 r = bmp_data[i * 4 + 3];

		bmp_data[i * 4 + 0] = r;
		bmp_data[i * 4 + 1] = g;
		bmp_data[i * 4 + 2] = b;
		bmp_data[i * 4 + 3] = a;
	}

	Texture tex = {};
	tex.width = bmp_header->width;
	tex.height = bmp_header->height;
	tex.id = gl::create_texture(bmp_data, tex.width, tex.height, GL_RGBA, filter_mode, GL_CLAMP_TO_EDGE);
	ASSERT(tex.id);

	delete[] file_buffer;

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

math::Vec4 char_to_tex_coord(char char_) {
	u32 x = (char_ - 24) % 12;
	u32 y = (char_ - 24) / 12;

	u32 tex_size = 64;

	u32 glyph_width = 5;
	u32 glyph_height = 7;
	u32 glyph_padding = 1;

	u32 u = x * glyph_width + glyph_padding;
	u32 v = y * glyph_height + glyph_padding;

	math::Vec4 tex_coord = math::vec4(0.0f);
	tex_coord.x = u / (f32)tex_size;
	tex_coord.y = (tex_size - (v + 5)) / (f32)tex_size;
	tex_coord.z = (u + 3) / (f32)tex_size;
	tex_coord.w = (tex_size - v) / (f32)tex_size;
	return tex_coord; 
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_pool = create_memory_pool((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

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

		u32 font_vert = gl::compile_shader_from_source(FONT_VERT_SRC, GL_VERTEX_SHADER);
		u32 font_frag = gl::compile_shader_from_source(FONT_FRAG_SRC, GL_FRAGMENT_SHADER);
		game_state->font_program = gl::link_shader_program(font_vert, font_frag);

		game_state->font_tex = load_texture_from_file("font.bmp", GL_NEAREST);

		{
			char const * str = "DOLLY DOLLY DOLLY DAYS! [21-12-2015]";
			u32 len = str_len(str);

			u32 verts_per_char = 6;
			u32 vert_size = 5;
			u32 vert_array_length = len * verts_per_char * vert_size;

			f32 * font_verts = PUSH_ARRAY(&game_state->memory_pool, f32, vert_array_length);

			f32 glyph_scale = 0.05f;
			f32 glyph_width = 0.3f * glyph_scale;
			f32 glyph_height = 0.5f * glyph_scale;
			f32 glyph_spacing = 0.1f * glyph_scale;

			f32 anchor_x = -0.626f;
			f32 anchor_y = -0.35f;

			for(u32 i = 0; i < len; i++) {
				f32 x0 = anchor_x + (glyph_width + glyph_spacing) * i;
				f32 y0 = anchor_y;

				f32 x1 = x0 + glyph_width;
				f32 y1 = y0 + glyph_height;

				math::Vec4 tex_coord = char_to_tex_coord(str[i]);

				u32 e = i * verts_per_char * vert_size;
				font_verts[e++] = x0; font_verts[e++] = y0; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.x; font_verts[e++] = tex_coord.y;
				font_verts[e++] = x1; font_verts[e++] = y1; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.z; font_verts[e++] = tex_coord.w;
				font_verts[e++] = x0; font_verts[e++] = y1; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.x; font_verts[e++] = tex_coord.w;
				font_verts[e++] = x0; font_verts[e++] = y0; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.x; font_verts[e++] = tex_coord.y;
				font_verts[e++] = x1; font_verts[e++] = y1; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.z; font_verts[e++] = tex_coord.w;
				font_verts[e++] = x1; font_verts[e++] = y0; font_verts[e++] = 0.0f; font_verts[e++] = tex_coord.z; font_verts[e++] = tex_coord.y;
			}

			game_state->font_vertex_buffer = gl::create_vertex_buffer(font_verts, vert_array_length, vert_size, GL_STATIC_DRAW);
		}

		math::Vec3 camera_forward = math::VEC3_FORWARD;
		math::Vec3 camera_pos = camera_forward * 0.62f;
		game_state->view_matrix = math::look_at(camera_pos, camera_pos - camera_forward, math::VEC3_UP);

		f32 aspect_ratio = (f32)game_input->back_buffer_width / (f32)game_input->back_buffer_height;
		game_state->projection_matrix = math::perspective_projection(aspect_ratio, 60.0f, 0.03f, F32_MAX);

		if(game_input->audio_supported) {
			//TODO: Temp until asset pack!!
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_sin_440], "sin_440.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_beep], "beep.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_woosh], "woosh.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_music], "music.wav");

			game_state->music = play_audio_clip(game_state, AudioClipId_music, true);
			change_volume(game_state->music, math::vec2(0.0f), 0.0f);
		}

		f32 tex_scale = 1.0f / 1000.0f;

		game_state->player = PUSH_STRUCT(&game_state->memory_pool, Entity);
		game_state->player->pos = math::vec2(-0.5f, 0.0f);
		game_state->player->tex = load_texture_from_file("dolly.bmp");
		game_state->player->scale = math::vec2(game_state->player->tex.width, game_state->player->tex.height) * tex_scale;

		game_state->background = PUSH_STRUCT(&game_state->memory_pool, Entity);
		game_state->background->pos = math::vec2(0.0f);
		game_state->background->tex = load_texture_from_file("background.bmp");
		game_state->background->scale = math::vec2(game_state->background->tex.width, game_state->background->tex.height) * tex_scale;
		game_state->background->scroll_velocity = 1.0f;


		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	Entity * player = game_state->player;
	f32 player_accel = 1.0f * game_input->delta_time;

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

	background->tex_offset += background->scroll_velocity * game_input->delta_time;
	change_pitch(game_state->music, background->scroll_velocity);

	math::Mat4 view_projection_matrix = game_state->projection_matrix * game_state->view_matrix;
	
	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(game_state->entity_program);

	render_entity(game_state, game_state->background, view_projection_matrix);
	render_entity(game_state, game_state->player, view_projection_matrix);

	{
		glUseProgram(game_state->font_program);

		math::Mat4 xform = view_projection_matrix;

		u32 xform_id = glGetUniformLocation(game_state->font_program, "xform");
		glUniformMatrix4fv(xform_id, 1, GL_FALSE, xform.v);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, game_state->font_tex.id);
		glUniform1i(glGetUniformLocation(game_state->font_program, "tex0"), 0);

		glBindBuffer(GL_ARRAY_BUFFER, game_state->font_vertex_buffer.id);

		u32 stride = game_state->font_vertex_buffer.vert_size * sizeof(f32);

		glVertexAttribPointer(0, 3, GL_FLOAT, 0, stride, 0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, 0, stride, (void *)(3 * sizeof(f32)));
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLES, 0, game_state->font_vertex_buffer.vert_count);		
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
	for(u32 i = 0, sample_index = 0; i < samples_to_write; i++) {
		sample_memory_ptr[sample_index++] = 0;
		sample_memory_ptr[sample_index++] = 0;
	}

	if(game_memory->initialized) {
		GameState * game_state = (GameState *)game_memory->ptr;

		f32 playback_rate = samples_per_second != AUDIO_CLIP_SAMPLES_PER_SECOND ? (f32)AUDIO_CLIP_SAMPLES_PER_SECOND / (f32)samples_per_second : 1.0f;
		f32 seconds_per_sample = 1.0f / samples_per_second;

		AudioSource ** source_ptr = &game_state->audio_sources;
		while(*source_ptr) {
			AudioSource * source = *source_ptr;
			AudioClip * clip = game_state->audio_clips + source->clip_id;

			f32 pitch = source->pitch * playback_rate;
			AudioVal64 pitch64 = audio_val64(pitch);

			u32 samples_left_to_write = samples_to_write;
			u32 samples_written = 0;

			while(samples_left_to_write) {
				u32 samples_to_play = samples_left_to_write;

				AudioVal64 samples_left_high = audio_val64((f32)(clip->samples - source->sample_pos.int_part) / pitch);
				AudioVal64 samples_left_low = audio_val64(source->sample_pos.frc_part / pitch);

				u32 samples_left = samples_left_high.int_part - samples_left_low.int_part;
				if((samples_left_high.frc_part - samples_left_low.frc_part) > 0.0f) {
					samples_left++;
				}

				if(samples_left < samples_to_play) {
					samples_to_play = samples_left;
				}

				ASSERT(samples_to_play);

				math::Vec2 volume_delta = source->volume_delta * seconds_per_sample;
				b32 volume_ended[AUDIO_CHANNELS] = {};
				for(int i = 0; i < AUDIO_CHANNELS; i++) {
					if(volume_delta.v[i]) {
						f32 volume_diff = source->target_volume.v[i] - source->volume.v[i];

						u32 volume_samples_left = (u32)(math::max(volume_diff / volume_delta.v[i], 0.0f) + 0.5f);
						if(volume_samples_left < samples_to_play) {
							samples_to_play = volume_samples_left;
							volume_ended[i] = true;
						}
					}
				}

				for(u32 i = 0; i < samples_to_play; i++) {
					for(u32 ii = 0; ii < AUDIO_CHANNELS; ii++) {
						u32 sample_index = source->sample_pos.int_part;
						ASSERT(sample_index < clip->samples);

						i16 sample_i16 = clip->sample_data[sample_index * 2 + ii];
						f32 sample_f32 = (f32)sample_i16 / (sample_i16 > 0 ? 32767.0f : 32768.0f);

						u32 next_sample_index = sample_index + 1;
						ASSERT(next_sample_index < (clip->samples + AUDIO_PADDING_SAMPLES));

						i16 next_sample_i16 = clip->sample_data[next_sample_index * 2 + ii];
						f32 next_sample_f32 = (f32)next_sample_i16 / (next_sample_i16 > 0 ? 32767.0f : 32768.0f);

						sample_f32 = math::lerp(sample_f32, next_sample_f32, source->sample_pos.frc_part);

						u32 out_sample_index = samples_written * 2 + ii;
						i16 out_sample_i16 = sample_memory_ptr[out_sample_index];
						f32 out_sample_f32 = (f32)out_sample_i16 / (out_sample_i16 > 0 ? 32767.0f : 32768.0f);

						out_sample_f32 += sample_f32 * source->volume.v[ii];
						out_sample_f32 = math::clamp(out_sample_f32, -1.0f, 1.0f);

						sample_memory_ptr[out_sample_index] = out_sample_f32 > 0.0f ? (i16)(out_sample_f32 * 32767.0f) : (i16)(out_sample_f32 * 32768.0f);
					}

					source->sample_pos.frc_part += pitch64.frc_part;
					u32 sample_int = (u32)source->sample_pos.frc_part;
					source->sample_pos.frc_part -= sample_int;
					source->sample_pos.int_part += pitch64.int_part + sample_int;

					samples_written++;
					source->volume += volume_delta;
				}

				ASSERT(samples_written <= samples_to_write);

				for(int i = 0; i < AUDIO_CHANNELS; i++) {
					if(volume_ended[i]) {
						source->volume.v[i] = source->target_volume.v[i];
						source->volume_delta.v[i] = 0.0f;
					}
				}

				samples_left_to_write -= samples_to_play;
				if(source->sample_pos.int_part >= clip->samples) {
					if(source->loop) {
						source->sample_pos.int_part -= clip->samples;
					}
					else {
						samples_left_to_write = 0;
					}
				}
			}

			if(source->sample_pos.int_part >= clip->samples && !source->loop) {
				*source_ptr = source->next;
				source->next = game_state->audio_source_free_list;
				game_state->audio_source_free_list = source;
			}
			else {
				source_ptr = &source->next;
			}
		}
	}
}
#endif


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
	s32 width;
	s32 height;
	u16 planes;
	u16 bits_per_pixel;
	u32 compression;
	u32 size_of_bitmap;
	s32 horz_res;
	s32 vert_res;
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
	ASSERT((wav_format->bits_per_sample / 8) == sizeof(s16));

	WavChunkHeader * wav_data_header = (WavChunkHeader *)((u8 *)wav_format_header + sizeof(WavChunkHeader) + wav_format_header->size);
	ASSERT(wav_data_header->id == RiffCode_data);

	s16 * wav_data = (s16 *)((u8 *)wav_data_header + sizeof(WavChunkHeader));

	clip->samples = wav_data_header->size / (wav_format->channels * sizeof(s16));
	u32 samples_with_padding = clip->samples + AUDIO_PADDING_SAMPLES;
	//TODO: Should all clips be forced stereo??
	clip->sample_data = PUSH_ARRAY(&game_state->memory_pool, s16, samples_with_padding * 2);
	if(wav_format->channels == 2) {
		for(u32 i = 0; i < samples_with_padding * 2; i++) {
			clip->sample_data[i] = wav_data[i % (clip->samples * 2)];
		}
	}
	else {
		for(u32 i = 0, sample_index = 0; i < samples_with_padding; i++) {
			s16 sample = wav_data[i % clip->samples];
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

void change_audio_volume(AudioSource * source, math::Vec2 volume, f32 time_) {
	//TODO: Assert or ignore null ptrs??
	ASSERT(source);

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

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_pool = create_memory_pool((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

		u32 vert_id = gl::compile_shader_from_source(BASIC_VERT_SRC, GL_VERTEX_SHADER);
		u32 frag_id = gl::compile_shader_from_source(BASIC_FRAG_SRC, GL_FRAGMENT_SHADER);
		game_state->program_id = gl::link_shader_program(vert_id, frag_id);

		f32 verts[] = {
			-0.5f,-0.5f, 0.0f,
			 0.5f, 0.5f, 0.0f,
			-0.5f, 0.5f, 0.0f,
			-0.5f,-0.5f, 0.0f,
			 0.5f, 0.5f, 0.0f,
			 0.5f,-0.5f, 0.0f, 
		};

		game_state->vertex_buffer = gl::create_vertex_buffer(verts, ARRAY_COUNT(verts), 3, GL_STATIC_DRAW);

		{
			u8 * file_buffer = read_file_to_memory("texture_256.bmp");

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

			game_state->texture = gl::create_texture(bmp_data, bmp_header->width, bmp_header->height, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);

			delete[] file_buffer;
		}

		math::Vec3 camera_forward = math::VEC3_FORWARD;
		math::Vec3 camera_pos = camera_forward * 2.0f;
		game_state->view_matrix = math::look_at(camera_pos, camera_pos - camera_forward, math::VEC3_UP);

		f32 aspect_ratio = (f32)game_input->back_buffer_width / (f32)game_input->back_buffer_height;
		game_state->projection_matrix = math::perspective_projection(aspect_ratio, 60.0f, 0.03f, sys::FLOAT_MAX);

		if(game_input->audio_supported) {
			//TODO: Temp until asset pack!!
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_sin_440], "sin_440.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_beep], "beep.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_woosh], "woosh.wav");
			load_audio_clip_from_file(game_state, &game_state->audio_clips[AudioClipId_music], "music.wav");

			game_state->music = play_audio_clip(game_state, AudioClipId_sin_440, true);			
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	AudioSource * music = game_state->music;
	if(music) {
		if(game_input->num_keys[1]) {
			change_audio_volume(music, math::vec2(0.0f), 5.0f);
		}

		if(game_input->num_keys[2]) {
			change_audio_volume(music, math::vec2(1.0f), 5.0f);
		}

		f32 pitch_step = 0.02f;

		if(game_input->num_keys[3]) {
			music->pitch -= pitch_step;
			if(music->pitch < pitch_step) {
				music->pitch = pitch_step;
			}

			std::printf("LOG: pitch: %f\n", music->pitch);
		}

		if(game_input->num_keys[4]) {
			music->pitch += pitch_step;
			std::printf("LOG: pitch: %f\n", music->pitch);
		}		
	}

	math::Mat4 view_projection_matrix = game_state->projection_matrix * game_state->view_matrix;
	
	glViewport(0, 0, game_input->back_buffer_width, game_input->back_buffer_height);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(game_state->program_id);

	math::Mat4 xform = view_projection_matrix * math::rotate_around_y(game_input->total_time);

	u32 xform_id = glGetUniformLocation(game_state->program_id, "xform");
	glUniformMatrix4fv(xform_id, 1, GL_FALSE, xform.v);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, game_state->texture);
	glUniform1i(glGetUniformLocation(game_state->program_id, "tex0"), 0);

	glBindBuffer(GL_ARRAY_BUFFER, game_state->vertex_buffer.id);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, game_state->vertex_buffer.vert_count);

	glDisable(GL_BLEND);
}

#if 0
static u32 global_sample_index = 0;

void game_sample(GameMemory * game_memory, s16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
	f32 tone_hz = 440.0f;

	for(u32 i = 0, sample_index = 0; i < samples_to_write; i++) {
		f32 sample_time = (f32)global_sample_index++ / (f32)samples_per_second;

		f32 sample_f32 = intrin::sin(sample_time * math::TAU * tone_hz);
		s16 sample = (s16)(sample_f32 * (sample_f32 > 0.0f ? 32767.0f : 32768.0f));

		sample_memory_ptr[sample_index++] = sample;
		sample_memory_ptr[sample_index++] = sample;
	}
}
#else
void game_sample(GameMemory * game_memory, s16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
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

						s16 sample_s16 = clip->sample_data[sample_index * 2 + ii];
						f32 sample_f32 = (f32)sample_s16 / (sample_s16 > 0 ? 32767.0f : 32768.0f);

						u32 next_sample_index = sample_index + 1;
						ASSERT(next_sample_index < (clip->samples + AUDIO_PADDING_SAMPLES));

						s16 next_sample_s16 = clip->sample_data[next_sample_index * 2 + ii];
						f32 next_sample_f32 = (f32)next_sample_s16 / (next_sample_s16 > 0 ? 32767.0f : 32768.0f);

						sample_f32 = math::lerp(sample_f32, next_sample_f32, source->sample_pos.frc_part);

						u32 out_sample_index = samples_written * 2 + ii;
						s16 out_sample_s16 = sample_memory_ptr[out_sample_index];
						f32 out_sample_f32 = (f32)out_sample_s16 / (out_sample_s16 > 0 ? 32767.0f : 32768.0f);

						out_sample_f32 += sample_f32 * source->volume.v[ii];
						out_sample_f32 = math::clamp(out_sample_f32, -1.0f, 1.0f);

						sample_memory_ptr[out_sample_index] = out_sample_f32 > 0.0f ? (s16)(out_sample_f32 * 32767.0f) : (s16)(out_sample_f32 * 32768.0f);
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

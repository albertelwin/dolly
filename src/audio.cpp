
#include <audio.hpp>

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
#pragma pack(pop)

AudioVal64 audio_val64(f32 val_f32) {
	AudioVal64 val;
	val.int_part = (u32)val_f32;
	val.frc_part = val_f32 - val.int_part;
	return val;
}

f32 audio_i16_to_f32(i16 sample) {
	return (f32)sample / (sample > 0 ? 32767.0f : 32768.0f);
}

i16 audio_f32_to_i16(f32 sample) {
	return sample > 0.0f ? (i16)(sample * 32767.0f) : (i16)(sample * 32768.0f);
}

void load_audio_clip_from_file(AudioState * audio_state, AudioClip * clip, char const * file_name) {
	FileBuffer file_buffer = read_file_to_memory(file_name);

	WavHeader * wav_header = (WavHeader *)file_buffer.ptr;
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
	clip->sample_data = PUSH_ARRAY(audio_state->memory_pool, i16, samples_with_padding * 2);
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

	delete[] file_buffer.ptr;
}

AudioClip * get_audio_clip(AudioState * audio_state, AudioClipId clip_id, u32 index) {
	AudioClip * clip = 0;

	AudioClipGroup * clip_group = audio_state->clip_groups + clip_id;
	if(clip_group->index) {
		u32 clip_index = clip_group->index + index;
		clip = audio_state->clips + clip_index;
	}

	return clip;
}

u32 get_audio_clip_count(AudioState * audio_state, AudioClipId clip_id) {
	AudioClipGroup * clip_group = audio_state->clip_groups + clip_id;
	return clip_group->count;
}

AudioSource * play_audio_clip(AudioState * audio_state, AudioClip * clip, b32 loop = false) {
	AudioSource * source = 0;
	if(clip) {
		if(!audio_state->source_free_list) {
			audio_state->source_free_list = PUSH_STRUCT(audio_state->memory_pool, AudioSource);
			audio_state->source_free_list->next = 0;
		}

		source = audio_state->source_free_list;
		audio_state->source_free_list = source->next;

		source->next = audio_state->sources;
		audio_state->sources = source;

		source->clip = clip;
		source->sample_pos = audio_val64(0.0f);
		source->loop = loop;
		source->pitch = 1.0f;
		source->volume = math::vec2(1.0f);
		source->target_volume = source->volume;
		source->volume_delta = math::vec2(0.0f);
	}

	return source;
}

AudioSource * play_audio_clip(AudioState * audio_state, AudioClipId clip_id, b32 loop = false) {
	AudioClip * clip = get_audio_clip(audio_state, clip_id, 0);
	return play_audio_clip(audio_state, clip, loop);
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

void audio_output_samples(AudioState * audio_state, i16 * sample_memory_ptr, u32 samples_to_write, u32 samples_per_second) {
	f32 playback_rate = samples_per_second != AUDIO_CLIP_SAMPLES_PER_SECOND ? (f32)AUDIO_CLIP_SAMPLES_PER_SECOND / (f32)samples_per_second : 1.0f;
	f32 seconds_per_sample = 1.0f / samples_per_second;

	AudioSource ** source_ptr = &audio_state->sources;
	while(*source_ptr) {
		AudioSource * source = *source_ptr;
		AudioClip * clip = source->clip;

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
					f32 sample_f32 = audio_i16_to_f32(sample_i16);

					u32 next_sample_index = sample_index + 1;
					ASSERT(next_sample_index < (clip->samples + AUDIO_PADDING_SAMPLES));

					i16 next_sample_i16 = clip->sample_data[next_sample_index * 2 + ii];
					f32 next_sample_f32 = audio_i16_to_f32(next_sample_i16);

					sample_f32 = math::lerp(sample_f32, next_sample_f32, source->sample_pos.frc_part);

					u32 out_sample_index = samples_written * 2 + ii;
					i16 out_sample_i16 = sample_memory_ptr[out_sample_index];
					f32 out_sample_f32 = audio_i16_to_f32(out_sample_i16);

					out_sample_f32 += sample_f32 * source->volume.v[ii] * audio_state->master_volume;
					out_sample_f32 = math::clamp(out_sample_f32, -1.0f, 1.0f);

					sample_memory_ptr[out_sample_index] = audio_f32_to_i16(out_sample_f32);
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
			source->next = audio_state->source_free_list;
			audio_state->source_free_list = source;
		}
		else {
			source_ptr = &source->next;
		}
	}
}

void push_audio_clip(AudioState * audio_state, AudioClipId id, char const * file_name) {
	AudioClipGroup * clip_group = audio_state->clip_groups + id;

	u32 clip_index = audio_state->clip_count++;
	if(!clip_group->index) {
		ASSERT(clip_group->count == 0);
		clip_group->index = clip_index;
	}
	else {
		//NOTE: Clip variations must be added sequentially!!
		ASSERT((clip_index - clip_group->index) == clip_group->count);
	}

	AudioClip * clip = audio_state->clips + clip_index;
	load_audio_clip_from_file(audio_state, clip, file_name);

	clip_group->count++;
}

void load_audio(AudioState * audio_state, MemoryPool * pool, b32 supported) {
	//TODO: Allocate sub-pool!!
	audio_state->memory_pool = pool;
	audio_state->supported = supported;
	audio_state->master_volume = 1.0f;

	if(audio_state->supported) {
		audio_state->clip_count = 0;
		audio_state->clips = PUSH_ARRAY(audio_state->memory_pool, AudioClip, 64);

		//TODO: Temp until asset pack!!
		push_audio_clip(audio_state, AudioClipId_sin_440, "sin_440.wav");
		push_audio_clip(audio_state, AudioClipId_beep, "beep.wav");
		push_audio_clip(audio_state, AudioClipId_woosh, "woosh.wav");
		push_audio_clip(audio_state, AudioClipId_pickup, "pickup0.wav");
		push_audio_clip(audio_state, AudioClipId_pickup, "pickup1.wav");
		push_audio_clip(audio_state, AudioClipId_pickup, "pickup2.wav");
		push_audio_clip(audio_state, AudioClipId_music, "music.wav");
	}
}
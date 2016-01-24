
#include <audio.hpp>

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

AudioSource * play_audio_clip(AudioState * audio_state, AudioClip * clip, b32 loop = false) {
	AudioSource * source = 0;
	if(audio_state->supported && clip) {
		if(!audio_state->source_free_list) {
			audio_state->source_free_list = PUSH_STRUCT(audio_state->arena, AudioSource);
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

AudioSource * play_audio_clip(AudioState * audio_state, AssetId clip_id, b32 loop = false) {
	AudioClip * clip = get_audio_clip_asset(audio_state->assets, clip_id, 0);
	return play_audio_clip(audio_state, clip, loop);
}

void stop_audio_clip(AudioState * audio_state, AudioSource * source) {
	//TODO: Remove audio source without iterating over entire list??
	if(source) {
		AudioSource ** source_ptr = &audio_state->sources;
		while(*source_ptr) {
			AudioSource * source_ = *source_ptr;

			if(source == source_) {
				*source_ptr = source->next;
				source->next = audio_state->source_free_list;
				audio_state->source_free_list = source_;

				source = 0;
				break;
			}
			else {
				source_ptr = &source->next;
			}
		}

		ASSERT(!source);
	}	
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

		u32 valid_samples = clip->samples;
		if(!source->loop) {
			valid_samples--;
			ASSERT(valid_samples);
		}

		while(samples_left_to_write) {
			u32 samples_to_play = samples_left_to_write;

			AudioVal64 samples_left_high = audio_val64((f32)(valid_samples - source->sample_pos.int_part) / pitch);
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
					ASSERT(sample_index < valid_samples);

					i16 sample_i16 = clip->sample_data[sample_index * 2 + ii];
					f32 sample_f32 = audio_i16_to_f32(sample_i16);

					u32 next_sample_index = sample_index + 1;
					ASSERT(next_sample_index < (valid_samples + AUDIO_PADDING_SAMPLES));

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
			if(source->sample_pos.int_part >= valid_samples) {
				if(source->loop) {
					source->sample_pos.int_part -= valid_samples;
				}
				else {
					samples_left_to_write = 0;
				}
			}
		}

		if(source->sample_pos.int_part >= valid_samples && !source->loop) {
			*source_ptr = source->next;
			source->next = audio_state->source_free_list;
			audio_state->source_free_list = source;
		}
		else {
			source_ptr = &source->next;
		}
	}
}

void load_audio(AudioState * audio_state, MemoryArena * arena, AssetState * assets, b32 supported) {
	//TODO: Allocate sub-pool!!
	audio_state->arena = arena;
	audio_state->assets = assets;
	audio_state->supported = supported;
	audio_state->master_volume = 1.0f;
}

#include <emscripten.h>
#include <emscripten/html5.h>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include <sys.hpp>
#include <gl.hpp>

#include <platform.hpp>

#include <math.hpp>
#include <game.hpp>

#include <math.cpp>
#include <game.cpp>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern "C" {
	extern b32 web_audio_init(u32 channels, u32 samples, u32 * samples_per_second, void * callback, void * user_ptr);
	extern void web_audio_suspend();
	extern void web_audio_resume();
	extern void web_audio_close();
	extern void web_audio_request_samples();
}

struct MainLoopArgs {	
	GameMemory game_memory;
	GameInput game_input;

	u32 game_button_to_key_map[ButtonId_count];

	u32 samples_per_second;
	u32 bytes_per_sample;
	u32 channels;

	//TODO: Use f64 for more precise timings??
	f32 frame_time;
	b32 page_hidden;
	f32 time_when_hidden;
	f32 time_when_shown;
};

EM_BOOL visibility_callback(int event_type, EmscriptenVisibilityChangeEvent const * event, void * user_ptr) {
	MainLoopArgs * args = (MainLoopArgs *)user_ptr;

	if(event->hidden) {
		args->page_hidden = true;
		args->time_when_hidden = (f32)(emscripten_get_now() / 1000.0);
		web_audio_suspend();
	}
	else {
		args->page_hidden = false;
		args->time_when_shown = (f32)(emscripten_get_now() / 1000.0);
		web_audio_resume();
	}

	return 0;
}

void async_audio_request(void * user_ptr) {
	web_audio_request_samples();
	emscripten_async_call(async_audio_request, 0, 1);
}

void audio_callback(void * user_ptr, u8 * buffer_ptr, i32 buffer_size) {
	MainLoopArgs * args = (MainLoopArgs *)user_ptr;

	game_sample(&args->game_memory, (i16 *)buffer_ptr, (u32)buffer_size / (args->bytes_per_sample * args->channels), args->samples_per_second);
}

void main_loop(void * user_ptr) {
	MainLoopArgs * args = (MainLoopArgs *)user_ptr;
	if(!args->page_hidden) {
		f32 last_frame_time = args->frame_time;
		args->frame_time = (f32)(emscripten_get_now() / 1000.0);

		args->game_input.delta_time = args->frame_time - last_frame_time;
		if(args->time_when_hidden > 0.0f) {
			ASSERT(args->time_when_hidden < args->time_when_shown);

			args->game_input.delta_time -= (args->time_when_shown - args->time_when_hidden);
			args->time_when_hidden = 0.0f;
			args->time_when_shown = 0.0f;
		}

		args->game_input.total_time += args->game_input.delta_time;

		for(u32 i = 0; i < ButtonId_count; i++) {
			u8 val = args->game_input.buttons[i];
			val &= ~(1 << KEY_PRESSED_BIT);
			val &= ~(1 << KEY_RELEASED_BIT);

			args->game_input.buttons[i] = val;
		}

		SDL_PumpEvents();

		i32 mouse_x;
		i32 mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);

		math::Vec2 mouse_pos = math::vec2((f32)mouse_x, (f32)mouse_y);
		args->game_input.mouse_delta = mouse_pos - args->game_input.mouse_pos;
		args->game_input.mouse_pos = mouse_pos;

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN: {
					if(event.key.keysym.sym == SDLK_ESCAPE) {
						web_audio_close();
						emscripten_cancel_main_loop();
					}

					for(u32 i = 0; i < ButtonId_count; i++) {
						if(event.key.keysym.sym == args->game_button_to_key_map[i]) {
							u8 val = args->game_input.buttons[i];
							val |= ((!(val & KEY_DOWN)) << KEY_PRESSED_BIT);
							val |= (1 << KEY_DOWN_BIT);

							args->game_input.buttons[i] = val;
						}
					}

					break;
				}

				case SDL_KEYUP: {
					for(u32 i = 0; i < ButtonId_count; i++) {
						if(event.key.keysym.sym == args->game_button_to_key_map[i]) {
							u8 val = args->game_input.buttons[i];
							val |= ((val & KEY_DOWN) << KEY_RELEASED_BIT);
							val &= ~(1 << KEY_DOWN_BIT);

							args->game_input.buttons[i] = val;
						}
					}


					break;
				}
			}
		}

		web_audio_request_samples();

		game_tick(&args->game_memory, &args->game_input);

		web_audio_request_samples();

		SDL_GL_SwapBuffers();
	}
}

int main() {
	ASSERT(SDL_Init(SDL_INIT_VIDEO) == 0);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	u32 window_width = 960;
	u32 window_height = 540;
	// u32 window_width = 640;
	// u32 window_height = 360;

	SDL_Surface * surface = SDL_SetVideoMode(window_width, window_height, 32, SDL_OPENGL);

	MainLoopArgs args = {};
	args.game_memory.size = MEGABYTES(8);
	args.game_memory.ptr = new u8[args.game_memory.size];

	args.game_input.back_buffer_width = window_width;
	args.game_input.back_buffer_height = window_height;

	u32 null_key_code = ~0;
	for(u32 i = 0; i < ButtonId_count; i++) {
		args.game_button_to_key_map[i] = null_key_code;
	}

	args.game_button_to_key_map[ButtonId_left] = SDLK_LEFT;
	args.game_button_to_key_map[ButtonId_right] = SDLK_RIGHT;
	args.game_button_to_key_map[ButtonId_up] = SDLK_UP;
	args.game_button_to_key_map[ButtonId_down] = SDLK_DOWN;

	args.game_button_to_key_map[ButtonId_mute] = SDLK_RETURN;

	for(u32 i = 0; i < ButtonId_count; i++) {
		ASSERT(args.game_button_to_key_map[i] != null_key_code);
	}
	
	args.bytes_per_sample = sizeof(i16);
	args.channels = 2;

	if(web_audio_init(args.channels, 2048, &args.samples_per_second, (void *)audio_callback, &args)) {
		args.game_input.audio_supported = true;
		async_audio_request(0);
	}
	else {
		args.game_input.audio_supported = false;
	}

	args.frame_time = (f32)(emscripten_get_now() / 1000.0);
	args.page_hidden = false;
	args.time_when_hidden = 0.0f;
	args.time_when_shown = 0.0f;

	emscripten_set_visibilitychange_callback(&args, 1, visibility_callback);

	emscripten_set_main_loop_arg(main_loop, &args, 0, true);

	return 0;
}
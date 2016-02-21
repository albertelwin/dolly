
#include <emscripten.h>
#include <emscripten/html5.h>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include <sys.hpp>

#include <game.cpp>

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

	f64 frame_time;
	b32 page_hidden;
	f64 time_when_hidden;
	f64 time_when_shown;
};

PlatformAsyncFile * platform_open_async_file(char const * file_name) {
	PlatformAsyncFile * async_file = 0;

	if(EM_ASM_INT_V({ return Module.local_store_safe_to_read_write; })) {
		ASSERT(c_str_len(file_name) < ARRAY_COUNT(async_file->name));

		async_file = ALLOC_STRUCT(PlatformAsyncFile);
		c_str_copy(async_file->name, file_name);
		async_file->memory = read_file_to_memory(file_name);
	}

	return async_file;
}

void platform_close_async_file(PlatformAsyncFile * async_file) {
	FREE_MEMORY(async_file->memory.ptr);
	FREE_MEMORY(async_file);
}

b32 platform_write_async_file(PlatformAsyncFile * async_file, void * ptr, size_t size) {
	b32 written = false;

	if(EM_ASM_INT_V({ return Module.local_store_safe_to_read_write; })) {
		std::FILE * file = std::fopen(async_file->name, "wb");
		if(file) {
			std::fwrite(ptr, size, 1, file);
			std::fclose(file);
			
			written = true;

			EM_ASM(
				try {
					Module.local_store_safe_to_read_write = false;
					FS.syncfs(function(err) {
						if(err) {
							console.error("ERROR: " + err + "\n");
						}
						else {
							Module.local_store_safe_to_read_write = true;
						}
					});					
				}
				catch(e) {
					console.error("ERROR: " + e + "\n");
					Module.local_store_safe_to_read_write = false;
				}
			);	
		}
	}

	return written;
}

EM_BOOL visibility_callback(int event_type, EmscriptenVisibilityChangeEvent const * event, void * user_ptr) {
	MainLoopArgs * args = (MainLoopArgs *)user_ptr;

	if(event->hidden) {
		args->page_hidden = true;
		args->time_when_hidden = emscripten_get_now();
		web_audio_suspend();
	}
	else {
		args->page_hidden = false;
		args->time_when_shown = emscripten_get_now();
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

math::Vec2 get_mouse_pos() {
	i32 mouse_x;
	i32 mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	return math::vec2(mouse_x, mouse_y);
}

void main_loop(void * user_ptr) {
	MainLoopArgs * args = (MainLoopArgs *)user_ptr;
	if(!args->page_hidden) {
		f64 last_frame_time = args->frame_time;
		args->frame_time = emscripten_get_now();

		f64 delta_time = args->frame_time - last_frame_time;
		if(args->time_when_hidden > 0.0f) {
			ASSERT(args->time_when_hidden < args->time_when_shown);

			delta_time -= (args->time_when_shown - args->time_when_hidden);
			args->time_when_hidden = 0.0;
			args->time_when_shown = 0.0;
		}

		// if(delta_time > 33.333333333333333333333333333333f) {
		// 	std::printf("LOG: dt: %f\n", delta_time);
		// }

		args->game_input.delta_time = (f32)(delta_time / 1000.0);
		args->game_input.total_time += args->game_input.delta_time;

		{
			u8 val = args->game_input.mouse_button;
			val &= ~(1 << KEY_PRESSED_BIT);
			val &= ~(1 << KEY_RELEASED_BIT);

			args->game_input.mouse_button = val;
		}

		for(u32 i = 0; i < ButtonId_count; i++) {
			u8 val = args->game_input.buttons[i];
			val &= ~(1 << KEY_PRESSED_BIT);
			val &= ~(1 << KEY_RELEASED_BIT);

			args->game_input.buttons[i] = val;
		}

		SDL_PumpEvents();

		math::Vec2 mouse_pos = get_mouse_pos();
		args->game_input.last_mouse_pos = args->game_input.mouse_pos;
		args->game_input.mouse_pos = mouse_pos;

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN: {
#if 0
					if(event.key.keysym.sym == SDLK_ESCAPE) {
						web_audio_close();
						emscripten_cancel_main_loop();
					}
#endif

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

				case SDL_MOUSEBUTTONDOWN: {
					if(event.button.button == SDL_BUTTON_LEFT) {
						u8 val = args->game_input.mouse_button;
						val |= ((!(val & KEY_DOWN)) << KEY_PRESSED_BIT);
						val |= (1 << KEY_DOWN_BIT);

						args->game_input.mouse_button = val;
					}

					break;
				}

				case SDL_MOUSEBUTTONUP: {
					if(event.button.button == SDL_BUTTON_LEFT) {
						u8 val = args->game_input.mouse_button;
						val |= ((val & KEY_DOWN) << KEY_RELEASED_BIT);
						val &= ~(1 << KEY_DOWN_BIT);

						args->game_input.mouse_button = val;
					}

					break;
				}
			}
		}

		web_audio_request_samples();

		game_tick(&args->game_memory, &args->game_input);

		web_audio_request_samples();

#if DEBUG_ENABLED
		debug_game_tick(&args->game_memory, &args->game_input);
#endif

		SDL_GL_SwapBuffers();
	}
}

int main_init() {
	if(SDL_Init(SDL_INIT_VIDEO)) {
		ASSERT(!"Failed to initialise SDL!");
	}

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);

	// u32 window_width = 640, window_height = 360;
	u32 window_width = 960, window_height = 540;
	// u32 window_width = 1280, window_height = 720;
	SDL_Surface * surface = SDL_SetVideoMode(window_width, window_height, 32, SDL_OPENGL);

	MainLoopArgs args = {};
	args.game_memory.size = MEGABYTES(16);
	args.game_memory.ptr = ALLOC_MEMORY(u8, args.game_memory.size);
	zero_memory(args.game_memory.ptr, args.game_memory.size);

	args.game_input.back_buffer_width = window_width;
	args.game_input.back_buffer_height = window_height;

	u32 null_key_code = ~0;
	for(u32 i = 0; i < ButtonId_count; i++) {
		args.game_button_to_key_map[i] = null_key_code;
	}

	args.game_button_to_key_map[ButtonId_start] = SDLK_SPACE;
	args.game_button_to_key_map[ButtonId_left] = SDLK_LEFT;
	args.game_button_to_key_map[ButtonId_right] = SDLK_RIGHT;
	args.game_button_to_key_map[ButtonId_up] = SDLK_UP;
	args.game_button_to_key_map[ButtonId_down] = SDLK_DOWN;

	args.game_button_to_key_map[ButtonId_quit] = SDLK_ESCAPE;
	args.game_button_to_key_map[ButtonId_mute] = SDLK_RETURN;

	args.game_button_to_key_map[ButtonId_debug] = SDLK_0;

	for(u32 i = 0; i < ButtonId_count; i++) {
		ASSERT(args.game_button_to_key_map[i] != null_key_code);
	}

	math::Vec2 mouse_pos = get_mouse_pos();
	args.game_input.last_mouse_pos = mouse_pos;
	args.game_input.mouse_pos = mouse_pos;
	
	args.bytes_per_sample = sizeof(i16);
	args.channels = 2;

	if(web_audio_init(args.channels, 2048, &args.samples_per_second, (void *)audio_callback, &args)) {
		args.game_input.audio_supported = true;
		async_audio_request(0);
	}
	else {
		args.game_input.audio_supported = false;
	}

	args.frame_time = emscripten_get_now();
	args.page_hidden = false;
	args.time_when_hidden = 0.0f;
	args.time_when_shown = 0.0f;

	emscripten_set_visibilitychange_callback(&args, 1, visibility_callback);

	emscripten_set_main_loop_arg(main_loop, &args, 0, true);

	return 0;
}

#if DEV_ENABLED
static u32 debug_global_file_loaded_counter = 0;

void wget_callback(char const * str) {
	debug_global_file_loaded_counter--;
	if(!debug_global_file_loaded_counter) {
		main_init();
	}
}

int main() {
	for(u32 i = 0; i < ARRAY_COUNT(debug_global_asset_file_names); i++) {
		char const * file_name = debug_global_asset_file_names[i];
		emscripten_async_wget(file_name, file_name, wget_callback, wget_callback);
		debug_global_file_loaded_counter++;
	}

	return 0;
}
#else
int main() {
	return main_init();
}
#endif

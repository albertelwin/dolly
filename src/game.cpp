
#include <game.hpp>

#include <asset.cpp>
#include <audio.cpp>
#include <math.cpp>
#include <render.cpp>

u32 begin_transition(GameState * game_state, TransitionType type = TransitionType_pixelate) {
	ASSERT(type != TransitionType_null);

	u32 id = 0;
	if(!game_state->transitioning) {
		if(game_state->transition_id == U32_MAX) {
			game_state->transition_id = 1;
		}
		else {
			game_state->transition_id++;
		}

		game_state->transitioning = true;
		game_state->transition_type = type;

		id = game_state->transition_id;
	}

	return id;
}

b32 transition_should_flip(GameState * game_state, u32 transition_id) {
	b32 used = false;
	if(game_state->transitioning && game_state->transition_id == transition_id && game_state->transition_flip) {
		ASSERT(game_state->transition_id != 0);

		game_state->transition_flip = false;
		used = true;
	}

	return used;
}

math::Rec2 get_entity_render_bounds(AssetState * assets, Entity * entity) {
	Asset * asset = get_asset(assets, entity->asset_id, entity->asset_index);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

	return math::rec2_pos_dim(asset->texture.offset, asset->texture.dim * entity->scale);
}

math::Rec2 get_entity_collider_bounds(Entity * entity) {
	return math::rec_scale(math::rec_offset(entity->collider, entity->pos.xy), entity->scale);
}

Entity * push_entity(EntityArray * entities, AssetState * assets, AssetId asset_id, u32 asset_index, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(entities->count < ARRAY_COUNT(entities->elems));

	//TODO: Should entities only be sprites??
	Asset * asset = get_asset(assets, asset_id, asset_index);
	ASSERT(asset);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

	Entity * entity = entities->elems + entities->count++;
	entity->pos = pos;
	entity->scale = 1.0f;
	entity->color = math::vec4(1.0f);

	entity->asset_type = asset->type;
	entity->asset_id = asset_id;
	entity->asset_index = asset_index;

	entity->d_pos = math::vec2(0.0f);
	entity->speed = math::vec2(500.0f);
	entity->damp = 0.0f;
	entity->use_gravity = false;
	entity->collider = get_entity_render_bounds(assets, entity);

	entity->anim_time = 0.0f;
	entity->initial_x = pos.x;
	entity->hit = false;

	return entity;
}

Str * allocate_str(MemoryArena * arena, u32 max_len) {
	Str * str = PUSH_STRUCT(arena, Str);
	str->max_len = max_len;
	str->len = 0;
	str->ptr = PUSH_ARRAY(arena, char, max_len);	
	return str;
}

void push_player_clone(Player * player) {
	for(u32 i = 0; i < ARRAY_COUNT(player->clones); i++) {
		Entity * entity = player->clones[i];

		if(entity->hidden) {
			entity->hidden = false;
			entity->hit = false;
			entity->color.a = 0.0f;

			break;
		}
	}
}

void pop_player_clones(Player * player, u32 count) {
	ASSERT(count);

	u32 remaining = count;
	for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
		Entity * entity = player->clones[i];

		if(!entity->hidden && !entity->hit) {
			entity->hit = true;

			if(!--remaining) {
				break;
			}
		}
	}

	if(remaining == count) {
		player->dead = true;
		player->e->damp = 0.0f;
		player->e->use_gravity = true;
		player->e->d_pos = math::vec2(0.0f, 1000.0f);
	}
}

void begin_rocket_sequence(GameState * game_state, MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	RocketSequence * seq = &main_state->rocket_seq;

	if(!seq->playing) {
		seq->transition_id = begin_transition(game_state);
		if(seq->transition_id) {
			seq->playing = true;
			seq->time_ = 0.0f;

			math::Rec2 render_bounds = get_entity_render_bounds(meta_state->assets, seq->rocket);
			f32 height = math::rec_dim(render_bounds).y;

			Entity * rocket = seq->rocket;
#if 0
			rocket->pos = math::vec3(main_state->player.e->pos.x, -(meta_state->render_state->screen_height * 0.5f + height * 0.5f), 0.0f);
#else
			rocket->pos = ENTITY_NULL_POS;
#endif
			rocket->d_pos = math::vec2(0.0f);
			rocket->speed = math::vec2(0.0f, 10000.0f);
			rocket->damp = 0.065f;
		}
	}
}

void play_rocket_sequence(GameState * game_state, MetaState * meta_state, f32 dt) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	RocketSequence * seq = &main_state->rocket_seq;

	if(seq->playing) {
		RenderState * render_state = meta_state->render_state;

		Player * player = &main_state->player;
#if 0
		Camera * camera = main_state->camera;
		Entity * rocket = seq->rocket;

		rocket->d_pos += rocket->speed * dt;
		rocket->d_pos *= (1.0f - rocket->damp);

		math::Vec2 d_pos = rocket->d_pos * dt;
		math::Vec2 new_pos = rocket->pos.xy + d_pos;

		if(rocket->pos.y < main_state->location_y_offset) {
			if(new_pos.y < main_state->location_y_offset) {
				math::Rec2 player_bounds = get_entity_collider_bounds(player->e);
				math::Rec2 collider_bounds = get_entity_collider_bounds(rocket);
				if(math::rec_overlap(collider_bounds, player_bounds)) {
					player->e->d_pos = math::vec2(0.0f);
					player->e->pos.xy += d_pos;
					player->allow_input = false;
				}

				main_state->camera->offset = (math::rand_vec2() * 2.0f - 1.0f) * 10.0f;
			}
			else {
				player->e->pos.y = main_state->location_y_offset;
				player->allow_input = true;

				main_state->current_location = LocationId_space;
				main_state->entity_emitter.pos.y = main_state->location_y_offset;
				main_state->entity_emitter.time_until_next_spawn = 0.0f;
			}
		}

		rocket->pos.xy = new_pos;
		seq->time_ += dt;

		f32 seq_max_time = 20.0f;
		if(seq->time_ > seq_max_time) {
			f32 new_pixelate_time = camera->pixelate_time + dt * 1.5f;
			if(camera->pixelate_time < 1.0f && new_pixelate_time >= 1.0f) {
				rocket->pos = ENTITY_NULL_POS;

				player->e->d_pos = math::vec2(0.0f);
				player->e->pos.xy = math::vec2(player->e->initial_x, 0.0f);
				player->allow_input = true;

				main_state->current_location = LocationId_city;
				main_state->entity_emitter.pos.y = 0.0f;
				main_state->entity_emitter.time_until_next_spawn = 0.0f;
			}

			camera->pixelate_time = new_pixelate_time;
			if(camera->pixelate_time >= 2.0f) {
				camera->pixelate_time = 0.0f;

				seq->playing = false;
				seq->time_ = 0.0f;
			}
		}
#else
		//TODO: Use the transition system!!
		f32 new_time = seq->time_ + dt;
		f32 seq_max_time = 10.0f;

		if(seq->time_ < seq_max_time) {
			if(transition_should_flip(game_state, seq->transition_id)) {
				player->e->d_pos = math::vec2(0.0f);
				player->e->pos.xy = math::vec2(player->e->initial_x, 0.0f);
				player->e->pos.y += main_state->location_y_offset;

				main_state->current_location = LocationId_space;
				main_state->entity_emitter.pos.y = main_state->location_y_offset;
				main_state->entity_emitter.time_until_next_spawn = 0.0f;

				seq->transition_id = 0;
			}

			if(new_time > seq_max_time && seq->transition_id == 0) {
				//TODO: Properly handle the case where this doesn't succeed!!
				seq->transition_id = begin_transition(game_state);
			}
		}
		else {
			if(transition_should_flip(game_state, seq->transition_id)) {
				player->e->d_pos = math::vec2(0.0f);
				player->e->pos.xy = math::vec2(player->e->initial_x, 0.0f);

				main_state->current_location = LocationId_city;
				main_state->entity_emitter.pos.y = 0.0f;
				main_state->entity_emitter.time_until_next_spawn = 0.0f;				
			}

			if(!game_state->transitioning) {
				seq->playing = false;
			}
		}

		seq->time_ = new_time;
#endif
	}
}

b32 move_entity(MetaState * meta_state, RenderTransform * transform, Entity * entity, f32 dt) {
	b32 off_screen = false;

	//TODO: Move by player speed!!
	entity->pos.x -= 500.0f * dt;

	math::Rec2 bounds = get_entity_render_bounds(meta_state->assets, entity);
	math::Vec2 screen_pos = project_pos(transform, entity->pos + math::vec3(math::rec_pos(bounds), 0.0f));
	f32 width = math::rec_dim(bounds).x;

	if(screen_pos.x < -(meta_state->render_state->screen_width * 0.5f + width * 0.5f)) {
		off_screen = true;

		if(entity->scrollable) {
			screen_pos.x += width * 2.0f;
			entity->pos = unproject_pos(transform, screen_pos, entity->pos.z);
		}
	}

	return off_screen;
}

MetaState * allocate_meta_state(GameState * game_state, MetaStateType type) {
	MemoryArena * arena = &game_state->memory_arena;

	MetaState * meta_state = PUSH_STRUCT(arena, MetaState);
	meta_state->assets = &game_state->assets;
	meta_state->audio_state = &game_state->audio_state;
	meta_state->render_state = &game_state->render_state;

	size_t arena_size = KILOBYTES(256);

	meta_state->type = type;
	switch(type) {
		case MetaStateType_menu: {
			meta_state->menu = PUSH_STRUCT(arena, MenuMetaState);
			break;
		}

		case MetaStateType_main: {
			meta_state->main = PUSH_STRUCT(arena, MainMetaState);
			arena_size = MEGABYTES(2);
			break;
		}

		case MetaStateType_game_over: {
			meta_state->game_over = PUSH_STRUCT(arena, GameOverMetaState);
			break;
		}

		INVALID_CASE();
	}

	meta_state->arena = allocate_sub_arena(arena, arena_size);
	
	return meta_state;
}

MetaState * get_meta_state(GameState * game_state, MetaStateType type) {
	MetaState * meta_state = game_state->meta_states[type];
	ASSERT(meta_state->type == type);

	return meta_state;
}

void init_menu_meta_state(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_menu);
	MenuMetaState * menu_state = meta_state->menu;

	zero_memory_arena(&meta_state->arena);
	zero_memory(menu_state, sizeof(MenuMetaState));

	EntityArray * entities = &menu_state->entities;

	menu_state->credits = push_entity(entities, meta_state->assets, AssetId_menu_credits, 0);
	menu_state->score = push_entity(entities, meta_state->assets, AssetId_menu_score, 0);
	menu_state->play = push_entity(entities, meta_state->assets, AssetId_menu_play, 0);
}

void init_main_meta_state(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	ASSERT(!main_state->music);

	zero_memory_arena(&meta_state->arena);
	zero_memory(main_state, sizeof(MainMetaState));

	AssetState * assets = meta_state->assets;

	u32 screen_width = meta_state->render_state->screen_width;
	u32 screen_height = meta_state->render_state->screen_height;

	main_state->music = play_audio_clip(meta_state->audio_state, AssetId_music, true);
	change_volume(main_state->music, math::vec2(0.0f), 0.0f);
	change_volume(main_state->music, math::vec2(1.0f), 1.0f);

	main_state->render_transform = create_render_transform(math::vec2(0.0f));

	EntityArray * entities = &main_state->entities;

	main_state->entity_gravity = math::vec2(0.0f, -6000.0f);

	main_state->current_location = LocationId_city;

	f32 location_z_offsets[PARALLAX_LAYER_COUNT] = {
		 10.0f,
		 10.0f,
		  7.5f,
		  5.0f,
		 -0.5f,
	};

	f32 location_max_z = location_z_offsets[0];
	main_state->location_y_offset = (f32)screen_height / (1.0f / (location_max_z + 1.0f));
	main_state->ground_height = -(f32)screen_height * 0.5f;

	AssetId location_layer_ids[LocationId_count];
	location_layer_ids[LocationId_city] = AssetId_city;
	location_layer_ids[LocationId_space] = AssetId_space;
	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = main_state->locations + i;
		AssetId asset_id = location_layer_ids[i];

		location->min_y = main_state->location_y_offset * i;
		location->max_y = location->min_y + main_state->location_y_offset * 0.5f;

		for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers) - 1; layer_index++) {
			Entity * entity = push_entity(entities, meta_state->assets, asset_id, layer_index);

			entity->pos.y = main_state->location_y_offset * i;
			entity->pos.z = location_z_offsets[layer_index];
			entity->scrollable = true;

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

		main_state->locations[LocationId_city].emitter_type_count = ARRAY_COUNT(emitter_types);
		main_state->locations[LocationId_city].emitter_types = PUSH_COPY_ARRAY(&meta_state->arena, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
	}

	{
		AssetId emitter_types[] = {
			AssetId_smiley,
			AssetId_dolly,
			AssetId_dolly,
		};

		main_state->locations[LocationId_space].emitter_type_count = ARRAY_COUNT(emitter_types);
		main_state->locations[LocationId_space].emitter_types = PUSH_COPY_ARRAY(&meta_state->arena, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
	}

	Player * player = &main_state->player;
	player->e = push_entity(entities, assets, AssetId_dolly, 0, math::vec3(0.0f));
	player->e->pos = math::vec3((f32)screen_width * -0.25f, 0.0f, 0.0f);
	player->e->initial_x = player->e->pos.x;
	player->e->speed = math::vec2(50.0f, 6000.0f);
	player->e->damp = 0.15f;
	player->allow_input = true;

	player->clone_offset = math::vec2(-5.0f, 0.0f);
	for(u32 i = 0; i < ARRAY_COUNT(player->clones); i++) {
		Entity * entity = push_entity(entities, assets, AssetId_dolly, 0, ENTITY_NULL_POS);
		entity->color.a = 0.0f;
		entity->hidden = true;

		player->clones[i] = entity;
	}
	
	EntityEmitter * emitter = &main_state->entity_emitter;
	Texture * emitter_sprite = get_sprite_asset(meta_state->assets, AssetId_teacup, 0);
	emitter->pos = math::vec3(screen_width * 0.75f, 0.0f, 0.0f);
	emitter->time_until_next_spawn = 0.0f;
	emitter->entity_count = 0;
	for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
		emitter->entity_array[i] = push_entity(entities, assets, AssetId_teacup, 0, emitter->pos);
	}

	RocketSequence * seq = &main_state->rocket_seq;
	seq->rocket = push_entity(entities, assets, AssetId_large_rocket, 0, ENTITY_NULL_POS);
	seq->rocket->collider = math::rec_offset(seq->rocket->collider, math::vec2(0.0f, -math::rec_dim(seq->rocket->collider).y * 0.5f));
	seq->playing = false;
	seq->time_ = 0.0f;

	//TODO: Adding foreground layer at the end until we have sorting!!
	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = main_state->locations + i;
		AssetId asset_id = location_layer_ids[i];

		u32 layer_index = ARRAY_COUNT(location->layers) - 1;

		Entity * entity = push_entity(entities, assets, asset_id, layer_index);
		entity->pos.y = main_state->location_y_offset * i;
		entity->pos.z = location_z_offsets[layer_index];
		entity->scrollable = true;

		location->layers[layer_index] = entity;
	}

	main_state->d_time = 1.0f;
	main_state->score = 0;
	main_state->distance = 0.0f;
}

void init_game_over_meta_state(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_game_over);
	GameOverMetaState * game_over_state = meta_state->game_over;

	zero_memory_arena(&meta_state->arena);
	zero_memory(game_over_state, sizeof(GameOverMetaState));

	AssetState * assets = meta_state->assets;

	game_over_state->music = play_audio_clip(meta_state->audio_state, AssetId_death_music, true);
	change_volume(game_over_state->music, math::vec2(0.0f), 0.0f);
	change_volume(game_over_state->music, math::vec2(1.0f), 1.0f);

	meta_state->render_state->fade_amount = 1.0f;

	EntityArray * entities = &game_over_state->entities;
	push_entity(entities, assets, AssetId_car, 0);	
}

void change_meta_state(GameState * game_state, MetaStateType type) {
	ASSERT(game_state->meta_state != type);
	game_state->meta_state = type;

	MetaState * meta_state = get_meta_state(game_state, type);
	switch(type) {
		case MetaStateType_menu: {
			init_menu_meta_state(meta_state);
			break;
		}

		case MetaStateType_main: {
			init_main_meta_state(meta_state);
			break;
		}

		case MetaStateType_game_over: {
			init_game_over_meta_state(meta_state);
			break;
		}

		INVALID_CASE();
	}
}

void game_tick(GameMemory * game_memory, GameInput * game_input) {
	ASSERT(sizeof(GameState) <= game_memory->size);
	GameState * game_state = (GameState *)game_memory->ptr;

	if(!game_memory->initialized) {
		game_memory->initialized = true;

		game_state->memory_arena = create_memory_arena((u8 *)game_memory->ptr + sizeof(GameState), game_memory->size - sizeof(GameState));

		load_assets(&game_state->assets, &game_state->memory_arena);
		load_audio(&game_state->audio_state, &game_state->memory_arena, &game_state->assets, game_input->audio_supported);
		load_render(&game_state->render_state, &game_state->memory_arena, &game_state->assets, game_input->back_buffer_width, game_input->back_buffer_height);

		// game_state->audio_state.master_volume = 0.0f;

		game_state->meta_state = MetaStateType_null;
		for(u32 i = 0; i < ARRAY_COUNT(game_state->meta_states); i++) {
			game_state->meta_states[i] = allocate_meta_state(game_state, (MetaStateType)i);
		}

		change_meta_state(game_state, MetaStateType_menu);

		game_state->auto_save_time = 5.0f;
		game_state->save.code = SAVE_FILE_CODE;
		game_state->save.version = SAVE_FILE_VERSION;
		game_state->save.plays = 0;
		game_state->save.high_score = 0;

		game_state->debug_str = allocate_str(&game_state->memory_arena, 1024);
		game_state->str = allocate_str(&game_state->memory_arena, 256);
	}

	if(!game_state->save_file) {
		PlatformAsyncFile * file = platform_open_async_file("/user_dat/save.dat");
		if(file) {
			if(file->memory.size >= sizeof(SaveFileHeader)) {
				SaveFileHeader * header = (SaveFileHeader *)file->memory.ptr;

				if(header->code == SAVE_FILE_CODE && header->version == SAVE_FILE_VERSION) {
					game_state->save.plays = header->plays + 1;
					game_state->save.high_score = math::max(game_state->save.high_score, header->high_score);
					game_state->save.longest_run = math::max(game_state->save.longest_run, header->longest_run);
				}
			}

			platform_write_async_file(file, (void *)&game_state->save, sizeof(SaveFileHeader));
			game_state->save_file = file;
		}
	}
	else {
		game_state->time_until_next_save -= game_input->delta_time;
		if(game_state->time_until_next_save <= 0.0f) {
			if(platform_write_async_file(game_state->save_file, (void *)&game_state->save, sizeof(SaveFileHeader))) {
				game_state->time_until_next_save = game_state->auto_save_time;
			}
		}
	}

	AssetState * assets = &game_state->assets;
	AudioState * audio_state = &game_state->audio_state;
	RenderState * render_state = &game_state->render_state;

	str_clear(game_state->str);
	str_print(game_state->str, "DOLLY DOLLY DOLLY DAYS!\nDT: %f\n\n", game_input->delta_time);
	str_print(game_state->str, "PLAYS: %u | HIGH_SCORE: %u | LONGEST_RUN: %f\n", game_state->save.plays, game_state->save.high_score, game_state->save.longest_run);

	if(game_input->buttons[ButtonId_debug] & KEY_PRESSED) {
		render_state->debug_render_entity_bounds = !render_state->debug_render_entity_bounds;
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		// game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;
	}

	Shader * basic_shader = &render_state->basic_shader;
	Shader * post_shader = &render_state->post_shader;

	switch(game_state->meta_state) {
		case MetaStateType_menu: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_menu);
			MenuMetaState * menu_state = meta_state->menu;

			render_state->letterboxed_height = render_state->screen_height;

			if(!game_state->transitioning) {
				math::Vec2 screen_dim = math::vec2(render_state->screen_width, render_state->screen_height);
				math::Vec2 buffer_dim = math::vec2(game_input->back_buffer_width, game_input->back_buffer_height);

				math::Vec2 mouse_pos = game_input->mouse_pos * (screen_dim / buffer_dim) - screen_dim * 0.5f;
				mouse_pos.y = -mouse_pos.y;

				Entity * hot_entity = 0;
				for(u32 i = 0; i < menu_state->entities.count; i++) {
					Entity * entity = menu_state->entities.elems + i;
					entity->asset_index = 0;

					math::Rec2 bounds = get_entity_collider_bounds(entity);
					if(math::inside_rec(bounds, mouse_pos)) {
						hot_entity = entity;
					}
				}

				if(hot_entity) {
					hot_entity->asset_index = 1;

					if(game_input->mouse_button & KEY_PRESSED) {
						if(hot_entity == menu_state->play) {
							menu_state->transition_id = begin_transition(game_state);
						}
					}
				}				
			}
			else {
				if(transition_should_flip(game_state, menu_state->transition_id)) {
					change_meta_state(game_state, MetaStateType_main);
				}
			}

			break;
		}

		case MetaStateType_main: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_main);
			MainMetaState * main_state = meta_state->main;

			RenderTransform * render_transform = &main_state->render_transform;

			Player * player = &main_state->player;

			if(!game_state->transitioning) {
				if(game_input->buttons[ButtonId_quit] & KEY_PRESSED) {
					main_state->quit_transition_id = begin_transition(game_state, TransitionType_pixelate);
				}
				else if(player->dead && player->e->pos.y <= render_transform->pos.y - meta_state->render_state->screen_height * 0.5f) {
					main_state->death_transition_id = begin_transition(game_state, TransitionType_fade);
				}

				//TODO: Fade out and stop??
				if(game_state->transitioning) {
					change_volume(main_state->music, math::vec2(0.0f), 1.0f);
				}
			}
			else {
				MetaStateType new_state = MetaStateType_null;
				if(transition_should_flip(game_state, main_state->quit_transition_id)) {
					new_state = MetaStateType_menu;
				}
				else if(transition_should_flip(game_state, main_state->death_transition_id)) {
					new_state = MetaStateType_game_over;
				}

				if(new_state != MetaStateType_null) {
					stop_audio_clip(meta_state->audio_state, main_state->music);
					main_state->music = 0;

					game_state->time_until_next_save = 0.0f;

					change_meta_state(game_state, new_state);
				}
			}

			if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
				begin_rocket_sequence(game_state, meta_state);
			}

			if(!player->dead) {
				main_state->d_time += 0.02f * game_input->delta_time;
				main_state->d_time = math::clamp(main_state->d_time, 0.0f, 2.0f);

				f32 pitch = main_state->d_time;
				if(main_state->d_time >= 1.0f) {
					pitch = 1.0f + (main_state->d_time - 1.0f) * 0.02f;
					
				}

				change_pitch(main_state->music, pitch);
			}

			f32 adjusted_dt = main_state->d_time * game_input->delta_time;

			player->active_clone_count = 0;
			for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
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

				// f32 amplitude = math::max(50.0f / math::max(main_state->d_time, 1.0f), 0.0f);
				f32 amplitude = 50.0f;
				entity->pos.y += math::sin((game_input->total_time * 0.25f + i * (3.0f / 13.0f)) * math::TAU) * amplitude;

				if(!entity->hidden) {
					if(entity->hit) {
						entity->color.a = (u32)(entity->hit_time * 30.0f) & 1;

						entity->hit_time += game_input->delta_time;
						if(entity->hit_time >= 1.0f) {
							entity->hit_time = 0.0f;
							entity->hidden = true;
							
							entity->color.a = 0.0f;
						}
					}
					else {
						entity->color.a += 4.0f * game_input->delta_time;
						entity->color.a = math::min(entity->color.a, 1.0f);

						player->active_clone_count++;
					}
				}
				else {
					entity->color.a = 0.0f;
				}

				//TODO: Offset each clone in the animation!!
				entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
				entity->asset_index = (u32)entity->anim_time % get_asset_count(assets, entity->asset_id);
			}

			math::Vec2 player_dd_pos = math::vec2(0.0f);

#if 0
			if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
				if(player->running) {
					player->running = false;
					player->grounded = false;

					player->e->damp = 0.15f;
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

			if(!player->dead) {
				if(player->allow_input) {
					if(!player->running) {
						f32 half_buffer_height = (f32)game_input->back_buffer_height * 0.5f;
						
						if(game_input->buttons[ButtonId_up] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y < half_buffer_height)) {
							player_dd_pos.y += player->e->speed.y;
						}

						if(game_input->buttons[ButtonId_down] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y >= half_buffer_height)) {
							player_dd_pos.y -= player->e->speed.y;
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

				main_state->distance += adjusted_dt;
			}

			if(player->e->use_gravity) {
				ASSERT(player->e->damp == 0.0f);
				player_dd_pos += main_state->entity_gravity;
			}

			player->e->d_pos += player_dd_pos * game_input->delta_time;
			//TODO: Don't apply damping to gravity!!
			player->e->d_pos *= (1.0f - player->e->damp);

			player->e->pos.xy += player->e->d_pos * game_input->delta_time;

			if(!player->dead) {
				f32 ground_height = main_state->ground_height + math::rec_dim(get_entity_collider_bounds(player->e)).y * 0.5f;
				if(player->e->pos.y <= ground_height) {
					player->e->pos.y = ground_height;
					player->e->d_pos.y = 0.0f;
					player->grounded = player->running;
				}
				else {
					player->grounded = false;
				}				
			}

			f32 max_dist_x = 20.0f;
			player->e->pos.x = math::clamp(player->e->pos.x, player->e->initial_x - max_dist_x, player->e->initial_x + max_dist_x);

			player->e->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
			player->e->asset_index = (u32)player->e->anim_time % get_asset_count(assets, player->e->asset_id);

			render_transform->offset = (math::rand_vec2() * 2.0f - 1.0f) * math::max(main_state->d_time - 2.0f, 0.0f);

			if(main_state->rocket_seq.playing) {
				play_rocket_sequence(game_state, meta_state, game_input->delta_time);
			}

			Location * current_location = main_state->locations + main_state->current_location;

#if 1
			render_state->letterboxed_height = render_state->screen_height - math::max(main_state->d_time - 1.0f, 0.0f) * 20.0f;
			render_state->letterboxed_height = math::max(render_state->letterboxed_height, render_state->screen_height / 3.0f);
#endif
			render_transform->pos.x = 0.0f;
#if 0
			render_transform->pos.y = math::max(player->e->pos.y, 0.0f);
			if(player->allow_input) {
				render_transform->pos.y = math::clamp(render_transform->pos.y, current_location->min_y, current_location->max_y);
			}
#else
			render_transform->pos.y = main_state->current_location * main_state->location_y_offset;
#endif

			math::Rec2 player_bounds = get_entity_collider_bounds(player->e);

			EntityEmitter * emitter = &main_state->entity_emitter;
			emitter->time_until_next_spawn -= adjusted_dt;
			if(emitter->time_until_next_spawn <= 0.0f) {
				if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
					Entity * entity = emitter->entity_array[emitter->entity_count++];

					entity->pos = emitter->pos;
					entity->pos.y += (math::rand_f32() * 2.0f - 1.0f) * 250.0f;
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

					entity->collider = get_entity_render_bounds(assets, entity);
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
					if(move_entity(meta_state, render_transform, entity, adjusted_dt)) {
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
				if(rec_overlap(player_bounds, bounds) && !player->dead && !entity->hit) {
					entity->hit = true;
					entity->scale *= 2.0f;

					AssetId clip_id = AssetId_pickup;

					switch(entity->asset_id) {
						case AssetId_dolly: {
							push_player_clone(player);
							main_state->score++;
							clip_id = AssetId_baa;

							break;
						}

						case AssetId_telly: {
							pop_player_clones(player, 5);
							clip_id = AssetId_explosion;
							
							break;
						}

						case AssetId_rocket: {
							begin_rocket_sequence(game_state, meta_state);
							break;
						}

						default: {
							main_state->score++;
							break;
						}
					}
					
					//TODO: Should there be a helper function for this??
					AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
					AudioSource * source = play_audio_clip(&game_state->audio_state, clip);
					change_pitch(source, math::lerp(0.9f, 1.1f, math::rand_f32()));
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
				Location * location = main_state->locations + i;
				for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers); layer_index++) {
					move_entity(meta_state, render_transform, location->layers[layer_index], adjusted_dt);
				}
			}

			game_state->save.high_score = math::max(game_state->save.high_score, main_state->score);
			game_state->save.longest_run = math::max(game_state->save.longest_run, main_state->distance);

			str_print(game_state->str, "SPEED: %f | SCORE: %u | CLONES: %u\n\n", main_state->d_time, main_state->score, main_state->player.active_clone_count);

			break;
		}

		case MetaStateType_game_over: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_game_over);
			GameOverMetaState * game_over_state = meta_state->game_over;

			render_state->letterboxed_height = render_state->screen_height;

			if(!game_state->transitioning) {
				if(game_input->buttons[ButtonId_start] & KEY_PRESSED || game_input->mouse_button & KEY_PRESSED) {
					game_over_state->transition_id = begin_transition(game_state);
					if(game_over_state->transition_id) {
						change_volume(game_over_state->music, math::vec2(0.0f), 1.0f / 1.5f);
					}
				}
			}
			else {
				if(transition_should_flip(game_state, game_over_state->transition_id)) {
					stop_audio_clip(meta_state->audio_state, game_over_state->music);

					game_state->save.plays++;
					change_meta_state(game_state, MetaStateType_main);					
				}
			}

			str_print(game_state->str, "\nGAMEOVER!\nPRESS SPACE TO RESTART\n\n");

			break;
		}

		INVALID_CASE();
	}

	if(game_state->transitioning) {
		ASSERT(!game_state->transition_flip);

		f32 new_transition_time = game_state->transition_time + game_input->delta_time;

		switch(game_state->transition_type) {
			case TransitionType_pixelate: {
				f32 new_pixelate_time = new_transition_time * 2.0f;
				if(render_state->pixelate_time < 1.0f && new_pixelate_time >= 1.0f) {
					game_state->transition_flip = true;
				}

				render_state->pixelate_time = new_pixelate_time;
				if(render_state->pixelate_time >= 2.0f) {
					render_state->pixelate_time = 0.0f;

					game_state->transitioning = false;
				}

				break;
			}

			case TransitionType_fade: {
				if(new_transition_time < 1.0f) {
					render_state->fade_amount = new_transition_time;
				}
				else {
					if(game_state->transition_time < 1.0f) {
						game_state->transition_flip = true;
					}

					render_state->fade_amount = 1.0f - (new_transition_time - 1.0f);
					if(new_transition_time > 2.0f) {
						game_state->transitioning = false;
						render_state->fade_amount = 0.0f;
					}
				}

				break;
			}

			INVALID_CASE();
		}

		if(!game_state->transitioning) {
			game_state->transition_time = 0.0f;
		}
		else {
			game_state->transition_time = new_transition_time;
		}
	}

	Font * debug_font = render_state->debug_font;
	debug_font->pos = debug_font->anchor;

	push_str(debug_font, game_state->str);
#if 0
	push_str(debug_font, game_state->debug_str);
#endif

	begin_render(render_state);

	switch(game_state->meta_state) {
		case MetaStateType_menu: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_menu);

			RenderTransform render_transform = create_render_transform(math::vec2(0.0f));

			EntityArray * entities = &meta_state->menu->entities;
			render_entities(render_state, &render_transform, entities->elems, entities->count);

			break;
		}

		case MetaStateType_main: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_main);
			MainMetaState * main_state = meta_state->main;

			EntityArray * entities = &main_state->entities;
			render_entities(render_state, &main_state->render_transform, entities->elems, entities->count);

			break;
		}

		case MetaStateType_game_over: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_game_over);

			RenderTransform render_transform = create_render_transform(math::vec2(0.0f));

			EntityArray * entities = &meta_state->game_over->entities;
			render_entities(render_state, &render_transform, entities->elems, entities->count);

			break;
		}

		INVALID_CASE();
	}

	end_render(render_state);
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

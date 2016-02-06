
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

math::Rec2 get_asset_bounds(AssetState * assets, AssetId asset_id, u32 asset_index, f32 scale = 1.0f) {
	Asset * asset = get_asset(assets, asset_id, asset_index);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);

	return math::rec2_pos_dim(asset->texture.offset, asset->texture.dim * scale);
}

math::Rec2 get_entity_render_bounds(AssetState * assets, Entity * entity) {
	return get_asset_bounds(assets, entity->asset_id, entity->asset_index, entity->scale);
}

math::Rec2 get_entity_collider_bounds(Entity * entity) {
	return math::rec_scale(math::rec_offset(entity->collider, entity->pos.xy), entity->scale);
}

void change_entity_asset(Entity * entity, AssetState * assets, AssetId asset_id, u32 asset_index) {
	//TODO: Should entities only be sprites??
	Asset * asset = get_asset(assets, asset_id, asset_index);
	ASSERT(asset);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);
	
	entity->asset_id = asset_id;
	entity->asset_index = asset_index;

	//TODO: Always do this??
	entity->collider = get_entity_render_bounds(assets, entity);
}

Entity * push_entity(EntityArray * entities, AssetState * assets, AssetId asset_id, u32 asset_index, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(entities->count < ARRAY_COUNT(entities->elems));

	Entity * entity = entities->elems + entities->count++;
	entity->pos = pos;
	entity->scale = 1.0f;
	entity->color = math::vec4(1.0f);

	change_entity_asset(entity, assets, asset_id, asset_index);

	entity->d_pos = math::vec2(0.0f);
	entity->speed = math::vec2(500.0f);
	entity->damp = 0.0f;
	entity->use_gravity = false;

	entity->anim_time = 0.0f;
	entity->initial_x = pos.x;
	entity->hit = false;

	return entity;
}

UiElement * pick_ui_elem(RenderState * render_state, RenderGroup * render_group, UiElement * elems, u32 elem_count, math::Vec2 raw_mouse_pos) {
	math::Vec2 screen_dim = math::vec2(render_group->transform.projection_width, render_group->transform.projection_height);
	math::Vec2 buffer_dim = math::vec2(render_state->back_buffer_width, render_state->back_buffer_height);
	math::Vec2 mouse_pos = raw_mouse_pos * (screen_dim / buffer_dim) - screen_dim * 0.5f;
	mouse_pos.y = -mouse_pos.y;

	//TODO: Should this just return the index in the array??
	UiElement * hot_elem = 0;
	for(u32 i = 0; i < elem_count; i++) {
		UiElement * elem = elems + i;
		elem->asset_index = 0;

		math::Rec2 bounds = get_asset_bounds(render_group->assets, elem->asset_id, elem->asset_index);
		if(math::inside_rec(bounds, mouse_pos)) {
			hot_elem = elem;
		}
	}

	return hot_elem;
}

void kill_player(MainMetaState * main_state, Player * player) {
	if(!player->dead && player->allow_input) {
		player->dead = true;
		player->allow_input = false;

		player->e->damp = 0.0f;
		player->e->use_gravity = true;
		player->e->d_pos = math::vec2(0.0f, 1000.0f);

		main_state->accel_time = 0.0f;
	}
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
}

b32 change_location(MainMetaState * main_state, LocationId location_id) {
	ASSERT(location_id != LocationId_null);

	b32 changed = false;

	if(main_state->current_location != location_id && main_state->next_location == LocationId_null) {
		main_state->next_location = location_id;
		changed = true;
	}
	
	return changed;
}

b32 begin_location_transition(GameState * game_state, MainMetaState * main_state, LocationId location_id) {
	b32 started = false;

	if(!game_state->transitioning) {
		if(change_location(main_state, location_id)) {
			ASSERT(main_state->location_transition_id == 0);
			main_state->location_transition_id = begin_transition(game_state);
			ASSERT(main_state->location_transition_id);

			started = true;
		}		
	}

	return started;
}

void begin_rocket_sequence(GameState * game_state, MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	RocketSequence * seq = &main_state->rocket_seq;

	if(!seq->playing) {
		seq->playing = true;
		seq->time_ = 0.0f;

		math::Rec2 render_bounds = get_entity_render_bounds(meta_state->assets, seq->rocket);
		f32 height = math::rec_dim(render_bounds).y;

		Entity * rocket = seq->rocket;
		rocket->pos = math::vec3(main_state->player.e->pos.x, -(meta_state->render_state->screen_height * 0.5f + height * 0.5f), 0.0f);
		rocket->d_pos = math::vec2(0.0f);
		rocket->speed = math::vec2(0.0f, 10000.0f);
		rocket->damp = 0.065f;
	}
}

void play_rocket_sequence(GameState * game_state, MetaState * meta_state, f32 dt) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	RocketSequence * seq = &main_state->rocket_seq;

	if(seq->playing) {
		RenderState * render_state = meta_state->render_state;
		Player * player = &main_state->player;
		Entity * rocket = seq->rocket;

		rocket->d_pos += rocket->speed * dt;
		rocket->d_pos *= (1.0f - rocket->damp);

		math::Vec2 d_pos = rocket->d_pos * dt;
		math::Vec2 new_pos = rocket->pos.xy + d_pos;

		if(!player->dead) {
			f32 new_time = seq->time_ + dt;

			Location * space_location = main_state->locations + LocationId_space;
			if(rocket->pos.y < space_location->y) {
				main_state->render_group->transform.pos.y = math::max(main_state->rocket_seq.rocket->pos.y, 0.0f);

				if(new_pos.y < space_location->y) {
					math::Rec2 player_bounds = get_entity_collider_bounds(player->e);
					math::Rec2 collider_bounds = get_entity_collider_bounds(rocket);
					if(math::rec_overlap(collider_bounds, player_bounds)) {
						player->e->d_pos = math::vec2(0.0f);
						player->e->pos.xy += d_pos;
						player->allow_input = false;
						main_state->accel_time = 0.0f;
					}

					main_state->render_group->transform.offset = (math::rand_vec2() * 2.0f - 1.0f) * 10.0f;
				}
				else {
					player->allow_input = true;

					ASSERT(main_state->next_location == LocationId_null);
					b32 changed = change_location(main_state, LocationId_space);
					//TODO: What happens if we can't change the location yet??
					ASSERT(changed);
				}

				new_time = 0.0f;
			}

			f32 seq_max_time = 15.0f;
			if(seq->time_ < seq_max_time) {
				if(new_time > seq_max_time && main_state->next_location == LocationId_null) {
					LocationId last_location = main_state->last_location;
					if(last_location == LocationId_null) {
						last_location = LocationId_city;
					}
					
					//TODO: Properly handle the case where this doesn't succeed!!
					begin_location_transition(game_state, main_state, last_location);
				}
			}
			else {
				if(main_state->next_location == LocationId_null) {
					seq->playing = false;
				}
			}

			seq->time_ = new_time;
		}

		rocket->pos.xy = new_pos;
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

	size_t arena_size = KILOBYTES(64);

	meta_state->type = type;
	switch(type) {
		case MetaStateType_menu: {
			meta_state->menu = PUSH_STRUCT(arena, MenuMetaState);
			break;
		}

		case MetaStateType_intro: {
			meta_state->intro = PUSH_STRUCT(arena, IntroMetaState);
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

	RenderState * render_state = meta_state->render_state;
	menu_state->render_group = allocate_render_group(render_state, &meta_state->arena, render_state->screen_width, render_state->screen_height);

	menu_state->buttons[MenuButtonId_credits].asset_id = AssetId_menu_btn_credits;
	menu_state->buttons[MenuButtonId_score].asset_id = AssetId_menu_btn_score;
	menu_state->buttons[MenuButtonId_play].asset_id = AssetId_menu_btn_play;
	for(u32 i = 0; i < ARRAY_COUNT(menu_state->buttons); i++) {
		ASSERT(menu_state->buttons[i].asset_id);
	}
}

void init_intro_meta_state(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_intro);
	IntroMetaState * intro_state = meta_state->intro;

	zero_memory_arena(&meta_state->arena);
	zero_memory(intro_state, sizeof(IntroMetaState));

	RenderState * render_state = meta_state->render_state;
	intro_state->render_group = allocate_render_group(render_state, &meta_state->arena, render_state->screen_width, render_state->screen_height);

	for(u32 i = 0; i < ARRAY_COUNT(intro_state->frames); i++) {
		IntroFrame * frame = intro_state->frames + i;
		frame->alpha = 0.0f;
	}
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

	main_state->music = play_audio_clip(meta_state->audio_state, AssetId_game_music, true);
	change_volume(main_state->music, math::vec2(0.0f), 0.0f);
	change_volume(main_state->music, math::vec2(1.0f), 1.0f);

	main_state->render_group = allocate_render_group(meta_state->render_state, &meta_state->arena, screen_width, screen_height);
	main_state->ui_render_group = allocate_render_group(meta_state->render_state, &meta_state->arena, screen_width, screen_height);
	main_state->letterboxed_height = (f32)screen_height;
	main_state->font = allocate_font(meta_state->render_state, 65536, screen_width, screen_height);

	EntityArray * entities = &main_state->entities;

	main_state->entity_gravity = math::vec2(0.0f, -6000.0f);

	main_state->current_location = LocationId_city;
	main_state->next_location = LocationId_null;
	main_state->last_location = LocationId_null;
	main_state->location_transition_id = 0;

	f32 layer_z_offsets[PARALLAX_LAYER_COUNT] = {
		 10.0f,
		 10.0f,
		  7.5f,
		  5.0f,
		 -0.5f,
	};

	f32 location_max_z = layer_z_offsets[0];
	f32 location_y_offset = (f32)screen_height / (1.0f / (location_max_z + 1.0f));

	main_state->locations[LocationId_city].y = 0.0f;
	main_state->locations[LocationId_city].asset_id = AssetId_city;

	main_state->locations[LocationId_mountains].y = location_y_offset * 2;
	main_state->locations[LocationId_mountains].asset_id = AssetId_city;

	main_state->locations[LocationId_space].y = location_y_offset;
	main_state->locations[LocationId_space].asset_id = AssetId_space;

	main_state->ground_height = -(f32)screen_height * 0.5f;

	AssetId first_location_asset_id = AssetId_city;
	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = main_state->locations + i;
	
		for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers) - 1; layer_index++) {
			Entity * entity = push_entity(entities, meta_state->assets, location->asset_id, layer_index);

			entity->pos.y = location->y;
			entity->pos.z = layer_z_offsets[layer_index];
			entity->scrollable = true;

			location->layers[layer_index] = entity;
		}
	}

	//TODO: Collapse this!!
	{
		//TODO: We probably want to have a probability tree!!
		AssetId emitter_types[] = {
			AssetId_rocket,
			// AssetId_boots,

			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,
			AssetId_collectable_telly,

			AssetId_collectable_blob,
			AssetId_collectable_clock,
			AssetId_collectable_diamond,
			AssetId_collectable_flower,
			AssetId_collectable_heart,
			AssetId_collectable_paw,
			AssetId_collectable_smiley,
			AssetId_collectable_speech,
			AssetId_collectable_speed_up,
		};

		main_state->locations[LocationId_city].emitter_type_count = ARRAY_COUNT(emitter_types);
		main_state->locations[LocationId_city].emitter_types = PUSH_COPY_ARRAY(&meta_state->arena, AssetId, emitter_types, ARRAY_COUNT(emitter_types));

		main_state->locations[LocationId_mountains].emitter_type_count = ARRAY_COUNT(emitter_types);
		main_state->locations[LocationId_mountains].emitter_types = PUSH_COPY_ARRAY(&meta_state->arena, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
	}

	{
		AssetId emitter_types[] = {
			AssetId_collectable_clock,

			AssetId_dolly,
			AssetId_dolly,
			AssetId_dolly,
			AssetId_dolly,
			AssetId_dolly,
			AssetId_dolly,
			AssetId_dolly,
		};

		main_state->locations[LocationId_space].emitter_type_count = ARRAY_COUNT(emitter_types);
		main_state->locations[LocationId_space].emitter_types = PUSH_COPY_ARRAY(&meta_state->arena, AssetId, emitter_types, ARRAY_COUNT(emitter_types));
	}

	Player * player = &main_state->player;
	player->sheild = push_entity(entities, assets, AssetId_shield, 0);
	player->sheild->color.a = 0.0f;
	player->clone_offset = math::vec2(-5.0f, 0.0f);

	for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
		Entity * entity = push_entity(entities, assets, AssetId_dolly, 0, ENTITY_NULL_POS);
		entity->scale = 0.75f;
		entity->color.a = 0.0f;

		entity->rand_id = math::rand_u32();
		entity->hidden = true;

		player->clones[i] = entity;
	}

	player->e = push_entity(entities, assets, AssetId_dolly, 0);
	player->e->pos = math::vec3((f32)screen_width * -0.25f, 0.0f, 0.0f);
	player->e->initial_x = player->e->pos.x;
	player->e->speed = math::vec2(50.0f, 6000.0f);
	player->e->damp = 0.15f;
	player->allow_input = true;

	EntityEmitter * emitter = &main_state->entity_emitter;
	emitter->pos = math::vec3(screen_width * 0.75f, 0.0f, 0.0f);
	emitter->time_until_next_spawn = 0.0f;
	emitter->entity_count = 0;
	for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
		emitter->entity_array[i] = push_entity(entities, assets, AssetId_first_collectable, 0, emitter->pos);
	}

	RocketSequence * seq = &main_state->rocket_seq;
	seq->rocket = push_entity(entities, assets, AssetId_large_rocket, 0, ENTITY_NULL_POS);
	seq->rocket->collider = math::rec_offset(seq->rocket->collider, math::vec2(0.0f, -math::rec_dim(seq->rocket->collider).y * 0.5f));
	seq->playing = false;
	seq->time_ = 0.0f;

#if 0
	//TODO: Support arbitary sized maze chunks??
	f32 maze_chunk_width = 64.0f;
	main_state->maze_chunk_count = get_asset_count(assets, AssetId_maze_top);
	ASSERT(main_state->maze_chunk_count == get_asset_count(assets, AssetId_maze_bottom));
	main_state->maze_chunks = PUSH_ARRAY(&meta_state->arena, MazeChunk, main_state->maze_chunk_count);
	f32 x = -(f32)render_state->screen_width;
	for(u32 i = 0; i < main_state->maze_chunk_count; i++) {
		MazeChunk * chunk = main_state->maze_chunks + i;

		math::Vec3 pos = math::vec3(x, 0.0f, 0.0f);
		chunk->top = push_entity(entities, assets, AssetId_maze_top, i, pos);
		chunk->bottom = push_entity(entities, assets, AssetId_maze_bottom, i, pos);

		x += maze_chunk_width;
	}
#endif

	//TODO: Adding foreground layer at the end until we have sorting!!
	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = main_state->locations + i;

		u32 layer_index = ARRAY_COUNT(location->layers) - 1;

		Entity * entity = push_entity(entities, assets, location->asset_id, layer_index);
		entity->pos.y = location->y;
		entity->pos.z = layer_z_offsets[layer_index];
		entity->scrollable = true;

		location->layers[layer_index] = entity;
	}

	main_state->d_speed = 0.0f;
	main_state->dd_speed = 0.0f;

	main_state->start_time = 30.0f;
	main_state->max_time = main_state->start_time;
	main_state->countdown_time = 10.0f;
	main_state->time_remaining = main_state->start_time;
	main_state->clock_scale = 1.0f;

	main_state->score_buttons[ScoreButtonId_menu].asset_id = AssetId_score_btn_menu;
	main_state->score_buttons[ScoreButtonId_replay].asset_id = AssetId_score_btn_replay;
	for(u32 i = 0; i < ARRAY_COUNT(main_state->score_buttons); i++) {
		ASSERT(main_state->score_buttons[i].asset_id);
	}
	main_state->score_str = allocate_str(&meta_state->arena, 256);
	main_state->score_values[ScoreValueId_time_played].is_f32 = true;
	main_state->score_values[ScoreValueId_points].is_f32 = false;
}

void init_game_over_meta_state(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_game_over);
	GameOverMetaState * game_over_state = meta_state->game_over;

	zero_memory_arena(&meta_state->arena);
	zero_memory(game_over_state, sizeof(GameOverMetaState));

	AssetState * assets = meta_state->assets;

	// game_over_state->music = play_audio_clip(meta_state->audio_state, AssetId_death_music, true);
	change_volume(game_over_state->music, math::vec2(0.0f), 0.0f);
	change_volume(game_over_state->music, math::vec2(1.0f), 1.0f);

	RenderState * render_state = meta_state->render_state;
	game_over_state->render_group = allocate_render_group(render_state, &meta_state->arena, render_state->screen_width, render_state->screen_height);
}

void change_meta_state(GameState * game_state, MetaStateType type) {
	// ASSERT(game_state->meta_state != type);
	game_state->meta_state = type;

	MetaState * meta_state = get_meta_state(game_state, type);
	switch(type) {
		case MetaStateType_menu: {
			init_menu_meta_state(meta_state);
			break;
		}

		case MetaStateType_intro: {
			init_intro_meta_state(meta_state);
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

					for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collectable_unlock_states); i++) {
						if(header->collectable_unlock_states[i]) {
							game_state->save.collectable_unlock_states[i] = true;
						}
					}
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
	// str_print(game_state->str, "DOLLY DOLLY DOLLY DAYS!\nDT: %f\n\n", game_input->delta_time);
	// str_print(game_state->str, "PLAYS: %u | HIGH_SCORE: %u | LONGEST_RUN: %.2f\n\n", game_state->save.plays, game_state->save.high_score, game_state->save.longest_run);

#if 0
	for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collectable_unlock_states); i++){
		b32 unlocked = game_state->save.collectable_unlock_states[i];

		char * unlocked_str = (char *)"UNLOCKED";
		char * locked_str = (char *)"LOCKED";

		str_print(game_state->str, "[%u]: %s\n", i, unlocked ? unlocked_str : locked_str);
	}
	str_print(game_state->str, "\n");
#endif

	if(game_input->buttons[ButtonId_debug] & KEY_PRESSED) {
		render_state->debug_render_entity_bounds = !render_state->debug_render_entity_bounds;
	}

	if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
		game_state->audio_state.master_volume = game_state->audio_state.master_volume > 0.0f ? 0.0f : 1.0f;
	}

	switch(game_state->meta_state) {
		case MetaStateType_menu: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_menu);
			MenuMetaState * menu_state = meta_state->menu;

			if(!game_state->transitioning) {
				UiElement * hot_elem = pick_ui_elem(meta_state->render_state, menu_state->render_group, menu_state->buttons, ARRAY_COUNT(menu_state->buttons), game_input->mouse_pos);
				if(hot_elem) {
					hot_elem->asset_index = 1;

					if(game_input->mouse_button & KEY_PRESSED) {
						if(hot_elem == &menu_state->buttons[MenuButtonId_play]) {
							menu_state->transition_id = begin_transition(game_state);
						}
					}
				}
			}
			else {
				if(transition_should_flip(game_state, menu_state->transition_id)) {
					change_meta_state(game_state, MetaStateType_intro);
				}
			}

			break;
		}

		case MetaStateType_intro: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_intro);
			IntroMetaState * intro_state = meta_state->intro;

			//TODO: Collapse this!!
			if(!game_state->transitioning) {
				f32 frame_index = (u32)intro_state->anim_time + 1;
				intro_state->anim_time += game_input->delta_time * 1.5f;
				f32 fade_dt = game_input->delta_time * 8.0f;

				u32 last_frame_index = ARRAY_COUNT(intro_state->frames) - 1;
				if(frame_index <= last_frame_index) {
					for(u32 i = 0; i < math::min(frame_index, last_frame_index); i++) {
						IntroFrame * frame = intro_state->frames + i;
						frame->alpha += fade_dt;
						frame->alpha = math::clamp01(frame->alpha);
					}
				}
				else {
					for(u32 i = 0; i < last_frame_index; i++) {
						IntroFrame * frame = intro_state->frames + i;
						frame->alpha -= fade_dt;
						frame->alpha = math::clamp01(frame->alpha);
					}

					if(frame_index > ARRAY_COUNT(intro_state->frames)) {
						IntroFrame * frame = intro_state->frames + last_frame_index;
						frame->alpha += fade_dt;
						frame->alpha = math::clamp01(frame->alpha);
					}

					if(frame_index > ARRAY_COUNT(intro_state->frames) + 2) {
						intro_state->transition_id = begin_transition(game_state);
					}
				}
			}
			else {
				if(transition_should_flip(game_state, intro_state->transition_id)) {
					change_meta_state(game_state, MetaStateType_main);
				}
			}

			break;
		}

		case MetaStateType_main: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_main);
			MainMetaState * main_state = meta_state->main;

			RenderTransform * render_transform = &main_state->render_group->transform;

			Player * player = &main_state->player;

			if(!game_state->transitioning) {
				if(game_input->buttons[ButtonId_quit] & KEY_PRESSED) {
					main_state->quit_transition_id = begin_transition(game_state, TransitionType_pixelate);
				}
			}
			else {
				if(main_state->quit_transition_id || main_state->restart_transition_id || main_state->death_transition_id) {
					if(main_state->music) {
						fade_out_audio_clip(main_state->music, 1.0f);
						main_state->music = 0;
					}
				}

				MetaStateType new_state = MetaStateType_null;
				if(transition_should_flip(game_state, main_state->quit_transition_id)) {
					new_state = MetaStateType_menu;
				}
				else if(transition_should_flip(game_state, main_state->death_transition_id)) {
					new_state = MetaStateType_game_over;
				}
				else if(transition_should_flip(game_state, main_state->restart_transition_id)) {
					new_state = MetaStateType_main;
				}

				if(new_state != MetaStateType_null) {
					if(main_state->tick_tock) {
						stop_audio_clip(meta_state->audio_state, main_state->tick_tock);
						main_state->tick_tock = 0;
					}

					game_state->time_until_next_save = 0.0f;

					change_meta_state(game_state, new_state);
				}
			}

			main_state->clock_scale = math::lerp(main_state->clock_scale, 1.0f, game_input->delta_time * 12.0f);

			if(!player->dead) {
				f32 dt = game_input->delta_time;
				f32 new_time_remaining = main_state->time_remaining - dt;
				if(new_time_remaining < 0.0f) {
					dt = -new_time_remaining;
					new_time_remaining = 0.0f;

					kill_player(main_state, player);
				}
				else if(new_time_remaining <= main_state->countdown_time && main_state->time_remaining > main_state->countdown_time) {
					ASSERT(!main_state->tick_tock);
					main_state->tick_tock = play_audio_clip(&game_state->audio_state, get_audio_clip_asset(assets, AssetId_tick_tock, 0));
				}

				main_state->time_remaining = new_time_remaining;
				main_state->score_values[ScoreValueId_time_played].f32_ += dt;
			}
			else {
				if(player->e->pos.y <= render_transform->pos.y - meta_state->render_state->screen_height * 0.5f) {
					main_state->show_score_overlay = true;
				}
			}

			{
				main_state->accel_time -= game_input->delta_time;
				if(main_state->accel_time <= 0.0f) {
					main_state->accel_time = 0.0f;
					main_state->dd_speed = 0.0f;
				}

				f32 damp = 0.1f;
				main_state->d_speed += main_state->dd_speed * game_input->delta_time;
				main_state->d_speed *= (1.0f - damp);

				f32 pitch = math::max(main_state->d_speed + 1.0f, 0.5f);
				if(pitch >= 1.0f) {
					pitch = 1.0f + (pitch - 1.0f) * 0.02f;
				}

				change_pitch(main_state->music, pitch);				
			}

			f32 zero_speed = 1.5f;
			f32 d_time = math::clamp(main_state->d_speed + zero_speed, 0.5f, 4.0f);
			f32 adjusted_dt = d_time * game_input->delta_time;

			main_state->letterboxed_height = render_state->screen_height - math::max(main_state->d_speed, 0.0f) * 40.0f;
			main_state->letterboxed_height = math::max(main_state->letterboxed_height, render_state->screen_height / 3.0f);

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
				entity->pos.y += math::sin((game_input->total_time * 0.25f + i * (3.0f / 13.0f)) * math::TAU) * 50.0f;

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

				entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
				entity->asset_index = ((u32)entity->anim_time + entity->rand_id) % get_asset_count(assets, entity->asset_id);
			}

			math::Vec2 player_dd_pos = math::vec2(0.0f);

			if(player->allow_input) {
				f32 half_buffer_height = (f32)game_input->back_buffer_height * 0.5f;
				
				if(game_input->buttons[ButtonId_up] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y < half_buffer_height)) {
					player_dd_pos.y += player->e->speed.y;
				}

				if(game_input->buttons[ButtonId_down] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y >= half_buffer_height)) {
					player_dd_pos.y -= player->e->speed.y;
				}
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
				}
			}

			player->e->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
			player->e->asset_index = (u32)player->e->anim_time % get_asset_count(assets, player->e->asset_id);

			player->sheild->pos = player->e->pos;

			render_transform->pos.x = 0.0f;
			render_transform->pos.y = main_state->locations[main_state->current_location].y;
			render_transform->offset = (math::rand_vec2() * 2.0f - 1.0f) * math::max(main_state->d_speed, 0.0f);

			if(main_state->rocket_seq.playing) {
				play_rocket_sequence(game_state, meta_state, game_input->delta_time);
			}

			if(main_state->next_location != LocationId_null) {
				if(transition_should_flip(game_state, main_state->location_transition_id) || !main_state->location_transition_id) {
					main_state->location_transition_id = 0;

					main_state->last_location = main_state->current_location;
					main_state->current_location = main_state->next_location;
					main_state->next_location = LocationId_null;

					Location * location = main_state->locations + main_state->current_location;
					Player * player = &main_state->player;

					player->e->d_pos = math::vec2(0.0f);
					player->e->pos.xy = math::vec2(player->e->initial_x, location->y);

					main_state->entity_emitter.pos.y = location->y;
					main_state->entity_emitter.time_until_next_spawn = 0.0f;
					main_state->accel_time = 0.0f;

					render_transform->pos.y = location->y;
				}
			}

			Location * current_location = main_state->locations + main_state->current_location;

			math::Rec2 player_bounds = get_entity_collider_bounds(player->e);

			EntityEmitter * emitter = &main_state->entity_emitter;
			emitter->time_until_next_spawn -= adjusted_dt;
			if(!player->dead && emitter->time_until_next_spawn <= 0.0f) {
				if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
					Entity * entity = emitter->entity_array[emitter->entity_count++];

					entity->pos = emitter->pos;
					entity->pos.y += (math::rand_f32() * 2.0f - 1.0f) * 250.0f;
					entity->scale = 1.0f;
					entity->color = math::vec4(1.0f);

					u32 type_index = (u32)(math::rand_f32() * current_location->emitter_type_count);
					ASSERT(type_index < current_location->emitter_type_count);
					AssetId asset_id = current_location->emitter_types[type_index];

					change_entity_asset(entity, assets, asset_id, 0);
					if(entity->asset_id == AssetId_collectable_telly) {
						entity->collider = math::rec_scale(entity->collider, 0.5f);
					}

					entity->hit = false;
				}

				emitter->time_until_next_spawn = 0.5f;
			}

			for(u32 i = 0; i < emitter->entity_count; i++) {
				Entity * entity = emitter->entity_array[i];
				b32 destroy = false;

				if(move_entity(meta_state, render_transform, entity, adjusted_dt)) {
					destroy = true;
				}

				if(entity->hit) {
					f32 anim_speed = 4.0f;

					entity->scale += anim_speed * game_input->delta_time;
					if(entity->scale > 1.0f) {
						entity->scale= 1.0f;
					}

					entity->color.a -= anim_speed * game_input->delta_time;
					if(entity->color.a < 0.0f) {
						entity->color.a = 0.0f;
						destroy = true;
					}					
				}

				entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
				entity->asset_index = (u32)entity->anim_time % get_asset_count(assets, entity->asset_id);

				math::Rec2 bounds = get_entity_collider_bounds(entity);
				//TODO: When should this check happen??
				if(rec_overlap(player_bounds, bounds) && !player->dead && !entity->hit) {
					entity->hit = true;

					AssetId clip_id = AssetId_pickup;
					b32 is_slow_down = false;

					switch(entity->asset_id) {
						case AssetId_dolly: {
							push_player_clone(player);
							main_state->score_values[ScoreValueId_points].u32_++;
							clip_id = AssetId_baa;

							break;
						}

						case AssetId_collectable_telly: {
							clip_id = AssetId_explosion;
							
							if(main_state->dd_speed <= 0.0f) {
								pop_player_clones(player, 5);
								is_slow_down = true;
							}
							
							break;
						}

						case AssetId_rocket: {
							begin_rocket_sequence(game_state, meta_state);
							break;
						}

						case AssetId_collectable_clock: {
							main_state->time_remaining += main_state->countdown_time;
							main_state->time_remaining = math::min(main_state->time_remaining, main_state->max_time);
							stop_audio_clip(meta_state->audio_state, main_state->tick_tock);
							main_state->tick_tock = 0;

							main_state->clock_scale = 1.25f;

							break;
						}

						case AssetId_collectable_speed_up: {
							if(main_state->accel_time == 0.0f) {
								main_state->dd_speed = 40.0f;
								main_state->accel_time = 5.0f;								
							}
							else if(main_state->dd_speed < 0.0f) {
								main_state->accel_time = 0.0f;
							}

							break;
						}

						case AssetId_boots: {
							begin_location_transition(game_state, main_state, LocationId_mountains);
							break;
						}

						case AssetId_collectable_speech: {
							begin_location_transition(game_state, main_state, LocationId_city);
							break;
						}

						default: {
							main_state->score_values[ScoreValueId_points].u32_++;
							break;
						}
					}

					if(is_slow_down && main_state->accel_time == 0.0f) {
						main_state->dd_speed = -40.0f;
						main_state->accel_time = 1.5f;
					}

					if(entity->asset_id >= AssetId_first_collectable && entity->asset_id < AssetId_one_past_last_collectable) {
						u32 collectable_id = entity->asset_id - AssetId_first_collectable;
						ASSERT(collectable_id < ARRAY_COUNT(game_state->save.collectable_unlock_states));
						game_state->save.collectable_unlock_states[collectable_id] = true;
						game_state->time_until_next_save = 0.0f;
					}
					
					//TODO: Should there be a helper function for this??
					AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
					f32 pitch = math::lerp(0.9f, 1.1f, math::rand_f32());
					fire_audio_clip(meta_state->audio_state, clip, math::vec2(1.0f), pitch);

					change_entity_asset(entity, assets, AssetId_shield, 0);
					entity->scale *= 0.5f;
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

			if(main_state->show_score_overlay) {
				UiElement * hot_elem = pick_ui_elem(render_state, main_state->ui_render_group, main_state->score_buttons, ARRAY_COUNT(main_state->score_buttons), game_input->mouse_pos);
				if(hot_elem) {
					hot_elem->asset_index = 1;

					if(game_input->mouse_button & KEY_PRESSED) {
						if(hot_elem == &main_state->score_buttons[ScoreButtonId_menu]) {
							main_state->quit_transition_id = begin_transition(game_state);
						}
						else if(hot_elem == &main_state->score_buttons[ScoreButtonId_replay]) {
							main_state->restart_transition_id = begin_transition(game_state);
						}
					}
				}

				if(main_state->score_value_index < ARRAY_COUNT(main_state->score_values)) {
					ScoreValue * value = main_state->score_values + main_state->score_value_index;

					f32 to = value->is_f32 ? value->f32_ : (f32)value->u32_;
					value->display = math::lerp(0.0f, to, math::clamp01(value->time_));
					value->time_ += game_input->delta_time;

					if(value->time_ > 2.0f) {
						main_state->score_value_index++;
					}
				}

				str_clear(main_state->score_str);
				str_print(main_state->score_str, "SCORE\n\n");
				str_print(main_state->score_str, "TIME PLAYED: %.2f\n", main_state->score_values[ScoreValueId_time_played].display);
				str_print(main_state->score_str, "POINTS: %u\n", (u32)main_state->score_values[ScoreValueId_points].display);
			}

			game_state->save.high_score = math::max(game_state->save.high_score, main_state->score_values[ScoreValueId_points].u32_);
			game_state->save.longest_run = math::max(game_state->save.longest_run, main_state->score_values[ScoreValueId_time_played].f32_);

			break;
		}

		case MetaStateType_game_over: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_game_over);
			GameOverMetaState * game_over_state = meta_state->game_over;

			if(!game_state->transitioning) {
				if(game_input->buttons[ButtonId_start] & KEY_PRESSED || game_input->mouse_button & KEY_PRESSED) {
					game_over_state->transition_id = begin_transition(game_state);
					fade_out_audio_clip(game_over_state->music, 1.0f);
					game_over_state->music = 0;
				}
			}
			else {
				if(transition_should_flip(game_state, game_over_state->transition_id)) {
					game_state->save.plays++;
					change_meta_state(game_state, MetaStateType_main);					
				}
			}

			str_print(game_state->str, "GAMEOVER!\nPRESS SPACE TO RESTART\n\n");

			break;
		}

		INVALID_CASE();
	}

	if(game_state->transitioning) {
		ASSERT(!game_state->transition_flip);

		f32 new_transition_time = game_state->transition_time + game_input->delta_time;

		switch(game_state->transition_type) {
			case TransitionType_pixelate: {
				f32 new_pixelate_time = new_transition_time * 1.5f;
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

	begin_render(render_state);

	switch(game_state->meta_state) {
		case MetaStateType_menu: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_menu);
			MenuMetaState * menu_state = meta_state->menu;

			push_textured_quad(menu_state->render_group, AssetId_menu_background, 0);

			u32 display_item_count = ASSET_GROUP_COUNT(display);
			for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collectable_unlock_states); i++) {
				if(game_state->save.collectable_unlock_states[i]) {
					push_textured_quad(menu_state->render_group, (AssetId)(AssetId_first_display + i), 0);
				}
			}

			for(u32 i = 0; i < ARRAY_COUNT(menu_state->buttons); i++) {
				UiElement * elem = menu_state->buttons + i;
				push_textured_quad(menu_state->render_group, elem->asset_id, elem->asset_index);
			}

			render_and_clear_render_group(meta_state->render_state, menu_state->render_group);

			break;
		}

		case MetaStateType_intro: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_intro);
			IntroMetaState * intro_state = meta_state->intro;

			RenderGroup * render_group = intro_state->render_group;

			for(u32 i = 0; i < ARRAY_COUNT(intro_state->frames); i++) {
				IntroFrame * frame = intro_state->frames + i;
				push_textured_quad(render_group, AssetId_intro, i, math::vec3(0.0f), 1.0f, math::vec4(1.0f, 1.0f, 1.0f, frame->alpha));
			}

			render_and_clear_render_group(meta_state->render_state, render_group);

			break;
		}

		case MetaStateType_main: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_main);
			MainMetaState * main_state = meta_state->main;

			EntityArray * entities = &main_state->entities;
			for(u32 i = 0; i < entities->count; i++) {
				Entity * entity = entities->elems + i;
				push_textured_quad(main_state->render_group, entity->asset_id, entity->asset_index, entity->pos, entity->scale, entity->color, entity->scrollable);
			}

			render_and_clear_render_group(meta_state->render_state, main_state->render_group);

			RenderGroup * ui_render_group = main_state->ui_render_group;
			math::Vec2 projection_dim = math::vec2(ui_render_group->transform.projection_width, ui_render_group->transform.projection_height);

			f32 letterbox_pixels = (projection_dim.y - main_state->letterboxed_height) * 0.5f;
			if(letterbox_pixels > 0.0f) {
				push_colored_quad(ui_render_group, math::vec3(0.0f, projection_dim.y - letterbox_pixels, 0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
				push_colored_quad(ui_render_group, math::vec3(0.0f,-projection_dim.y + letterbox_pixels, 0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			}

			if(main_state->show_score_overlay) {
				push_colored_quad(ui_render_group, math::vec3(0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 0.5f));
				push_textured_quad(ui_render_group, AssetId_score_background, 0);

				for(u32 i = 0; i < ARRAY_COUNT(main_state->score_buttons); i++) {
					UiElement * elem = main_state->score_buttons + i;
					push_textured_quad(ui_render_group, elem->asset_id, elem->asset_index);
				}
			}

			render_and_clear_render_group(meta_state->render_state, ui_render_group);

			f32 font_scale = 8.0f;

			{
				char time_str_buf[256];
				Str time_str = str_fixed_size(time_str_buf, ARRAY_COUNT(time_str_buf));
				str_print(&time_str, "TIME REMAINING: %.2f", main_state->time_remaining);

				f32 scale = font_scale * main_state->clock_scale;
				f32 render_width = get_str_render_width(main_state->font, scale, &time_str);
				FontLayout font_layout = create_font_layout(main_state->font, projection_dim, scale, FontLayoutAnchor_top_centre, math::vec2(-render_width * 0.5f, 0.0f));
				push_str_to_batch(main_state->font, &font_layout, &time_str);
			}

			{
				char * location_str = 0;
				switch(main_state->current_location) {
					case LocationId_city: {
						location_str = (char *)"CITY";
						break;
					}

					case LocationId_mountains: {
						location_str = (char *)"MOUNTAINS";
						break;
					}

					case LocationId_space: {
						location_str = (char *)"SPACE";
						break;
					}

					INVALID_CASE();
				}

				FontLayout font_layout = create_font_layout(main_state->font, projection_dim, font_scale, FontLayoutAnchor_bottom_left);
				push_c_str_to_batch(main_state->font, &font_layout, location_str);
			}

			{
				math::Vec2 font_layout_dim = math::rec_dim(get_asset_bounds(meta_state->assets, AssetId_score_background, 0));
				FontLayout font_layout = create_font_layout(main_state->font, font_layout_dim, font_scale, FontLayoutAnchor_top_left, math::vec2(24.0f, -24.0f));
				push_str_to_batch(main_state->font, &font_layout, main_state->score_str);
			}

			render_font(render_state, main_state->font, &ui_render_group->transform);

			break;
		}

		case MetaStateType_game_over: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_game_over);
			GameOverMetaState * game_over_state = meta_state->game_over;

			push_textured_quad(game_over_state->render_group, AssetId_car, 0);
			render_and_clear_render_group(meta_state->render_state, game_over_state->render_group);

			break;
		}

		INVALID_CASE();
	}

	Font * debug_font = render_state->debug_font;
	FontLayout debug_font_layout = create_font_layout(debug_font, math::vec2(render_state->back_buffer_width, render_state->back_buffer_height), 4.0f, FontLayoutAnchor_top_left);
	push_str_to_batch(debug_font, &debug_font_layout, game_state->str);
#if 0
	push_str_to_batch(debug_font, &debug_font_layout, game_state->debug_str);
#endif

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

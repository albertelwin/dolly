
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

math::Rec2 get_asset_bounds(AssetState * assets, AssetId asset_id, u32 asset_index, math::Vec2 scale = math::vec2(1.0f)) {
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

void change_entity_asset(Entity * entity, AssetState * assets, AssetId asset_id, u32 asset_index = 0) {
#if DEBUG_ENABLED
	Asset * asset = get_asset(assets, asset_id, asset_index);
	ASSERT(asset);
	ASSERT(asset->type == AssetType_texture || asset->type == AssetType_sprite);
#endif

	if(entity->asset_id != asset_id) {
		entity->asset_id = asset_id;
		entity->asset_index = asset_index;

		//TODO: Always do this??
		entity->collider = get_entity_render_bounds(assets, entity);

		entity->anim_time = 0.0f;		
	}
}

Entity * push_entity(EntityArray * entities, AssetState * assets, AssetId asset_id, u32 asset_index, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(entities->count < ARRAY_COUNT(entities->elems));

	Entity * entity = entities->elems + entities->count++;
	entity->pos = pos;
	entity->offset = math::vec2(0.0f);
	entity->scale = math::vec2(1.0f);
	entity->color = math::vec4(1.0f);

	change_entity_asset(entity, assets, asset_id, asset_index);

	entity->d_pos = math::vec2(0.0f);
	entity->speed = math::vec2(500.0f);
	entity->damp = 0.0f;
	entity->use_gravity = false;

	entity->anim_time = 0.0f;
	entity->hit = false;

	entity->rand_id = math::rand_u32();

	return entity;
}

UiElement * pick_ui_elem(RenderState * render_state, RenderGroup * render_group, UiElement * elems, u32 elem_count, math::Vec2 raw_mouse_pos) {
	math::Vec2 screen_dim = math::vec2(render_group->transform.projection_width, render_group->transform.projection_height);
	math::Vec2 buffer_dim = math::vec2(render_state->back_buffer_width, render_state->back_buffer_height);
	math::Vec2 mouse_pos = raw_mouse_pos * (screen_dim / buffer_dim) - screen_dim * 0.5f;
	mouse_pos.y = -mouse_pos.y;

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

void change_player_speed(MainMetaState * main_state, Player * player, f32 accel, f32 time_) {
	main_state->dd_speed = accel;
	main_state->accel_time = time_;

	player->invincibility_time = time_ + 2.0f;
}

void kill_player(MainMetaState * main_state, Player * player) {
	if(!player->dead && player->allow_input) {
		player->dead = true;
		player->allow_input = false;

		change_player_speed(main_state, player, main_state->slow_down_speed, F32_MAX);
		player->invincibility_time = 0.0f;
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

			entity->d_pos = math::vec2(0.0f);
			entity->d_pos.x = (math::rand_f32() * 2.0f - 1.0f) * 500.0f;
			entity->d_pos.y = math::rand_f32() * 1000.0f;

			entity->pos.xy = player->e->pos.xy - entity->offset;

			if(!--remaining) {
				break;
			}
		}
	}
}

//TODO: Do we still need this??
b32 change_location(MainMetaState * main_state, LocationId location_id) {
	ASSERT(location_id != LocationId_null);

	b32 changed = false;

	if(main_state->next_location == LocationId_null) {
		main_state->next_location = location_id;
		changed = true;
	}
	
	return changed;
}

void change_location_asset_type(MainMetaState * main_state, LocationId location_id, AssetState * assets, AssetId asset_id) {
	Location * location = main_state->locations + location_id;

	location->tile_to_asset_table[TileId_bad] = AssetId_glitched_telly;
	location->tile_to_asset_table[TileId_time] = AssetId_clock;
	location->tile_to_asset_table[TileId_clone] = AssetId_clone;
	location->tile_to_asset_table[TileId_collect] = AssetId_first_collect;
	location->tile_to_asset_table[TileId_concord] = AssetId_rocket;
	location->tile_to_asset_table[TileId_rocket] = AssetId_rocket;

	location->asset_id = asset_id;
	switch(location->asset_id) {
		case AssetId_city: {
			location->name = (char *)"DUNDEE";

			break;
		}

		case AssetId_highlands: {
			location->name = (char *)"HIGHLANDS";

			break;
		}

		case AssetId_ocean: {
			location->name = (char *)"OCEAN";

			break;
		}

		case AssetId_space: {
			location->name = (char *)"SPACE";

			location->tile_to_asset_table[TileId_clone] = AssetId_clone_space;
			location->tile_to_asset_table[TileId_rocket] = AssetId_clone_space;

			break;
		}

		INVALID_CASE();
	}

#if DEBUG_ENABLED
	for(u32 i = 0; i < ARRAY_COUNT(location->tile_to_asset_table); i++) {
		ASSERT(location->tile_to_asset_table[i]);
	}
#endif

	for(u32 i = 0; i < ARRAY_COUNT(location->layers); i++) {
		Entity * entity = location->layers[i];
		if(entity) {
			change_entity_asset(entity, assets, location->asset_id, i);
		}
	}

	if(location_id == LocationId_lower) {
		Entity * bg0 = main_state->background[0];
		Entity * bg1 = main_state->background[1];

		u32 bg_index = asset_id - AssetId_city;
		if(bg1->asset_index != bg_index) {
			bg0->asset_index = bg1->asset_index;
			bg0->color.a = 1.0f;

			bg1->asset_index = bg_index;
			bg1->color.a = 0.0f;
		}
	}
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
		rocket->anim_time = 0.0f;

		fire_audio_clip(meta_state->audio_state, AssetId_rocket_sfx);

		Player * player = &main_state->player;
		player->invincibility_time = F32_MAX;

		fade_out_audio_clip(&main_state->music, 1.0f);
		main_state->music = play_audio_clip(meta_state->audio_state, AssetId_space_music, true);
		change_volume(main_state->music, math::vec2(0.0f), 0.0f);
		change_volume(main_state->music, math::vec2(1.0f), 1.0f);
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

		rocket->anim_time += dt * ANIMATION_FRAMES_PER_SEC;
		rocket->asset_index = (u32)rocket->anim_time % get_asset_count(meta_state->assets, rocket->asset_id);

		rocket->d_pos += rocket->speed * dt;
		rocket->d_pos *= (1.0f - rocket->damp);

		math::Vec2 d_pos = rocket->d_pos * dt;
		rocket->pos.xy += d_pos;

		if(!player->dead) {
			f32 new_time = seq->time_ + dt;

			if(main_state->current_location != LocationId_upper) {
				f32 drop_height = main_state->locations[LocationId_upper].y + main_state->height_above_ground;

				if((player->e->pos.y + d_pos.y) < drop_height) {
					math::Rec2 player_bounds = get_entity_collider_bounds(player->e);
					math::Rec2 collider_bounds = get_entity_collider_bounds(rocket);
					if(math::rec_overlap(collider_bounds, player_bounds)) {
						player->e->d_pos = math::vec2(0.0f);
						player->e->pos.xy += d_pos;
						player->e->color.a = 0.0f;
						player->allow_input = false;
						main_state->accel_time = 0.0f;
					}

					main_state->render_group->transform.offset = (math::rand_vec2() * 2.0f - 1.0f) * 10.0f;
				}
				else {
					player->allow_input = true;
					player->invincibility_time = 0.0f;
					player->e->pos.y = drop_height;
					player->e->color.a = 1.0f;

					b32 changed = change_location(main_state, LocationId_upper);
					ASSERT(changed);
				}
			}

			f32 seq_max_time = 15.0f;
			if(seq->time_ < seq_max_time) {
				if(new_time > seq_max_time && main_state->next_location == LocationId_null) {
					fade_out_audio_clip(&main_state->music, 1.0f);
					main_state->music = play_audio_clip(meta_state->audio_state, AssetId_game_music, true);
					change_volume(main_state->music, math::vec2(0.0f), 0.0f);
					change_volume(main_state->music, math::vec2(1.0f), 1.0f);
					
					player->allow_input = false;
					player->invincibility_time = F32_MAX;
					player->e->use_gravity = true;
					change_entity_asset(player->e, meta_state->assets, AssetId_dolly_fall);

					AssetId first_asset_id = AssetId_city;
					u32 asset_id_offset = (main_state->locations[LocationId_lower].asset_id - first_asset_id) + 1;
					if(asset_id_offset >= 3) {
						asset_id_offset = 0;
					}

					change_location_asset_type(main_state, LocationId_lower, meta_state->assets, (AssetId)(asset_id_offset + first_asset_id));
				}
			}
			else {
				//TODO: Calculate the ideal distance to stop gravity so that the player lands roughly at the height_above_ground
				if(player->e->pos.y < (f32)main_state->render_group->transform.projection_height * 1.8f) {
					b32 changed = change_location(main_state, LocationId_lower);
					ASSERT(changed);

					player->e->use_gravity = false;
					player->allow_input = true;
					player->invincibility_time = 2.0f;
					change_entity_asset(player->e, meta_state->assets, AssetId_dolly_idle);

					seq->playing = false;
				}
			}

			seq->time_ = new_time;
		}
	}
}

b32 move_entity(MetaState * meta_state, RenderTransform * transform, Entity * entity, f32 d_pos) {
	b32 off_screen = false;

	entity->pos.x -= d_pos;

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

UiElement * push_ui_elem(UiLayer * ui_layer, u32 id, AssetId asset_id, AssetId clip_id) {
	ASSERT(ui_layer->elem_count < ARRAY_COUNT(ui_layer->elems));

	UiElement * elem = ui_layer->elems + ui_layer->elem_count++;
	elem->id = id;
	elem->asset_id = asset_id;
	elem->asset_index = 0;
	elem->clip_id = clip_id;
	return elem;
}

void change_page(MenuMetaState * menu_state, MenuPageId next_page) {
	UiLayer * current_page = menu_state->pages + menu_state->current_page;
	for(u32 i = 0; i < current_page->elem_count; i++) {
		UiElement * elem = current_page->elems + i;
		elem->asset_index = 0;
	}

	menu_state->current_page = next_page;
	current_page->interact_elem = 0;
}

UiElement * process_ui_layer(MetaState * meta_state, UiLayer * ui_layer, RenderGroup * render_group, GameInput * game_input) {
	//TODO: Name this better??
	UiElement * interact_elem = 0;

	UiElement * hot_elem = pick_ui_elem(meta_state->render_state, render_group, ui_layer->elems, ui_layer->elem_count, game_input->mouse_pos);

	if(hot_elem) {
		if(game_input->mouse_button & KEY_PRESSED) {
			ui_layer->interact_elem = hot_elem;
		}
	}

	if(ui_layer->interact_elem) {
		if(ui_layer->interact_elem != hot_elem || game_input->mouse_button & KEY_RELEASED) {
			ui_layer->interact_elem = 0;
		}
		else {
			ui_layer->interact_elem->asset_index = 1;


		}
	}
 
	if(ui_layer->interact_elem) {
		if(ui_layer->interact_elem != hot_elem || game_input->mouse_button & KEY_RELEASED) {
			ui_layer->interact_elem = 0;
		}
		else {
			ui_layer->interact_elem->asset_index = 1;

			if(game_input->mouse_button & KEY_PRESSED) {
				interact_elem = ui_layer->interact_elem;

				AssetId clip_id = ui_layer->interact_elem->clip_id;
				AudioClip * clip = get_audio_clip_asset(meta_state->assets, clip_id, math::rand_i32() % get_asset_count(meta_state->assets, clip_id));
				f32 pitch = math::lerp(0.9f, 1.1f, math::rand_f32());
				fire_audio_clip(meta_state->audio_state, clip, math::vec2(1.0f), pitch);
			}
		}
	}

	return interact_elem;
}

void push_ui_layer_to_render_group(UiLayer * ui_layer, RenderGroup * render_group) {
	for(u32 i = 0; i < ui_layer->elem_count; i++) {
		UiElement * elem = ui_layer->elems + i;
		push_textured_quad(render_group, elem->asset_id, elem->asset_index);
	}
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

	push_ui_elem(menu_state->pages + MenuPageId_main, MenuButtonId_play, AssetId_btn_play, AssetId_click_yes);
	push_ui_elem(menu_state->pages + MenuPageId_main, MenuButtonId_about, AssetId_btn_about, AssetId_click_no);

	push_ui_elem(menu_state->pages + MenuPageId_about, MenuButtonId_back, AssetId_btn_back, AssetId_click_no);
	push_ui_elem(menu_state->pages + MenuPageId_about, MenuButtonId_baa, AssetId_btn_baa, AssetId_baa);
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

	RenderState * render_state = meta_state->render_state;
	u32 screen_width = render_state->screen_width;
	u32 screen_height = render_state->screen_height;

	main_state->music = play_audio_clip(meta_state->audio_state, AssetId_game_music, true);
	change_volume(main_state->music, math::vec2(0.0f), 0.0f);
	change_volume(main_state->music, math::vec2(1.0f), 1.0f);

	main_state->render_group = allocate_render_group(render_state, &meta_state->arena, screen_width, screen_height, 1024);
	main_state->ui_render_group = allocate_render_group(render_state, &meta_state->arena, screen_width, screen_height);
	main_state->letterboxed_height = (f32)screen_height;
	main_state->fixed_letterboxing = 80.0f;
	main_state->font = allocate_font(render_state, 65536, screen_width, screen_height);

	EntityArray * entities = &main_state->entities;
	main_state->entity_gravity = math::vec2(0.0f, -6000.0f);

	main_state->height_above_ground = (f32)screen_height * 0.5f;
	main_state->max_height_above_ground = main_state->height_above_ground + (f32)screen_height * 0.5f;
	main_state->camera_movement = 0.75f;

	f32 layer_z_offsets[PARALLAX_LAYER_COUNT] = {
		 8.0f,
		 4.0f,
		 2.0f,
		 0.0f,
	};

	f32 location_max_z = layer_z_offsets[0];
	f32 location_y_offset = (f32)screen_height / (1.0f / (location_max_z + 1.0f));

	for(u32 i = 0; i < ARRAY_COUNT(main_state->background); i++) {
		Entity * entity = push_entity(entities, meta_state->assets, AssetId_background, i);

		f32 z = location_max_z;

		math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
		f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

		entity->pos.y = y_offset;
		entity->pos.z = z;
		entity->scale.x = (f32)screen_width / dim.x;
		entity->scrollable = true;

		main_state->background[i] = entity;
	}

	change_location_asset_type(main_state, LocationId_lower, meta_state->assets, AssetId_city);
	change_location_asset_type(main_state, LocationId_upper, meta_state->assets, AssetId_space);

	main_state->background[0]->color.a = 0.0f;
	main_state->background[1]->color.a = 1.0f;

	for(u32 i = 0; i < LocationId_count; i++) {	
		Location * location = main_state->locations + i;

		location->y = location_y_offset * i;
		location->tint = math::vec4(1.0f);
	
		for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers) - 1; layer_index++) {
			Entity * entity = push_entity(entities, meta_state->assets, location->asset_id, layer_index);

			f32 z = layer_z_offsets[layer_index];

			math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
			f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

			entity->pos.y = location->y + y_offset;
			entity->pos.z = z;
			entity->scrollable = true;

			location->layers[layer_index] = entity;
		}
	}

	main_state->locations[LocationId_lower].map_id = AssetId_tutorial_map;

	main_state->current_location = LocationId_lower;
	main_state->next_location = LocationId_null;
	main_state->last_location = LocationId_null;

	Player * player = &main_state->player;
	player->clone_offset = math::vec2(-5.0f, 0.0f);

	for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
		Entity * entity = push_entity(entities, assets, AssetId_clone, 0, ENTITY_NULL_POS);
		entity->scale = math::vec2(0.75f);
		entity->color.rgb = math::lerp(math::rand_vec3(), math::vec3(1.0f), 0.75f);
		entity->color.a = 0.0f;

		entity->hidden = true;

		player->clones[i] = entity;
	}

	player->e = push_entity(entities, assets, AssetId_dolly_idle, 0, math::vec3((f32)screen_width * -0.25f, main_state->height_above_ground, 0.0f));
	player->e->speed = math::vec2(750.0f, 4000.0f);
	player->e->damp = 0.1f;
	player->allow_input = true;

	EntityEmitter * emitter = &main_state->entity_emitter;
	emitter->pos = math::vec3(screen_width * 0.75f, main_state->height_above_ground, 0.0f);
	for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
		emitter->entity_array[i] = push_entity(entities, assets, AssetId_first_collect, 0, emitter->pos);
	}

	RocketSequence * seq = &main_state->rocket_seq;
	seq->rocket = push_entity(entities, assets, AssetId_rocket_large, 0, ENTITY_NULL_POS);
	seq->rocket->collider = math::rec_offset(seq->rocket->collider, math::vec2(0.0f, -math::rec_dim(seq->rocket->collider).y * 0.35f));
	seq->playing = false;
	seq->time_ = 0.0f;

	for(u32 i = 0; i < ARRAY_COUNT(main_state->clouds); i++) {
		Entity * entity = push_entity(entities, meta_state->assets, AssetId_clouds, i);

		entity->pos.y = main_state->max_height_above_ground * i;
		entity->scrollable = true;

		main_state->clouds[i] = entity;
	}

	//TODO: Adding foreground layer at the end until we have sorting!!
	for(u32 i = 0; i < LocationId_count; i++) {
		Location * location = main_state->locations + i;

		u32 layer_index = ARRAY_COUNT(location->layers) - 1;

		Entity * entity = push_entity(entities, assets, location->asset_id, layer_index);

		f32 z = layer_z_offsets[layer_index];

		math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
		f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

		entity->pos.y = location->y + y_offset;
		entity->pos.z = z;
		entity->scrollable = true;

		location->layers[layer_index] = entity;
	}

	push_ui_elem(&main_state->score_ui, ScoreButtonId_back, AssetId_btn_back, AssetId_click_no);
	push_ui_elem(&main_state->score_ui, ScoreButtonId_replay, AssetId_btn_replay, AssetId_click_yes);
	main_state->score_values[ScoreValueId_time_played].is_f32 = true;
	main_state->score_values[ScoreValueId_points].is_f32 = false;

	main_state->arrow_buttons[0].asset_id = AssetId_btn_up;
	main_state->arrow_buttons[1].asset_id = AssetId_btn_down;

	main_state->start_time = 30.0f;

	main_state->boost_speed = 2.5f;
	main_state->boost_accel = 40.0f;
	main_state->boost_time = 5.0f;
	main_state->slow_down_speed = 0.25f;
	main_state->slow_down_time = 1.5f;

	//TODO: Make this platform independent!!
#if __EMSCRIPTEN__ && DEV_ENABLED
	main_state->start_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.start_time });

	main_state->boost_always_on = EM_ASM_INT_V({ return Prefs.boost_always_on });
	main_state->boost_speed = (f32)EM_ASM_DOUBLE_V({ return Prefs.boost_speed });
	main_state->boost_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.boost_time });
	main_state->slow_down_speed = (f32)EM_ASM_DOUBLE_V({ return Prefs.slow_down_speed });
	main_state->slow_down_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.slow_down_time; });
#endif

	main_state->max_time = main_state->start_time;
	main_state->countdown_time = 10.0f;
	main_state->time_remaining = main_state->start_time;

	main_state->clock_text_scale = 1.0f;
	main_state->clone_text_scale = 1.0f;
}

void change_meta_state(GameState * game_state, MetaStateType type) {
	game_state->meta_state = type;

	//TODO: We should probably defer the init to the end of frame!!
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

		game_state->debug_render_entity_bounds = false;

#if __EMSCRIPTEN__ && DEV_ENABLED
		if(EM_ASM_INT_V({ return Prefs.mute })) {
			game_state->audio_state.master_volume = 0.0f;
		}
#endif
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

					for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collect_unlock_states); i++) {
						if(header->collect_unlock_states[i]) {
							game_state->save.collect_unlock_states[i] = true;
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
	// str_print(game_state->str, "PLAYS: %u | HIGH_SCORE: %u | LONGEST_RUN: %.2f\n\n", game_state->save.plays, game_state->save.high_score, game_state->save.longest_run);

#if 0
	for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collect_unlock_states); i++){
		b32 unlocked = game_state->save.collect_unlock_states[i];

		char * unlocked_str = (char *)"UNLOCKED";
		char * locked_str = (char *)"LOCKED";

		str_print(game_state->str, "[%u]: %s\n", i, unlocked ? unlocked_str : locked_str);
	}
	str_print(game_state->str, "\n");
#endif

	if(game_input->buttons[ButtonId_debug] & KEY_PRESSED) {
		game_state->debug_render_entity_bounds = !game_state->debug_render_entity_bounds;
	}

	switch(game_state->meta_state) {
		case MetaStateType_menu: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_menu);
			MenuMetaState * menu_state = meta_state->menu;

			UiLayer * current_page = menu_state->pages + menu_state->current_page;
			UiElement * interact_elem = process_ui_layer(meta_state, current_page, menu_state->render_group, game_input);
			if(!game_state->transitioning) {
				if(interact_elem) {
					u32 interact_id = interact_elem->id;
					switch(menu_state->current_page) {
						case MenuPageId_main: {
							if(interact_id == MenuButtonId_play) {
								menu_state->play_transition_id = begin_transition(game_state);
							}
							else if(interact_id == MenuButtonId_about) {
								menu_state->about_transition_id = begin_transition(game_state);
							}

							break;
						}

						case MenuPageId_about: {
							if(interact_id == MenuButtonId_back) {
								menu_state->back_transition_id = begin_transition(game_state);
							}

							break;
						}

						INVALID_CASE();
					}
				}				
			}
			else {
				if(transition_should_flip(game_state, menu_state->play_transition_id)) {
					change_meta_state(game_state, MetaStateType_intro);
				}
				else if(transition_should_flip(game_state, menu_state->about_transition_id)) {
					change_page(menu_state, MenuPageId_about);
				}
				else if(transition_should_flip(game_state, menu_state->back_transition_id)) {
					change_page(menu_state, MenuPageId_main);
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

				u32 last_frame_index = ARRAY_COUNT(intro_state->frames) - 1;
				if(frame_index <= last_frame_index) {
					for(u32 i = 0; i < math::min(frame_index, last_frame_index); i++) {
						IntroFrame * frame = intro_state->frames + i;
						frame->alpha = 1.0f;
					}
				}
				else {
					for(u32 i = 0; i < last_frame_index; i++) {
						IntroFrame * frame = intro_state->frames + i;
						frame->alpha = 0.0f;
					}

					if(frame_index > ARRAY_COUNT(intro_state->frames)) {
						IntroFrame * frame = intro_state->frames + last_frame_index;
						frame->alpha = 1.0f;
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
				if(main_state->quit_transition_id || main_state->replay_transition_id) {
					if(main_state->music) {
						fade_out_audio_clip(&main_state->music, 1.0f);
					}
				}

				MetaStateType new_state = MetaStateType_null;
				if(transition_should_flip(game_state, main_state->quit_transition_id)) {
					new_state = MetaStateType_menu;
				}
				else if(transition_should_flip(game_state, main_state->replay_transition_id)) {
					new_state = MetaStateType_main;
				}

				if(new_state != MetaStateType_null) {
					if(main_state->tick_tock) {
						stop_audio_clip(meta_state->audio_state, &main_state->tick_tock);
					}

					game_state->time_until_next_save = 0.0f;

					change_meta_state(game_state, new_state);
				}
			}

#if 0
			if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
				Location * lower_location = main_state->locations + LocationId_lower;
				change_location(main_state, LocationId_lower);
			}
#endif

			main_state->clone_text_scale = math::lerp(main_state->clone_text_scale, 1.0f, game_input->delta_time * 12.0f);
			main_state->clock_text_scale = math::lerp(main_state->clock_text_scale, 1.0f, game_input->delta_time * 12.0f);

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
				if(player->e->pos.x > (f32)render_transform->projection_width * 2.0f) {
					main_state->show_score_overlay = true;
				}
			}

			{
				main_state->accel_time -= game_input->delta_time;
				if(main_state->accel_time <= 0.0f) {
					main_state->dd_speed = 0.0f;
					main_state->accel_time = 0.0f;
				}

				if(main_state->boost_always_on) {
					main_state->dd_speed = main_state->boost_accel;
					main_state->accel_time = F32_MAX;
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

			f32 player_d_pos = player->e->speed.x * math::clamp(main_state->d_speed + 1.0f, main_state->slow_down_speed, main_state->boost_speed) * game_input->delta_time;

			f32 letterboxing = math::max(main_state->d_speed, 0.0f) * main_state->boost_accel;
			letterboxing = math::min(letterboxing, render_state->screen_height * 0.25f);
			main_state->letterboxed_height = (f32)render_state->screen_height - (main_state->fixed_letterboxing * 2.0f + letterboxing);

			player->active_clone_count = 0;
			for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
				Entity * entity = player->clones[i];

				if(i == 0) {
					entity->offset = player->e->pos.xy + player->clone_offset * 4.0f;
				}
				else {
					Entity * next_entity = player->clones[i - 1];
					entity->offset = next_entity->offset + player->clone_offset;
				}

				if(!entity->hidden) {
					if(entity->hit) {
						math::Vec2 dd_pos = main_state->entity_gravity * 0.5f;
						entity->d_pos += dd_pos * game_input->delta_time;
						// entity->d_pos *= (1.0f - entity->damp);
						entity->pos.xy += entity->d_pos * game_input->delta_time;

						entity->hit_time += game_input->delta_time;
						if(entity->hit_time >= 2.0f) {
							entity->hit_time = 0.0f;
							entity->hidden = true;
							
							entity->color.a = 0.0f;
						}
					}
					else {
						entity->d_pos = math::vec2(0.0f);
						entity->pos.x = 0.0f;
						entity->pos.y = math::sin((game_input->total_time * 0.25f + i * (3.0f / 13.0f)) * math::TAU) * 50.0f;

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

			for(u32 i = 0; i < ARRAY_COUNT(main_state->arrow_buttons); i++) {
				UiElement * elem = main_state->arrow_buttons + i;
				elem->asset_index = 0;
			}

			AssetId player_idle_id = AssetId_dolly_idle;
			AssetId player_up_id = AssetId_dolly_up;
			AssetId player_down_id = AssetId_dolly_down;
			if(main_state->current_location == LocationId_upper) {
				player_idle_id = AssetId_dolly_space_idle;
				player_up_id = AssetId_dolly_space_up;
				player_down_id = AssetId_dolly_space_down;
			}

			if(!player->dead) {
				if(player->allow_input) {
					f32 half_buffer_height = (f32)game_input->back_buffer_height * 0.5f;
					
					if(game_input->buttons[ButtonId_up] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y < half_buffer_height)) {
						player_dd_pos.y += player->e->speed.y;

						main_state->arrow_buttons[0].asset_index = 1;
						change_entity_asset(player->e, meta_state->assets, player_up_id);
					}

					if(game_input->buttons[ButtonId_down] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y >= half_buffer_height)) {
						player_dd_pos.y -= player->e->speed.y;

						main_state->arrow_buttons[1].asset_index = 1;

						change_entity_asset(player->e, meta_state->assets, player_down_id);
					}

					if(!player_dd_pos.y) {
						change_entity_asset(player->e, meta_state->assets, player_idle_id);
					}
				}		
			}
			else {
				player_dd_pos.x += player->e->speed.x * 10.0f;
				change_entity_asset(player->e, meta_state->assets, player_idle_id);
			}

			player->e->d_pos += player_dd_pos * game_input->delta_time;
			if(player->e->use_gravity) {
				player->e->d_pos += main_state->entity_gravity * game_input->delta_time;
			}
			else {
				//TODO: Apply damping to other forces when gravity is enabled!!
				player->e->d_pos *= (1.0f - player->e->damp);
			}

			player->e->pos.xy += player->e->d_pos * game_input->delta_time;

			if(!player->dead && player->allow_input) {
				f32 location_y = main_state->locations[main_state->current_location].y;

				f32 ground_height = location_y;
				if(player->e->pos.y <= ground_height) {
					player->e->pos.y = ground_height;
					player->e->d_pos.y = 0.0f;
				}

				f32 max_height = location_y + main_state->max_height_above_ground;
				if(player->e->pos.y >= max_height && player->e->d_pos.y > 0.0f) {
					player->e->pos.y = max_height;
					player->e->d_pos.y = 0.0f;
				}
			}

			player->e->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;;
			player->e->asset_index = (u32)player->e->anim_time % get_asset_count(assets, player->e->asset_id);

			player->invincibility_time -= game_input->delta_time;
			player->invincibility_time = math::max(player->invincibility_time, 0.0f);
#if 0
			if(player->invincibility_time) {
				player->e->color = math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			}
			else {
				player->e->color = math::vec4(1.0f);
			}
#endif

			f32 zero_height = main_state->locations[main_state->current_location].y + main_state->height_above_ground;

			f32 camera_movement = 0.75f;
			if(!player->dead && !player->allow_input) {
				camera_movement = 1.0f;
			}
			main_state->camera_movement = math::lerp(main_state->camera_movement, camera_movement, game_input->delta_time * 4.0f);
			f32 camera_y = (player->e->pos.y - zero_height) * main_state->camera_movement + zero_height;

			render_transform->pos.x = 0.0f;
			render_transform->pos.y = camera_y;
			render_transform->pos.y = math::max(render_transform->pos.y, 0.0f);
			render_transform->offset = (math::rand_vec2() * 2.0f - 1.0f) * math::max(main_state->d_speed, 0.0f);

			if(main_state->rocket_seq.playing) {
				play_rocket_sequence(game_state, meta_state, game_input->delta_time);
			}

			if(main_state->next_location != LocationId_null) {
				main_state->last_location = main_state->current_location;
				main_state->current_location = main_state->next_location;
				main_state->next_location = LocationId_null;

				main_state->entity_emitter.pos.y = main_state->locations[main_state->current_location].y + main_state->height_above_ground;
				main_state->entity_emitter.cursor = 0.0f;
				main_state->entity_emitter.last_read_pos = 0;
				main_state->entity_emitter.map_index = 0;

				if(main_state->current_location == LocationId_lower) {
					Location * lower_location = main_state->locations + LocationId_lower;
					lower_location->map_id = AssetId_lower_map;
				}
				else {
					Location * upper_location = main_state->locations + LocationId_upper;
					upper_location->map_id = AssetId_upper_map;
				}
			}

			Location * current_location = main_state->locations + main_state->current_location;

			math::Rec2 player_bounds = get_entity_collider_bounds(player->e);

			EntityEmitter * emitter = &main_state->entity_emitter;

			for(u32 i = 0; i < emitter->entity_count; i++) {
				Entity * entity = emitter->entity_array[i];
				b32 destroy = false;

				if(move_entity(meta_state, render_transform, entity, player_d_pos)) {
					destroy = true;
				}

				entity->offset.y = math::sin((game_input->total_time * 0.2f + entity->rand_id * (3.0f / 13.0f)) * math::TAU) * 10.0f;

				if(entity->hit) {
					entity->offset.y = 0.0f;

#if 1
					f32 anim_speed = 4.0f;

					entity->scale += anim_speed * game_input->delta_time;
					//TODO: Vec2 clamp??
					entity->scale.x = math::min(entity->scale.x, 1.0f);
					entity->scale.y = math::min(entity->scale.y, 1.0f);

					entity->color.a -= anim_speed * game_input->delta_time;
					if(entity->color.a < 0.0f) {
						entity->color.a = 0.0f;
						destroy = true;
					}
#else
					if((u32)entity->anim_time >= (get_asset_count(assets, entity->asset_id) - 1)) {
						entity->color.a = 0.0f;
						destroy = true;
					}
#endif
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
						case AssetId_clone_space:
						case AssetId_clone: {
							push_player_clone(player);
							main_state->clone_text_scale = 2.0f;
							main_state->score_values[ScoreValueId_points].u32_++;
							clip_id = AssetId_baa;

							break;
						}

						case AssetId_glitched_telly: {
							clip_id = AssetId_bang;

							if(!player->invincibility_time) {	
								pop_player_clones(player, 50);
								is_slow_down = true;
							}

							break;			
						}

						case AssetId_rocket: {
							begin_rocket_sequence(game_state, meta_state);
							break;
						}

						case AssetId_clock: {
							main_state->time_remaining += main_state->countdown_time;
							main_state->time_remaining = math::min(main_state->time_remaining, main_state->max_time);
							stop_audio_clip(meta_state->audio_state, &main_state->tick_tock);

							main_state->clock_text_scale = 2.0f;

							break;
						}

#if 0
						case AssetId_concord: {
							change_player_speed(main_state, player, main_state->boost_accel, main_state->boost_time);

							break;
						}
#endif

						default: {
							main_state->score_values[ScoreValueId_points].u32_++;
							break;
						}
					}

					if(!player->invincibility_time && is_slow_down) {
						change_player_speed(main_state, player, -main_state->boost_accel, main_state->slow_down_time);
					}

					if(entity->asset_id >= AssetId_first_collect && entity->asset_id < AssetId_one_past_last_collect) {
						u32 collect_id = entity->asset_id - AssetId_first_collect;
						ASSERT(collect_id < ARRAY_COUNT(game_state->save.collect_unlock_states));
						game_state->save.collect_unlock_states[collect_id] = true;
						game_state->time_until_next_save = 0.0f;

						clip_id = AssetId_special;
					}
					
					//TODO: Should there be a helper function for this??
					AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
					f32 pitch = math::lerp(0.9f, 1.1f, math::rand_f32());
					fire_audio_clip(meta_state->audio_state, clip, math::vec2(1.0f), pitch);

					//TODO: Allocate these separately (or bring closer when we have sorting) for more efficient batching!!
					change_entity_asset(entity, assets, AssetId_shield);
					entity->anim_time = 0.0f;
					entity->scale = math::vec2(0.5f);
					// entity->scale = math::vec2(1.0f);
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

			if(!player->dead) {
				math::Vec2 projection_dim = math::vec2(main_state->render_group->transform.projection_width, main_state->render_group->transform.projection_height);

				AssetId map_id = current_location->map_id;
#if DEV_ENABLED
				map_id = AssetId_debug_tile_map;
#endif
				u32 map_asset_count = get_asset_count(meta_state->assets, map_id);
				ASSERT(map_asset_count);

				TileMap * map = get_tile_map_asset(meta_state->assets, map_id, emitter->map_index);
				ASSERT(map);

				f32 tile_size_pixels = projection_dim.y / (f32)TILE_MAP_HEIGHT;

				emitter->cursor += player_d_pos;

				f32 read_cursor = emitter->cursor / tile_size_pixels;
				u32 read_cursor_int = (u32)read_cursor;
				f32 read_cursor_frac = read_cursor - read_cursor_int;
				u32 reads_ahead = read_cursor_int - emitter->last_read_pos;

				u32 read_pos = emitter->last_read_pos;
				while(reads_ahead) {
					if(read_pos >= map->width) {
						emitter->cursor -= map->width * tile_size_pixels;

						emitter->map_index++;
						if(emitter->map_index >= map_asset_count) {
							emitter->map_index = 0;
						}

						map = get_tile_map_asset(meta_state->assets, map_id, emitter->map_index);
						ASSERT(map);

						read_pos = 0;
					}

					f32 x_offset = -(read_cursor_frac + (reads_ahead - 1)) * tile_size_pixels;

					Tiles * tiles = map->tiles + read_pos;
					for(u32 y = 0; y < TILE_MAP_HEIGHT; y++) {
						u32 tile_id = tiles->ids[y];
						if(tile_id != TileId_null) {
							if(emitter->entity_count < ARRAY_COUNT(emitter->entity_array)) {
								Entity * entity = emitter->entity_array[emitter->entity_count++];

								entity->pos = emitter->pos;
								entity->pos.x += x_offset;
								entity->pos.y += (((f32)y + 0.5f) / (f32)TILE_MAP_HEIGHT) * projection_dim.y - projection_dim.y * 0.5f;
								entity->scale = math::vec2(1.0f);
								entity->color = math::vec4(1.0f);

								entity->anim_time = 0.0f;

								AssetId asset_id = current_location->tile_to_asset_table[tile_id];
								if(tile_id == TileId_collect) {
									asset_id = (AssetId)(AssetId_first_collect + (math::rand_u32() % ARRAY_COUNT(game_state->save.collect_unlock_states)));
								}

								change_entity_asset(entity, assets, asset_id);
								if(tile_id == TileId_bad) {
									entity->collider = math::rec_scale(get_entity_render_bounds(assets, entity), 0.5f);
								}

								entity->hit = false;
							}
						}
					}					

					read_pos++;
					reads_ahead--;
				}

				emitter->last_read_pos = read_pos;
			}

			main_state->background[1]->color.a += game_input->delta_time;
			if(main_state->background[1]->color.a >= 1.0f) {
				main_state->background[1]->color.a = 1.0f;
				main_state->background[0]->color.a = 0.0f;
			}

			for(u32 i = 0; i < ARRAY_COUNT(main_state->background); i++) {
				move_entity(meta_state, render_transform, main_state->background[i], 0.0f);
			}

			for(u32 i = 0; i < ARRAY_COUNT(main_state->clouds); i++) {
				move_entity(meta_state, render_transform, main_state->clouds[i], player_d_pos * 1.5f);
			}

			for(u32 i = 0; i < LocationId_count; i++) {
				Location * location = main_state->locations + i;
				for(u32 layer_index = 0; layer_index < ARRAY_COUNT(location->layers); layer_index++) {
					Entity * entity = location->layers[layer_index];

					f32 d_pos = player_d_pos;
#if 1
					//TODO: HACK!!!!!
					if(layer_index == ARRAY_COUNT(location->layers) - 1) {
						d_pos *= 2.0f;
					}
#endif

					//TODO: Could we actually just use the emitter cursor to position these??
					move_entity(meta_state, render_transform, location->layers[layer_index], d_pos);
				}
			}

			if(main_state->show_score_overlay) {
				UiElement * interact_elem = process_ui_layer(meta_state, &main_state->score_ui, main_state->ui_render_group, game_input);

				if(interact_elem) {
					if(!game_state->transitioning) {
						u32 interact_id = interact_elem->id;
						if(interact_id == ScoreButtonId_back) {
							main_state->quit_transition_id = begin_transition(game_state);
						}
						else if(interact_id == ScoreButtonId_replay) {
							main_state->replay_transition_id = begin_transition(game_state);
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
			}

			game_state->save.high_score = math::max(game_state->save.high_score, main_state->score_values[ScoreValueId_points].u32_);
			game_state->save.longest_run = math::max(game_state->save.longest_run, main_state->score_values[ScoreValueId_time_played].f32_);

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

			RenderGroup * render_group = menu_state->render_group;

			push_textured_quad(render_group, AssetId_menu_background, menu_state->current_page);

			if(menu_state->current_page == MenuPageId_main) {
				u32 display_item_count = ASSET_GROUP_COUNT(display);
				for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collect_unlock_states); i++) {
					if(game_state->save.collect_unlock_states[i]) {
						push_textured_quad(render_group, (AssetId)(AssetId_first_display + i), 0);
					}
				}				
			}

			push_ui_layer_to_render_group(menu_state->pages + menu_state->current_page, render_group);

			render_and_clear_render_group(meta_state->render_state, render_group);

			break;
		}

		case MetaStateType_intro: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_intro);
			IntroMetaState * intro_state = meta_state->intro;

			RenderGroup * render_group = intro_state->render_group;

			for(u32 i = 0; i < ARRAY_COUNT(intro_state->frames); i++) {
				IntroFrame * frame = intro_state->frames + i;
				push_textured_quad(render_group, AssetId_intro, i, math::vec3(0.0f), math::vec2(1.0f), math::vec4(1.0f, 1.0f, 1.0f, frame->alpha));
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
				push_textured_quad(main_state->render_group, entity->asset_id, entity->asset_index, entity->pos + math::vec3(entity->offset, 0.0f), entity->scale, entity->color, entity->scrollable);
			}

			render_and_clear_render_group(meta_state->render_state, main_state->render_group);

			if(game_state->debug_render_entity_bounds) {
				RenderBatch * render_batch = render_state->render_batch;
				render_batch->tex = get_texture_asset(render_state->assets, AssetId_white, 0);
				render_batch->mode = RenderMode_lines;
				
				RenderTransform * render_transform = &main_state->render_group->transform;
				math::Mat4 projection = math::orthographic_projection((f32)render_transform->projection_width, (f32)render_transform->projection_height);

				for(u32 i = 0; i < entities->count; i++) {
					Entity * entity = entities->elems + i;

					// math::Rec2 bounds = get_entity_render_bounds(assets, entity);
					math::Rec2 bounds = math::rec_scale(entity->collider, entity->scale);
					math::Vec2 pos = project_pos(render_transform, entity->pos + math::vec3(math::rec_pos(bounds), 0.0f));
					bounds = math::rec2_pos_dim(pos, math::rec_dim(bounds));

					u32 elems_remaining = render_batch->v_len - render_batch->e;
					if(elems_remaining < QUAD_LINES_ELEM_COUNT) {
						render_and_clear_render_batch(render_batch, &render_state->basic_shader, &projection);
					}

					push_quad_lines_to_batch(render_batch, &bounds, math::vec4(1.0f));
				}

				render_and_clear_render_batch(render_batch, &render_state->basic_shader, &projection);
			}

			RenderGroup * ui_render_group = main_state->ui_render_group;
			math::Vec2 projection_dim = math::vec2(ui_render_group->transform.projection_width, ui_render_group->transform.projection_height);

			f32 letterbox_pixels = (projection_dim.y - main_state->letterboxed_height) * 0.5f;
			if(letterbox_pixels > 0.0f) {
				push_colored_quad(ui_render_group, math::vec3(0.0f, projection_dim.y - letterbox_pixels, 0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
				push_colored_quad(ui_render_group, math::vec3(0.0f,-projection_dim.y + letterbox_pixels, 0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			}

			f32 icon_size = 64.0f;

			if(!main_state->show_score_overlay) {
				push_textured_quad(ui_render_group, AssetId_icon_clone, 0, math::vec3((-projection_dim.x + icon_size) * 0.5f, (projection_dim.y - icon_size) * 0.5f, 0.0f), math::vec2(main_state->clone_text_scale));
				push_textured_quad(ui_render_group, AssetId_icon_clock, 0, math::vec3((projection_dim.x - icon_size) * 0.5f, (projection_dim.y - icon_size) * 0.5f, 0.0f), math::vec2(main_state->clock_text_scale));

				for(u32 i = 0; i < ARRAY_COUNT(main_state->arrow_buttons); i++) {
					UiElement * elem = main_state->arrow_buttons + i;
					push_textured_quad(ui_render_group, elem->asset_id, elem->asset_index);
				}
			}
			else {
				push_colored_quad(ui_render_group, math::vec3(0.0f), projection_dim, math::vec4(0.0f, 0.0f, 0.0f, 0.8f));

				push_ui_layer_to_render_group(&main_state->score_ui, ui_render_group);
			}

			render_and_clear_render_group(meta_state->render_state, ui_render_group);

#if 1
			//TODO: Need a convenient way to make temp strs!!
			char temp_buf[256];
			Str temp_str = str_fixed_size(temp_buf, ARRAY_COUNT(temp_buf));

			if(!main_state->show_score_overlay) {
				{
					str_clear(&temp_str);
					str_print(&temp_str, "CLONES: %u", main_state->player.active_clone_count);

					math::Vec2 offset = math::vec2(0.0f);
					offset.x = icon_size;
					offset.y = (-icon_size + (f32)(main_state->font->glyph_height + main_state->font->glyph_spacing * 2)) * 0.5f;
					FontLayout font_layout = create_font_layout(main_state->font, projection_dim, 1.0f, FontLayoutAnchor_top_left, offset);
					push_str_to_batch(main_state->font, &font_layout, &temp_str);
				}

				{
					str_clear(&temp_str);
					str_print(&temp_str, "TIME: %.2f", main_state->time_remaining);

					math::Vec2 offset = math::vec2(0.0f);
					offset.x = -icon_size;
					offset.y = (-icon_size + (f32)(main_state->font->glyph_height + main_state->font->glyph_spacing * 2)) * 0.5f;
					FontLayout font_layout = create_font_layout(main_state->font, projection_dim, 1.0f, FontLayoutAnchor_top_right, offset);
					push_str_to_batch(main_state->font, &font_layout, &temp_str);
				}

				{
					FontLayout font_layout = create_font_layout(main_state->font, projection_dim, 1.0f, FontLayoutAnchor_bottom_left);
					push_c_str_to_batch(main_state->font, &font_layout, main_state->locations[main_state->current_location].name);
				}				
			}
			else {
				str_clear(&temp_str);
				str_print(&temp_str, "SCORE\n\n");
				str_print(&temp_str, "TIME PLAYED: %.2f\n", main_state->score_values[ScoreValueId_time_played].display);
				str_print(&temp_str, "POINTS: %u\n", (u32)main_state->score_values[ScoreValueId_points].display);

				FontLayout font_layout = create_font_layout(main_state->font, projection_dim, 1.0f, FontLayoutAnchor_top_centre, math::vec2(0.0f, -(main_state->fixed_letterboxing + 40.0f)));
				push_str_to_batch(main_state->font, &font_layout, &temp_str);				
			}
#endif

			render_font(render_state, main_state->font, &ui_render_group->transform);

			break;
		}

		INVALID_CASE();
	}

	Font * debug_font = render_state->debug_font;
	FontLayout debug_font_layout = create_font_layout(debug_font, math::vec2(render_state->back_buffer_width, render_state->back_buffer_height), 1.0f, FontLayoutAnchor_top_left);
	push_str_to_batch(debug_font, &debug_font_layout, game_state->str);
#if 0
	{
		// math::Vec4 debug_color = math::vec4(1.0f);
		math::Vec4 debug_color = math::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		char temp_buf[128];
		Str temp_str = str_fixed_size(temp_buf, ARRAY_COUNT(temp_buf));
		str_print(&temp_str, "DT: %f\n", game_input->delta_time);
		str_print(&temp_str, "SOURCES PLAYING: %u | SOURCES TO FREE %u\n", game_state->audio_state.debug_sources_playing, game_state->audio_state.debug_sources_to_free);
		str_print(&temp_str, "\n");
		push_str_to_batch(debug_font, &debug_font_layout, &temp_str, debug_color);

		push_str_to_batch(debug_font, &debug_font_layout, game_state->debug_str, debug_color);
	}
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

#if DEBUG_ENABLED
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
#endif

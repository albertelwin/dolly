
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
	return math::rec_offset(get_asset_bounds(assets, entity->asset.id, entity->asset.index, entity->scale), entity->offset);
}

math::Rec2 get_entity_collider_bounds(Entity * entity) {
	return math::rec_scale(math::rec_offset(entity->collider, entity->pos.xy + entity->offset), entity->scale);
}

void change_entity_asset(Entity * entity, AssetState * assets, AssetRef asset) {
#if DEBUG_ENABLED
	Asset * asset_ = get_asset(assets, asset.id, asset.index);
	ASSERT(asset_);
	ASSERT(asset_->type == AssetType_texture || asset_->type == AssetType_sprite);
#endif

	//TODO: Should we also check asset.index??
	if(entity->asset.id != asset.id) {
		entity->asset = asset;
		//TODO: Always do this??
		entity->collider = get_asset_bounds(assets, entity->asset.id, entity->asset.index);
		entity->anim_time = 0.0f;		
	}
}

Entity * push_entity(EntityArray * entities, AssetState * assets, AssetRef asset, math::Vec3 pos = math::vec3(0.0f)) {
	ASSERT(entities->count < ARRAY_COUNT(entities->elems));

	Entity * entity = entities->elems + entities->count++;
	entity->pos = pos;
	entity->offset = math::vec2(0.0f);
	entity->scale = math::vec2(1.0f);
	entity->color = math::vec4(1.0f);

	change_entity_asset(entity, assets, asset);

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
		elem->asset.index = 0;

		math::Rec2 bounds = get_asset_bounds(render_group->assets, elem->asset.id, elem->asset.index);
		if(math::inside_rec(bounds, mouse_pos)) {
			hot_elem = elem;
		}
	}

	return hot_elem;
}

void change_player_speed(MainMetaState * main_state, Player * player, f32 accel, f32 time_) {
	main_state->dd_speed = accel;
	main_state->accel_time = time_;

	player->invincibility_time = time_ + 1.0f;
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

void change_scene(MainMetaState * main_state, SceneId scene_id) {
	if(main_state->current_scene != scene_id) {
		main_state->current_scene = scene_id;

		main_state->entity_emitter.pos.y = main_state->scenes[main_state->current_scene].y + main_state->height_above_ground;
		main_state->entity_emitter.cursor = 0.0f;
		main_state->entity_emitter.last_read_pos = 0;
		main_state->entity_emitter.map_index = 0;
	}
}

void change_scene_asset_type(MainMetaState * main_state, SceneId scene_id, AssetState * assets, AssetId asset_id) {
	Scene * scene = main_state->scenes + scene_id;

	scene->tile_to_asset_table[TileId_bad] = AssetId_atom_smasher;
	scene->tile_to_asset_table[TileId_time] = AssetId_clock;
	scene->tile_to_asset_table[TileId_clone] = AssetId_clone;
	scene->tile_to_asset_table[TileId_collect] = ASSET_FIRST_GROUP_ID(collect);
	scene->tile_to_asset_table[TileId_concord] = AssetId_goggles;
	scene->tile_to_asset_table[TileId_rocket] = AssetId_rocket;

	scene->asset_id = asset_id;
	switch(scene->asset_id) {
		case AssetId_scene_city: {
			scene->name = (char *)"DUNDEE";

			break;
		}

		case AssetId_scene_highlands: {
			scene->name = (char *)"HIGHLANDS";

			break;
		}

		case AssetId_scene_ocean: {
			scene->name = (char *)"OCEAN";

			break;
		}

		case AssetId_scene_space: {
			scene->name = (char *)"SPACE";

			scene->tile_to_asset_table[TileId_clone] = AssetId_clone_space;
			scene->tile_to_asset_table[TileId_rocket] = AssetId_clone_space;

			break;
		}

		INVALID_CASE();
	}

#if DEBUG_ENABLED
	for(u32 i = 0; i < ARRAY_COUNT(scene->tile_to_asset_table); i++) {
		ASSERT(scene->tile_to_asset_table[i]);
	}
#endif

	for(u32 i = 0; i < ARRAY_COUNT(scene->layers); i++) {
		Entity * entity = scene->layers[i];
		if(entity) {
			change_entity_asset(entity, assets, asset_ref(scene->asset_id, i));
		}
	}

	if(scene_id == SceneId_lower) {
		Entity * bg0 = main_state->background[0];
		Entity * bg1 = main_state->background[1];

		u32 bg_index = asset_id - ASSET_FIRST_GROUP_ID(lower_scene);
		if(bg1->asset.index != bg_index) {
			bg0->asset.index = bg1->asset.index;
			bg0->color.a = 1.0f;

			bg1->asset.index = bg_index;
			bg1->color.a = 0.0f;
		}

		main_state->sun->color.a = asset_id == AssetId_scene_city ? 1.0f : 0.0f;
	}
}

void switch_lower_scene_asset_type(MainMetaState * main_state, AssetState * assets) {
	u32 id = main_state->scenes[SceneId_lower].asset_id + 1;
	if(id > ASSET_LAST_GROUP_ID(lower_scene)) {
		id = ASSET_FIRST_GROUP_ID(lower_scene);
	}

	change_scene_asset_type(main_state, SceneId_lower, assets, (AssetId)id);
}

void begin_concord_sequence(MetaState * meta_state) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	Player * player = &main_state->player;
	Concord * concord = &main_state->concord;
	concord->playing = true;

	change_player_speed(main_state, player, main_state->boost_accel, main_state->boost_time);

	f32 width = math::rec_dim(get_entity_render_bounds(meta_state->assets, concord->e)).x;
	concord->e->pos.x = -(f32)main_state->render_group->transform.projection_width * 0.5f - width * 0.5f;

	main_state->score_system.concord = true;
}

void begin_rocket_sequence(MetaState * meta_state) {
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
		rocket->color.a = 1.0f;

		fire_audio_clip(meta_state->audio_state, AssetId_rocket_sfx);

		Player * player = &main_state->player;
		player->invincibility_time = F32_MAX;

		fade_out_audio_clip(&main_state->music, 1.0f);
		main_state->music = play_audio_clip(meta_state->audio_state, AssetId_space_music, true);
		change_volume(main_state->music, math::vec2(0.0f), 0.0f);
		change_volume(main_state->music, math::vec2(1.0f), 1.0f);

		main_state->score_system.rocket = true;
	}
}

void play_rocket_sequence(MetaState * meta_state, f32 dt) {
	ASSERT(meta_state->type == MetaStateType_main);
	MainMetaState * main_state = meta_state->main;

	RocketSequence * seq = &main_state->rocket_seq;

	if(seq->playing) {
		RenderState * render_state = meta_state->render_state;
		Player * player = &main_state->player;
		Entity * rocket = seq->rocket;

		rocket->anim_time += dt * ANIMATION_FRAMES_PER_SEC;
		rocket->asset.index = (u32)rocket->anim_time % get_asset_count(meta_state->assets, rocket->asset.id);

		rocket->d_pos += rocket->speed * dt;
		rocket->d_pos *= (1.0f - rocket->damp);

		math::Vec2 d_pos = rocket->d_pos * dt;
		rocket->pos.xy += d_pos;

		if(!player->dead) {
			f32 new_time = seq->time_ + dt;

			if(main_state->current_scene != SceneId_upper) {
				f32 drop_height = main_state->scenes[SceneId_upper].y + main_state->height_above_ground;

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

					change_scene(main_state, SceneId_upper);
				}
			}

			f32 seq_max_time = 15.0f;
			if(seq->time_ < seq_max_time) {
				if(new_time > seq_max_time) {
					fade_out_audio_clip(&main_state->music, 1.0f);
					main_state->music = play_audio_clip(meta_state->audio_state, AssetId_game_music, true);
					change_volume(main_state->music, math::vec2(0.0f), 0.0f);
					change_volume(main_state->music, math::vec2(1.0f), 1.0f);
					
					player->allow_input = false;
					main_state->accel_time = 0.0f;
					player->invincibility_time = F32_MAX;
					player->e->use_gravity = true;
					change_entity_asset(player->e, meta_state->assets, asset_ref(AssetId_dolly_fall));

					switch_lower_scene_asset_type(main_state, meta_state->assets);
				}
			}
			else {
				//TODO: Calculate the ideal distance to stop gravity so that the player lands roughly at the height_above_ground
				if(player->e->pos.y < (f32)main_state->render_group->transform.projection_height * 1.8f) {
					change_scene(main_state, SceneId_lower);

					player->e->use_gravity = false;
					player->allow_input = true;
					player->invincibility_time = 1.0f;
					change_entity_asset(player->e, meta_state->assets, asset_ref(AssetId_dolly_idle));

					seq->playing = false;
					rocket->color.a = 0.0f;
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

void change_info_display(InfoDisplay * display, AssetId asset_id) {
	display->asset.id = asset_id;
	display->asset.index = 0;
	display->time_ = 0.0f;
	display->alpha = 1.0f;
	display->scale = 2.0f;
}

UiElement * push_ui_elem(UiLayer * ui_layer, u32 id, AssetId asset_id, AssetId clip_id) {
	ASSERT(ui_layer->elem_count < ARRAY_COUNT(ui_layer->elems));

	UiElement * elem = ui_layer->elems + ui_layer->elem_count++;
	elem->id = id;
	elem->asset = asset_ref(asset_id);
	elem->clip_id = clip_id;
	return elem;
}

void change_page(MenuMetaState * menu_state, MenuPageId next_page) {
	UiLayer * current_page = menu_state->pages + menu_state->current_page;
	for(u32 i = 0; i < current_page->elem_count; i++) {
		UiElement * elem = current_page->elems + i;
		elem->asset.index = 0;
	}

	menu_state->current_page = next_page;
	current_page->interact_elem = 0;
}

void show_score(ScoreSystem * score_system) {
	score_system->show = true;
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
			ui_layer->interact_elem->asset.index = 1;


		}
	}
 
	if(ui_layer->interact_elem) {
		if(ui_layer->interact_elem != hot_elem || game_input->mouse_button & KEY_RELEASED) {
			ui_layer->interact_elem = 0;
		}
		else {
			ui_layer->interact_elem->asset.index = 1;

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
		push_textured_quad(render_group, elem->asset);
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

	menu_state->music = play_audio_clip(meta_state->audio_state, AssetId_menu_music, true, math::vec2(0.0f));
	change_volume(menu_state->music, math::vec2(1.0f), 1.0f);

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

	push_ui_elem(&intro_state->ui_layer, 0, AssetId_btn_skip, AssetId_click_yes);
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

	main_state->music = play_audio_clip(meta_state->audio_state, AssetId_game_music, true, math::vec2(0.0f));
	change_volume(main_state->music, math::vec2(1.0f), 1.0f);

	main_state->render_group = allocate_render_group(render_state, &meta_state->arena, screen_width, screen_height, 1024);
	main_state->ui_render_group = allocate_render_group(render_state, &meta_state->arena, screen_width, screen_height, 512);
	main_state->letterboxed_height = (f32)screen_height;
	main_state->fixed_letterboxing = 80.0f;

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

	f32 scene_max_z = layer_z_offsets[0];
	f32 scene_y_offset = (f32)screen_height / (1.0f / (scene_max_z + 1.0f));

	for(u32 i = 0; i < ARRAY_COUNT(main_state->background); i++) {
		Entity * entity = push_entity(entities, meta_state->assets, asset_ref(AssetId_background, i));

		f32 z = scene_max_z;

		math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
		f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

		entity->pos.y = y_offset;
		entity->pos.z = z;
		entity->scale.x = (f32)screen_width / dim.x;
		entity->scrollable = true;

		main_state->background[i] = entity;
	}

	change_scene_asset_type(main_state, SceneId_lower, meta_state->assets, AssetId_scene_city);
	change_scene_asset_type(main_state, SceneId_upper, meta_state->assets, AssetId_scene_space);

	main_state->background[0]->color.a = 0.0f;
	main_state->background[1]->color.a = 1.0f;

	{
		AssetRef asset = asset_ref(AssetId_sun);

		//TODO: Unproject texture alignment properly!!
		math::Vec2 align = math::rec_pos(get_asset_bounds(meta_state->assets, asset.id, asset.index));
		math::Vec3 pos = unproject_pos(align, scene_max_z);

		main_state->sun = push_entity(entities, meta_state->assets, asset, pos);
	}

	for(u32 i = 0; i < SceneId_count; i++) {	
		Scene * scene = main_state->scenes + i;

		scene->y = scene_y_offset * i;
		scene->tint = math::vec4(1.0f);
	
		for(u32 layer_index = 0; layer_index < ARRAY_COUNT(scene->layers) - 1; layer_index++) {
			Entity * entity = push_entity(entities, meta_state->assets, asset_ref(scene->asset_id, layer_index));

			f32 z = layer_z_offsets[layer_index];

			math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
			f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

			entity->pos.y = scene->y + y_offset;
			entity->pos.z = z;
			entity->scrollable = true;

			scene->layers[layer_index] = entity;
		}
	}

	main_state->scenes[SceneId_lower].map_id = AssetId_tutorial_map;
	main_state->scenes[SceneId_upper].map_id = AssetId_upper_map;
	main_state->current_scene = SceneId_lower;

	Player * player = &main_state->player;
	player->clone_offset = math::vec2(-5.0f, 0.0f);

	for(i32 i = ARRAY_COUNT(player->clones) - 1; i >= 0; i--) {
		Entity * entity = push_entity(entities, assets, asset_ref(AssetId_clone));
		entity->scale = math::vec2(0.75f);
		entity->color.rgb = math::lerp(math::rand_vec3(), math::vec3(1.0f), 0.75f);
		entity->color.a = 0.0f;

		entity->hidden = true;

		player->clones[i] = entity;
	}

	player->initial_pos = math::vec3((f32)screen_width * -0.25f, main_state->height_above_ground, 0.0f);
	player->e = push_entity(entities, assets, asset_ref(AssetId_dolly_idle), player->initial_pos);
	player->e->speed = math::vec2(750.0f, 4000.0f);
	player->e->damp = 0.1f;
	player->allow_input = true;

	EntityEmitter * emitter = &main_state->entity_emitter;
	emitter->pos = math::vec3(screen_width * 0.75f, main_state->height_above_ground, 0.0f);
	for(u32 i = 0; i < ARRAY_COUNT(emitter->entity_array); i++) {
		emitter->entity_array[i] = push_entity(entities, assets, asset_ref(ASSET_FIRST_GROUP_ID(collect)), emitter->pos);
	}

	RocketSequence * seq = &main_state->rocket_seq;
	seq->rocket = push_entity(entities, assets, asset_ref(AssetId_rocket_large));
	seq->rocket->collider = math::rec_offset(seq->rocket->collider, math::vec2(0.0f, -math::rec_dim(seq->rocket->collider).y * 0.35f));
	seq->rocket->color.a = 0.0f;
	seq->playing = false;
	seq->time_ = 0.0f;

	Concord * concord = &main_state->concord;
	concord->e = push_entity(entities, assets, asset_ref(AssetId_concord), math::vec3(screen_width * 4.0f, main_state->height_above_ground, 0.0f));

	f32 cloud_y_offsets[ARRAY_COUNT(main_state->clouds)] = {
		-220.0f,
		//TODO: Automatically position this based on the cloud height??
		main_state->max_height_above_ground + (f32)screen_height * 0.5f - 140.0f,
	};

	for(u32 i = 0; i < ARRAY_COUNT(main_state->clouds); i++) {
		Entity * entity = push_entity(entities, meta_state->assets, asset_ref(AssetId_clouds), math::vec3(0.0f, cloud_y_offsets[i], 0.0f));
		entity->scrollable = true;
		// entity->color.a = 0.0f;

		main_state->clouds[i] = entity;
	}

	//TODO: Adding foreground layer at the end until we have sorting!!
	for(u32 i = 0; i < SceneId_count; i++) {
		Scene * scene = main_state->scenes + i;

		u32 layer_index = ARRAY_COUNT(scene->layers) - 1;

		Entity * entity = push_entity(entities, assets, asset_ref(scene->asset_id, layer_index));

		f32 z = layer_z_offsets[layer_index];

		math::Vec2 dim = math::rec_dim(get_entity_render_bounds(meta_state->assets, entity));
		f32 y_offset = ((dim.y - (f32)screen_height) * 0.5f) * (z + 1.0f);

		entity->pos.y = scene->y + y_offset;
		entity->pos.z = z;
		entity->scrollable = true;

		scene->layers[layer_index] = entity;
	}

	ScoreSystem * score_system = &main_state->score_system;
	push_ui_elem(&score_system->ui, ScoreButtonId_back, AssetId_btn_back, AssetId_click_no);
	push_ui_elem(&score_system->ui, ScoreButtonId_replay, AssetId_btn_replay, AssetId_click_yes);
	score_system->values[ScoreValueId_clones] = { (char *)"Clones", 0, 10, true };
	score_system->values[ScoreValueId_items] = { (char *)"Items", 0, 1000, true };
	score_system->values[ScoreValueId_rocket] = { (char *)"Rocket bonus", 0, 500, false };
	score_system->values[ScoreValueId_concord] = { (char *)"Concord bonus", 0, 500, false };
	score_system->values[ScoreValueId_time_played] = { (char *)"Time played", 0, 10, true };

	main_state->arrow_buttons[0].asset = asset_ref(AssetId_btn_up);
	main_state->arrow_buttons[1].asset = asset_ref(AssetId_btn_down);

	main_state->info_display.asset = asset_ref(ASSET_FIRST_GROUP_ID(collect));

	main_state->start_time = 60.0f;
	main_state->clock_pickup_time = 10.0f;

	main_state->boost_letterboxing = 80.0f;
	main_state->boost_speed = 3.0f;
	main_state->boost_accel = 40.0f;
	main_state->boost_time = 4.0f;
	main_state->slow_down_speed = 0.25f;
	main_state->slow_down_time = 1.0f;

	//TODO: Make this platform independent!!
#if __EMSCRIPTEN__ && DEV_ENABLED
	main_state->start_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.start_time });
	main_state->clock_pickup_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.clock_pickup_time });

	main_state->boost_always_on = EM_ASM_INT_V({ return Prefs.boost_always_on });
	main_state->boost_letterboxing = (f32)EM_ASM_DOUBLE_V({ return Prefs.boost_letterboxing });
	main_state->boost_speed = (f32)EM_ASM_DOUBLE_V({ return Prefs.boost_speed });
	main_state->boost_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.boost_time });
	main_state->slow_down_speed = (f32)EM_ASM_DOUBLE_V({ return Prefs.slow_down_speed });
	main_state->slow_down_time = (f32)EM_ASM_DOUBLE_V({ return Prefs.slow_down_time; });
#endif

	main_state->max_time = main_state->start_time;
	main_state->countdown_time = 10.0f;
	main_state->time_remaining = main_state->start_time;

	main_state->clock_label_scale = 1.0f;
	main_state->clock_label_alpha = 1.0f;
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

		change_meta_state(game_state, MetaStateType_main);

		game_state->auto_save_time = 5.0f;
		game_state->save.code = SAVE_FILE_CODE;
		game_state->save.version = SAVE_FILE_VERSION;
		game_state->save.plays = 0;

		game_state->debug_render_group = allocate_render_group(&game_state->render_state, &game_state->memory_arena, game_input->back_buffer_width, game_input->back_buffer_height, 1024);
		game_state->debug_str = allocate_str(&game_state->memory_arena, 1024);
		game_state->debug_show_overlay = false;

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
		game_state->debug_show_overlay = !game_state->debug_show_overlay;
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
				if(menu_state->play_transition_id && menu_state->music) {
					fade_out_audio_clip(&menu_state->music, 1.0f);
				}

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

			UiElement * interact_elem = process_ui_layer(meta_state, &intro_state->ui_layer, intro_state->render_group, game_input);
			if(!game_state->transitioning) {
				if(interact_elem) {
					intro_state->transition_id = begin_transition(game_state);
				}

				f32 frame_index = (u32)intro_state->anim_time + 1;
				intro_state->anim_time += game_input->delta_time * 1.0f;

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
					game_state->time_until_next_save = 0.0f;
					change_meta_state(game_state, new_state);
				}
			}

			if(game_input->buttons[ButtonId_mute] & KEY_PRESSED) {
				switch_lower_scene_asset_type(main_state, meta_state->assets);
			}

			
			{
				InfoDisplay * info_display = &main_state->info_display;
				info_display->time_ += game_input->delta_time;
				info_display->scale = math::lerp(info_display->scale, 1.0f, game_input->delta_time * 12.0f);
				if(info_display->time_ > 3.0f) {
					info_display->alpha -= game_input->delta_time * 12.0f;
				}
			}

			ScoreSystem * score_system = &main_state->score_system;
			b32 time_frozen = main_state->concord.playing || main_state->rocket_seq.playing;
			
			if(!player->dead) {
				f32 dt = game_input->delta_time;

				if(!time_frozen) {
					f32 new_time_remaining = main_state->time_remaining - dt;
					if(new_time_remaining < 0.0f) {
						dt = -new_time_remaining;
						new_time_remaining = 0.0f;

						kill_player(main_state, player);
					}

					main_state->time_remaining = new_time_remaining;
				}

				score_system->time_played += dt;
			}
			else {
				if(player->e->pos.x > (f32)render_transform->projection_width * 2.0f) {
					show_score(score_system);
				}
			}

			main_state->clock_label_scale = math::lerp(main_state->clock_label_scale, 1.0f, game_input->delta_time * 12.0f);
			main_state->clock_label_alpha = ((u32)(math::clamp(main_state->time_remaining, 0.0f, 10.0f) * 4.0f) & 1) == 0 ? 1.0f : 0.0f;

			main_state->accel_time -= game_input->delta_time;
			if(main_state->accel_time <= 0.0f) {
				main_state->dd_speed = 0.0f;
				main_state->accel_time = 0.0f;
			}

			if(main_state->boost_always_on) {
				main_state->dd_speed = main_state->boost_accel;
				main_state->accel_time = F32_MAX;
			}

			main_state->d_speed += main_state->dd_speed * game_input->delta_time;
			main_state->d_speed *= 0.9f;

			f32 player_d_pos = player->e->speed.x * math::clamp(main_state->d_speed + 1.0f, main_state->slow_down_speed, main_state->boost_speed) * game_input->delta_time;

			f32 letterboxing = math::max(main_state->d_speed, 0.0f) * (main_state->boost_letterboxing / 3.0f);
			letterboxing = math::min(letterboxing, main_state->boost_letterboxing * 2.0f);
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

				entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
				entity->asset.index = ((u32)entity->anim_time + entity->rand_id) % get_asset_count(assets, entity->asset.id);
			}

			math::Vec2 player_dd_pos = math::vec2(0.0f);

			for(u32 i = 0; i < ARRAY_COUNT(main_state->arrow_buttons); i++) {
				UiElement * elem = main_state->arrow_buttons + i;
				elem->asset.index = 0;
			}

			AssetId player_idle_id = AssetId_dolly_idle;
			AssetId player_up_id = AssetId_dolly_up;
			AssetId player_down_id = AssetId_dolly_down;
			if(main_state->current_scene == SceneId_upper) {
				player_idle_id = AssetId_dolly_space_idle;
				player_up_id = AssetId_dolly_space_up;
				player_down_id = AssetId_dolly_space_down;
			}

			if(!player->dead) {
				if(player->allow_input) {
					f32 half_buffer_height = (f32)game_input->back_buffer_height * 0.5f;
					
					if(game_input->buttons[ButtonId_up] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y < half_buffer_height)) {
						player_dd_pos.y += player->e->speed.y;

						main_state->arrow_buttons[0].asset.index = 1;
						change_entity_asset(player->e, meta_state->assets, asset_ref(player_up_id));
					}

					if(game_input->buttons[ButtonId_down] & KEY_DOWN || (game_input->mouse_button & KEY_DOWN && game_input->mouse_pos.y >= half_buffer_height)) {
						player_dd_pos.y -= player->e->speed.y;

						main_state->arrow_buttons[1].asset.index = 1;
						change_entity_asset(player->e, meta_state->assets, asset_ref(player_down_id));
					}

					if(!player_dd_pos.y) {
						change_entity_asset(player->e, meta_state->assets, asset_ref(player_idle_id));
					}
				}
			}
			else {
				player_dd_pos.x += player->e->speed.x * 10.0f;
				change_entity_asset(player->e, meta_state->assets, asset_ref(player_idle_id));
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
			//TODO: Really player d_pos should be driving d_speed and not the other way around!!
			if(!player->dead) {
				f32 offset_x = math::clamp(main_state->d_speed * 40.0f, -80.0f, 40.0f);
				player->e->pos.x = math::lerp(player->e->pos.x, player->initial_pos.x + offset_x, game_input->delta_time * 8.0f);
			}

			if(!player->dead && player->allow_input) {
				f32 scene_y = main_state->scenes[main_state->current_scene].y;

				f32 ground_height = scene_y;
				if(player->e->pos.y <= ground_height) {
					player->e->pos.y = ground_height;
					player->e->d_pos.y = 0.0f;
				}

				f32 max_height = scene_y + main_state->max_height_above_ground;
				if(player->e->pos.y >= max_height && player->e->d_pos.y > 0.0f) {
					player->e->pos.y = max_height;
					player->e->d_pos.y = 0.0f;
				}
			}

			//TODO: This could be pulled out!!
			player->e->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
			player->e->asset.index = (u32)player->e->anim_time % get_asset_count(assets, player->e->asset.id);

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

			{
				Concord * concord = &main_state->concord;

				concord->e->pos.x += player_d_pos + game_input->delta_time * 2000.0f;
				if(main_state->dd_speed > 1.0f) {
					concord->e->pos.x = math::min(concord->e->pos.x, player->e->pos.x - 465.0f);
					concord->e->pos.y = player->e->pos.y;
				}
				else {
					concord->playing = false;
				}

				concord->e->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
				concord->e->asset.index = (u32)concord->e->anim_time % get_asset_count(assets, concord->e->asset.id);
			}

			f32 zero_height = main_state->scenes[main_state->current_scene].y + main_state->height_above_ground;

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
				play_rocket_sequence(meta_state, game_input->delta_time);
			}

			Scene * current_scene = main_state->scenes + main_state->current_scene;

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
				}

				entity->anim_time += game_input->delta_time * ANIMATION_FRAMES_PER_SEC;
				entity->asset.index = ((u32)entity->anim_time + entity->rand_id) % get_asset_count(assets, entity->asset.id);

				math::Rec2 bounds = get_entity_collider_bounds(entity);
				//TODO: When should this check happen??
				if(rec_overlap(player_bounds, bounds) && !player->dead && !entity->hit) {
					entity->hit = true;

					AssetId clip_id = AssetId_pickup;
					b32 is_slow_down = false;

					switch(entity->asset.id) {
						case AssetId_clone_space:
						case AssetId_clone: {
							push_player_clone(player);

							score_system->clones++;

							clip_id = AssetId_baa;

							break;
						}

						case AssetId_atom_smasher: {
							if(!player->invincibility_time) {	
								pop_player_clones(player, 50);
								is_slow_down = true;
							}

							clip_id = AssetId_bang;

							break;
						}

						case AssetId_rocket: {
							begin_rocket_sequence(meta_state);

							break;
						}

						case AssetId_clock: {
							main_state->time_remaining += main_state->clock_pickup_time;
							main_state->time_remaining = math::min(main_state->time_remaining, main_state->max_time);

							main_state->clock_label_scale = 2.0f;

							break;
						}

						case AssetId_goggles: {
							begin_concord_sequence(meta_state);

							break;
						}

						default: {
							break;
						}
					}

					if(!player->invincibility_time && is_slow_down) {
						change_player_speed(main_state, player, -main_state->boost_accel, main_state->slow_down_time);
					}

					if(entity->asset.id >= ASSET_FIRST_GROUP_ID(collect) && entity->asset.id <= ASSET_LAST_GROUP_ID(collect)) {
						u32 collect_index = ASSET_ID_TO_GROUP_INDEX(collect, entity->asset.id);
						ASSERT(collect_index < ARRAY_COUNT(game_state->save.collect_unlock_states));

						game_state->save.collect_unlock_states[collect_index] = true;
						game_state->time_until_next_save = 0.0f;

						score_system->items++;

						change_info_display(&main_state->info_display, entity->asset.id);

						clip_id = AssetId_special;
					}
					
					//TODO: Should there be a helper function for this??
					AudioClip * clip = get_audio_clip_asset(assets, clip_id, math::rand_i32() % get_asset_count(assets, clip_id));
					f32 pitch = math::lerp(0.9f, 1.1f, math::rand_f32());
					fire_audio_clip(meta_state->audio_state, clip, math::vec2(1.0f), pitch);

					//TODO: Allocate these separately (or bring closer when we have sorting) for more efficient batching!!
					change_entity_asset(entity, assets, asset_ref(AssetId_shield));
					entity->anim_time = 0.0f;
					entity->scale = math::vec2(0.5f);
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

				AssetId map_id = current_scene->map_id;
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
							//TODO: Should we clear the emitter here??
							if(map_id == AssetId_tutorial_map) {
								ASSERT(current_scene == &main_state->scenes[main_state->current_scene]);
								map_id = current_scene->map_id = AssetId_lower_map;
							}

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

								AssetId asset_id = current_scene->tile_to_asset_table[tile_id];
								if(tile_id == TileId_collect) {
									asset_id = (AssetId)(ASSET_FIRST_GROUP_ID(collect) + math::rand_u32() % ASSET_GROUP_COUNT(collect));
								}

								change_entity_asset(entity, assets, asset_ref(asset_id));
								if(tile_id == TileId_bad) {
									entity->collider = math::rec_scale(get_asset_bounds(assets, entity->asset.id, entity->asset.index), 0.5f);
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

			move_entity(meta_state, render_transform, main_state->sun, 0.0f);

			for(u32 i = 0; i < ARRAY_COUNT(main_state->clouds); i++) {
				move_entity(meta_state, render_transform, main_state->clouds[i], player_d_pos * 1.5f);
			}

			for(u32 i = 0; i < SceneId_count; i++) {
				Scene * scene = main_state->scenes + i;
				for(u32 layer_index = 0; layer_index < ARRAY_COUNT(scene->layers); layer_index++) {
					Entity * entity = scene->layers[layer_index];

					f32 d_pos = player_d_pos;
#if 1
					//TODO: HACK!!!!!
					if(layer_index == ARRAY_COUNT(scene->layers) - 1) {
						d_pos *= 2.0f;
					}
#endif

					//TODO: Could we actually just use the emitter cursor to position these??
					move_entity(meta_state, render_transform, scene->layers[layer_index], d_pos);
				}
			}

			if(score_system->show) {
				UiElement * interact_elem = process_ui_layer(meta_state, &score_system->ui, main_state->ui_render_group, game_input);

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

				score_system->values[ScoreValueId_clones].value = score_system->clones;
				score_system->values[ScoreValueId_items].value = score_system->items;
				score_system->values[ScoreValueId_time_played].value = (u32)(score_system->time_played + 0.5f);
				score_system->values[ScoreValueId_rocket].value = score_system->rocket ? 1 : 0;
				score_system->values[ScoreValueId_concord].value = score_system->concord ? 1 : 0;

				f32 delay_time = 0.5f;

				if(score_system->value_count < ARRAY_COUNT(score_system->values)) {
					if(score_system->time_ >= delay_time) {
						score_system->value_count++;
						score_system->time_ = 0.0f;
					}
				}
				else {
					score_system->target_total = 0;
					for(u32 i = 0; i < ARRAY_COUNT(score_system->values); i++) {
						ScoreValue * value = score_system->values + i;
						score_system->target_total += value->value * value->points_per_value;
					}

					score_system->display_total = (u32)math::lerp((f32)score_system->current_total, (f32)score_system->target_total, math::clamp01(score_system->time_ - delay_time));
				}

				score_system->time_ += game_input->delta_time;
			}

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

			push_textured_quad(render_group, asset_ref(AssetId_menu_background, menu_state->current_page));

			if(menu_state->current_page == MenuPageId_main) {
				u32 display_item_count = ASSET_GROUP_COUNT(menu_collect);
				for(u32 i = 0; i < ARRAY_COUNT(game_state->save.collect_unlock_states); i++) {
					if(game_state->save.collect_unlock_states[i]) {
						push_textured_quad(render_group, asset_ref(ASSET_GROUP_INDEX_TO_ID(menu_collect, i)));
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

			push_textured_quad(render_group, asset_ref(AssetId_intro_background));

			for(u32 i = 0; i < ARRAY_COUNT(intro_state->frames); i++) {
				IntroFrame * frame = intro_state->frames + i;
				push_textured_quad(render_group, asset_ref(AssetId_intro, i), math::vec3(0.0f), math::vec2(1.0f), 0.0f, math::vec4(1.0f, 1.0f, 1.0f, frame->alpha));
			}

			push_ui_layer_to_render_group(&intro_state->ui_layer, render_group);

			render_and_clear_render_group(meta_state->render_state, render_group);

			break;
		}

		case MetaStateType_main: {
			MetaState * meta_state = get_meta_state(game_state, MetaStateType_main);
			MainMetaState * main_state = meta_state->main;

			EntityArray * entities = &main_state->entities;
			for(u32 i = 0; i < entities->count; i++) {
				Entity * entity = entities->elems + i;
				push_textured_quad(main_state->render_group, entity->asset, entity->pos + math::vec3(entity->offset, 0.0f), entity->scale, entity->angle, entity->color, entity->scrollable);
			}

			render_and_clear_render_group(meta_state->render_state, main_state->render_group);

			if(game_state->debug_show_overlay) {
				RenderBatch * render_batch = render_state->render_batch;
				render_batch->tex = get_texture_asset(render_state->assets, AssetId_white, 0);
				render_batch->mode = RenderMode_lines;
				
				RenderTransform * render_transform = &main_state->render_group->transform;
				math::Mat4 projection = math::orthographic_projection((f32)render_transform->projection_width, (f32)render_transform->projection_height);

				for(u32 i = 0; i < entities->count; i++) {
					Entity * entity = entities->elems + i;

					math::Rec2 bounds = math::rec_scale(entity->collider, entity->scale);
					math::Vec2 pos = project_pos(render_transform, entity->pos + math::vec3(math::rec_pos(bounds) + entity->offset, 0.0f));
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
				push_colored_quad(ui_render_group, math::vec3(0.0f, projection_dim.y - letterbox_pixels, 0.0f), projection_dim, 0.0f, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
				push_colored_quad(ui_render_group, math::vec3(0.0f,-projection_dim.y + letterbox_pixels, 0.0f), projection_dim, 0.0f, math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			}

			f32 fixed_letterboxing = main_state->fixed_letterboxing;

			Font * font = get_font_asset(meta_state->assets, AssetId_supersrc, 0);
			//TODO: Need a convenient way to make temp strs!!
			char temp_buf[256];
			Str temp_str = str_fixed_size(temp_buf, ARRAY_COUNT(temp_buf));

			{
				math::Vec3 label_pos = math::vec3(0.0f, projection_dim.y * 0.5f - (main_state->fixed_letterboxing + 12.0f * main_state->clock_label_scale), 0.0f);
				math::Vec4 label_color = math::vec4(1.0f, 1.0f, 1.0f, main_state->clock_label_alpha);
				push_textured_quad(ui_render_group, asset_ref(AssetId_label_clock), label_pos, math::vec2(main_state->clock_label_scale), 0.0f, label_color);

				str_clear(&temp_str);
				str_print(&temp_str, "%u", (u32)math::ceil_to_i32(main_state->time_remaining));

				f32 font_scale = 1.0f * main_state->clock_label_scale;
				FontLayout font_layout = create_font_layout(font, projection_dim, font_scale, FontLayoutAnchor_top_centre, math::vec2(3.0f, -main_state->fixed_letterboxing + 20.0f * font_scale), false);
				push_str_to_render_group(ui_render_group, font, &font_layout, &temp_str, math::vec4(0.0f, 0.0f, 0.0f, main_state->clock_label_alpha));
			}

			{
				FontLayout font_layout = create_font_layout(font, projection_dim, 1.0f, FontLayoutAnchor_bottom_centre, math::vec2(0.0f, fixed_letterboxing - (font->ascent - font->descent) - 21.0f));
				push_c_str_to_render_group(ui_render_group, font, &font_layout, main_state->scenes[main_state->current_scene].name);
			}

			{
				InfoDisplay * info_display = &main_state->info_display;
				u32 info_index = ASSET_ID_TO_GROUP_INDEX(collect, info_display->asset.id);

				char const * info_strs[] = {
					"Dolly found the WW Stool!!",
					"Dolly found the Hamilton-Rothschild tazza!!",
					"Dolly found the Mini Cooper!!",
					"Dolly found the mobile phone!!",
					"Dolly found the Coral Cement necklace!!",
					"Dolly found the McQueen ankle boots!!",
				};

				FontLayout font_layout = create_font_layout(font, projection_dim, info_display->scale, FontLayoutAnchor_top_centre, math::vec2(0.0f, -170.0f), false);
				push_c_str_to_render_group(ui_render_group, font, &font_layout, info_strs[info_index], math::vec4(1.0f, 1.0f, 1.0f, info_display->alpha));
			}

			//TODO: Bake the offset into the texture!!
			push_textured_quad(ui_render_group, main_state->arrow_buttons[0].asset, math::vec3(0.0f, 30.0f, 0.0f));
			push_textured_quad(ui_render_group, main_state->arrow_buttons[1].asset, math::vec3(0.0f,-30.0f, 0.0f));

			ScoreSystem * score_system = &main_state->score_system;
			if(score_system->show) {
				push_colored_quad(ui_render_group, math::vec3(0.0f), projection_dim, 0.0f, math::vec4(0.0f, 0.0f, 0.0f, 0.8f));

				str_clear(&temp_str);
				str_print(&temp_str, "Score\n");
				str_print(&temp_str, "\n");

				FontLayout title_layout = create_font_layout(font, projection_dim, 1.0f, FontLayoutAnchor_top_centre, math::vec2(0.0f, -(main_state->fixed_letterboxing + 40.0f)));
				push_str_to_render_group(ui_render_group, font, &title_layout, &temp_str);

				f32 offset_x = projection_dim.x * 0.25f;

				{
					str_clear(&temp_str);
					for(u32 i = 0; i < score_system->value_count; i++) {
						ScoreValue * score_value = score_system->values + i;
						if(score_value->show_value) {
							str_print(&temp_str, "%s: %u\n", score_value->name, score_value->value);
						}
						else {
							str_print(&temp_str, "%s\n", score_value->name);
						}
					}

					FontLayout font_layout = create_font_layout(font, projection_dim, 1.0f, FontLayoutAnchor_top_left, math::vec2(offset_x, 0.0f));
					font_layout.pos.y = title_layout.pos.y;
					push_str_to_render_group(ui_render_group, font, &font_layout, &temp_str);
				}

				{
					str_clear(&temp_str);
					for(u32 i = 0; i < score_system->value_count; i++) {
						ScoreValue * score_value = score_system->values + i;
						str_print(&temp_str, "+%u\n", score_value->value * score_value->points_per_value);
					}

					FontLayout font_layout = create_font_layout(font, projection_dim, 1.0f, FontLayoutAnchor_top_right, math::vec2(-offset_x, 0.0f));
					font_layout.pos.y = title_layout.pos.y;
					push_str_to_render_group(ui_render_group, font, &font_layout, &temp_str);
				}

				{
					str_clear(&temp_str);
					for(u32 i = 0; i < ARRAY_COUNT(score_system->values); i++) {
						str_print(&temp_str, "\n");
					}
					str_print(&temp_str, "\n");
					str_print(&temp_str, "Total: %u\n", score_system->display_total);

					FontLayout font_layout = create_font_layout(font, projection_dim, 1.0f, FontLayoutAnchor_top_centre);
					font_layout.pos.y = title_layout.pos.y;
					push_str_to_render_group(ui_render_group, font, &font_layout, &temp_str);					
				}

				push_ui_layer_to_render_group(&score_system->ui, ui_render_group);
			}

			render_and_clear_render_group(meta_state->render_state, ui_render_group);

			break;
		}

		INVALID_CASE();
	}

#if DEBUG_ENABLED
	if(game_state->debug_show_overlay) {
		RenderGroup * debug_render_group = game_state->debug_render_group;

		Font * debug_font = get_font_asset(assets, AssetId_pragmata_pro, 0);
		FontLayout debug_font_layout = create_font_layout(debug_font, math::vec2(render_state->back_buffer_width, render_state->back_buffer_height), 1.0f, FontLayoutAnchor_top_left, math::vec2(debug_font->whitespace_advance, 0.0f));
		push_str_to_render_group(debug_render_group, debug_font, &debug_font_layout, game_state->str);

		char temp_buf[128];
		Str temp_str = str_fixed_size(temp_buf, ARRAY_COUNT(temp_buf));
		str_print(&temp_str, "decompression time: %fms\n", debug_decompression_time);
		str_print(&temp_str, "dt: %fms\n", game_input->delta_time);
		str_print(&temp_str, "sources playing: %u | sources to free: %u\n", game_state->audio_state.debug_sources_playing, game_state->audio_state.debug_sources_to_free);
		str_print(&temp_str, "\n");
		push_str_to_render_group(debug_render_group, debug_font, &debug_font_layout, &temp_str);

		push_str_to_render_group(debug_render_group, debug_font, &debug_font_layout, game_state->debug_str);

		render_and_clear_render_group(render_state, debug_render_group);		
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

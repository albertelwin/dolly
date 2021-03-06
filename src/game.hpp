
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>
#include <render.hpp>

#define ANIMATION_FRAMES_PER_SEC 30
#define PARALLAX_LAYER_COUNT 4

struct UiElement {
	u32 id;

	AssetRef asset;
	AssetId clip_id;
};

struct UiLayer {
	u32 elem_count;
	UiElement elems[8];

	UiElement * interact_elem;

	AssetRef background;
};

struct Entity {
	math::Vec3 pos;
	math::Vec2 offset;
	math::Vec2 scale;
	math::Vec4 color;
	f32 angle;
	b32 scrollable;

	AssetRef asset;

	math::Vec2 d_pos;
	math::Vec2 speed;
	f32 damp;
	b32 use_gravity;
	math::Rec2 collider;

	f32 anim_time;
	b32 hit;
	f32 hit_time;

	u32 rand_id;
	b32 hidden;
};

struct EntityArray {
	u32 count;
	Entity elems[512];
};

struct Player {
	Entity * e;

	math::Vec3 initial_pos;

	b32 dead;
	b32 allow_input;
	f32 invincibility_time;

	Entity * clones[100];
	math::Vec2 clone_offset;
	u32 active_clone_count;

	Entity * shield_clones[12];
	Entity * shield;
	f32 shield_radius;
	b32 has_shield;
};

struct Concord {
	Entity * e;

	b32 playing;
};

struct EntityEmitter {
	f32 cursor;
	u32 last_read_pos;

	AssetId rocket_map_id;
	u32 rocket_map_index;
	u32 rocket_map_count;

	math::Vec3 pos;

	u32 entity_count;
	Entity * entity_array[256];
	Entity * glow;
};

enum SceneId {
	SceneId_lower,
	SceneId_upper,

	SceneId_count,
};

struct Scene {
	char * name;

	AssetId asset_id;
	Entity * layers[PARALLAX_LAYER_COUNT];
	f32 y;

	AssetId tile_to_asset_table[TileId_count];

	AssetId map_id;
	u32 map_count;
	u32 map_array[64];
	u32 map_index;
};

//TODO: Rename this to Rocket
struct RocketSequence {
	Entity * rocket;

	AssetId return_map_id;
	u32 return_map_count;
	u32 return_map_index;

	b32 playing;
	f32 time_;
	u32 transition_id;
};

struct ScoreSystem {
	b32 show;

	UiLayer ui;

	u32 clones;

	f32 alpha;
	f32 time_;
	u32 item_display_count;
};

enum MenuButtonId {
	MenuButtonId_play,
	MenuButtonId_about,
	MenuButtonId_back,
	MenuButtonId_baa,

	MenuButtonId_count,
};

enum MenuPageId {
	MenuPageId_main,
	MenuPageId_about,

	MenuPageId_count,
};

enum ScoreButtonId {
	ScoreButtonId_back,
	ScoreButtonId_replay,

	ScoreButtonId_count,
};

enum TransitionType {
	TransitionType_pixelate,
	TransitionType_fade,

	TransitionType_count,
	TransitionType_null = TransitionType_count,
};

enum IntroFrameFlags {
	IntroFrameFlags_play_once = 0x1,
};
struct IntroFrame {
	AssetRef asset;
	char * str;
	f32 scale;

	u32 flags;
};

struct InfoDisplay {
	AssetRef asset;
	u32 found_index;

	f32 time_;
	f32 total_time;
	f32 alpha;
	f32 scale;
};

enum ButtonId {
	ButtonId_up,
	ButtonId_down,

	ButtonId_quit,

	ButtonId_debug,
	ButtonId_debug_mode,
	ButtonId_debug_switch,

	ButtonId_count,
};

enum MetaStateType {
	MetaStateType_menu,
	MetaStateType_intro,
	MetaStateType_main,

	MetaStateType_count,
	MetaStateType_null = MetaStateType_count,
};

struct MetaStateHeader {
	MemoryArena arena;
	AssetState * assets;
	AudioState * audio_state;
	RenderState * render_state;

	MetaStateType type;
};

struct MenuMetaState {
	MetaStateHeader header;

	AudioSource * music;

	RenderGroup * render_group;

	UiLayer pages[MenuPageId_count];
	MenuPageId current_page;

	u32 play_transition_id;
	u32 about_transition_id;
	u32 back_transition_id;
};

struct IntroMetaState {
	MetaStateHeader header;

	AudioSource * music;

	RenderGroup * render_group;

	UiLayer ui_layer;

	IntroFrame frames[ASSET_GROUP_COUNT(intro)];
	u32 current_frame_index;
	b32 frame_visible;
	f32 time_;

	u32 end_transition_id;
};

struct MainMetaState {
	MetaStateHeader header;

	AudioSource * music;
	AudioSource * shield_loop;

	RenderGroup * render_group;
	RenderGroup * ui_render_group;
	f32 letterboxed_height;
	f32 fixed_letterboxing;

	EntityArray entities;
	math::Vec2 entity_gravity;

	f32 height_above_ground;
	f32 max_height_above_ground;
	f32 camera_movement;

	Entity * background[2];
	Entity * clouds[2];
	Entity * sun;

	Scene scenes[SceneId_count];
	SceneId current_scene;

	Player player;

	EntityEmitter entity_emitter;
	RocketSequence rocket_seq;

	Concord concord;

	//TODO: Could actually pack this into a single u32!!
	b32 item_found[ASSET_GROUP_COUNT(collect)];
	b32 item_removed_from_pool[ASSET_GROUP_COUNT(collect)];

	//TODO: Should this be part of the player??
	f32 d_speed;
	f32 dd_speed;
	f32 accel_time;

	b32 boost_always_on;
	f32 boost_letterboxing;
	f32 boost_speed;
	f32 boost_accel;
	f32 boost_time;
	f32 slow_down_speed;
	f32 slow_down_time;

	u32 clones_lost_on_hit;
	
	UiElement arrow_buttons[2];
	InfoDisplay info_display;
	f32 label_clone_scale;

	ScoreSystem score_system;

	u32 quit_transition_id;
	u32 replay_transition_id;
};

struct GameMemory {
	size_t size;
	u8 * ptr;

	b32 initialised;
};

struct GameInput {
	u32 back_buffer_width;
	u32 back_buffer_height;

	b32 audio_supported;

	f32 delta_time;
	f32 total_time;

	math::Vec2 mouse_pos;
	math::Vec2 last_mouse_pos;
	b32 hide_mouse;
	u8 mouse_button;

	u8 buttons[ButtonId_count];
};

struct GameState {
	MemoryArena arena;

	b32 loaded;
	AssetState assets;

	//TODO: This is nasty!!
	b32 initialised;

	AudioState audio_state;
	RenderState render_state;

	RenderGroup * loading_render_group;

	MetaStateType meta_state;
	MetaStateHeader * meta_states[MetaStateType_count];

	u32 transition_id;
	b32 transitioning;
	TransitionType transition_type;
	f32 transition_time;
	b32 transition_flip;

	RenderGroup * debug_render_group;
	Str * debug_str;
	b32 debug_show_overlay;
};

#endif
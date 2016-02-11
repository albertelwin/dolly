
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <platform.hpp>

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>
#include <render.hpp>

#define ANIMATION_FRAMES_PER_SEC 30
#define PARALLAX_LAYER_COUNT 4

#define ENTITY_NULL_POS math::vec3(F32_MAX, F32_MAX, 0.0f)

//TODO: Formalise sprite animation!!
// struct SpriteAnimation {
// 	Sprite * sprites;
// 	f32 anim_time;
// };

struct UiElement {
	u32 id;

	AssetId asset_id;
	u32 asset_index;
};

struct Entity {
	math::Vec3 pos;
	f32 scale;
	math::Vec4 color;
	b32 scrollable;

	AssetId asset_id;
	u32 asset_index;

	math::Vec2 d_pos;
	math::Vec2 speed;
	f32 damp;
	b32 use_gravity;
	math::Rec2 collider;

	f32 anim_time;
	b32 hit;
	f32 hit_time;

	u32 rand_id;
	math::Vec2 chain_pos;
	b32 hidden;
};

struct EntityArray {
	u32 count;
	Entity elems[512];
};

struct Player {
	Entity * e;
	Entity * sheild;

	b32 dead;
	b32 invincible;
	b32 allow_input;

	Entity * clones[50];
	math::Vec2 clone_offset;
	u32 active_clone_count;
};

struct EntityEmitter {
	f32 cursor;
	u32 last_read_pos;
	u32 map_index;

	math::Vec3 pos;

	u32 entity_count;
	Entity * entity_array[256];
};

enum LocationId {
	LocationId_city,
	LocationId_mountains,
	LocationId_ocean,
	LocationId_space,

	LocationId_count,
	LocationId_null = LocationId_count,
};

struct Location {
	char * name;

	AssetId asset_id;
	Entity * layers[PARALLAX_LAYER_COUNT];
	f32 y;
	math::Vec4 tint;

	AssetId tile_to_asset_table[TileId_count];
};

struct RocketSequence {
	Entity * rocket;

	b32 playing;
	f32 time_;
	u32 transition_id;
};

enum ScoreValueId {
	ScoreValueId_time_played,
	ScoreValueId_points,

	ScoreValueId_count,
};

struct ScoreValue {
	f32 display;
	f32 time_;

	b32 is_f32;
	union {
		f32 f32_;
		u32 u32_;
	};
};

enum MenuButtonId {
	MenuButtonId_play,
	MenuButtonId_score,
	MenuButtonId_credits,
	MenuButtonId_back,

	MenuButtonId_count,
};

enum MenuPageId {
	MenuPageId_main,
	MenuPageId_hiscore,
	MenuPageId_credits,

	MenuPageId_count,
};

struct MenuPage {
	u32 button_count;
	UiElement buttons[3];
};

enum ScoreButtonId {
	ScoreButtonId_menu,
	ScoreButtonId_replay,

	ScoreButtonId_count,
};

enum TransitionType {
	TransitionType_pixelate,
	TransitionType_fade,

	TransitionType_count,
	TransitionType_null = TransitionType_count,
};

struct IntroFrame {
	f32 alpha;
};

enum ButtonId {
	ButtonId_start,
	ButtonId_left,
	ButtonId_right,
	ButtonId_up,
	ButtonId_down,

	ButtonId_quit,
	ButtonId_mute,

	ButtonId_debug,

	ButtonId_count,
};

#define SAVE_FILE_CODE 0x594C4C44 //"DLLY"
//TODO: Can we inc this automatically somehow??
#define SAVE_FILE_VERSION 5
#pragma pack(push, 1)
struct SaveFileHeader {
	u32 code;
	u32 version;

	u32 plays;
	u32 high_score;
	f32 longest_run;

	b32 collectable_unlock_states[ASSET_GROUP_COUNT(collectable)];
};
#pragma pack(pop)

enum MetaStateType {
	MetaStateType_menu,
	MetaStateType_intro,
	MetaStateType_main,
	MetaStateType_game_over,

	MetaStateType_count,
	MetaStateType_null = MetaStateType_count,
};

struct MenuMetaState {
	RenderGroup * render_group;

	MenuPage pages[MenuPageId_count];
	MenuPageId current_page;

	u32 transition_id;
};

struct IntroMetaState {
	RenderGroup * render_group;

	IntroFrame frames[4];
	f32 anim_time;

	u32 transition_id;
};

struct MainMetaState {
	AudioSource * music;
	AudioSource * tick_tock;

	RenderGroup * render_group;
	RenderGroup * ui_render_group;
	f32 letterboxed_height;
	Font * font;

	EntityArray entities;
	math::Vec2 entity_gravity;

	f32 height_above_ground;

	Entity * background[2];

	Location locations[LocationId_count];
	LocationId current_location;
	LocationId next_location;
	LocationId last_location;

	Player player;

	EntityEmitter entity_emitter;
	RocketSequence rocket_seq;

	//TODO: Should this be part of the player??
	f32 d_speed;
	f32 dd_speed;
	f32 accel_time;

	b32 boost_always_on;
	f32 boost_speed;
	f32 boost_accel;
	f32 boost_time;
	f32 slow_down_speed;
	f32 slow_down_time;
	
	f32 time_remaining;
	f32 start_time;
	f32 max_time;
	f32 countdown_time;
	f32 clock_scale;

	b32 show_score_overlay;
	UiElement score_buttons[ScoreButtonId_count];
	Str * score_str;
	u32 score_value_index;
	ScoreValue score_values[ScoreValueId_count];

	u32 quit_transition_id;
	u32 restart_transition_id;
	u32 death_transition_id;
};

struct GameOverMetaState {
	AudioSource * music;

	RenderGroup * render_group;

	u32 transition_id;
};

struct MetaState {
	MemoryArena arena;
	AssetState * assets;
	AudioState * audio_state;
	RenderState * render_state;

	MetaStateType type;
	union {
		MenuMetaState * menu;
		IntroMetaState * intro;
		MainMetaState * main;
		GameOverMetaState * game_over;
	};
};

struct GameMemory {
	size_t size;
	u8 * ptr;

	b32 initialized;
};

struct GameInput {
	u32 back_buffer_width;
	u32 back_buffer_height;

	b32 audio_supported;

	f32 delta_time;
	f32 total_time;

	math::Vec2 mouse_pos;
	math::Vec2 last_mouse_pos;
	u8 mouse_button;

	u8 buttons[ButtonId_count];
};

struct GameState {
	MemoryArena memory_arena;

	AssetState assets;
	AudioState audio_state;
	RenderState render_state;

	MetaStateType meta_state;
	MetaState * meta_states[MetaStateType_count];

	u32 transition_id;
	b32 transitioning;
	TransitionType transition_type;
	f32 transition_time;
	b32 transition_flip;

	PlatformAsyncFile * save_file;
	SaveFileHeader save;
	f32 auto_save_time;
	f32 time_until_next_save;

	Str * debug_str;
	Str * str;

	b32 debug_render_entity_bounds;
};

#endif
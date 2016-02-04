
#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <platform.hpp>

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>
#include <render.hpp>

#define ANIMATION_FRAMES_PER_SEC 30
#define PARALLAX_LAYER_COUNT 5

#define ENTITY_NULL_POS math::vec3(F32_MAX, F32_MAX, 0.0f)

//TODO: Formalise sprite animation!!
// struct SpriteAnimation {
// 	Sprite * sprites;
// 	f32 anim_time;
// };

struct Entity {
	math::Vec3 pos;
	f32 scale;
	math::Vec4 color;
	b32 scrollable;

	AssetType asset_type;
	AssetId asset_id;
	u32 asset_index;

	math::Vec2 d_pos;
	math::Vec2 speed;
	f32 damp;
	b32 use_gravity;
	math::Rec2 collider;

	f32 anim_time;
	//TODO: Initial pos!!
	f32 initial_x;
	b32 hit;
	f32 hit_time;

	u32 rand_id;
	math::Vec2 chain_pos;
	b32 hidden;
};

struct EntityArray {
	u32 count;
	Entity elems[256];
};

struct Player {
	Entity * e;
	Entity * sheild;

	b32 dead;

	b32 allow_input;

	Entity * clones[50];
	math::Vec2 clone_offset;
	u32 active_clone_count;
};

struct EntityEmitter {
	math::Vec3 pos;
	f32 time_until_next_spawn;

	u32 entity_count;
	Entity * entity_array[16];
};

enum LocationId {
	LocationId_city,
	LocationId_mountains,
	LocationId_space,

	LocationId_count,
	LocationId_null = LocationId_count,
};

struct Location {
	Entity * layers[PARALLAX_LAYER_COUNT];
	f32 y;

	u32 emitter_type_count;
	AssetId * emitter_types;
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

enum TransitionType {
	TransitionType_pixelate,
	TransitionType_fade,

	TransitionType_count,
	TransitionType_null = TransitionType_count,
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

enum MenuButtonId {
	MenuButtonId_play,
	MenuButtonId_score,
	MenuButtonId_credits,

	MenuButtonId_count,
};

struct MenuMetaState {
	//TODO: Should entities be part of the meta state??
	EntityArray entities;

	Entity * display_items[ASSET_GROUP_COUNT(display)];
	Entity * buttons[MenuButtonId_count];

	u32 transition_id;
};

struct IntroMetaState {
	EntityArray entities;

	Entity * panels[3];
	f32 anim_time;

	u32 transition_id;
};

struct MainMetaState {
	AudioSource * music;
	AudioSource * tick_tock;

	RenderTransform render_transform;
	f32 letterboxed_height;
	Font * font;

	EntityArray entities;
	math::Vec2 entity_gravity;

	f32 ground_height;
	Location locations[LocationId_count];
	LocationId current_location;
	LocationId next_location;
	LocationId last_location;
	u32 location_transition_id;

	Player player;

	EntityEmitter entity_emitter;
	RocketSequence rocket_seq;

	//TODO: Should this be part of the player??
	f32 d_speed;
	f32 dd_speed;
	f32 accel_time;
	
	f32 time_remaining;
	f32 start_time;
	f32 max_time;
	f32 countdown_time;

	b32 show_score_overlay;
	Entity * score_overlay;
	Str * score_str;
	u32 score_value_index;
	ScoreValue score_values[ScoreValueId_count];

	u32 quit_transition_id;
	u32 death_transition_id;
};

struct GameOverMetaState {
	AudioSource * music;

	EntityArray entities;

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
};

#endif
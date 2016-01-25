
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

	math::Vec2 chain_pos;
	b32 hidden;
};

struct EntityArray {
	u32 count;
	Entity elems[256];
};

struct Player {
	Entity * e;

	b32 dead;
	f32 death_time;

	b32 running;
	b32 grounded;

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
	LocationId_space,

	LocationId_count,
};

struct Location {
	Entity * layers[PARALLAX_LAYER_COUNT];

	f32 min_y;
	f32 max_y;

	u32 emitter_type_count;
	AssetId * emitter_types;
};

struct RocketSequence {
	Entity * rocket;

	b32 playing;
	f32 time_;
};

enum ButtonId {
	ButtonId_start,
	ButtonId_left,
	ButtonId_right,
	ButtonId_up,
	ButtonId_down,

	//TODO: Temp!!
	ButtonId_mute,

	ButtonId_debug,

	ButtonId_count,
};

// #define SAVE_FILE_CODE *(u32 *)"DLLY"
#define SAVE_FILE_CODE 0x594C4C44
//TODO: Can we inc this automatically somehow??
#define SAVE_FILE_VERSION 3
#pragma pack(push, 1)
struct SaveFileHeader {
	u32 code;
	u32 version;

	u32 plays;
	u32 high_score;
	f32 longest_run;
};
#pragma pack(pop)

enum MetaStateType {
	MetaStateType_menu,
	MetaStateType_main,
	MetaStateType_game_over,

	MetaStateType_count,
	MetaStateType_null = MetaStateType_count,
};

struct MenuMetaState {
	
};

struct MainMetaState {
	AudioSource * music;

	Camera * camera;

	EntityArray entities;
	math::Vec2 entity_gravity;

	LocationId current_location;
	f32 location_y_offset;
	f32 ground_height;
	Location locations[LocationId_count];

	Player player;

	EntityEmitter entity_emitter;
	RocketSequence rocket_seq;

	//TODO: This should just be player speed!!
	f32 d_time;
	u32 score;
	f32 distance;
};

struct GameOverMetaState {
	EntityArray entities;
};

struct MetaState {
	MemoryArena arena;
	AssetState * assets;
	AudioState * audio_state;
	RenderState * render_state;

	MetaStateType type;
	union {
		MenuMetaState * menu;
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
	math::Vec2 mouse_delta;

	u8 buttons[ButtonId_count];
};

struct GameState {
	MemoryArena memory_arena;

	AssetState assets;
	AudioState audio_state;
	RenderState render_state;

	MetaStateType meta_state;
	MetaState * meta_states[MetaStateType_count];

	PlatformAsyncFile * save_file;
	SaveFileHeader save;
	f32 auto_save_time;
	f32 time_until_next_save;

	Str * debug_str;
	Str * str;
};

#endif

#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <platform.hpp>

#include <gl.hpp>
#include <basic.vert>
#include <basic.frag>
#include <screen_quad.vert>
#include <post_filter.frag>

#include <audio.hpp>
#include <asset.hpp>
#include <debug.hpp>
#include <math.hpp>

#define ANIMATION_FRAMES_PER_SEC 30
#define VERT_ELEM_COUNT 8
#define QUAD_ELEM_COUNT (VERT_ELEM_COUNT * 6)

#define PARALLAX_LAYER_COUNT 5

#define ENTITY_NULL_POS math::vec3(F32_MAX, F32_MAX, 0.0f)

//TODO: Automatically generate these structs for shaders!!
struct Shader {
	u32 id;

	u32 i_position;
	u32 i_tex_coord;
	u32 i_color;

	u32 transform;
	u32 color;
	u32 tex0;
	u32 tex0_dim;

	u32 pixelate_scale;
	u32 fade_amount;
};

enum RenderMode {
	RenderMode_triangles = GL_TRIANGLES,

	RenderMode_lines = GL_LINES,
	RenderMode_line_strip = GL_LINE_STRIP,
};

struct RenderBatch {
	Texture * tex;

	u32 v_len;
	f32 * v_arr;
	gl::VertexBuffer v_buf;
	u32 e;

	RenderMode mode;
};

//TODO: Formalise sprite animation!!
// struct SpriteAnimation {
// 	Sprite * sprites;
// 	f32 anim_time;
// };

struct Entity {
	math::Vec3 pos;
	f32 scale;
	math::Vec4 color;

	AssetType asset_type;
	AssetId asset_id;
	u32 asset_index;

	gl::VertexBuffer * v_buf;

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

struct Camera {
	math::Vec2 pos;
	math::Vec2 offset;

	f32 letterboxed_height;
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

enum MetaGame {
	MetaGame_main,
	MetaGame_end,

	MetaGame_count,
};

//TODO: Fill this with "main" game variables/etc.
// struct MainMetaGame {

// };

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

struct Font {
	RenderBatch * batch;

	math::Mat4 projection_matrix;

	u32 glyph_width;
	u32 glyph_height;
	u32 glyph_spacing;

	f32 scale;
	math::Vec2 anchor;
	math::Vec2 pos;
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
	MemoryPool memory_pool;

	AssetState assets;

	PlatformAsyncFile * save_file;
	SaveFileHeader save;
	f32 auto_save_time;
	f32 last_saved_time;

	AudioState audio_state;
	AudioSource * music;

	Shader basic_shader;
	Shader post_shader;

	gl::FrameBuffer frame_buffer;
	gl::VertexBuffer screen_quad_v_buf;

	gl::VertexBuffer entity_v_buf;
	gl::VertexBuffer bg_v_buf;

	Font * debug_font;
	Str * debug_str;
	Str * title_str;

	u32 ideal_window_width;
	u32 ideal_window_height;

	b32 debug_render_entity_bounds;
	RenderBatch * debug_batch;

	//TODO: Do we need multiple batches?? We only use one at a time anyway!
	u32 sprite_batch_count;
	RenderBatch ** sprite_batches;

	u32 entity_count;
	Entity entity_array[256];

	MetaGame meta_game;

	math::Vec3 entity_null_pos;
	math::Vec2 entity_gravity;

	LocationId current_location;
	f32 location_y_offset;
	f32 ground_height;
	Location locations[LocationId_count];

	Player player;

	EntityEmitter entity_emitter;
	RocketSequence rocket_seq;

	Camera camera;
	f32 pixelate_time;
	f32 fade_amount;

	//TODO: This should just be player speed!!
	f32 d_time;
	f32 pitch;
	u32 score;
	f32 distance;
};

#endif

#ifndef DEBUG_HPP_INCLUDED
#define DEBUG_HPP_INCLUDED

#define DEBUG_TIME_BLOCK__(x) DebugTimedBlock __timed_block_##x(__COUNTER__, (char *)__FUNCTION__, (char *)__FILE__, __LINE__)
#define DEBUG_TIME_BLOCK_(x) DEBUG_TIME_BLOCK__(x)
#define DEBUG_TIME_BLOCK() DEBUG_TIME_BLOCK_(__LINE__)

struct DebugBlockProfile {
	char * func;
	char * file;
	u32 line;

	u32 id;

	u32 hits;
	f64 ms;
};

extern DebugBlockProfile debug_block_profiles[];

struct DebugTimedBlock {
	DebugBlockProfile * profile;

	u32 hits;
	f64 ms;

	DebugTimedBlock(u32 id, char * func, char * file, u32 line, u32 hits = 1) {
		profile = debug_block_profiles + id;
		profile->id = id;

		profile->func = func;
		profile->file = file;
		profile->line = line;

		this->hits = hits;
		this->ms = emscripten_get_now();
	}

	~DebugTimedBlock() {
		profile->hits += this->hits;
		profile->ms += emscripten_get_now() - this->ms;
	}
};

#endif
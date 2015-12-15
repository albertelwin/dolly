
#ifndef NAMESPACE_SYS_INCLUDED
#define NAMESPACE_SYS_INCLUDED

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define __TOKEN_STRINGIFY(x) #x
#define TOKEN_STRINGIFY(x) __TOKEN_STRINGIFY(x)

#define __ASSERT(x) __assert_func(x, "" #x " : " __FILE__ " : " TOKEN_STRINGIFY(__LINE__))
void __assert_func(bool expression, char const * message) {
	if(!expression) {
#ifdef WIN32
		MessageBoxA(0, message, "ASSERT", MB_OK | MB_ICONERROR);
#endif
		std::fprintf(stderr, "ASSERT: %s\n", message);

#ifdef __EMSCRIPTEN__
		emscripten_force_exit(EXIT_FAILURE);
#else
		std::exit(EXIT_FAILURE);
#endif

		// *((int *)(0)) = 0;
	}
}

#define ASSERT(x) __ASSERT(x)
#define ARRAY_COUNT(x) (sizeof((x)) / sizeof((x)[0]))

#define KILOBYTES(x) (1024 * (x))
#define MEGABYTES(x) (1024 * KILOBYTES(x))
#define GIGABYTES(x) (1024 * MEGABYTES(x))
#define TERABYTES(x) (1024 * GIGABYTES(x))

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef uint32_t b32;

struct MemoryPool {
	size_t size;
	size_t used;

	u8 * base_address;
};

MemoryPool create_memory_pool(void * base_address, size_t size) {
	MemoryPool pool = {};
	pool.size = size;
	pool.used = 0;
	pool.base_address = (u8 *)base_address;
	return pool;
}

#define PUSH_STRUCT(pool, type) (type *)push_memory_(pool, sizeof(type))
#define PUSH_ARRAY(pool, type, length) (type *)push_memory_(pool, sizeof(type) * (length))
#define PUSH_MEMORY(pool, size) push_memory_(pool, size)
void * push_memory_(MemoryPool * pool, size_t size) {
	ASSERT((pool->used + size) <= pool->size);
	
	void * ptr = pool->base_address + pool->used;
	pool->used += size;
	return ptr;
}

namespace sys {
	//NOTE: No constexpr in VS :(
	f32 const FLOAT_MAX = 1e+37f;
}

#endif
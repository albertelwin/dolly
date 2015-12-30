
#ifndef SYS_HPP_INCLUDED
#define SYS_HPP_INCLUDED

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

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uint32_t b32;

#define F32_MAX 1e+37f

#define ASCII_NUMBER_BASE 48

//TODO: Str struct!!
// struct Str {
// 	char * buf;
// 	u32 len;
// };

u32 str_length(char const * str) {
	u32 len = 0;
	while(str[len]) {
		len++;
	}

	return len;
}

void str_copy(char * dst, char const * src) {
	while(*src) {
		*dst++ = *src++;
	}

	*dst = 0;
}

void str_push(char * dst, char const * src) {
	while(*dst) {
		dst++;
	}

	while(*src) {
		*dst++ = *src++;
	}

	*dst = 0;
}

void str_push(char * dst, u32 src) {
	while(*dst) {
		dst++;
	}

	if(!src) {
		*dst++ = ASCII_NUMBER_BASE;
		*dst = 0;
	}
	else {
		u32 src_ = src;
		while(src_) {
			dst++;
			src_ /= 10;
		}

		*dst-- = 0;

		while(src) {
			*dst-- = ASCII_NUMBER_BASE + src % 10;
			src /= 10; 
		}
	}
}

void str_push(char * dst, f32 src) {
	str_push(dst, (u32)src);

	while(*dst) {
		dst++;
	}

	*dst++ = '.';

	u32 p = 10000;
	u32 frc = (u32)((src - (u32)src) * p);

	p /= 10;
	while(p) {
		*dst++ = ASCII_NUMBER_BASE + ((frc / p) % 10);
		p /= 10;
	}

	*dst = 0;
}

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

void zero_memory(u8 * ptr, size_t size) {
	for(u32 i = 0; i < size; i++) {
		ptr[i] = 0;
	}
}

#define KEY_DOWN_BIT 0
#define KEY_PRESSED_BIT 1
#define KEY_RELEASED_BIT 2

#define KEY_DOWN (1 << KEY_DOWN_BIT)
#define KEY_PRESSED (1 << KEY_PRESSED_BIT)
#define KEY_RELEASED (1 << KEY_RELEASED_BIT)

#endif

#ifndef SYS_HPP_INCLUDED
#define SYS_HPP_INCLUDED

#include <cstdarg>
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

struct Str {
	char * ptr;
	u32 len;
	u32 max_len;
};

void str_clear(Str * str) {
	str->ptr[0] = 0;
	str->len = 0;
}

void str_push(Str * str, char const * val) {
	char * dst = str->ptr + str->len;

	while(*val) {
		*dst++ = *val++; str->len++;
	}

	*dst = 0;

	ASSERT(str->len < str->max_len);
}

void str_push(Str * str, u32 val) {
	char * dst = str->ptr + str->len;

	if(!val) {
		*dst++ = '0'; str->len++;
		*dst = 0;
	}
	else {
		u32 val_ = val;
		while(val_) {
			dst++; str->len++;
			val_ /= 10;
		}

		*dst-- = 0;

		while(val) {
			*dst-- = '0' + val % 10;
			val /= 10; 
		}
	}

	ASSERT(str->len < str->max_len);
}

void str_push(Str * str, f32 val) {
	if(val < 0.0f) {
		str->ptr[str->len++] = '-';
		val = -val;
	}

	str_push(str, (u32)val);

	char * dst = str->ptr + str->len;

	*dst++ = '.'; str->len++;

	u32 p = 10000;
	u32 frc = (u32)((val - (u32)val) * p);

	p /= 10;
	while(p) {
		*dst++ = '0' + ((frc / p) % 10); str->len++;
		p /= 10;
	}

	*dst = 0;

	ASSERT(str->len < str->max_len);
}

void str_print(Str * str, char const * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	while(*fmt) {
		if(fmt[0] == '%') {
			switch(fmt[1]) {
				case 'f': {
					str_push(str, (f32)va_arg(args, f64));
					break;
				}

				case 'u': {
					str_push(str, va_arg(args, u32));
					break;
				}

				default: {
					ASSERT(false);
					break;
				}
			}

			fmt += 2;
		}
		else {
			str->ptr[str->len++] = *fmt;
			fmt++;			
		}
	}

	str->ptr[str->len] = 0;
	va_end(args);
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

#ifndef SYS_HPP_INCLUDED
#define SYS_HPP_INCLUDED

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

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

#define U32_MAX 4294967295

#define F32_MAX 1e+37f

#define F32_SIGN_MASK 0x80000000
#define F32_EXPONENT_MASK 0x7F800000
#define F32_MANTISSA_MASK 0x007FFFFF

b32 f32_is_nan(f32 val) {
	u32 bits = *((u32 *)&val);
	return (bits & F32_EXPONENT_MASK) == F32_EXPONENT_MASK && (bits & F32_MANTISSA_MASK) != 0;
}

b32 f32_is_inf(f32 val) {
	u32 bits = *((u32 *)&val);
	return (bits & F32_EXPONENT_MASK) == F32_EXPONENT_MASK && (bits & F32_MANTISSA_MASK) == 0;
}

b32 f32_is_neg(f32 val) {
	u32 bits = *((u32 *)&val);
	return (bits & F32_SIGN_MASK) == F32_SIGN_MASK;
}

u32 c_str_len(char const * str) {
	u32 len = 0;
	while(*str) {
		str++;
		len++;
	}

	return len;
}

struct Str {
	char * ptr;
	u32 len;
	u32 max_len;
};

Str str_fixed_size(char const * ptr, u32 len) {
	Str str;
	str.ptr = (char *)ptr;
	str.max_len = str.len = len;
	return str;
}

Str str_from_c_str(char const * c_str) {
	Str str;
	str.max_len = str.len = c_str_len(c_str);
	str.ptr = (char *)c_str;
	return str;
}

void str_clear(Str * str) {
	str->len = 0;
	str->ptr[0] = 0;
}

b32 str_equal(Str * str_x, Str * str_y) {
	b32 equal;
	if(str_x->len == str_y->len) {
		equal = true;

		for(u32 i = 0; i < str_x->len; i++) {
			if(str_x->ptr[i] != str_y->ptr[i]) {
				equal = false;
				break;
			}
		}
	}
	else {
		equal = false;
	}

	return equal;
}

b32 str_equal(Str * str_x, char const * c_str) {
	Str str_y = str_from_c_str(c_str);
	return str_equal(str_x, &str_y);
}

void str_push(Str * str, char char_) {
	ASSERT(str->len < (str->max_len - 1));

	str->ptr[str->len++] = char_;
	str->ptr[str->len] = 0;
}

void str_push_c_str(Str * str, char const * val) {
	while(*val) {
		str_push(str, *val++);
	}
}

void str_push_u32(Str * str, u32 val) {
	if(!val) {
		str_push(str, '0');
	}
	else {
		char * dst = str->ptr + str->len;

		u32 val_ = val;
		while(val_) {
			dst++;
			str->len++;
			val_ /= 10;
		}

		*dst-- = 0;

		while(val) {
			*dst-- = '0' + val % 10;
			val /= 10; 
		}
	}
}

void str_push_f32(Str * str, f32 val) {
	if(f32_is_neg(val)) {
		str_push(str, '-');
		val = -val;
	}

	if(f32_is_inf(val)) {
		str_push_c_str(str, "inf");
	}
	else if(f32_is_nan(val)) {
		str_push_c_str(str, "nan");
	}
	else {
		str_push_u32(str, (u32)val);
		str_push(str, '.');

		u32 p = 10000;
		u32 frc = (u32)((val - (u32)val) * p);

		p /= 10;
		while(p) {
			str_push(str, '0' + ((frc / p) % 10));
			p /= 10;
		}
	}
}

void str_print(Str * str, char const * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	while(*fmt) {
		if(fmt[0] == '%') {
			switch(fmt[1]) {
				case 's': {
					str_push_c_str(str, va_arg(args, char *));
					break;
				}

				case 'f': {
					str_push_f32(str, (f32)va_arg(args, f64));
					break;
				}

				case 'u': {
					str_push_u32(str, va_arg(args, u32));
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
			str_push(str, *fmt++);
		}
	}

	str->ptr[str->len] = 0;
	va_end(args);
}

struct MemoryPtr {
	size_t size;
	u8 * ptr;
};

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

void zero_memory(void * ptr, size_t size) {
	u8 * ptr_u8 = (u8 *)ptr;
	for(u32 i = 0; i < size; i++) {
		ptr_u8[i] = 0;
	}
}

void copy_memory(void * dst, void * src, size_t size) {
	u8 * dst_u8 = (u8 *)dst;
	u8 * src_u8 = (u8 *)src;
	for(u32 i = 0; i < size; i++) {
		dst_u8[i] = src_u8[i];
	}
}

#define PUSH_STRUCT(pool, type) (type *)push_memory_(pool, sizeof(type))
#define PUSH_ARRAY(pool, type, length) (type *)push_memory_(pool, sizeof(type) * (length))
#define PUSH_MEMORY(pool, type, size) (type *)push_memory_(pool, size)
void * push_memory_(MemoryPool * pool, size_t size) {
	ASSERT((pool->used + size) <= pool->size);
	
	void * ptr = pool->base_address + pool->used;
	pool->used += size;
	return ptr;
}

#define PUSH_COPY_ARRAY(pool, type, array, length) (type *)push_copy_memory_(pool, array, sizeof(type) * (length))
void * push_copy_memory_(MemoryPool * pool, void * src, size_t size) {
	void * dst = push_memory_(pool, size);
	copy_memory(dst, src, size);
	return dst;
}

#define ALLOC_STRUCT(type) (type *)alloc_memory_(sizeof(type))
#define ALLOC_ARRAY(type, length) (type *)alloc_memory_(sizeof(type) * length)
#define ALLOC_MEMORY(type, size) (type *)alloc_memory_(size)
void * alloc_memory_(size_t size) {
	void * ptr = std::malloc(size);
	return ptr;
}

#define FREE_MEMORY(ptr) free_memory_(ptr)
void free_memory_(void * ptr) {
	std::free(ptr);
}

#define KEY_DOWN_BIT 0
#define KEY_PRESSED_BIT 1
#define KEY_RELEASED_BIT 2

#define KEY_DOWN (1 << KEY_DOWN_BIT)
#define KEY_PRESSED (1 << KEY_PRESSED_BIT)
#define KEY_RELEASED (1 << KEY_RELEASED_BIT)

MemoryPtr read_file_to_memory(char const * file_name) {
	std::FILE * file_ptr = std::fopen(file_name, "rb");
	ASSERT(file_ptr != 0);

	MemoryPtr mem_ptr;

	std::fseek(file_ptr, 0, SEEK_END);
	mem_ptr.size = (size_t)std::ftell(file_ptr);
	std::rewind(file_ptr);

	mem_ptr.ptr = ALLOC_MEMORY(u8, mem_ptr.size);
	size_t read_result = std::fread(mem_ptr.ptr, 1, mem_ptr.size, file_ptr);
	ASSERT(read_result == mem_ptr.size);

	std::fclose(file_ptr);

	return mem_ptr;
}

#endif
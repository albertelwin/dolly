
#ifndef PLATFORM_HPP_INCLUDED
#define PLATFORM_HPP_INCLUDED

#include <sys.hpp>

struct PlatformAsyncFile {
	MemoryPtr memory;
};

PlatformAsyncFile * platform_open_async_file(char const * file_name);
void platform_close_async_file(PlatformAsyncFile * file);
b32 platform_write_async_file(PlatformAsyncFile * file, void * ptr, size_t size);

#endif
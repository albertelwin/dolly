
#include <emscripten.h>

#include <cstdio>

#define MINIZ_NO_TIME
#include <miniz.c>

#include <sys.hpp>

extern "C" void EMSCRIPTEN_KEEPALIVE worker_func() {
	MemoryPtr file_buf;
	file_buf.ptr = (u8 *)mz_zip_extract_archive_file_to_heap("asset.zip", "asset.pak", &file_buf.size, 0);
	u8 * file_ptr = file_buf.ptr;

	std::printf("LOG: file_buf.size: %u\n", file_buf.size);

	FREE_MEMORY(file_buf.ptr);
}
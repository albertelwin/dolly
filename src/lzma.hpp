
#ifndef LZMA_HPP_INCLUDED
#define LZMA_HPP_INCLUDED

#include <sys.hpp>

#define ZIP_LOCAL_FILE_HEADER_SIGNATURE 0x04034b50
#define ZIP_LZMA_COMPRESSION_METHOD 14

#pragma pack(push, 1)
struct ZipLocalFileHeader {
	u32 signature;
	u16 version_to_extract;
	u16 flags;
	u16 compression_method;
	u16 last_modified_time;
	u16 last_modified_date;
	u32 crc_32;
	u32 compressed_size;
	u32 uncompressed_size;
	u16 file_name_length;
	u16 extra_field_length;
};

struct ZipLzmaHeader {
	u16 version;
	u16 properties_size;
	u8 properties_byte;
	u32 dictionary_size;
};
#pragma pack(pop)

#define LZMA_MIN_DICTIONARY_SIZE (1 << 12)
#define LZMA_MAX_RANGE_VALUE ((u32)1 << 24)
#define LZMA_PROBABILITY_BITS 11
#define LZMA_PROBABILITY_MOVE_BITS 5
#define LZMA_INITIAL_PROBABILITY ((1 << LZMA_PROBABILITY_BITS) / 2)
#define LZMA_MAX_POS_BITS 4
#define LZMA_LENGTH_TO_POS_STATES 4
#define LZMA_ALIGN_BITS 4
#define LZMA_END_POS_INDEX 14
#define LZMA_FULL_DISTANCES_COUNT (1 << (LZMA_END_POS_INDEX >> 1))

struct LzmaWindow {
	u32 size;
	u8 * ptr;

	u32 pos;
	b32 is_full;

	u8 * stream;
	u32 stream_pos;
};

struct LzmaRangeDecoder {
	u32 range;
	u32 code;
	u8 * stream;

	b32 corrupted;
};

struct LzmaProbArray {
	u32 count;
	u16 * array;
};

struct LzmaBitTreeDecoder {
	u32 bit_count;
	LzmaProbArray probs;
};

enum LzmaLengthDecodorId {
	LzmaLengthDecodorId_simple,
	LzmaLengthDecodorId_rep_match,

	LzmaLengthDecodorId_count,
};

struct LzmaLengthDecodor {
	LzmaBitTreeDecoder low_tree[1 << LZMA_MAX_POS_BITS];
	LzmaBitTreeDecoder mid_tree[1 << LZMA_MAX_POS_BITS];
	LzmaBitTreeDecoder hig_tree;

	u16 choices[2];
};

struct LzmaDecoder {
	u8 * compressed_stream;
	u8 * uncompressed_stream;

	u32 literal_context_bits;
	u32 literal_pos_bits;
	u32 pos_bits;

	u32 dictionary_size;

	LzmaProbArray literal_probs;

	LzmaWindow window;
	LzmaRangeDecoder range_decoder;

	LzmaLengthDecodor length_decoders[LzmaLengthDecodorId_count];

	LzmaBitTreeDecoder pos_slot_decoders[LZMA_LENGTH_TO_POS_STATES];
	LzmaProbArray pos_decoders;
	LzmaBitTreeDecoder align_decoder;
};

LzmaProbArray lzma_allocate_prob_array(u32 count) {
	LzmaProbArray probs;
	probs.count = count;
	probs.array = ALLOC_ARRAY(u16, count);
	for(u32 i = 0; i < probs.count; i++) {
		probs.array[i] = LZMA_INITIAL_PROBABILITY;
	}

	return probs;
}

void lzma_free_prob_array(LzmaProbArray * probs) {
	FREE_MEMORY(probs->array);
	probs->count = 0;
}

LzmaBitTreeDecoder lzma_allocate_bit_tree_decoder(u32 bit_count) {
	LzmaBitTreeDecoder bit_tree;
	bit_tree.bit_count = bit_count;
	bit_tree.probs = lzma_allocate_prob_array((u32)1 << bit_count);
	return bit_tree;
}

void lzma_free_bit_tree_decoder(LzmaBitTreeDecoder * bit_tree) {
	lzma_free_prob_array(&bit_tree->probs);
}

LzmaDecoder * lzma_allocate_decoder(u8 * compressed_stream, u8 * uncompressed_stream, u32 properties_byte, u32 dictionary_size_) {
	LzmaDecoder * decoder = ALLOC_STRUCT(LzmaDecoder);
	decoder->compressed_stream = compressed_stream;
	decoder->uncompressed_stream = uncompressed_stream;

	decoder->literal_context_bits = properties_byte % 9;
	ASSERT(decoder->literal_context_bits <= 8);

	decoder->literal_pos_bits = (properties_byte / 9) % 5;
	ASSERT(decoder->literal_pos_bits <= 4);

	decoder->pos_bits = (properties_byte / 9) / 5;
	ASSERT(decoder->pos_bits <= 4);

	decoder->dictionary_size = dictionary_size_;
	if(decoder->dictionary_size < LZMA_MIN_DICTIONARY_SIZE) {
		decoder->dictionary_size = LZMA_MIN_DICTIONARY_SIZE;
	}
	
	decoder->literal_probs = lzma_allocate_prob_array((u32)0x300 << (decoder->literal_context_bits + decoder->literal_pos_bits));

	LzmaRangeDecoder * range_decoder = &decoder->range_decoder;
	range_decoder->range = 0xFFFFFFFF;
	range_decoder->code = 0;
	range_decoder->stream = compressed_stream;
	range_decoder->corrupted = false;

	u8 first_byte = *range_decoder->stream++;
	for(u32 i = 0; i < 4; i++) {
		range_decoder->code = (range_decoder->code << 8) | *range_decoder->stream++;
	}

	if(first_byte != 0 || range_decoder->code == range_decoder->range) {
		range_decoder->corrupted = true;
	}

	ASSERT(!range_decoder->corrupted);

	LzmaWindow * window = &decoder->window;
	window->size = decoder->dictionary_size;
	window->ptr = ALLOC_MEMORY(u8, window->size);
	window->pos = 0;
	window->is_full = false;
	window->stream = decoder->uncompressed_stream;
	window->stream_pos = 0;

	for(u32 i = 0; i < ARRAY_COUNT(decoder->length_decoders); i++) {
		LzmaLengthDecodor * length_decoder = decoder->length_decoders + i;

		length_decoder->hig_tree = lzma_allocate_bit_tree_decoder(8);
		for(u32 ii = 0; ii < ARRAY_COUNT(length_decoder->low_tree); ii++) {
			length_decoder->low_tree[ii] = lzma_allocate_bit_tree_decoder(3);
			length_decoder->mid_tree[ii] = lzma_allocate_bit_tree_decoder(3);
		}

		for(u32 ii = 0; ii < ARRAY_COUNT(length_decoder->choices); ii++) {
			length_decoder->choices[ii] = LZMA_INITIAL_PROBABILITY;
		}
	}

	decoder->pos_decoders = lzma_allocate_prob_array(1 + LZMA_FULL_DISTANCES_COUNT - LZMA_END_POS_INDEX);
	decoder->align_decoder = lzma_allocate_bit_tree_decoder(LZMA_ALIGN_BITS);
	for(u32 i = 0; i < ARRAY_COUNT(decoder->pos_slot_decoders); i++) {
		decoder->pos_slot_decoders[i] = lzma_allocate_bit_tree_decoder(6);
	}

	return decoder;
}

void lzma_free_decoder(LzmaDecoder * decoder) {
	lzma_free_prob_array(&decoder->pos_decoders);
	lzma_free_bit_tree_decoder(&decoder->align_decoder);
	for(u32 i = 0; i < ARRAY_COUNT(decoder->pos_slot_decoders); i++) {
		lzma_free_bit_tree_decoder(decoder->pos_slot_decoders + i);
	}

	for(u32 i = 0; i < ARRAY_COUNT(decoder->length_decoders); i++) {
		LzmaLengthDecodor * length_decoder = decoder->length_decoders + i;

		lzma_free_bit_tree_decoder(&length_decoder->hig_tree);
		for(u32 ii = 0; ii < ARRAY_COUNT(length_decoder->low_tree); ii++) {
			lzma_free_bit_tree_decoder(length_decoder->low_tree + ii);
			lzma_free_bit_tree_decoder(length_decoder->mid_tree + ii);
		}
	}

	FREE_MEMORY(decoder->window.ptr);
	lzma_free_prob_array(&decoder->literal_probs);

	FREE_MEMORY(decoder);
}

void lzma_write_byte(LzmaWindow * window, u8 byte) {
	window->ptr[window->pos++] = byte;
	if(window->pos == window->size) {
		window->pos = 0;
		window->is_full = true;
	}

	window->stream[window->stream_pos++] = byte;
}

u8 lzma_read_byte(LzmaWindow * window, u32 dist) {
	u32 index = dist <= window->pos ? window->pos - dist : window->size - dist + window->pos;
	return window->ptr[index];
}

void lzma_write_copy_match(LzmaWindow * window, u32 dist, u32 len) {
	for(u32 i = len; i > 0; i--) {
		u8 byte = lzma_read_byte(window, dist);
		lzma_write_byte(window, byte);
	}
}

b32 lzma_check_distance(LzmaWindow * window, u32 dist) {
	return dist <= window->pos || window->is_full;
}

b32 lzma_window_is_empty(LzmaWindow * window) {
	return window->pos == 0 && !window->is_full;
}

void lzma_normalize_range_decoder(LzmaRangeDecoder * decoder) {
	if(decoder->range < LZMA_MAX_RANGE_VALUE) {
		decoder->range <<= 8;
		decoder->code = (decoder->code << 8) | *decoder->stream++;
	}
}

u32 lzma_decode_direct_bits(LzmaRangeDecoder * decoder, u32 bit_count) {
	u32 bits = 0;
	for(u32 i = 0; i < bit_count; i++) {
		decoder->range >>= 1;
		decoder->code -= decoder->range;
		u32 t = 0 - ((u32)decoder->code >> 31);
		decoder->code += decoder->range & t;

		if(decoder->code == decoder->range) {
			decoder->corrupted = true;
		}

		lzma_normalize_range_decoder(decoder);
		bits <<= 1;
		bits += t + t;
	}

	return bits;
}

u32 lzma_decode_bit(LzmaRangeDecoder * decoder, u16 * probability) {
	u32 p32 = *probability;
	u32 bound = (decoder->range >> LZMA_PROBABILITY_BITS) * p32;

	u32 bit = 0;
	if(decoder->code < bound) {
		p32 += ((1 << LZMA_PROBABILITY_BITS) - p32) >> LZMA_PROBABILITY_MOVE_BITS;
		decoder->range = bound;
	}
	else {
		p32 -= p32 >> LZMA_PROBABILITY_MOVE_BITS;
		decoder->code -= bound;
		decoder->range -= bound;
		bit = 1;
	}

	*probability = (u16)p32;
	lzma_normalize_range_decoder(decoder);

	return bit;
}

u32 lzma_bit_tree_decode(LzmaBitTreeDecoder * bit_tree, LzmaRangeDecoder * decoder) {
	u32 m = 1;
	for(u32 i = 0; i < bit_tree->bit_count; i++) {
		m = (m << 1) + lzma_decode_bit(decoder, bit_tree->probs.array + m);
	}

	return m - bit_tree->bit_count;
}

u32 lzma_bit_tree_reverse_decode(u16 * probs, LzmaRangeDecoder * decoder, u32 bit_count) {
	u32 m = 1;
	u32 sym = 0;
	for(u32 i = 0; i < bit_count; i++) {
		u32 bit = lzma_decode_bit(decoder, probs + m);
		m = (m << 1) + bit;
		sym |= (bit << i);
	}

	return sym;
}

void lzma_decode_literal(LzmaDecoder * decoder, u32 state, u32 rep0) {
	u32 literal_context = decoder->literal_context_bits;
	u32 literal_pos = decoder->literal_pos_bits;

	LzmaWindow * window = &decoder->window;
	LzmaRangeDecoder * range_decoder = &decoder->range_decoder;

	u32 prev_byte = 0;
	if(!lzma_window_is_empty(window)) {
		prev_byte = lzma_read_byte(window, 1);
	}


	u32 sym = 1;
	u32 literal_state = ((window->stream_pos & ((1 << literal_pos) - 1)) << literal_context) + (prev_byte >> (8 - literal_context));
	u16 * probs = decoder->literal_probs.array + (u32)0x300 * literal_state;

	if(state >= 7) {
		u32 match_byte = lzma_read_byte(window, rep0 + 1);
		do {
			u32 match_bit = (match_byte >> 7) & 1;
			match_byte <<= 1;

			u16 * prob = probs + ((1 + match_bit) << 8) + sym;
			u32 bit = lzma_decode_bit(range_decoder, prob);
			sym = (sym << 1) | bit;

			if(match_bit != bit) {
				break;
			}

		} while(sym < 0x100);
	}

	while(sym < 0x100) {
		sym = (sym << 1) | lzma_decode_bit(range_decoder, probs + sym);
	}

	lzma_write_byte(window, (u8)(sym - 0x100));
}

u32 lzma_decode_length(LzmaLengthDecodor * length_decoder, LzmaRangeDecoder * range_decoder, u32 pos_state) {
	u32 len;
	if(lzma_decode_bit(range_decoder, &length_decoder->choices[0]) == 0) {
		len = lzma_bit_tree_decode(length_decoder->low_tree + pos_state, range_decoder);
	}
	else if(lzma_decode_bit(range_decoder, &length_decoder->choices[1]) == 0) {
		len = 8 + lzma_bit_tree_decode(length_decoder->mid_tree + pos_state, range_decoder);
	}
	else {
		len = 16 + lzma_bit_tree_decode(&length_decoder->hig_tree, range_decoder);
	}

	return len;
}

u32 lzma_decode_distance(LzmaDecoder * decoder, u32 len) {
	u32 len_state = len;
	if(len_state > LZMA_LENGTH_TO_POS_STATES - 1) {
		len_state = LZMA_LENGTH_TO_POS_STATES - 1;
	}

	LzmaRangeDecoder * range_decoder = &decoder->range_decoder;

	u32 dist;
	u32 pos_slot = lzma_bit_tree_decode(decoder->pos_slot_decoders + len_state, range_decoder);
	if(pos_slot < 4) {
		dist = pos_slot;
	}
	else {
		u32 direct_bit_count = (u32)((pos_slot >> 1) - 1);
		dist = ((2 | (pos_slot & 1)) << direct_bit_count);

		if(pos_slot < LZMA_END_POS_INDEX) {
			dist += lzma_bit_tree_reverse_decode(decoder->pos_decoders.array + dist - pos_slot, range_decoder, direct_bit_count);
		}
		else {
			dist += lzma_decode_direct_bits(range_decoder, direct_bit_count - LZMA_ALIGN_BITS) << LZMA_ALIGN_BITS;
			dist += lzma_bit_tree_reverse_decode(decoder->align_decoder.probs.array, range_decoder, decoder->align_decoder.bit_count);
		}
	}

	return dist;	
}

MemoryPtr load_and_decompress_zip_file(char const * file_name) {
	MemoryPtr file_ptr = read_file_to_memory(file_name);
	ASSERT(file_ptr.ptr);

	ZipLocalFileHeader * local_file_header = (ZipLocalFileHeader *)file_ptr.ptr;
	ASSERT(local_file_header->signature = ZIP_LOCAL_FILE_HEADER_SIGNATURE);
	ASSERT(local_file_header->compression_method == ZIP_LZMA_COMPRESSION_METHOD);

	Str local_file_name = str_fixed_size((char *)(local_file_header + 1), local_file_header->file_name_length);
	ASSERT(str_equal(&local_file_name, "asset.pak"));

	// std::printf("LOG: __ZIP_LOCAL_FILE_HEADER_______\n");
	// std::printf("LOG:  - signature 0x%08x\n", local_file_header->signature);
	// std::printf("LOG:  - version_to_extract %u\n", local_file_header->version_to_extract);
	// std::printf("LOG:  - flags %u\n", local_file_header->flags);
	// std::printf("LOG:  - compression_method %u\n", local_file_header->compression_method);
	// std::printf("LOG:  - last_modified_time %u\n", local_file_header->last_modified_time);
	// std::printf("LOG:  - last_modified_date %u\n", local_file_header->last_modified_date);
	// std::printf("LOG:  - crc_32 %u\n", local_file_header->crc_32);
	// std::printf("LOG:  - compressed_size %u\n", local_file_header->compressed_size);
	// std::printf("LOG:  - uncompressed_size %u\n", local_file_header->uncompressed_size);
	// std::printf("LOG:  - file_name_length %u\n", local_file_header->file_name_length);
	// std::printf("LOG:  - extra_field_length %u\n", local_file_header->extra_field_length);
	// std::printf("LOG:\n");

	u32 local_file_header_size = sizeof(ZipLocalFileHeader) + local_file_header->file_name_length + local_file_header->extra_field_length * sizeof(u16) * 2;
	u8 * local_file_data = file_ptr.ptr + local_file_header_size;

	ZipLzmaHeader * lzma_header = (ZipLzmaHeader *)local_file_data;
	ASSERT(lzma_header->properties_size == 5);

	u8 * uncompressed_data = ALLOC_MEMORY(u8, local_file_header->uncompressed_size);
	LzmaDecoder * lzma_decoder = lzma_allocate_decoder((u8 *)(lzma_header + 1), uncompressed_data, lzma_header->properties_byte, lzma_header->dictionary_size);

	// ASSERT(range_decoder.code == 0);

	lzma_free_decoder(lzma_decoder);

	MemoryPtr mem_ptr;
	mem_ptr.size = local_file_header->uncompressed_size;
	mem_ptr.ptr = uncompressed_data;
	return mem_ptr;
}

#endif

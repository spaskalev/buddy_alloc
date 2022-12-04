/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */


#define start_test printf("Running test: %s in %s:%d\n", __func__, __FILE__, __LINE__);

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUDDY_ALLOC_SAFETY
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc.h"
#undef BUDDY_ALLOC_IMPLEMENTATION

/*
 * The tests are written with 64-bit arch in mind.
 * To allow running them on 32-bit the following macros are used
 *
 * PSS(x) - Platform-specific size. Does nothing on 64 bit. Halves value on 32 bit.
 * OF(x,y) - One of - picks the first argument on 64 bit and the second on 32 bit.
 */

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
#define PSS(x) (x)
#define OF(x, y) (x)
#elif SIZE_MAX == 0xFFFFFFFF
#define PSS(x) ((x)/2)
#define OF(x, y) (y)
#else
#error Unsupported platform
#endif

void test_highest_bit_position() {
	assert(highest_bit_position(1) == 1);
	assert(highest_bit_position(2) == 2);
	assert(highest_bit_position(3) == 2);
	assert(highest_bit_position(4) == 3);
	assert(highest_bit_position(SIZE_MAX-1) == (sizeof(size_t) * CHAR_BIT));
	assert(highest_bit_position(SIZE_MAX) == (sizeof(size_t) * CHAR_BIT));
}

void test_ceiling_power_of_two() {
	assert(ceiling_power_of_two(0) == 1);
	assert(ceiling_power_of_two(1) == 1);
	assert(ceiling_power_of_two(2) == 2);
	assert(ceiling_power_of_two(3) == 4);
	assert(ceiling_power_of_two(4) == 4);
	assert(ceiling_power_of_two(5) == 8);
	assert(ceiling_power_of_two(6) == 8);
	assert(ceiling_power_of_two(7) == 8);
	assert(ceiling_power_of_two(8) == 8);
}

void test_popcount_byte() {
	for (uint8_t i = 0; i < 256; i++) {
		assert(popcount_byte(i) == (popcount_byte(i / 2) + (i & 1)));
	}
}

void test_bitset_basic() {
	start_test;
	unsigned char buf[4] = {0};
	assert(bitset_sizeof(7) == 1);
	assert(bitset_sizeof(8) == 1);
	assert(bitset_sizeof(9) == 2);
	assert(!bitset_test(buf, 0));
	bitset_set(buf, 0);
	assert(bitset_test(buf, 0));
}

void test_bitset_range() {
	start_test;
	unsigned char buf[4] = {0};
	size_t bitset_length = 32;
	for (size_t i = 0; i < bitset_length; i++) {
		for (size_t j = 0; j <= i; j++) {
			memset(buf, 0, 4);
			bitset_set_range(buf, j, i);
			for (size_t k = 0; k < bitset_length; k++) {
				if ((k >= j) && (k <= i)) {
					assert(bitset_test(buf, k));
				} else {
					assert(!bitset_test(buf, k));
				}
			}
			bitset_clear_range(buf, j, i);
			for (size_t k = j; k < i; k++) {
				assert(!bitset_test(buf, k));
			}
		}
	}
}

void test_bitset_shift() {
	start_test;
	unsigned char* buf = malloc(bitset_sizeof(16));
	for (size_t i = 0; i < bitset_sizeof(16); i++) {
		buf[i] = 0;
	}
	for (size_t i = 0; i < 16; i++) {
		bitset_clear(buf, i);
	}
	bitset_set(buf, 0);
	bitset_set(buf, 3);
	bitset_set(buf, 4);
	bitset_set(buf, 7);
	bitset_shift_right(buf, 0, 8, 4);
	assert(!bitset_test(buf, 0));
	assert(!bitset_test(buf, 1));
	assert(!bitset_test(buf, 2));
	assert(!bitset_test(buf, 3));
	assert(bitset_test(buf, 4));
	assert(!bitset_test(buf, 5));
	assert(!bitset_test(buf, 6));
	assert(bitset_test(buf, 7));
	assert(bitset_test(buf, 8));
	assert(!bitset_test(buf, 9));
	assert(!bitset_test(buf, 10));
	assert(bitset_test(buf, 11));
	assert(!bitset_test(buf, 12));
	assert(!bitset_test(buf, 13));
	assert(!bitset_test(buf, 14));
	assert(!bitset_test(buf, 15));
	bitset_shift_left(buf, 4, 12, 4);
	assert(bitset_test(buf, 0));
	assert(!bitset_test(buf, 1));
	assert(!bitset_test(buf, 2));
	assert(bitset_test(buf, 3));
	assert(bitset_test(buf, 4));
	assert(!bitset_test(buf, 5));
	assert(!bitset_test(buf, 6));
	assert(bitset_test(buf, 7));
	assert(!bitset_test(buf, 8));
	assert(!bitset_test(buf, 9));
	assert(!bitset_test(buf, 10));
	assert(!bitset_test(buf, 11));
	assert(!bitset_test(buf, 12));
	assert(!bitset_test(buf, 13));
	assert(!bitset_test(buf, 14));
	assert(!bitset_test(buf, 15));
	free(buf);
}

void test_bitset_shift_invalid() {
	start_test;
	unsigned char buf[4096] = {0};
	bitset_set_range(buf, 1, 0); /* no-op */
	assert(!bitset_test(buf, 0));
	assert(!bitset_test(buf, 1));
	bitset_set_range(buf, 0, 1);
	assert(bitset_test(buf, 0));
	assert(bitset_test(buf, 1));
	bitset_clear_range(buf, 1, 0) /* no-op */;
	assert(bitset_test(buf, 0));
	assert(bitset_test(buf, 1));
	bitset_clear_range(buf, 0, 1);
	assert(!bitset_test(buf, 0));
	assert(!bitset_test(buf, 1));
}

void test_bitset_debug() {
	start_test;
	unsigned char buf[4096] = {0};
	bitset_set(buf, 0);
	bitset_clear(buf, 1);
	bitset_debug(stdout, buf, 2); /* code coverage */
}

void test_buddy_init_null() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	{
		struct buddy *buddy = buddy_init(NULL, data_buf, 4096);
		assert(buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, NULL, 4096);
		assert(buddy == NULL);
	}
	free(buddy_buf);
}

void test_buddy_init_overlap() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(data_buf, data_buf, 4096);
	assert(buddy == NULL);
}

struct wrong_alignment {
	size_t placement_1;
	unsigned char buffer_1[4096];
	size_t placement_2;
	unsigned char buffer_2[4096];
};

void test_buddy_misalignment() {
	start_test;
	struct wrong_alignment wa = {0};
	{
		struct buddy *buddy = buddy_init(wa.buffer_1 + 1, wa.buffer_2, 4096);
		assert(buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(wa.buffer_1, wa.buffer_2+1, 2048);
		assert(buddy == NULL);
	}
}

void test_buddy_embed_misalignment() {
	start_test;
	struct wrong_alignment wa = {0};
	{
		struct buddy *buddy = buddy_embed(wa.buffer_1 + 1, 2048);
		assert(buddy == NULL);
	}
}

void test_buddy_invalid_datasize() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	{
		assert(buddy_sizeof(0) == 0);
		assert(buddy_sizeof(BUDDY_ALLOC_ALIGN-1) == 0);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 0);
		assert(buddy == NULL);
	}
	free(buddy_buf);
}

void test_buddy_init() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
		assert(buddy != NULL);
	}
	free(buddy_buf);
}

void test_buddy_init_virtual_slots() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1020);
	assert(buddy != NULL);
	free(buddy_buf);
}

void test_buddy_init_non_power_of_two_memory_01() {
	start_test;
	size_t buddy_size = PSS(4096);
	unsigned char* buddy_buf = malloc(buddy_sizeof(buddy_size));
	unsigned char* data_buf  = malloc(buddy_size);

	size_t cutoff = PSS(256);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, buddy_size-cutoff);
	assert(buddy != NULL);

	for (size_t i = 0; i < 60; i++) {
		assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) != NULL);
	}
	assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) == NULL);
	
	free(buddy_buf);
	free(data_buf);
}

void test_buddy_init_non_power_of_two_memory_02() {
	start_test;
	size_t buddy_size = PSS(4096);
	unsigned char* buddy_buf = malloc(buddy_sizeof(buddy_size));
	unsigned char* data_buf  = malloc(buddy_size);

	size_t cutoff = PSS(256+(sizeof(size_t)/2));
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, buddy_size-cutoff);
	assert(buddy != NULL);

	for (size_t i = 0; i < 59; i++) {
		assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) != NULL);
	}
	assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) == NULL);

	free(buddy_buf);
	free(data_buf);
}

void test_buddy_init_non_power_of_two_memory_03() {
	start_test;
	size_t buddy_size = PSS(4096);
	unsigned char* buddy_buf = malloc(buddy_sizeof(buddy_size));
	unsigned char* data_buf  = malloc(buddy_size);

	size_t cutoff = PSS(256-(sizeof(size_t)/2));
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, buddy_size-cutoff);
	assert(buddy != NULL);

	for (size_t i = 0; i < 60; i++) {
		assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) != NULL);
	}
	assert(buddy_malloc(buddy, BUDDY_ALLOC_ALIGN) == NULL);

	free(buddy_buf);
	free(data_buf);
}

void test_buddy_resize_noop() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 1024) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_up_within_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 896) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_up_at_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 1024) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_up_after_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 2048) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_to_virtual() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 832) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_to_virtual_partial() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 832-1) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_within_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 640) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_within_reserved_failure() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	void *r512 = buddy_malloc(buddy, 512);
	assert(r512 != NULL);
	void *r256 = buddy_malloc(buddy, 256);
	assert(r256 != NULL);
	buddy_free(buddy, r512);
	assert(buddy_resize(buddy, 640) == NULL);
	free(buddy_buf);
}

void test_buddy_resize_down_at_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 512) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_before_reserved() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 768);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 448) == buddy);
	free(buddy_buf);
}

void test_buddy_resize_down_already_used() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	void *r1024 = buddy_malloc(buddy, 1024);
	assert(r1024 == data_buf);
	assert(buddy_resize(buddy, 256) == NULL);
	free(buddy_buf);
}

void test_buddy_resize_embedded_up_within_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	buddy = buddy_resize(buddy, 896 + buddy_sizeof(896));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 512) == data_buf);
	assert(buddy_malloc(buddy, 256) == data_buf+512);
	assert(buddy_malloc(buddy, 128) == data_buf+512+256);
	/* No more space */
	assert(buddy_malloc(buddy, 64) == NULL);
	assert(buddy_malloc(buddy, 32) == NULL);
	assert(buddy_malloc(buddy, 16) == NULL);
	assert(buddy_malloc(buddy, 8) == NULL);
}

void test_buddy_resize_embedded_up_at_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy = buddy_resize(buddy, 1024 + (sizeof(size_t)*OF(4,5)) + buddy_sizeof(1024));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
}

void test_buddy_resize_embedded_up_after_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + (sizeof(size_t)*2) + buddy_sizeof(768));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy = buddy_resize(buddy, 2048 + (sizeof(size_t)*OF(3,9)) + buddy_sizeof(2048));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf + 1024);
	assert(buddy_malloc(buddy, 1024) == NULL);
}

void test_buddy_resize_embedded_down_within_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	buddy = buddy_resize(buddy, 640 + buddy_sizeof(640));
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 512) == data_buf);
	assert(buddy_malloc(buddy, 64) == data_buf+512);
	assert(buddy_malloc(buddy, 64) == data_buf+512+64);
	assert(buddy_malloc(buddy, 64) == NULL);
}

void test_buddy_resize_embedded_down_within_reserved_failure() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	void *r512 = buddy_malloc(buddy, 512);
	assert(r512 != NULL);
	void *r256 = buddy_malloc(buddy, 256);
	assert(r256 != NULL);
	buddy_free(buddy, r512);
	assert(buddy_resize(buddy, 640 + buddy_sizeof(640)) == NULL);
}

void test_buddy_resize_embedded_down_at_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 512 + buddy_sizeof(512)) != NULL);
}

void test_buddy_resize_embedded_down_before_reserved() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 768 + buddy_sizeof(768));
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 448 + buddy_sizeof(448)) != NULL);
}

void test_buddy_resize_embedded_down_already_used() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 4096);
	assert(buddy != NULL);
	buddy_malloc(buddy, 1024);
	assert(buddy_resize(buddy, 256 + buddy_sizeof(256)) == NULL);
}

void test_buddy_resize_embedded_too_small() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 4096);
	assert(buddy != NULL);
	assert(buddy_resize(buddy, 1) == NULL);
}

void test_buddy_debug() {
	start_test;
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_embed(data_buf, 256);
	buddy_debug(stdout, buddy); /* code coverage */
}

void test_buddy_can_shrink() {
	start_test;
    unsigned char data_buf[4096];
    unsigned char buddy_buf[4096];
	struct buddy *buddy = NULL;
	assert(buddy_can_shrink(buddy) == 0);
	buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy_can_shrink(buddy) == 1);
    void *r2048_1 = buddy_malloc(buddy, 2048);
    assert(r2048_1 == data_buf);
    void *r2048_2 = buddy_malloc(buddy, 2048);
    assert(r2048_2 == data_buf+2048);
    buddy_free(buddy, r2048_1);
    assert(buddy_can_shrink(buddy) == 0);
    buddy_free(buddy, r2048_2);
    void *r4096 = buddy_malloc(buddy, 4096);
    assert(r4096 == data_buf);
    assert(buddy_can_shrink(buddy) == 0);
}

void test_buddy_arena_size() {
	start_test;
	unsigned char data_buf[4096];
	unsigned char buddy_buf[4096];
	struct buddy *buddy = NULL;
	assert(buddy_arena_size(buddy) == 0);
	buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy_arena_size(buddy) == 4096);
}

void test_buddy_malloc_null() {
	start_test;
	assert(buddy_malloc(NULL, 1024) == NULL);
}

void test_buddy_malloc_zero() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	/* This is implementation-defined in the standard! */
	assert(buddy_malloc(buddy, 0) != NULL);
	free(buddy_buf);
}

void test_buddy_malloc_larger() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 8192) == NULL);
	free(buddy_buf);
}

void test_buddy_malloc_basic_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(1024));
	unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy_free(buddy, data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
	free(buddy_buf);
}

void test_buddy_malloc_basic_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
	free(buddy_buf);
}

void test_buddy_malloc_basic_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 1024) == data_buf+1024);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy_free(buddy, data_buf+1024);
	buddy_free(buddy, data_buf+2048);
	buddy_free(buddy, data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 1024) == data_buf+1024);
	assert(buddy_malloc(buddy, 1024) == NULL);
	free(buddy_buf);
}

void test_buddy_malloc_basic_04() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 64) == data_buf);
	assert(buddy_malloc(buddy, 32) == data_buf+64);
	free(buddy_buf);
}

void test_buddy_free_coverage() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf+1024, 1024);
	assert(buddy != NULL);
	buddy_free(NULL, NULL);
	buddy_free(NULL, data_buf+1024);
	buddy_free(buddy, NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
	free(buddy_buf);
}

void test_buddy_free_alignment() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	buddy_free(buddy, data_buf+1);
	free(buddy_buf);
}

void test_buddy_free_invalid_free_01() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_free(buddy, r);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a partially-allocated parent
	buddy_free(buddy, r);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_free_invalid_free_02() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_free(buddy, l);
	buddy_free(buddy, r);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a unallocated parent
	buddy_free(buddy, r);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_free_invalid_free_03() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_free(buddy, l);
	buddy_free(buddy, r);
	void *m = buddy_malloc(buddy, size); // full allocation
	assert(m == l); // at the start of the arena
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a fully-allocated parent
	buddy_free(buddy, r);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_free_invalid_free_04() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_free(buddy, l);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a left node
	// that will propagate to a partially-allocated parent
	buddy_free(buddy, l);
	/* the use check in buddy_tree_release handles this case */
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_coverage() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf+1024, 1024);
	assert(buddy != NULL);
	buddy_safe_free(NULL, NULL, 0);
	buddy_safe_free(NULL, data_buf+1024, 0);
	buddy_safe_free(buddy, NULL, 0);
	buddy_safe_free(buddy, data_buf, 0);
	buddy_safe_free(buddy, data_buf+2048, 0);
	free(buddy_buf);
}

void test_buddy_safe_free_alignment() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	buddy_safe_free(buddy, data_buf+1, 0);
	free(buddy_buf);
}

void test_buddy_safe_free_invalid_free_01() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a partially-allocated parent
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_02() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_safe_free(buddy, l, BUDDY_ALLOC_ALIGN);
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a unallocated parent
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_03() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_safe_free(buddy, l, BUDDY_ALLOC_ALIGN);
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	void *m = buddy_malloc(buddy, size); // full allocation
	assert(m == l); // at the start of the arena
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a right node
	// that will propagate to a fully-allocated parent
	buddy_safe_free(buddy, r, BUDDY_ALLOC_ALIGN);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_04() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(l != NULL);
	void *r = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	assert(r != NULL);
	assert(l != r);
	buddy_safe_free(buddy, l, BUDDY_ALLOC_ALIGN);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// double-free on a left node
	// that will propagate to a partially-allocated parent
	buddy_safe_free(buddy, l, BUDDY_ALLOC_ALIGN);
	/* the use check in buddy_tree_release handles this case */
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_05() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *l = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// safe free with double size
	buddy_safe_free(buddy, l, BUDDY_ALLOC_ALIGN * 2);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_06() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *m = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN*2);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// safe free with half size
	buddy_safe_free(buddy, m, BUDDY_ALLOC_ALIGN);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_safe_free_invalid_free_07() {
	start_test;
	size_t size = BUDDY_ALLOC_ALIGN * 2;
	unsigned char* buddy_buf = malloc(buddy_sizeof(size));
	unsigned char* buddy_buf_control = malloc(buddy_sizeof(size));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	void *m = buddy_malloc(buddy, BUDDY_ALLOC_ALIGN*2);
	memcpy(buddy_buf_control, buddy_buf, buddy_sizeof(size));
	// safe free with zero size
	buddy_safe_free(buddy, m, 0);
	assert(memcmp(buddy_buf_control, buddy_buf, buddy_sizeof(size)) == 0);
	free(buddy_buf);
	free(buddy_buf_control);
}

void test_buddy_demo() {
	size_t arena_size = 65536;
	/* You need space for the metadata and for the arena */
	void *buddy_metadata = malloc(buddy_sizeof(arena_size));
	void *buddy_arena = malloc(arena_size);
	struct buddy *buddy = buddy_init(buddy_metadata, buddy_arena, arena_size);

	/* Allocate using the buddy allocator */
	void *data = buddy_malloc(buddy, 2048);
	/* Free using the buddy allocator */
	buddy_free(buddy, data);

	assert(buddy_is_empty(NULL) == 1);
	assert(buddy_is_empty(buddy));

	free(buddy_metadata);
	free(buddy_arena);
}

void test_buddy_demo_embedded() {
	size_t arena_size = 65536;
	/* You need space for arena and builtin metadata */
	void *buddy_arena = malloc(arena_size);
	struct buddy *buddy = buddy_embed(buddy_arena, arena_size);

	/* Allocate using the buddy allocator */
	void *data = buddy_malloc(buddy, 2048);
	/* Free using the buddy allocator */
	buddy_free(buddy, data);

	free(buddy_arena);
}

void test_buddy_calloc() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	memset(data_buf, 1, 4096);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 4096);
	for (size_t i = 0; i < 4096; i++) {
		assert(result[i] == 0);
	}
	free(buddy_buf);
}

void test_buddy_calloc_no_members() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, 0, 4096);
	/* This is implementation-defined in the standard! */
	assert(result != NULL);
	free(buddy_buf);
}

void test_buddy_calloc_no_size() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 0);
	/* This is implementation-defined in the standard! */
	assert(result != NULL);
	free(buddy_buf);
}

void test_buddy_calloc_overflow() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	short *result = buddy_calloc(buddy, sizeof(short), SIZE_MAX);
	assert(result == NULL);
	free(buddy_buf);
}

void test_buddy_realloc_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	/* This is implementation-defined! */
	void *result = buddy_realloc(buddy, NULL, 0);
	assert(result != NULL);
	free(buddy_buf);
}

void test_buddy_realloc_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	free(buddy_buf);
}

void test_buddy_realloc_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 128);
	assert(result == data_buf);
	free(buddy_buf);
}

void test_buddy_realloc_04() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 64);
	assert(result == data_buf);
	free(buddy_buf);
}

void test_buddy_realloc_05() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 256);
	assert(result == data_buf);
	free(buddy_buf);
}

void test_buddy_realloc_06() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 0);
	assert(result == NULL);
	free(buddy_buf);
}

void test_buddy_realloc_07() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert(buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 1024);
	assert(result == NULL);
	free(buddy_buf);
}

void test_buddy_realloc_08() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 256) == data_buf);
	void *result = buddy_realloc(buddy, NULL, 256);
	assert(result == data_buf + 256);
	result = buddy_realloc(buddy, result, 512);
	assert(result == NULL);
	free(buddy_buf);
}

void test_buddy_realloc_alignment() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert(buddy_realloc(buddy, data_buf+1, 2048) == NULL);
	free(buddy_buf);
}

void test_buddy_reallocarray_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, 0, 0);
	/* This is implementation-defined! */
	assert(result != NULL);
	free(buddy_buf);
}

void test_buddy_reallocarray_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(short), SIZE_MAX);
	assert(result == NULL);
	free(buddy_buf);
}

void test_buddy_reallocarray_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(char), 256);
	assert(result == data_buf);
	free(buddy_buf);
}

void test_buddy_embedded_not_enough_memory() {
	start_test;
	unsigned char buf[4];
	assert(buddy_embed(buf, 4) == NULL);
	assert(buddy_embed(buf, 0) == NULL);
}

void test_buddy_embedded_null() {
	start_test;
	struct buddy *buddy = buddy_embed(NULL, 4096);
	assert(buddy == NULL);
}

void test_buddy_embedded_01() {
	start_test;
	unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
}

void test_buddy_embedded_malloc_01() {
	start_test;
	unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, buf);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
}

void test_buddy_mixed_use_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[8] = {0};
	for (size_t i = 0; i < 8; i++) {
		addr[i] = buddy_malloc(buddy, 64);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 8; i++) {
		if (i%2 == 0) {
			buddy_free(buddy, addr[i]);
		}
	}
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) == NULL);
	free(buddy_buf);
}

void test_buddy_mixed_use_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[8] = {0};
	for (size_t i = 0; i < 8; i++) {
		addr[i] = buddy_malloc(buddy, 64);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 8; i++) {
		buddy_free(buddy, addr[i]);
	}
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 128) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) != NULL);
	assert(buddy_malloc(buddy, 64) == NULL);
	free(buddy_buf);
}

void test_buddy_mixed_use_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *addr[4] = {0};
	for (size_t i = 0; i < 4; i++) {
		addr[i] = buddy_malloc(buddy, 128);
		assert(addr[i] != NULL);
	}
	for (size_t i = 0; i < 4; i++) {
		buddy_free(buddy, addr[i]);
	}
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 256) != NULL);
	assert(buddy_malloc(buddy, 256) == NULL);
	free(buddy_buf);
}

void test_buddy_large_arena() {
	start_test;
	size_t size = PSS(2147483648) /* 2/1 GB */;
	unsigned char *data_buf = malloc(size);
	unsigned char *buddy_buf = malloc(buddy_sizeof(size));
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, size);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, size) == data_buf);
	buddy_free(buddy, data_buf);
	free(buddy_buf);
	free(data_buf);
}

void *walker_01(void *ctx, void *addr, size_t size) {
	(void) addr;
	assert(size == 64);
	size_t *counter = (size_t *) ctx;
	(*counter)++;
	if ((*counter) > 2) {
		return addr; /* cause a stop */
	}
	return NULL;
}

void test_buddy_walk_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert(buddy_walk(NULL, walker_01, NULL) == NULL);
	assert(buddy_walk(buddy, NULL, NULL) == NULL);
	void *a = buddy_malloc(buddy, 64);
	buddy_malloc(buddy, 64);
	size_t counter = 0;
	assert(buddy_walk(buddy, walker_01, &counter) == NULL);
	assert(counter == 2);
	buddy_free(buddy, a);
	counter = 0;
	assert(buddy_walk(buddy, walker_01, &counter) == NULL);
	assert(counter == 1);
	counter = 0;
	buddy_malloc(buddy, 64);
	buddy_malloc(buddy, 64);
	buddy_malloc(buddy, 64);
	assert(buddy_walk(buddy, walker_01, &counter) != NULL);
	free(buddy_buf);
}

void *walker_02(void *ctx, void *addr, size_t size) {
	(void) addr;
	assert(size == 128);
	size_t *counter = (size_t *) ctx;
	(*counter)++;
	if ((*counter) > 2) {
		return addr; /* cause a stop */
	}
	return NULL;
}
void test_buddy_walk_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	size_t counter = 0;
	buddy_malloc(buddy, 128);
	buddy_malloc(buddy, 128);
	buddy_malloc(buddy, 128);
	assert(buddy_walk(buddy, walker_02, &counter) != NULL);
	free(buddy_buf);
}

struct walker_03_entry {
	void *addr;
	size_t size;
};

void *walker_03(void *ctx, void *addr, size_t size) {
	struct walker_03_entry (*context) = (struct walker_03_entry *) ctx;
	unsigned int found = 0;
	for (size_t i = 0; i < 3; i++) {
		struct walker_03_entry *current = context + i;
		if ((current->addr == addr) && (current->size) == size) {
			found = 1;
			break;
		}
	}
	assert(found);
	return NULL;
}

void test_buddy_walk_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	struct walker_03_entry context[3];
	context[0] = (struct walker_03_entry){.addr = buddy_malloc(buddy, 128), .size = 128};
	context[1] = (struct walker_03_entry){.addr = buddy_malloc(buddy, 64), .size = 64};
	context[2] = (struct walker_03_entry){.addr = buddy_malloc(buddy, 256), .size = 256};
	assert(buddy_walk(buddy, walker_03, &context) == NULL);
	free(buddy_buf);
}

void *walker_04(void *ctx, void *addr, size_t size) {
	struct buddy *buddy = (struct buddy *) ctx;
	assert(addr != NULL);
	assert(size != 0);
	buddy_free(buddy, addr);
	return NULL;
}

void test_buddy_walk_04() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_malloc(buddy, 128);
	buddy_malloc(buddy, 64);
	buddy_malloc(buddy, 256);
	assert(buddy_walk(buddy, walker_04, buddy) == NULL);
	assert(buddy_is_empty(buddy));
	free(buddy_buf);
}

void *walker_05(void *ctx, void *addr, size_t size) {
	(void) addr;
	(void) size;
	return ctx;
}

void test_buddy_walk_05() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(4096));
	unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 3648); // virtual slots
	unsigned int ctx = 0;
	assert(buddy_walk(buddy, walker_05, &ctx) == NULL);
	assert(walker_05(&ctx, NULL, 0) == &ctx); // coverage
	free(buddy_buf);
}

void *walker_06(void *ctx, void *addr, size_t size) {
	struct buddy *buddy = (struct buddy *) ctx;
	buddy_realloc(buddy, addr, size);
	return NULL;
}

void test_buddy_walk_06() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *a[8];
	a[0] = buddy_malloc(buddy, 64);
	a[1] = buddy_malloc(buddy, 64);
	a[2] = buddy_malloc(buddy, 64);
	a[3] = buddy_malloc(buddy, 64);
	a[4] = buddy_malloc(buddy, 64);
	a[5] = buddy_malloc(buddy, 64);
	a[6] = buddy_malloc(buddy, 64);
	a[7] = buddy_malloc(buddy, 64);
	buddy_free(buddy, a[1]);
	buddy_free(buddy, a[3]);
	buddy_free(buddy, a[5]);
	buddy_free(buddy, a[7]);
	// at this point we have 4x64 slots free but they are
	// fragmented and cannot be coalesced
	assert(buddy_malloc(buddy, 256) == NULL);
	buddy_walk(buddy, walker_06, buddy);
	assert(buddy_malloc(buddy, 256) != NULL);
	free(buddy_buf);
}

void test_buddy_reserve_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, buddy_buf, 512); // wrong dst
	assert(buddy_malloc(buddy, 512) == data_buf); // entire buddy should be free
	free(buddy_buf);
}

void test_buddy_reserve_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, data_buf, 0); // zero length
	assert(buddy_malloc(buddy, 512) == data_buf); // entire buddy should be free
	free(buddy_buf);
}

void test_buddy_reserve_03() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, data_buf, 512); // full length
	assert(buddy_malloc(buddy, 512) == NULL); // entire buddy should be busy
	free(buddy_buf);
}

void test_buddy_reserve_04() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, data_buf, 512-16); // almost-full length
	assert(buddy_malloc(buddy, 512) == NULL); // entire buddy should be busy
	free(buddy_buf);
}

void test_buddy_reserve_05() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, data_buf, 256); // half length
	assert(buddy_malloc(buddy, 512) == NULL);
	assert(buddy_malloc(buddy, 256) == (data_buf + 256));
	assert(buddy_malloc(buddy, 256) == NULL);
	free(buddy_buf);
}

void test_buddy_unsafe_release_01() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_malloc(buddy, 512); // Use all
	buddy_unsafe_release_range(buddy, data_buf, 0); // Zero length
	assert(buddy_malloc(buddy, 512) == NULL); // no release should have happened
	free(buddy_buf);
}

void test_buddy_unsafe_release_02() {
	start_test;
	unsigned char* buddy_buf = malloc(buddy_sizeof(512));
	unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	buddy_reserve_range(buddy, data_buf, 256); // half length
	buddy_malloc(buddy, 256); // Use the rest
	assert(buddy_malloc(buddy, 256) == NULL); // no space
	buddy_unsafe_release_range(buddy, data_buf, 256); // get back half
	assert(buddy_malloc(buddy, 256) == data_buf); // there should be space now
	free(buddy_buf);
}

void test_buddy_fragmentation() {
	start_test;

	// This test verified the same as test_buddy_tree_fragmentation,
	// just through the allocator public interface

	size_t buddy_size = PSS(265);
	unsigned char* buddy_buf = malloc(buddy_sizeof(buddy_size));
	unsigned char* data_buf = malloc(buddy_size);
	struct buddy *b = buddy_init(buddy_buf, data_buf, buddy_size);

	// No fragmentation for invalid allocator
	assert(buddy_fragmentation(NULL) == 0);

	// No fragmentation for empty allocator
	assert(buddy_fragmentation(b) == 0);

	// No fragmentation for full allocator either
	buddy_malloc(b, buddy_size);
	assert(buddy_fragmentation(b) == 0);
	buddy_free(b, data_buf);

	// Some fragmentation for partially-used allocator
	buddy_malloc(b, PSS(64));
	assert(buddy_fragmentation(b) == 0.4375);

	free(buddy_buf);
	free(data_buf);
}

void test_buddy_bias() {
	start_test;

	buddy_set_left_bias(NULL); // coverage
	buddy_set_optimal_fit(NULL);

	size_t buddy_size = PSS(265);
	unsigned char* buddy_buf = malloc(buddy_sizeof(buddy_size));
	unsigned char* data_buf = malloc(buddy_size);
	struct buddy *b = buddy_init(buddy_buf, data_buf, buddy_size);

	void *allocs[4];
	for (size_t i = 0; i < 4; i++) {
		allocs[i] = buddy_malloc(b, PSS(64));
	}

	for (size_t i = 0; i < 3; i++) {
		buddy_free(b, allocs[i]);
	}

	buddy_set_left_bias(b);
	assert(buddy_malloc(b, PSS(64)) == allocs[0]);
	buddy_free(b, allocs[0]);

	buddy_set_optimal_fit(b);
	assert(buddy_malloc(b, PSS(64)) == allocs[2]);
	
	free(buddy_buf);
	free(data_buf);
}

void test_buddy_tree_init() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	assert(buddy_tree_init(buddy_tree_buf, 8) != NULL);
}

void test_buddy_tree_valid() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_valid(t, (struct buddy_tree_pos){ 0, 0 }) == 0);
	assert(buddy_tree_valid(t, (struct buddy_tree_pos){ 256, 0 }) == 0);
	assert(buddy_tree_valid(t, (struct buddy_tree_pos){ 1, 1 }) == 1);
	assert(buddy_tree_valid(t, (struct buddy_tree_pos){ 255, 8 }) == 1);
}

void test_buddy_tree_order() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_order(t) == 8);
}

void test_buddy_tree_depth() {
	start_test;
	assert(buddy_tree_depth((struct buddy_tree_pos){ 1, 1 }) == 1);
    assert(buddy_tree_depth((struct buddy_tree_pos){ 2, 2 }) == 2);
    assert(buddy_tree_depth((struct buddy_tree_pos){ 3, 2 }) == 2);
}

void test_buddy_tree_left_child() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	pos = buddy_tree_left_child(pos);
	assert(buddy_tree_depth(pos) == 2);
	pos = buddy_tree_left_child(pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_right_child() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	pos = buddy_tree_right_child(pos);
	assert(buddy_tree_depth(pos) == 2);
	pos = buddy_tree_right_child(pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_parent() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(! buddy_tree_valid(t, buddy_tree_parent(pos)));
	assert(! buddy_tree_valid(t, buddy_tree_parent(INVALID_POS)));
	assert(buddy_tree_parent(buddy_tree_left_child(pos)).index == pos.index);
	assert(buddy_tree_parent(buddy_tree_right_child(pos)).index == pos.index);
}

void test_buddy_tree_right_adjacent() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(! buddy_tree_valid(t, buddy_tree_right_adjacent(pos)));
	assert(! buddy_tree_valid(t, buddy_tree_right_adjacent(buddy_tree_right_child(pos))));
	assert(buddy_tree_right_adjacent(buddy_tree_left_child(pos)).index == buddy_tree_right_child(pos).index);
}

void test_buddy_tree_index() {
	start_test;
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(buddy_tree_index(pos) == 0);
	assert(buddy_tree_index(buddy_tree_left_child(pos)) == 0);
	assert(buddy_tree_index(buddy_tree_right_child(pos)) == 1);
}

void test_buddy_tree_mark_status_release_01() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);

	struct buddy_tree_pos pos = buddy_tree_root();
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 1);
	buddy_tree_release(t, pos);
	assert(buddy_tree_status(t, pos) == 0);
}

void test_buddy_tree_mark_status_release_02() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 2);
}

void test_buddy_tree_mark_status_release_03() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 3);
}

void test_buddy_tree_mark_status_release_04() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 4);
	struct buddy_tree_pos pos = buddy_tree_root();
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 4);
}

void test_buddy_tree_duplicate_mark() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	struct buddy_tree_pos pos = buddy_tree_root();
	buddy_tree_mark(t, pos);
	buddy_tree_mark(t, pos);
}

void test_buddy_tree_duplicate_free() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	struct buddy_tree_pos pos = buddy_tree_root();
	buddy_tree_release(t, pos);
}

void test_buddy_tree_propagation_01() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct buddy_tree_pos pos = buddy_tree_root();
	struct buddy_tree_pos left = buddy_tree_left_child(pos);
	assert(buddy_tree_status(t, left) == 0);
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_propagation_02() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_root();
	struct buddy_tree_pos left = buddy_tree_left_child(buddy_tree_left_child(pos));
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_find_free() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_find_free(t, 1, 0);
	assert(buddy_tree_valid(t, pos) == 1);
	pos = buddy_tree_find_free(t, 2, 0);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_mark(t, pos);
	pos = buddy_tree_find_free(t, 2, 0);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_mark(t, pos);
	pos = buddy_tree_find_free(t, 2, 0);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_find_free_bias() {
	start_test;
	unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 4);
	struct buddy_tree_pos pos[4] = {0};

	for (size_t i = 0; i < 4; i++) { // mark all four
		pos[i] = buddy_tree_find_free(t, 3, 0);
		assert(buddy_tree_valid(t, pos[i]) == 1);
		buddy_tree_mark(t, pos[i]);
	}

	for (size_t i = 0; i < 3; i++) { // release first tree
		buddy_tree_release(t, pos[i]);
	}

	struct buddy_tree_pos found;
	// Test with no left-bias, the optimal fit should
	// find a free slot on the right side of the tree
	// as the right side is partially-used and the left
	// is empty
	found = buddy_tree_find_free(t, 3, 0);
	assert(found.index == pos[2].index);
	assert(found.depth == pos[2].depth);

	// Test with left bias, this should find the first
	// left-most slot available
	found = buddy_tree_find_free(t, 3, 1);
	assert(found.index == pos[0].index);
	assert(found.depth == pos[0].depth);
}

void test_buddy_tree_debug_coverage() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_mark(t, buddy_tree_root());
	buddy_tree_debug(stdout, t, buddy_tree_root(), 0);printf("\n"); /* code coverage */
}

void test_buddy_tree_check_invariant_positive_01() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct internal_position root_internal =
		buddy_tree_internal_position_tree(t, buddy_tree_root());
	write_to_internal_position(buddy_tree_bits(t), root_internal, 1);
	assert(buddy_tree_check_invariant(t, buddy_tree_root()));
}

void test_buddy_tree_check_invariant_positive_02() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	struct internal_position left_internal =
		buddy_tree_internal_position_tree(t, buddy_tree_left_child(buddy_tree_root()));
	write_to_internal_position(buddy_tree_bits(t), left_internal, 1);
	assert(buddy_tree_check_invariant(t, buddy_tree_root()));
}

void test_buddy_tree_check_invariant_negative_01() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_mark(t, buddy_tree_root());
	assert(! buddy_tree_check_invariant(t, buddy_tree_root()));
}

void test_buddy_tree_check_invariant_negative_02() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_mark(t, buddy_tree_left_child(buddy_tree_root()));
	assert(! buddy_tree_check_invariant(t, buddy_tree_root()));
}

void test_buddy_tree_resize_same_size() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_resize(t, 1);
}

void test_buddy_tree_resize_01() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_mark(t, buddy_tree_root());
	buddy_tree_resize(t, 2);
	assert(buddy_tree_order(t) == 2);
	assert(buddy_tree_status(t, buddy_tree_root()) == 1);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 1);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
	buddy_tree_resize(t, 3);
	assert(buddy_tree_status(t, buddy_tree_root()) == 1);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 1);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_left_child(buddy_tree_root()))) == 1);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_left_child(buddy_tree_root()))) == 0);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_right_child(buddy_tree_root()))) == 0);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_right_child(buddy_tree_root()))) == 0);
}

void test_buddy_tree_resize_02() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_mark(t, buddy_tree_left_child(buddy_tree_root()));
	buddy_tree_resize(t, 2);
	assert(buddy_tree_status(t, buddy_tree_root()) == 2);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 0);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
	buddy_tree_resize(t, 1);
	assert(buddy_tree_order(t) == 2); /* cannot shrink */
	assert(buddy_tree_status(t, buddy_tree_root()) == 2);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 0);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
}

void test_buddy_tree_resize_03() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_mark(t, buddy_tree_right_child(buddy_tree_root()));
	buddy_tree_resize(t, 1);
	assert(buddy_tree_order(t) == 2);
	assert(buddy_tree_status(t, buddy_tree_root()) == 1);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 0);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 1);
}

void test_buddy_tree_resize_04() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_mark(t, buddy_tree_root());
	buddy_tree_resize(t, 2);
	assert(buddy_tree_order(t) == 2);
	assert(buddy_tree_status(t, buddy_tree_root()) == 1);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 1);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
}

void test_buddy_tree_resize_05() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_resize(t, 2);
	assert(buddy_tree_order(t) == 2);
	assert(buddy_tree_status(t, buddy_tree_root()) == 0);
	assert(buddy_tree_status(t, buddy_tree_left_child(buddy_tree_root())) == 0);
	assert(buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) == 0);
}

void test_buddy_tree_leftmost_child() {
	start_test;
	{
		unsigned char buddy_tree_buf[4096] = {0};
		struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
		struct buddy_tree_pos leftmost = buddy_tree_leftmost_child(t);
		assert(buddy_tree_valid(t, leftmost));
		assert(leftmost.index == buddy_tree_root().index);
	}
	{
		unsigned char buddy_tree_buf[4096] = {0};
		struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
		struct buddy_tree_pos leftmost = buddy_tree_leftmost_child(t);
		assert(buddy_tree_valid(t, leftmost));
		assert(leftmost.index == buddy_tree_left_child(buddy_tree_root()).index);
	}
}

void test_buddy_tree_is_free_01() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
}

void test_buddy_tree_is_free_02() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
}
void test_buddy_tree_is_free_03() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	buddy_tree_mark(t, buddy_tree_parent(pos));
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 1);
}

void test_buddy_tree_is_free_04() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_mark(t, buddy_tree_root());
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 0);
	pos = buddy_tree_right_adjacent(pos);
	assert(buddy_tree_is_free(t, pos) == 0);
}

void test_buddy_tree_interval() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	struct buddy_tree_interval interval = buddy_tree_interval(t, pos);
	assert(interval.from.index == pos.index);
	assert(interval.to.index == pos.index);
	interval = buddy_tree_interval(t, buddy_tree_parent(pos));
	assert(interval.from.index == pos.index);
	assert(interval.to.index == buddy_tree_right_adjacent(pos).index);
}

void test_buddy_tree_interval_contains() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	struct buddy_tree_pos pos = buddy_tree_leftmost_child(t);
	struct buddy_tree_interval interval_low = buddy_tree_interval(t, pos);
	struct buddy_tree_interval interval_high = buddy_tree_interval(t, buddy_tree_parent(pos));
	assert(buddy_tree_interval_contains(interval_low, interval_low) == 1);
	assert(buddy_tree_interval_contains(interval_high, interval_low) == 1);
	assert(buddy_tree_interval_contains(interval_high, interval_high) == 1);
	assert(buddy_tree_interval_contains(interval_low, interval_high) == 0);
}

void test_buddy_tree_fragmentation() {
	start_test;
	unsigned char buddy_tree_buf[4096] = {0};
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);

	// No fragmentation for empty tree
	assert(buddy_tree_fragmentation(t) == 0);

	// No fragmentation for full tree either
	buddy_tree_mark(t, buddy_tree_root());
	assert(buddy_tree_fragmentation(t) == 0);
	buddy_tree_release(t, buddy_tree_root());

	// Some fragmentation for partially-allocated tree
	buddy_tree_mark(t, buddy_tree_left_child(buddy_tree_left_child(buddy_tree_root())));
	assert(buddy_tree_fragmentation(t) == 0.4375);
}

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);

	{
		test_highest_bit_position();
		test_ceiling_power_of_two();
		test_popcount_byte();
	}

	{
		test_bitset_basic();
		test_bitset_range();

		test_bitset_shift();
		test_bitset_shift_invalid();

		test_bitset_debug();
	}

	{
		test_buddy_init_null();
		test_buddy_init_overlap();
		test_buddy_misalignment();
		test_buddy_embed_misalignment();
		test_buddy_invalid_datasize();

		test_buddy_init();
		test_buddy_init_virtual_slots();
		test_buddy_init_non_power_of_two_memory_01();
		test_buddy_init_non_power_of_two_memory_02();
		test_buddy_init_non_power_of_two_memory_03();

		test_buddy_resize_noop();
		test_buddy_resize_up_within_reserved();
		test_buddy_resize_up_at_reserved();
		test_buddy_resize_up_after_reserved();
		test_buddy_resize_down_to_virtual();
		test_buddy_resize_down_to_virtual_partial();
		test_buddy_resize_down_within_reserved();
		test_buddy_resize_down_within_reserved_failure();
		test_buddy_resize_down_at_reserved();
		test_buddy_resize_down_before_reserved();
		test_buddy_resize_down_already_used();

		test_buddy_resize_embedded_up_within_reserved();
		test_buddy_resize_embedded_up_at_reserved();
		test_buddy_resize_embedded_up_after_reserved();
		test_buddy_resize_embedded_down_within_reserved();
		test_buddy_resize_embedded_down_within_reserved_failure();
		test_buddy_resize_embedded_down_at_reserved();
		test_buddy_resize_embedded_down_before_reserved();
		test_buddy_resize_embedded_down_already_used();
		test_buddy_resize_embedded_too_small();

		test_buddy_debug();

		test_buddy_can_shrink();
		test_buddy_arena_size();

		test_buddy_malloc_null();
		test_buddy_malloc_zero();
		test_buddy_malloc_larger();

		test_buddy_malloc_basic_01();
		test_buddy_malloc_basic_02();
		test_buddy_malloc_basic_03();
		test_buddy_malloc_basic_04();

		test_buddy_free_coverage();
		test_buddy_free_alignment();
		test_buddy_free_invalid_free_01();
		test_buddy_free_invalid_free_02();
		test_buddy_free_invalid_free_03();
		test_buddy_free_invalid_free_04();

		test_buddy_safe_free_coverage();
		test_buddy_safe_free_alignment();
		test_buddy_safe_free_invalid_free_01();
		test_buddy_safe_free_invalid_free_02();
		test_buddy_safe_free_invalid_free_03();
		test_buddy_safe_free_invalid_free_04();
		test_buddy_safe_free_invalid_free_05();
		test_buddy_safe_free_invalid_free_06();
		test_buddy_safe_free_invalid_free_07();

		test_buddy_demo();
		test_buddy_demo_embedded();

		test_buddy_calloc();
		test_buddy_calloc_no_members();
		test_buddy_calloc_no_size();
		test_buddy_calloc_overflow();

		test_buddy_realloc_01();
		test_buddy_realloc_02();
		test_buddy_realloc_03();
		test_buddy_realloc_04();
		test_buddy_realloc_05();
		test_buddy_realloc_06();
		test_buddy_realloc_07();
		test_buddy_realloc_08();
		test_buddy_realloc_alignment();

		test_buddy_reallocarray_01();
		test_buddy_reallocarray_02();
		test_buddy_reallocarray_03();

		test_buddy_embedded_not_enough_memory();
		test_buddy_embedded_null();
		test_buddy_embedded_01();
		test_buddy_embedded_malloc_01();

		test_buddy_mixed_use_01();
		test_buddy_mixed_use_02();
		test_buddy_mixed_use_03();

		test_buddy_large_arena();

		test_buddy_walk_01();
		test_buddy_walk_02();
		test_buddy_walk_03();
		test_buddy_walk_04();
		test_buddy_walk_05();
		test_buddy_walk_06();

		test_buddy_reserve_01();
		test_buddy_reserve_02();
		test_buddy_reserve_03();
		test_buddy_reserve_04();
		test_buddy_reserve_05();

		test_buddy_unsafe_release_01();
		test_buddy_unsafe_release_02();

		test_buddy_fragmentation();

		test_buddy_bias();
	}

	{
		test_buddy_tree_init();
		test_buddy_tree_valid();
		test_buddy_tree_order();
		test_buddy_tree_depth();
		test_buddy_tree_left_child();
		test_buddy_tree_right_child();
		test_buddy_tree_parent();
		test_buddy_tree_right_adjacent();
		test_buddy_tree_index();
		test_buddy_tree_mark_status_release_01();
		test_buddy_tree_mark_status_release_02();
		test_buddy_tree_mark_status_release_03();
		test_buddy_tree_mark_status_release_04();
		test_buddy_tree_duplicate_mark();
		test_buddy_tree_duplicate_free();
		test_buddy_tree_propagation_01();
		test_buddy_tree_propagation_02();
		test_buddy_tree_find_free();
		test_buddy_tree_find_free_bias();
		test_buddy_tree_debug_coverage();
		test_buddy_tree_check_invariant_positive_01();
		test_buddy_tree_check_invariant_positive_02();
		test_buddy_tree_check_invariant_negative_01();
		test_buddy_tree_check_invariant_negative_02();
		test_buddy_tree_resize_same_size();
		test_buddy_tree_resize_01();
		test_buddy_tree_resize_02();
		test_buddy_tree_resize_03();
		test_buddy_tree_resize_04();
		test_buddy_tree_resize_05();
		test_buddy_tree_leftmost_child();
		test_buddy_tree_is_free_01();
		test_buddy_tree_is_free_02();
		test_buddy_tree_is_free_03();
		test_buddy_tree_is_free_04();
		test_buddy_tree_interval();
		test_buddy_tree_interval_contains();

		test_buddy_tree_fragmentation();
	}
}

/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"

#include "bitset.h"
#include "buddy_alloc_tree.h"
#include "buddy_alloc.h"

void test_bitset_basic() {
	start_test;
	unsigned char buf[4] = {0};
	assert(bitset_size(8) == 1);
	assert(bitset_test(buf, 0) == 0);
	bitset_set(buf, 0);
	assert(bitset_test(buf, 0) == 1);
	bitset_flip(buf, 0);
	assert(bitset_test(buf, 0) == 0);
	bitset_flip(buf, 0);
	assert(bitset_test(buf, 0) == 1);
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

void test_buddy_init_null() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)+1];
	alignas(max_align_t) unsigned char data_buf[4096+1];
	{
		struct buddy *buddy = buddy_init(NULL, data_buf, 4096);
		assert (buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, NULL, 4096);
		assert (buddy == NULL);
	}
}

void test_buddy_misalignment() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)+1];
	alignas(max_align_t) unsigned char data_buf[4096+1];
	{
		struct buddy *buddy = buddy_init(buddy_buf+1, data_buf, 4096);
		assert (buddy == NULL);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf+1, 4096);
		assert (buddy == NULL);
	}
}

void test_buddy_invalid_datasize() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	{
		assert(buddy_sizeof(0) == 0);
		assert(buddy_sizeof(BUDDY_ALIGN-1) == 0);
	}
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 0);
		assert (buddy == NULL);
	}
}

void test_buddy_init() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	{
		struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
		assert (buddy != NULL);
	}
}

void test_buddy_init_non_power_of_two_memory() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];

	size_t cutoff = 256;
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096-cutoff);
	assert (buddy != NULL);

	for (size_t i = 0; i < 60; i++) {
		assert(buddy_malloc(buddy, BUDDY_ALIGN) != NULL);
	}
	assert(buddy_malloc(buddy, BUDDY_ALIGN) == NULL);
}

void test_buddy_malloc_null() {
	start_test;
	assert(buddy_malloc(NULL, 1024) == NULL);
}

void test_buddy_malloc_zero() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 0) == NULL);
}

void test_buddy_malloc_larger() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 8192) == NULL);
}

void test_buddy_malloc_basic_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(1024)];
	alignas(max_align_t) unsigned char data_buf[1024];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1024);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
	buddy_free(buddy, data_buf);
	assert(buddy_malloc(buddy, 1024) == data_buf);
	assert(buddy_malloc(buddy, 1024) == NULL);
}

void test_buddy_malloc_basic_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == data_buf);
	assert(buddy_malloc(buddy, 2048) == data_buf+2048);
	assert(buddy_malloc(buddy, 2048) == NULL);
}

void test_buddy_malloc_basic_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
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
}

void test_buddy_malloc_basic_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 64) == data_buf);
	assert(buddy_malloc(buddy, 32) == data_buf+64);
}

void test_buddy_free_coverage() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf+1024, 1024);
	assert (buddy != NULL);
	buddy_free(NULL, NULL);
	buddy_free(NULL, data_buf+1024);
	buddy_free(buddy, NULL);
	buddy_free(buddy, data_buf);
	buddy_free(buddy, data_buf+2048);
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
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	memset(data_buf, 1, 4096);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 4096);
	for (size_t i = 0; i < 4096; i++) {
		assert(result[i] == 0);
	}
}

void test_buddy_calloc_no_members() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, 0, 4096);
	assert(result == NULL);
}

void test_buddy_calloc_no_size() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	char *result = buddy_calloc(buddy, sizeof(char), 0);
	assert(result == NULL);
}

void test_buddy_calloc_overflow() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	short *result = buddy_calloc(buddy, sizeof(short), SIZE_MAX);
	assert(result == NULL);
}

void test_buddy_realloc_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 0);
	assert(result == NULL);
}

void test_buddy_realloc_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
}

void test_buddy_realloc_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 128);
	assert(result == data_buf);
}

void test_buddy_realloc_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(4096)];
	alignas(max_align_t) unsigned char data_buf[4096];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 4096);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 64);
	assert(result == data_buf);
}

void test_buddy_realloc_05() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 256);
	assert(result == data_buf);
}

void test_buddy_realloc_06() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 0);
	assert(result == NULL);
}

void test_buddy_realloc_07() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	void *result = buddy_realloc(buddy, NULL, 128);
	assert(result == data_buf);
	result = buddy_realloc(buddy, result, 1024);
	assert(result == NULL);
}

void test_buddy_realloc_08() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	assert (buddy != NULL);
	assert(buddy_malloc(buddy, 256) == data_buf);
	void *result = buddy_realloc(buddy, NULL, 256);
	assert(result == data_buf + 256);
	result = buddy_realloc(buddy, result, 512);
	assert(result == NULL);
}

void test_buddy_reallocarray_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, 0, 0);
	assert(result == NULL);
}

void test_buddy_reallocarray_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(short), SIZE_MAX);
	assert(result == NULL);
}

void test_buddy_reallocarray_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_buf[buddy_sizeof(512)];
	alignas(max_align_t) unsigned char data_buf[512];
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 512);
	void *result = buddy_reallocarray(buddy, NULL, sizeof(char), 256);
	assert(result == data_buf);
}

void test_buddy_embedded_not_enough_memory() {
	start_test;
	alignas(max_align_t) unsigned char buf[4];
	struct buddy *buddy = buddy_embed(buf, 4);
	assert(buddy == NULL);
}

void test_buddy_embedded_null() {
	start_test;
	struct buddy *buddy = buddy_embed(NULL, 4096);
	assert(buddy == NULL);
}

void test_buddy_embedded_01() {
	start_test;
	alignas(max_align_t) unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
}

void test_buddy_embedded_malloc_01() {
	start_test;
	alignas(max_align_t) unsigned char buf[4096];
	struct buddy *buddy = buddy_embed(buf, 4096);
	assert(buddy != NULL);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
	buddy_free(buddy, buf);
	assert(buddy_malloc(buddy, 2048) == buf);
	assert(buddy_malloc(buddy, 2048) == NULL);
}

void test_buddy_tree_sizeof() {
	start_test;
	assert(buddy_tree_sizeof(0) == 0);
	assert(buddy_tree_sizeof(1) == 2);
	assert(buddy_tree_sizeof(2) == 2);
	assert(buddy_tree_sizeof(3) == 3);
	assert(buddy_tree_sizeof(4) == 4);
	assert(buddy_tree_sizeof(5) == 8);
	assert(buddy_tree_sizeof(6) == 14);
	assert(buddy_tree_sizeof(7) == 27);
	assert(buddy_tree_sizeof(8) == 53);
	assert(buddy_tree_sizeof(9) == 106);
	assert(buddy_tree_sizeof(10) == 210);
	assert(buddy_tree_sizeof(11) == 419);
	assert(buddy_tree_sizeof(12) == 837);
	assert(buddy_tree_sizeof(13) == 1673);
	assert(buddy_tree_sizeof(14) == 3345);
	assert(buddy_tree_sizeof(15) == 6689);
	assert(buddy_tree_sizeof(16) == 13377);
	assert(buddy_tree_sizeof(17) == 26753);
	assert(buddy_tree_sizeof(18) == 53506);
	assert(buddy_tree_sizeof(19) == 107011);
	assert(buddy_tree_sizeof(20) == 214021);
	assert(buddy_tree_sizeof(21) == 428041);
	assert(buddy_tree_sizeof(22) == 856081);
	assert(buddy_tree_sizeof(23) == 1712161);
	assert(buddy_tree_sizeof(24) == 3424321);
	assert(buddy_tree_sizeof(25) == 6848641);
	assert(buddy_tree_sizeof(26) == 13697281);
	assert(buddy_tree_sizeof(27) == 27394561);
	assert(buddy_tree_sizeof(28) == 54789121);
	assert(buddy_tree_sizeof(29) == 109578241);
	assert(buddy_tree_sizeof(30) == 219156481);
	assert(buddy_tree_sizeof(31) == 438312961);
	assert(buddy_tree_sizeof(32) == 876625921);
	assert(buddy_tree_sizeof(33) == 1753251841);
	assert(buddy_tree_sizeof(34) == 3506503682); /* 3 GB */
	assert(buddy_tree_sizeof(35) == 7013007363);
	assert(buddy_tree_sizeof(36) == 14026014725);
	assert(buddy_tree_sizeof(37) == 28052029449);
	assert(buddy_tree_sizeof(38) == 56104058897);
	assert(buddy_tree_sizeof(39) == 112208117793);
	assert(buddy_tree_sizeof(40) == 224416235585);
	assert(buddy_tree_sizeof(41) == 448832471169);
	assert(buddy_tree_sizeof(42) == 897664942337);
	assert(buddy_tree_sizeof(43) == 1795329884673);
	assert(buddy_tree_sizeof(44) == 3590659769345);
	assert(buddy_tree_sizeof(45) == 7181319538689);
	assert(buddy_tree_sizeof(46) == 14362639077377);
	assert(buddy_tree_sizeof(47) == 28725278154753);
	assert(buddy_tree_sizeof(48) == 57450556309505); /* 52 TB .. */
}

void test_buddy_tree_init() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	assert(buddy_tree_init(NULL, 8) == NULL);
	assert(buddy_tree_init(buddy_tree_buf, 0) == NULL);
	assert(buddy_tree_init(buddy_tree_buf, 8) != NULL);
}

void test_buddy_tree_valid() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_valid(NULL, 1) == 0);
	assert(buddy_tree_valid(t, 0) == 0);
	assert(buddy_tree_valid(t, 256) == 0);
	assert(buddy_tree_valid(t, 1) == 1);
	assert(buddy_tree_valid(t, 255) == 1);
}

void test_buddy_tree_order() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_order(NULL) == 0);
	assert(buddy_tree_order(t) == 8);
}

void test_buddy_tree_root() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_root(NULL) == 0);
	assert(buddy_tree_root(t) == 1);
}

void test_buddy_tree_depth() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 8);
	assert(buddy_tree_depth(t, 0) == 0);
	assert(buddy_tree_depth(t, 1) == 1);
}

void test_buddy_tree_left_child() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_depth(t, pos) == 2);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_left_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_right_child() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_depth(t, pos) == 2);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_right_child(t, pos);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_parent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_parent(t, pos) == 0);
	assert(buddy_tree_parent(t, 0) == 0);
	assert(buddy_tree_parent(t, buddy_tree_left_child(t, pos)) == pos);
	assert(buddy_tree_parent(t, buddy_tree_right_child(t, pos)) == pos);
}

void test_buddy_tree_sibling() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_sibling(t, pos) == 0);
	assert(buddy_tree_sibling(t, 0) == 0);
	assert(buddy_tree_sibling(t, buddy_tree_left_child(t, pos)) == buddy_tree_right_child(t, pos));
	assert(buddy_tree_sibling(t, buddy_tree_right_child(t, pos)) == buddy_tree_left_child(t, pos));
}

void test_buddy_tree_left_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_left_adjacent(t, 0) == 0);
	assert(buddy_tree_left_adjacent(t, pos) == 0);
	assert(buddy_tree_left_adjacent(t, buddy_tree_left_child(t, pos)) == 0);
	assert(buddy_tree_left_adjacent(t, buddy_tree_right_child(t, pos)) == buddy_tree_left_child(t, pos));
}

void test_buddy_tree_right_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_right_adjacent(t, 0) == 0);
	assert(buddy_tree_right_adjacent(t, pos) == 0);
	assert(buddy_tree_right_adjacent(t, buddy_tree_right_child(t, pos)) == 0);
	assert(buddy_tree_right_adjacent(t, buddy_tree_left_child(t, pos)) == buddy_tree_right_child(t, pos));
}

void test_buddy_tree_index() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_index(t, 0) == 0);
	assert(buddy_tree_index(t, pos) == 0);
	assert(buddy_tree_index(t, buddy_tree_left_child(t, pos)) == 0);
	assert(buddy_tree_index(t, buddy_tree_right_child(t, pos)) == 1);
}

void test_buddy_tree_mark_status_release_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);

	assert(buddy_tree_status(t, 0) == 0);
	buddy_tree_mark(t, 0); /* coverage */
	buddy_tree_release(t, 0); /* coverage */

	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 1);
	buddy_tree_release(t, pos);
	assert(buddy_tree_status(t, pos) == 0);
}

void test_buddy_tree_mark_status_release_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 2);
}

void test_buddy_tree_mark_status_release_03() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 3);
}

void test_buddy_tree_mark_status_release_04() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 4);
	buddy_tree_pos pos = buddy_tree_root(t);
	assert(buddy_tree_status(t, pos) == 0);
	buddy_tree_mark(t, pos);
	assert(buddy_tree_status(t, pos) == 4);
}

void test_buddy_tree_duplicate_mark() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_mark(t, pos);
	buddy_tree_mark(t, pos);
}

void test_buddy_tree_duplicate_free() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 1);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_release(t, pos);
}

void test_buddy_tree_propagation_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 2);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_pos left = buddy_tree_left_child(t, pos);
	assert(buddy_tree_status(t, left) == 0);
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_propagation_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_root(t);
	buddy_tree_pos left = buddy_tree_left_child(t, buddy_tree_left_child(t, pos));
	buddy_tree_mark(t, left);
	assert(buddy_tree_status(t, left) == 1);
	assert(buddy_tree_status(t, pos) == 1);
}

void test_buddy_tree_find_free_01() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_find_free(t, 0);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_find_free(t, 4);
	assert(buddy_tree_valid(t, pos) == 0);
	pos = buddy_tree_find_free(NULL, 1);
	assert(buddy_tree_valid(t, pos) == 0);
}

void test_buddy_tree_find_free_02() {
	start_test;
	alignas(max_align_t) unsigned char buddy_tree_buf[4096];
	struct buddy_tree *t = buddy_tree_init(buddy_tree_buf, 3);
	buddy_tree_pos pos = buddy_tree_find_free(t, 1);
	assert(buddy_tree_valid(t, pos) == 1);
	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");
	buddy_tree_mark(t, pos);
	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");

	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 1);

	buddy_tree_mark(t, pos);
	buddy_tree_debug(t, buddy_tree_root(t));printf("\n");

	pos = buddy_tree_find_free(t, 2);
	assert(buddy_tree_valid(t, pos) == 0);
}

int main() {
	{
		test_bitset_basic();
		test_bitset_range();
	}

	{
		test_buddy_init_null();
		test_buddy_misalignment();
		test_buddy_invalid_datasize();

		test_buddy_init();
		test_buddy_init_non_power_of_two_memory();

		test_buddy_malloc_null();
		test_buddy_malloc_zero();
		test_buddy_malloc_larger();

		test_buddy_malloc_basic_01();
		test_buddy_malloc_basic_02();
		test_buddy_malloc_basic_03();
		test_buddy_malloc_basic_04();

		test_buddy_free_coverage();

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

		test_buddy_reallocarray_01();
		test_buddy_reallocarray_02();
		test_buddy_reallocarray_03();

		test_buddy_embedded_not_enough_memory();
		test_buddy_embedded_null();
		test_buddy_embedded_01();
		test_buddy_embedded_malloc_01();
	}
	
	{
		test_buddy_tree_sizeof();
		test_buddy_tree_init();
		test_buddy_tree_valid();
		test_buddy_tree_order();
		test_buddy_tree_root();
		test_buddy_tree_depth();
		test_buddy_tree_left_child();
		test_buddy_tree_right_child();
		test_buddy_tree_parent();
		test_buddy_tree_sibling();
		test_buddy_tree_left_adjacent();
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
		test_buddy_tree_find_free_01();
		test_buddy_tree_find_free_02();
	}
}

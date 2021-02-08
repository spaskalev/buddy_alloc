/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"
#include "bat.h"
#include "bbm.h"

#define start_test printf("Running test: %s in %s:%d\n", __func__, __FILE__, __LINE__);

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

void test_bbm_init_null() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)+1];
    alignas(max_align_t) unsigned char data_buf[4096+1];
    {
		struct bbm *bbm = bbm_init(NULL, data_buf, 4096);
		assert (bbm == NULL);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, NULL, 4096);
		assert (bbm == NULL);
	}
}

void test_bbm_misalignment() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)+1];
    alignas(max_align_t) unsigned char data_buf[4096+1];
    {
		struct bbm *bbm = bbm_init(bbm_buf+1, data_buf, 4096);
		assert (bbm == NULL);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf+1, 4096);
		assert (bbm == NULL);
	}
}

void test_bbm_invalid_datasize() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
    {
		assert(bbm_sizeof(0) == 0);
		assert(bbm_sizeof(BBM_ALIGN-1) == 0);
		assert(bbm_sizeof(BBM_ALIGN+1) == 0);
	}
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf, 0);
		assert (bbm == NULL);
	}
}

void test_bbm_init() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
    {
		struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
		assert (bbm != NULL);
	}
}

void test_bbm_malloc_null() {
	start_test;
	assert(bbm_malloc(NULL, 1024) == NULL);
}

void test_bbm_malloc_zero() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 0) == NULL);
}

void test_bbm_malloc_larger() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 8192) == NULL);
}

void test_bbm_malloc_basic_01() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(1024)];
    alignas(max_align_t) unsigned char data_buf[1024];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 1024);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 1024) == NULL);
	bbm_free(bbm, data_buf);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 1024) == NULL);
}

void test_bbm_malloc_basic_02() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 2048) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == NULL);
	bbm_free(bbm, data_buf);
	bbm_free(bbm, data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 2048) == NULL);
}

void test_bbm_malloc_basic_03() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 1024) == data_buf+1024);
	assert(bbm_malloc(bbm, 1024) == NULL);
	bbm_free(bbm, data_buf+1024);
	bbm_free(bbm, data_buf+2048);
	bbm_free(bbm, data_buf);
	assert(bbm_malloc(bbm, 1024) == data_buf);
	assert(bbm_malloc(bbm, 2048) == data_buf+2048);
	assert(bbm_malloc(bbm, 1024) == data_buf+1024);
	assert(bbm_malloc(bbm, 1024) == NULL);
}

void test_bbm_malloc_basic_04() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf, 4096);
	assert (bbm != NULL);
	assert(bbm_malloc(bbm, 64) == data_buf);
	assert(bbm_malloc(bbm, 32) == data_buf+64);
}

void test_bbm_free_coverage() {
	start_test;
    alignas(max_align_t) unsigned char bbm_buf[bbm_sizeof(4096)];
    alignas(max_align_t) unsigned char data_buf[4096];
	struct bbm *bbm = bbm_init(bbm_buf, data_buf+1024, 1024);
	assert (bbm != NULL);
	bbm_free(NULL, NULL);
	bbm_free(NULL, data_buf+1024);
	bbm_free(bbm, NULL);
	bbm_free(bbm, data_buf);
	bbm_free(bbm, data_buf+2048);
}

void test_bbm_demo() {
	size_t arena_size = 65536;
	/* You need space for the metadata and for the arena */
	void *buddy_metadata = malloc(bbm_sizeof(arena_size));
	void *buddy_arena = malloc(arena_size);
	struct bbm *buddy = bbm_init(buddy_metadata, buddy_arena, arena_size);

	/* Allocate using the buddy allocator */
	void *data = bbm_malloc(buddy, 2048);
	/* Free using the buddy allocator */
	bbm_free(buddy, data);

	free(buddy_metadata);
	free(buddy_arena);
}

void test_bat_sizeof() {
	start_test;
	assert(bat_sizeof(0) == 0);
	assert(bat_sizeof(1) == 2);
	assert(bat_sizeof(2) == 2);
	assert(bat_sizeof(3) == 3);
	assert(bat_sizeof(4) == 4);
	assert(bat_sizeof(5) == 8);
	assert(bat_sizeof(6) == 14);
	assert(bat_sizeof(7) == 27);
	assert(bat_sizeof(8) == 53);
	assert(bat_sizeof(9) == 106);
	assert(bat_sizeof(10) == 210);
	assert(bat_sizeof(11) == 419);
	assert(bat_sizeof(12) == 837);
	assert(bat_sizeof(13) == 1673);
	assert(bat_sizeof(14) == 3345);
	assert(bat_sizeof(15) == 6689);
	assert(bat_sizeof(16) == 13377);
	assert(bat_sizeof(17) == 26753);
	assert(bat_sizeof(18) == 53506);
	assert(bat_sizeof(19) == 107011);
	assert(bat_sizeof(20) == 214021);
	assert(bat_sizeof(21) == 428041);
	assert(bat_sizeof(22) == 856081);
	assert(bat_sizeof(23) == 1712161);
	assert(bat_sizeof(24) == 3424321);
	assert(bat_sizeof(25) == 6848641);
	assert(bat_sizeof(26) == 13697281);
	assert(bat_sizeof(27) == 27394561);
	assert(bat_sizeof(28) == 54789121);
	assert(bat_sizeof(29) == 109578241);
	assert(bat_sizeof(30) == 219156481);
	assert(bat_sizeof(31) == 438312961);
	assert(bat_sizeof(32) == 876625921);
	assert(bat_sizeof(33) == 1753251841);
	assert(bat_sizeof(34) == 3506503682); /* 3 GB */
	assert(bat_sizeof(35) == 7013007363);
	assert(bat_sizeof(36) == 14026014725);
	assert(bat_sizeof(37) == 28052029449);
	assert(bat_sizeof(38) == 56104058897);
	assert(bat_sizeof(39) == 112208117793);
	assert(bat_sizeof(40) == 224416235585);
	assert(bat_sizeof(41) == 448832471169);
	assert(bat_sizeof(42) == 897664942337);
	assert(bat_sizeof(43) == 1795329884673);
	assert(bat_sizeof(44) == 3590659769345);
	assert(bat_sizeof(45) == 7181319538689);
	assert(bat_sizeof(46) == 14362639077377);
	assert(bat_sizeof(47) == 28725278154753);
	assert(bat_sizeof(48) == 57450556309505); /* 52 TB .. */
}

void test_bat_init() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	assert(bat_init(NULL, 8) == NULL);
	assert(bat_init(bat_buf, 0) == NULL);
	assert(bat_init(bat_buf, 8) != NULL);
}

void test_bat_valid() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 8);
	assert(bat_valid(NULL, 1) == 0);
	assert(bat_valid(t, 0) == 0);
	assert(bat_valid(t, 256) == 0);
	assert(bat_valid(t, 1) == 1);
	assert(bat_valid(t, 255) == 1);
}

void test_bat_order() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 8);
	assert(bat_order(NULL) == 0);
	assert(bat_order(t) == 8);
}

void test_bat_root() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 8);
	assert(bat_root(NULL) == 0);
	assert(bat_root(t) == 1);
}

void test_bat_depth() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 8);
	assert(bat_depth(t, 0) == 0);
	assert(bat_depth(t, 1) == 1);
}

void test_bat_left_child() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	pos = bat_left_child(t, pos);
	assert(bat_depth(t, pos) == 2);
	pos = bat_left_child(t, pos);
	assert(bat_valid(t, pos) == 0);
	pos = bat_left_child(t, pos);
	assert(bat_valid(t, pos) == 0);
}

void test_bat_right_child() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	pos = bat_right_child(t, pos);
	assert(bat_depth(t, pos) == 2);
	pos = bat_right_child(t, pos);
	assert(bat_valid(t, pos) == 0);
	pos = bat_right_child(t, pos);
	assert(bat_valid(t, pos) == 0);
}

void test_bat_parent() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_parent(t, pos) == 0);
	assert(bat_parent(t, 0) == 0);
	assert(bat_parent(t, bat_left_child(t, pos)) == pos);
	assert(bat_parent(t, bat_right_child(t, pos)) == pos);
}

void test_bat_sibling() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_sibling(t, pos) == 0);
	assert(bat_sibling(t, 0) == 0);
	assert(bat_sibling(t, bat_left_child(t, pos)) == bat_right_child(t, pos));
	assert(bat_sibling(t, bat_right_child(t, pos)) == bat_left_child(t, pos));
}

void test_bat_left_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_left_adjacent(t, 0) == 0);
	assert(bat_left_adjacent(t, pos) == 0);
	assert(bat_left_adjacent(t, bat_left_child(t, pos)) == 0);
	assert(bat_left_adjacent(t, bat_right_child(t, pos)) == bat_left_child(t, pos));
}

void test_bat_right_adjacent() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_right_adjacent(t, 0) == 0);
	assert(bat_right_adjacent(t, pos) == 0);
	assert(bat_right_adjacent(t, bat_right_child(t, pos)) == 0);
	assert(bat_right_adjacent(t, bat_left_child(t, pos)) == bat_right_child(t, pos));
}

void test_bat_index() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_index(t, 0) == 0);
	assert(bat_index(t, pos) == 0);
	assert(bat_index(t, bat_left_child(t, pos)) == 0);
	assert(bat_index(t, bat_right_child(t, pos)) == 1);
}

void test_bat_mark_status_release_01() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 1);

	assert(bat_status(t, 0) == 0);
	bat_mark(t, 0); /* coverage */
	bat_release(t, 0); /* coverage */

	bat_pos pos = bat_root(t);
	assert(bat_status(t, pos) == 0);
	bat_mark(t, pos);
	assert(bat_status(t, pos) == 1);
	bat_release(t, pos);
	assert(bat_status(t, pos) == 0);
}

void test_bat_mark_status_release_02() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	assert(bat_status(t, pos) == 0);
	bat_mark(t, pos);
	assert(bat_status(t, pos) == 2);
}

void test_bat_mark_status_release_03() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 3);
	bat_pos pos = bat_root(t);
	assert(bat_status(t, pos) == 0);
	bat_mark(t, pos);
	assert(bat_status(t, pos) == 3);
}

void test_bat_mark_status_release_04() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 4);
	bat_pos pos = bat_root(t);
	assert(bat_status(t, pos) == 0);
	bat_mark(t, pos);
	assert(bat_status(t, pos) == 4);
}

void test_bat_propagation_01() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 2);
	bat_pos pos = bat_root(t);
	bat_pos left = bat_left_child(t, pos);
	assert(bat_status(t, left) == 0);
	bat_mark(t, left);
	assert(bat_status(t, left) == 1);
	assert(bat_status(t, pos) == 1);
}

void test_bat_propagation_02() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 3);
	bat_pos pos = bat_root(t);
	bat_pos left = bat_left_child(t, bat_left_child(t, pos));
	bat_mark(t, left);
	assert(bat_status(t, left) == 1);
	assert(bat_status(t, pos) == 1);
}

void test_bat_find_free_01() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 3);
	bat_pos pos = bat_find_free(t, 0);
	assert(bat_valid(t, pos) == 0);
	pos = bat_find_free(t, 4);
	assert(bat_valid(t, pos) == 0);
	pos = bat_find_free(NULL, 1);
	assert(bat_valid(t, pos) == 0);
}

void test_bat_find_free_02() {
	start_test;
	alignas(max_align_t) unsigned char bat_buf[4096];
	struct bat *t = bat_init(bat_buf, 3);
	bat_pos pos = bat_find_free(t, 1);
	assert(bat_valid(t, pos) == 1);
	pos = bat_find_free(t, 2);
	assert(bat_valid(t, pos) == 1);

	bat_debug(t, bat_root(t));printf("\n");
	bat_mark(t, pos);
	bat_debug(t, bat_root(t));printf("\n");

	pos = bat_find_free(t, 2);
	assert(bat_valid(t, pos) == 1);

	bat_mark(t, pos);
	bat_debug(t, bat_root(t));printf("\n");

	pos = bat_find_free(t, 2);
	assert(bat_valid(t, pos) == 0);
}

int main() {
    {
		test_bitset_basic();
		test_bitset_range();
	}

	{
		test_bbm_init_null();
		test_bbm_misalignment();
		test_bbm_invalid_datasize();

		test_bbm_init();

		test_bbm_malloc_null();
		test_bbm_malloc_zero();
		test_bbm_malloc_larger();

		test_bbm_malloc_basic_01();
		test_bbm_malloc_basic_02();
		test_bbm_malloc_basic_03();
		test_bbm_malloc_basic_04();

		test_bbm_free_coverage();

		test_bbm_demo();
	}
	
	{
		test_bat_sizeof();
		test_bat_init();
		test_bat_valid();
		test_bat_order();
		test_bat_root();
		test_bat_depth();
		test_bat_left_child();
		test_bat_right_child();
		test_bat_parent();
		test_bat_sibling();
		test_bat_left_adjacent();
		test_bat_right_adjacent();
		test_bat_index();
		test_bat_mark_status_release_01();
		test_bat_mark_status_release_02();
		test_bat_mark_status_release_03();
		test_bat_mark_status_release_04();
		test_bat_propagation_01();
		test_bat_propagation_02();
		test_bat_find_free_01();
		test_bat_find_free_02();
	}
}

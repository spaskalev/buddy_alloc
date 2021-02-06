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
#include "bbm.h"
#include "bbt.h"

#define start_test printf("Running test: %s in %s:%d\n", __func__, __FILE__, __LINE__);

void test_bitset_basic() {
	start_test;
	unsigned char buf[4] = {0};
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

void test_bbt_sizeof_invalid_order() {
	start_test;
	assert(bbt_sizeof(0) == 0);
	assert(bbt_sizeof(sizeof(size_t) * CHAR_BIT) == 0);
}

void test_bbt_init_invalid_order() {
	start_test;
	alignas(max_align_t) unsigned char buf[1024];
	assert(bbt_init(buf, 0) == NULL);
	assert(bbt_init(buf, sizeof(size_t) * CHAR_BIT) == NULL);
	assert(bbt_order(NULL) == 0);
}

void test_bbt_basic() {
	start_test;

	assert(bbt_pos_valid(NULL, 1) == 0);

	size_t bbt_order = 3;
	alignas(max_align_t) unsigned char buf[bbt_sizeof(bbt_order)];
	struct bbt *bbt = bbt_init(buf, bbt_order);
	assert(bbt != NULL);
	assert(bbt_pos_valid(bbt, 256) == 0);
	bbt_pos pos = 0;
	for (size_t i = 0; i < 3; i++) {
		pos = bbt_left_pos_at_depth(bbt, i);
		assert (pos != 0);
		assert (bbt_pos_depth(bbt, pos) == i);
	}
	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(bbt_pos_index(bbt, pos) == 0);
	assert(bbt_pos_sibling(bbt, pos) == 0);
	assert(bbt_pos_parent(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_sibling(bbt, pos)));
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_right_adjacent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert((pos = bbt_pos_left_adjacent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 1);
	assert((pos = bbt_pos_parent(bbt, pos)));
	assert(bbt_pos_depth(bbt, pos) == 0);

	bbt_pos_set(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 1);
	bbt_pos_clear(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 0);
	bbt_pos_flip(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 1);
	bbt_pos_flip(bbt, pos);
	assert(bbt_pos_test(bbt, pos) == 0);

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert((pos = bbt_pos_left_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_left_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_left_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert((pos = bbt_pos_right_child(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(!bbt_pos_valid(bbt, bbt_pos_parent(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 3);
	assert(!bbt_pos_valid(bbt, bbt_pos_depth(bbt, pos)));

	pos = bbt_left_pos_at_depth(bbt, 0);
	assert(!bbt_pos_valid(bbt, bbt_pos_left_adjacent(bbt, pos)));
	assert(!bbt_pos_valid(bbt, bbt_pos_right_adjacent(bbt, pos)));

	pos = 0; /* invalid pos */
	assert(bbt_pos_left_child(bbt, pos) == 0);
	assert(bbt_pos_right_child(bbt, pos) == 0);
	assert(bbt_pos_left_adjacent(bbt, pos) == 0);
	assert(bbt_pos_right_adjacent(bbt, pos) == 0);
	assert(bbt_pos_sibling(bbt, pos) == 0);
	assert(bbt_pos_parent(bbt, pos) == 0);
	assert(bbt_pos_test(bbt, pos) == 0);
	assert(bbt_pos_depth(bbt, pos) == 0);
	assert(bbt_pos_index(bbt, pos) == 0);
	/* coverage for void functions */
	bbt_pos_set(bbt, pos);
	bbt_pos_clear(bbt, pos);
	bbt_pos_flip(bbt, pos);
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
	{
		struct bbm *bbm = bbm_init(bbm_buf, data_buf, 32);
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

int main() {
    {
		test_bitset_basic();
		test_bitset_range();
	}

	{
		test_bbt_sizeof_invalid_order();
		test_bbt_init_invalid_order();

		test_bbt_basic();
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
	}
}

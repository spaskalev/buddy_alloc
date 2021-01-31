/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A boolean perfect binary tree
 */
#include "bbt.h"
#include "bitset.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct bbt {
	unsigned char order;
	unsigned char bits[];
};

static size_t highest_bit_set(size_t value);
static size_t highest_bit_set(size_t value) {
	size_t pos = 0;
	while ( value >>= 1u ) {
		pos++;
	}
	return pos;
}

size_t bbt_sizeof(unsigned char order) {
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		/* return something wild to detect failure
		 * the init function will also check this
		 * and fail to initialize */
		return 0;
	}
	size_t result = sizeof(struct bbt);
	result += bitset_size(1u << order);
	return result;
}

struct bbt *bbt_init(unsigned char *at, unsigned char order) {
	if ((order == 0) || (order >= (sizeof(size_t) * CHAR_BIT))) {
		return NULL;
	}
	struct bbt *t = (struct bbt*) at;
	t->order = order;
	size_t elements_count = 1u << order;
	memset(t->bits, 0, bitset_size(elements_count));
	return t;
}

unsigned char bbt_order(struct bbt *t) {
	if (t == NULL) {
		return 0;
	}
	return t->order;
}

bbt_pos bbt_left_pos_at_depth(struct bbt *t, size_t depth) {
	if (depth >= t->order) {
		return 0;
	}
	bbt_pos pos = 1;
	while (depth && (pos = bbt_pos_left_child(t, pos))) {
		depth--;
	}
	return pos;
}

bbt_pos bbt_pos_left_child(struct bbt *t, bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (bbt_pos_depth(t, pos) + 1 == t->order) {
		return 0;
	}
	pos = 2 * pos;
	return pos;
}

bbt_pos bbt_pos_right_child(struct bbt *t, bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (bbt_pos_depth(t, pos) + 1 == t->order) {
		return 0;
	}
	pos = (2 * pos) + 1;
	return pos;
}

bbt_pos bbt_pos_left_adjacent(struct bbt *t, bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no adjacent nodes */
	}
	if (highest_bit_set(pos) != highest_bit_set(pos - 1)) {
		return 0;
	}
	pos -= 1;
	return pos;
}

bbt_pos bbt_pos_right_adjacent(struct bbt *t, bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no adjacent nodes */
	}
	if (highest_bit_set(pos) != highest_bit_set(pos + 1)) {
		return 0;
	}
	pos += 1;
	return pos;
}

bbt_pos bbt_pos_sibling(struct bbt *t, bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no sibling node */
	}
	/* Flip first bit to get the sibling pos */
	pos ^= 1u;
	return pos;
}

bbt_pos bbt_pos_parent(struct bbt *t, bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	size_t parent = pos / 2;
	if ((parent != pos) && parent != 0) {
		return parent;
	}
	return 0; /* root node has no parent node */
}

_Bool bbt_pos_valid(struct bbt *t, bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (t == NULL) {
		return 0;
	}
	if (pos == 0) {
		return 0;
	}
	if (pos >= 1u << t->order) {
		return 0;
	}
	return 1;
}

void bbt_pos_set(struct bbt *t, const bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return; /* invalid pos */
	}
	bitset_set(t->bits, pos);
}

void bbt_pos_clear(struct bbt *t, const bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return; /* invalid pos */
	}
	bitset_clear(t->bits, pos);
}

void bbt_pos_flip(struct bbt *t, const bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return; /* invalid pos */
	}
	bitset_flip(t->bits, pos);
}

_Bool bbt_pos_test(struct bbt *t, const bbt_pos pos) {
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	return bitset_test(t->bits, pos);
}

size_t bbt_pos_depth(struct bbt *t, const bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	return highest_bit_set(pos);
}

size_t bbt_pos_index(struct bbt *t, const bbt_pos pos) {
	(void)(t); /* to keep a unified and future-proof api */
	if (!bbt_pos_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	/* Clear out the highest bit, this gives us the index
	 * in a row of sibling nodes */
	size_t mask = 1u << highest_bit_set(pos);
	size_t result = pos & ~mask;
	return result;
}

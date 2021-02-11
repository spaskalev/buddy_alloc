/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A buddy allocation tree
 */
#include "buddy_alloc_tree.h"
#include "bitset.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct bat {
	uint8_t order;
	unsigned char bits[];
};

struct internal_position {
	size_t propagation_depth;
	size_t total_offset;
	size_t local_offset;
	size_t local_index;
	size_t bitset_location;
};

static size_t highest_bit_set(size_t value);
static size_t size_for_order(uint8_t order, uint8_t to);
static struct internal_position bat_internal_position(struct bat *t, bat_pos pos);
static void update_parent_chain(struct bat *t, bat_pos pos);
static bat_pos bat_find_free_internal(struct bat *t, bat_pos start,
	uint8_t target_depth, uint8_t target_status);

static size_t highest_bit_set(size_t value) {
	size_t pos = ((_Bool) value) & 1u;
	while ( value >>= 1u ) {
		pos++;
	}
	return pos;
}

static size_t size_for_order(uint8_t order, uint8_t to) {
	size_t result = 0;
	size_t multi = 1u;
	while (order != to) {
		result += highest_bit_set(order) * multi;
		order -= 1u;
		multi <<= 1u;
	}
	return result;
}

static struct internal_position bat_internal_position(struct bat *t, bat_pos pos) {
	struct internal_position p = {0};
	p.propagation_depth = t->order - bat_depth(t, pos) + 1;
	p.total_offset = size_for_order(t->order, p.propagation_depth);
	p.local_offset = highest_bit_set(p.propagation_depth);
	p.local_index = bat_index(t, pos);
	p.bitset_location = p.total_offset + (p.local_offset*p.local_index);
	return p;
}

size_t bat_sizeof(uint8_t order) {
	if (order == 0) {
		return 0;
	}
	size_t result = 0;
	result = size_for_order(order, 0);
	result /= 8; /* convert bits to bytes */
	result += 1; /* allow for bits not landing on byte boundary */
	/* add the structure header */
	result += sizeof(struct bat);
	return result;
}

struct bat *bat_init(unsigned char *at, uint8_t order) {
	if (! at) {
		return NULL;
	}
	size_t size = bat_sizeof(order);
	if (! size) {
		return NULL;
	}
	struct bat *t = (struct bat*) at;
	memset(at, 0, size);
	t->order = order;
	return t;
}

_Bool bat_valid(struct bat *t, bat_pos pos) {
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

uint8_t bat_order(struct bat *t) {
	if (t == NULL) {
		return 0;
	}
	return t->order;
}

bat_pos bat_root(struct bat *t) {
	if (t == NULL) {
		return 0;
	}
	return 1;
}

size_t bat_depth(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	return highest_bit_set(pos);
}

bat_pos bat_left_child(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (bat_depth(t, pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	return pos;
}

bat_pos bat_right_child(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (bat_depth(t, pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	pos += 1;
	return pos;
}

bat_pos bat_parent(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	size_t parent = pos / 2;
	if ((parent != pos) && parent != 0) {
		return parent;
	}
	return 0; /* root node has no parent node */
}

bat_pos bat_sibling(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no sibling node */
	}
	/* Flip first bit to get the sibling pos */
	pos ^= 1u;
	return pos;
}

bat_pos bat_left_adjacent(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
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

bat_pos bat_right_adjacent(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
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

size_t bat_index(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	/* Clear out the highest bit, this gives us the index
	 * in a row of sibling nodes */
	/* % ((sizeof(size_t) * CHAR_BIT)-1) ensures we don't shift into
	 * undefined behavior and stops clang from barking :)
	 * Hopefully clang also optimizes it away :) */
	size_t mask = 1u << (highest_bit_set(pos) - 1) % ((sizeof(size_t) * CHAR_BIT)-1);
	size_t result = pos & ~mask;
	return result;
}

uint8_t bat_status(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return 0;
	}

	struct internal_position p = bat_internal_position(t, pos);
	uint8_t result = 0;
	size_t shift = 0;
	while (p.local_offset) {
		result |= (uint8_t) (bitset_test(t->bits, p.bitset_location) << shift);
		p.bitset_location += 1;
		p.local_offset -= 1;
		shift += 1;
	}
	return result;
}

void bat_mark(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return;
	}

	/* TODO check for status before marking */
	struct internal_position p = bat_internal_position(t, pos);
	while (p.local_offset) {
		if (p.propagation_depth & 1u) {
			bitset_set(t->bits, p.bitset_location);
		} else {
			bitset_clear(t->bits, p.bitset_location);
		}
		/*bitset_set(t->bits, p.bitset_location);*/
		p.bitset_location += 1;
		p.local_offset -= 1;
		p.propagation_depth >>= 1u;
	}

	update_parent_chain(t, bat_parent(t, pos));
}

void bat_release(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return;
	}

	/* TODO check for status before releasing */

	struct internal_position p = bat_internal_position(t, pos);
	while (p.local_offset) {
		bitset_clear(t->bits, p.bitset_location);
		p.bitset_location += 1;
		p.local_offset -= 1;
	}

	update_parent_chain(t, bat_parent(t, pos));
}

static void update_parent_chain(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return;
	}

	uint8_t left = bat_status(t, bat_left_child(t, pos));
	uint8_t right = bat_status(t, bat_right_child(t, pos));
	uint8_t free = left <= right ? left : right;

	free += 1;

	struct internal_position p = bat_internal_position(t, pos);
	while (p.local_offset) {
		if (free & 1u) {
			bitset_set(t->bits, p.bitset_location);
		} else {
			bitset_clear(t->bits, p.bitset_location);
		}
		free >>= 1u;
		p.bitset_location += 1;
		p.local_offset -= 1;
	}

	update_parent_chain(t, bat_parent(t, pos));
}

bat_pos bat_find_free(struct bat *t, uint8_t depth) {
	if (depth == 0) {
		return 0;
	}
	if (t == NULL) {
		return 0;
	}
	if (depth > t->order) {
		return 0;
	}
	return bat_find_free_internal(t, bat_root(t), depth, depth-1);
}

static bat_pos bat_find_free_internal(struct bat *t, bat_pos start,
		uint8_t target_depth, uint8_t target_status) {
	size_t current_depth = bat_depth(t, start);
	size_t current_status = bat_status(t, start);
	if (current_depth == target_depth) {
		/* Target depth reached */
		if (current_status == 0) {
			/* Position is free, success */
			return start;
		}
		/* Position is not free, fail */
		return 0;
	}
	if (current_status <= target_status) {
		/* A free position is available in this part of the tree, traverse left-first */
		bat_pos pos = bat_find_free_internal(t, bat_left_child(t, start), target_depth, target_status-1);
		if (bat_valid(t, pos)) {
			/* Position found, success */
			return pos;
		}
		/* The free position should be in the right branch otherwise */
		return bat_find_free_internal(t, bat_right_child(t, start), target_depth, target_status-1);
	}
	/* A free position is not available in this part of the tree, fail */
	return 0;
}

void bat_debug(struct bat *t, bat_pos pos) {
	if (!bat_valid(t, pos)) {
		return;
	}
	for (size_t j = bat_depth(t, pos); j > 0; j--) {
		printf(" ");
	}
	printf("pos: %zu status: %zu\n", pos, (size_t) bat_status(t, pos));
	bat_debug(t, bat_left_child(t, pos));
	bat_debug(t, bat_right_child(t, pos));
}

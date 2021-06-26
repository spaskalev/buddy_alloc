/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A buddy allocation tree
 */
#include "buddy_alloc_tree.h"
#include "bitset.h"
#include "bits.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct buddy_tree {
	uint8_t order;
	unsigned char bits[];
};

struct internal_position {
	size_t max_value;
	size_t local_offset;
	size_t local_index;
	size_t bitset_location;
};

static size_t size_for_order(uint8_t order, uint8_t to);
static size_t buddy_tree_depth_internal(buddy_tree_pos pos);
static size_t buddy_tree_index_internal(buddy_tree_pos pos);
static buddy_tree_pos buddy_tree_leftmost_child_internal(size_t tree_order);
static buddy_tree_pos buddy_tree_rightmost_child_internal(size_t tree_order);
static struct internal_position buddy_tree_internal_position(size_t tree_order, buddy_tree_pos pos);
static void buddy_tree_grow(struct buddy_tree *t, uint8_t desired_order);
static void buddy_tree_shrink(struct buddy_tree *t, uint8_t desired_order);
static void update_parent_chain(struct buddy_tree *t, buddy_tree_pos pos);
static buddy_tree_pos buddy_tree_find_free_internal(struct buddy_tree *t, buddy_tree_pos start,
	uint8_t target_depth, uint8_t target_status);

static void write_to_internal_position(unsigned char *bitset, struct internal_position pos, size_t value);
static size_t read_from_internal_position(unsigned char *bitset, struct internal_position pos);

static size_t size_for_order(uint8_t order, uint8_t to) {
	size_t result = 0;
	size_t multi = 1u;
	while (order != to) {
		result += highest_bit_position(order) * multi;
		order -= 1u;
		multi <<= 1u;
	}
	return result;
}

static struct internal_position buddy_tree_internal_position(size_t tree_order, buddy_tree_pos pos) {
	struct internal_position p = {0};
	p.max_value = tree_order - buddy_tree_depth_internal(pos) + 1;
	size_t total_offset = size_for_order(tree_order, p.max_value);
	p.local_offset = highest_bit_position(p.max_value);
	p.local_index = buddy_tree_index_internal(pos);
	p.bitset_location = total_offset + (p.local_offset*p.local_index);
	return p;
}

size_t buddy_tree_sizeof(uint8_t order) {
	if (order == 0) {
		return 0;
	}
	size_t result = 0;
	result = size_for_order(order, 0);
	result /= 8; /* convert bits to bytes */
	result += 1; /* any bits not landing on byte boundary */
	result += sizeof(struct buddy_tree);
	return result;
}

struct buddy_tree *buddy_tree_init(unsigned char *at, uint8_t order) {
	if (! at) {
		return NULL;
	}
	size_t size = buddy_tree_sizeof(order);
	if (! size) {
		return NULL;
	}
	struct buddy_tree *t = (struct buddy_tree*) at;
	memset(at, 0, size);
	t->order = order;
	return t;
}

void buddy_tree_resize(struct buddy_tree *t, uint8_t desired_order) {
	if (t == NULL) {
		return;
	}
	if (t->order == desired_order) {
		return;
	} else if (t->order < desired_order) {
		buddy_tree_grow(t, desired_order);
	} else {
		buddy_tree_shrink(t, desired_order);
	}
}

static void buddy_tree_grow(struct buddy_tree *t, uint8_t desired_order) {
	while (desired_order > t->order) {
		/* Grow the tree a single order at a time */
		size_t current_order = t->order;
		size_t next_order = current_order + 1;

		/* Store the rightmost nodes. They will be needed both as
		a starting point for the clear/copy as well as for traversing
		upwards to the next upper-rightmost nodes. */
		buddy_tree_pos current_rightmost = buddy_tree_rightmost_child_internal(current_order);
		buddy_tree_pos next_rightmost = buddy_tree_rightmost_child_internal(next_order);

		/* Iterators for the current and next orders */
		buddy_tree_pos current_pos = current_rightmost;
		buddy_tree_pos next_pos = next_rightmost;

		/* Depth iterator for clear/copy */
		size_t new_depth = buddy_tree_depth_internal(next_pos);

		while (new_depth != 1) {
			/* Node count on the 'next' order */
			size_t next_node_count = 1u << (new_depth - 1u);

			for (size_t i = next_node_count; i > 0; i--) {
				struct internal_position next_internal =
					buddy_tree_internal_position(next_order, next_pos);
				if (buddy_tree_index_internal(next_pos) >= (next_node_count/2)) {
					/* Clear the position */
					write_to_internal_position(t->bits, next_internal, 0);
				} else {
					/* Fill-in with current value */
					struct internal_position current_internal =
						buddy_tree_internal_position(current_order, current_pos);
					size_t value = read_from_internal_position(t->bits, current_internal);
					write_to_internal_position(t->bits, next_internal, value);
					/* Advance the 'current' position */
					current_pos -= 1u; /* warn :) */
				}
				/* Advance the 'next' position */
				next_pos -= 1u; /* warn :) */
			}

			/* Decrease the depth */
			new_depth -= 1;
			/* Advance rightmost nodes */
			current_rightmost = current_rightmost / 2; /* warn :) */
			next_rightmost = next_rightmost / 2; /* warn :) */
			/* Reset iterators */
			current_pos = current_rightmost;
			next_pos = next_rightmost;
		}

		/* Advance the order and refrest the root */
		t->order = next_order;
		update_parent_chain(t, buddy_tree_root(t));
	}
}

static void buddy_tree_shrink(struct buddy_tree *t, uint8_t desired_order) {
	while (desired_order < t->order) {
		if (desired_order == 0) {
			return; /* Cannot shrink to zero */
		}
		if (buddy_tree_status(t, buddy_tree_right_child(t, buddy_tree_root(t))) != 0) {
			return; /* Refusing to shrink with right subtree still used! */
		}
		struct internal_position root_internal =
			buddy_tree_internal_position(t->order, buddy_tree_root(t));
		size_t root_value = read_from_internal_position(t->bits, root_internal);
		if (root_value == root_internal.max_value) {
			return; /* Refusing to shrink with the root fully-allocated! */
		}

		/* Shrink the tree a single order at a time */
		size_t current_order = t->order;
		size_t next_order = current_order - 1;

		/* Iterator for the next order */
		buddy_tree_pos next_pos = 1u; /* root position on next */

		/* Total number of nodes && last node absolute index */
		size_t last_current = (1u << t->order) - 1u;

		/* Do a single left-to-right pass over the current tree */
		for (buddy_tree_pos current_pos = 2u; current_pos <= last_current; current_pos++) {
			size_t current_depth = buddy_tree_depth_internal(current_pos);
			size_t current_node_count = 1u << (current_depth - 1u);

			if (buddy_tree_index_internal(current_pos) < (current_node_count/2)) {

				/* Copy from current to next */
				struct internal_position current_internal =
					buddy_tree_internal_position(current_order, current_pos);
				struct internal_position next_internal =
					buddy_tree_internal_position(next_order, next_pos);
				size_t value = read_from_internal_position(t->bits, current_internal);
				write_to_internal_position(t->bits, next_internal, value);

				/* Advance next */
				next_pos += 1;
			}
		}

		/* Advance the order */
		t->order = next_order;
	}	
}

_Bool buddy_tree_valid(struct buddy_tree *t, buddy_tree_pos pos) {
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

uint8_t buddy_tree_order(struct buddy_tree *t) {
	if (t == NULL) {
		return 0;
	}
	return t->order;
}

buddy_tree_pos buddy_tree_root(struct buddy_tree *t) {
	if (t == NULL) {
		return 0;
	}
	return 1;
}

buddy_tree_pos buddy_tree_leftmost_child(struct buddy_tree *t) {
	if (t == NULL) {
		return 0; /* invalid */
	}
	return buddy_tree_leftmost_child_internal(t->order);
}

static buddy_tree_pos buddy_tree_leftmost_child_internal(size_t tree_order) {
	buddy_tree_pos pos = 1u << (tree_order - 1u);
	return pos;
}

buddy_tree_pos buddy_tree_rightmost_child(struct buddy_tree *t) {
	if (t == NULL) {
		return 0; /* invalid */
	}
	return buddy_tree_rightmost_child_internal(t->order);
}

static buddy_tree_pos buddy_tree_rightmost_child_internal(size_t tree_order) {
	size_t up_to = tree_order - 1;
	buddy_tree_pos pos = 1u;
	while (up_to) {
		pos <<= 1u;
		pos |= 1u;
		up_to -= 1u;
	}
	return pos;
}

size_t buddy_tree_depth(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	return buddy_tree_depth_internal(pos);
}

static size_t buddy_tree_depth_internal(buddy_tree_pos pos) {
	return highest_bit_position(pos);
}

buddy_tree_pos buddy_tree_left_child(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (buddy_tree_depth(t, pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	return pos;
}

buddy_tree_pos buddy_tree_right_child(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (buddy_tree_depth(t, pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	pos += 1;
	return pos;
}

buddy_tree_pos buddy_tree_parent(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	size_t parent = pos / 2;
	if ((parent != pos) && parent != 0) {
		return parent;
	}
	return 0; /* root node has no parent node */
}

buddy_tree_pos buddy_tree_sibling(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no sibling node */
	}
	/* Flip first bit to get the sibling pos */
	pos ^= 1u;
	return pos;
}

buddy_tree_pos buddy_tree_left_adjacent(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no adjacent nodes */
	}
	if (highest_bit_position(pos) != highest_bit_position(pos - 1)) {
		return 0;
	}
	pos -= 1;
	return pos;
}

buddy_tree_pos buddy_tree_right_adjacent(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	if (pos == 1) {
		return 0; /* root node has no adjacent nodes */
	}
	if (highest_bit_position(pos) != highest_bit_position(pos + 1)) {
		return 0;
	}
	pos += 1;
	return pos;
}

size_t buddy_tree_index(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0; /* invalid pos */
	}
	return buddy_tree_index_internal(pos);
}

static size_t buddy_tree_index_internal(buddy_tree_pos pos) {
	/* Clear out the highest bit, this gives us the index
	 * in a row of sibling nodes */
	/* % ((sizeof(size_t) * CHAR_BIT)-1) ensures we don't shift into
	 * undefined behavior and stops clang from barking :)
	 * Hopefully clang also optimizes it away :) */
	size_t mask = 1u << (highest_bit_position(pos) - 1) % ((sizeof(size_t) * CHAR_BIT)-1);
	size_t result = pos & ~mask;
	return result;
}

static void write_to_internal_position(unsigned char *bitset, struct internal_position pos, size_t value) {
	while (pos.local_offset) {
		if (value & 1u) {
			bitset_set(bitset, pos.bitset_location);
		} else {
			bitset_clear(bitset, pos.bitset_location);
		}
		value >>= 1u;
		pos.bitset_location += 1;
		pos.local_offset -= 1;
	}
}

static size_t read_from_internal_position(unsigned char *bitset, struct internal_position pos) {
	size_t result = 0;
	size_t shift = 0;
	while (pos.local_offset) {
		result |= (size_t) (bitset_test(bitset, pos.bitset_location) << shift);
		pos.bitset_location += 1;
		pos.local_offset -= 1;
		shift += 1;
	}
	return result;
}

size_t buddy_tree_status(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return 0;
	}

	struct internal_position internal = buddy_tree_internal_position(t->order, pos);
	return read_from_internal_position(t->bits, internal);
}

void buddy_tree_mark(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return;
	}

	struct internal_position internal = buddy_tree_internal_position(t->order, pos);
	if (read_from_internal_position(t->bits, internal)) {
		/* Calling mark on a used position is likely a bug in caller */
		return;
	}

	/* Mark the node as used */
	write_to_internal_position(t->bits, internal, internal.max_value);

	/* Update the tree upwards */
	update_parent_chain(t, buddy_tree_parent(t, pos));
}

void buddy_tree_release(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return;
	}

	struct internal_position internal = buddy_tree_internal_position(t->order, pos);

	if (read_from_internal_position(t->bits, internal) != internal.max_value) {
		/* Calling release on an unused or a partially-used position is
		   likely a bug in caller */
		return;
	}

	/* Mark the node as unused */
	write_to_internal_position(t->bits, internal, 0);

	/* Update the tree upwards */
	update_parent_chain(t, buddy_tree_parent(t, pos));
}

static void update_parent_chain(struct buddy_tree *t, buddy_tree_pos pos) {
	while (buddy_tree_valid(t, pos)) {

		size_t left = buddy_tree_status(t, buddy_tree_left_child(t, pos));
		size_t right = buddy_tree_status(t, buddy_tree_right_child(t, pos));
		size_t free = 0;
		if (left || right) {
			free = (left <= right ? left : right) + 1;
		}

		size_t current = buddy_tree_status(t, pos);
		if (free == current) {
			break; /* short the parent chain update */
		}

		/* Update the node */
		write_to_internal_position(t->bits, buddy_tree_internal_position(t->order, pos), free);

		/* Advance upwards */
		pos = buddy_tree_parent(t, pos);
	}
}

buddy_tree_pos buddy_tree_find_free(struct buddy_tree *t, uint8_t depth) {
	if (depth == 0) {
		return 0;
	}
	if (t == NULL) {
		return 0;
	}
	if (depth > t->order) {
		return 0;
	}
	return buddy_tree_find_free_internal(t, buddy_tree_root(t), depth, depth-1);
}

static buddy_tree_pos buddy_tree_find_free_internal(struct buddy_tree *t, buddy_tree_pos start,
		uint8_t target_depth, uint8_t target_status) {
	size_t current_depth = buddy_tree_depth(t, start);
	size_t current_status = buddy_tree_status(t, start);
	while (1) {
		if (current_depth == target_depth) {
			if (current_status == 0) {
				return start; /* Position is free, success */
			}
			return 0; /* Position is not free, fail */
		}
		if (current_status <= target_status) { /* A free position is available down the tree */
			/* Advance criteria */
			target_status -= 1;
			current_depth += 1;
			/* Test the left branch */
			buddy_tree_pos left_pos = buddy_tree_left_child(t, start);
			current_status = buddy_tree_status(t, left_pos);
			if (current_status <= target_status) {
				start = left_pos;
				continue;
			}
			start = buddy_tree_right_child(t, start);
			current_status = buddy_tree_status(t, start);
		} else {
			/* No position available down the tree */
			return 0;
		}
	}
}

void buddy_tree_debug(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return;
	}
	for (size_t j = buddy_tree_depth(t, pos); j > 0; j--) {
		printf(" ");
	}
	printf("pos: %zu status: %zu\n", pos, buddy_tree_status(t, pos));
	buddy_tree_debug(t, buddy_tree_left_child(t, pos));
	buddy_tree_debug(t, buddy_tree_right_child(t, pos));
	fflush(stdout);
}

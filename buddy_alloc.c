/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include "bits.h"
#include "buddy_alloc.h"
#include "buddy_alloc_tree.h"

#include <stddef.h>
#include <stdint.h>

struct buddy {
	unsigned char *main;
	size_t memory_size;
	alignas(max_align_t) unsigned char buddy_tree[];
};

static size_t buddy_tree_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct buddy *buddy, size_t requested_size);
static size_t size_for_depth(struct buddy *buddy, size_t depth);

static struct buddy_tree *buddy_tree(struct buddy *buddy);

size_t buddy_sizeof(size_t memory_size) {
	if ((memory_size % BUDDY_ALIGN) != 0) {
		return 0; /* invalid */
	}
	if (memory_size == 0) {
		return 0; /* invalid */
	}
	size_t buddy_tree_order = buddy_tree_order_for_memory(memory_size);
	return sizeof(struct buddy) + buddy_tree_sizeof(buddy_tree_order);
}

struct buddy *buddy_init(unsigned char *at, unsigned char *main, size_t memory_size) {
	if ((at == NULL) || (main == NULL)) {
		return NULL;
	}
	size_t at_alignment = ((uintptr_t) at) % alignof(max_align_t);
	if (at_alignment != 0) {
		return NULL;
	}
	size_t main_alignment = ((uintptr_t) main) % alignof(max_align_t);
	if (main_alignment != 0) {
		return NULL;
	}
	size_t size = buddy_sizeof(memory_size);
	if (size == 0) {
		return NULL;
	}
	size_t buddy_tree_order = buddy_tree_order_for_memory(memory_size);

	/* TODO check for overlap between buddy metadata and main block */
	struct buddy *buddy = (struct buddy *) at;
	buddy->main = main;
	buddy->memory_size = memory_size;
	buddy_tree_init(buddy->buddy_tree, buddy_tree_order);

	/* Mask the virtual space if memory is not a power of two */
	size_t effective_memory_size = ceiling_power_of_two(memory_size);
	if (effective_memory_size != memory_size) {
		size_t delta = effective_memory_size - memory_size;
		size_t delta_count = delta / BUDDY_ALIGN;
		buddy_tree_pos pos = buddy_tree_root(buddy_tree(buddy));
		for(size_t i = 0; i < buddy_tree_order-1; i++) {
			pos = buddy_tree_right_child(buddy_tree(buddy), pos);
		}
		while (delta_count) {
			buddy_tree_mark(buddy_tree(buddy), pos);
			pos = buddy_tree_left_adjacent(buddy_tree(buddy), pos);
			delta_count--;
		}
	}
	return buddy;
}

static size_t buddy_tree_order_for_memory(size_t memory_size) {
	size_t blocks = memory_size / BUDDY_ALIGN;
	return highest_bit_position(ceiling_power_of_two(blocks));
}

void *buddy_malloc(struct buddy *buddy, size_t requested_size) {
	if (buddy == NULL) {
		return NULL;
	}
	if (requested_size == 0) {
		return NULL;
	}
	if (requested_size > buddy->memory_size) {
		return NULL;
	}

	size_t target_depth = depth_for_size(buddy, requested_size);
	buddy_tree_pos pos = buddy_tree_find_free(buddy_tree(buddy), target_depth);

	if (! buddy_tree_valid(buddy_tree(buddy), pos)) {
		return NULL; /* no slot found */
	}

	/* Allocate the slot */
	buddy_tree_mark(buddy_tree(buddy), pos);

	/* Find and return the actual memory address */
	size_t block_size = size_for_depth(buddy, target_depth);
	size_t addr = block_size * buddy_tree_index(buddy_tree(buddy), pos);
	return (buddy->main + addr);
}

void buddy_free(struct buddy *buddy, void *ptr) {
	if (buddy == NULL) {
		return;
	}
	if (ptr == NULL) {
		return;
	}
	unsigned char *dst = (unsigned char *)ptr;
	if ((dst < buddy->main) || (dst > (buddy->main + buddy->memory_size))) {
		return;
	}

	/* Find the deepest position tracking this address */
	ptrdiff_t offset = dst - buddy->main;
	size_t index = offset / BUDDY_ALIGN;

	buddy_tree_pos pos = buddy_tree_root(buddy_tree(buddy));
	while(buddy_tree_valid(buddy_tree(buddy), buddy_tree_left_child(buddy_tree(buddy), pos))) {
		pos = buddy_tree_left_child(buddy_tree(buddy), pos);
	}
	while (index > 0) {
		pos = buddy_tree_right_adjacent(buddy_tree(buddy), pos);
		index--;
	}

	/* Find the actual allocated position tracking this address */
	while ((! buddy_tree_status(buddy_tree(buddy), pos)) && buddy_tree_valid(buddy_tree(buddy), pos)) {
		pos = buddy_tree_parent(buddy_tree(buddy), pos);
	}

	/* Release the position */
	buddy_tree_release(buddy_tree(buddy), pos);
}

static size_t depth_for_size(struct buddy *buddy, size_t requested_size) {
	size_t depth = 1;
	size_t memory_size = buddy->memory_size;
	while ((memory_size / requested_size) >> 1u) {
		depth++;
		memory_size >>= 1u;
	}
	return depth;
}

static size_t size_for_depth(struct buddy *buddy, size_t depth) {
	size_t result = buddy->memory_size >> (depth-1);
	return result;
}

static struct buddy_tree *buddy_tree(struct buddy *buddy) {
	struct buddy_tree* buddy_tree = (struct buddy_tree*) buddy->buddy_tree;
	return buddy_tree;
}



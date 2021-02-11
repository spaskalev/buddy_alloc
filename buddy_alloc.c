/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include "buddy_alloc.h"
#include "buddy_alloc_tree.h"

#include <stddef.h>
#include <stdint.h>

struct bbm {
	unsigned char *main;
	size_t memory_size;
	alignas(max_align_t) unsigned char bat[];
};

static size_t bat_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct bbm *bbm, size_t requested_size);
static size_t size_for_depth(struct bbm *bbm, size_t depth);

static struct bat *bat(struct bbm *bbm);

size_t bbm_sizeof(size_t memory_size) {
	if ((memory_size % BBM_ALIGN) != 0) {
		return 0; /* invalid */
	}
	if (memory_size == 0) {
		return 0; /* invalid */
	}
	size_t bat_order = bat_order_for_memory(memory_size);
	return sizeof(struct bbm) + bat_sizeof(bat_order);
}

struct bbm *bbm_init(unsigned char *at, unsigned char *main, size_t memory_size) {
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
	size_t size = bbm_sizeof(memory_size);
	if (size == 0) {
		return NULL;
	}
	size_t bat_order = bat_order_for_memory(memory_size);

	/* TODO check for overlap between bbm metadata and main block */
	struct bbm *bbm = (struct bbm *) at;
	bbm->main = main;
	bbm->memory_size = memory_size;
	bat_init(bbm->bat, bat_order);
	return bbm;
}

static size_t bat_order_for_memory(size_t memory_size) {
	size_t blocks = memory_size / BBM_ALIGN;
	size_t bat_order = 1;
	while (blocks >>= 1u) {
		bat_order++;
	}
	return bat_order;
}

void *bbm_malloc(struct bbm *bbm, size_t requested_size) {
	if (bbm == NULL) {
		return NULL;
	}
	if (requested_size == 0) {
		return NULL;
	}
	if (requested_size > bbm->memory_size) {
		return NULL;
	}

	size_t target_depth = depth_for_size(bbm, requested_size);
	bat_pos pos = bat_find_free(bat(bbm), target_depth);

	if (! bat_valid(bat(bbm), pos)) {
		return NULL; /* no slot found */
	}

	/* Allocate the slot */
	bat_mark(bat(bbm), pos);

	/* Find and return the actual memory address */
	size_t block_size = size_for_depth(bbm, target_depth);
	size_t addr = block_size * bat_index(bat(bbm), pos);
	return (bbm->main + addr);
}

void bbm_free(struct bbm *bbm, void *ptr) {
	if (bbm == NULL) {
		return;
	}
	if (ptr == NULL) {
		return;
	}
	unsigned char *dst = (unsigned char *)ptr;
	if ((dst < bbm->main) || (dst > (bbm->main + bbm->memory_size))) {
		return;
	}

	/* Find the deepest position tracking this address */
	ptrdiff_t offset = dst - bbm->main;
	size_t index = offset / BBM_ALIGN;

	bat_pos pos = bat_root(bat(bbm));
	while(bat_valid(bat(bbm), bat_left_child(bat(bbm), pos))) {
		pos = bat_left_child(bat(bbm), pos);
	}
	while (index > 0) {
		pos = bat_right_adjacent(bat(bbm), pos);
		index--;
	}

	/* Find the actual allocated position tracking this address */
	while ((! bat_status(bat(bbm), pos)) && bat_valid(bat(bbm), pos)) {
		pos = bat_parent(bat(bbm), pos);
	}

	/* Release the position */
	bat_release(bat(bbm), pos);
}

static size_t depth_for_size(struct bbm *bbm, size_t requested_size) {
	size_t depth = 1;
	size_t memory_size = bbm->memory_size;
	while ((memory_size / requested_size) >> 1u) {
		depth++;
		memory_size >>= 1u;
	}
	return depth;
}

static size_t size_for_depth(struct bbm *bbm, size_t depth) {
	size_t result = bbm->memory_size >> (depth-1);
	return result;
}

static struct bat *bat(struct bbm *bbm) {
	struct bat* bat = (struct bat*) bbm->bat;
	return bat;
}



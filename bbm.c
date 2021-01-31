/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include "bbm.h"
#include "bbt.h"

#include <stddef.h>
#include <stdint.h>

struct bbm {
	unsigned char *main;
	size_t memory_size;
	alignas(max_align_t) unsigned char bbt_backing[];
};

static size_t bbt_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct bbm *bbm, size_t requested_size);
static size_t size_for_depth(struct bbm *bbm, size_t depth);

static struct bbt *bbm_free_full_bbt(struct bbm *bbm);
static struct bbt *bbm_partial_bbt(struct bbm *bbm);

static bbt_pos find_free_slot(struct bbm *bbm, bbt_pos pos, size_t target_depth);
static void allocate_slot(struct bbm *bbm, bbt_pos pos);
static void update_parent_chain(struct bbm *bbm, bbt_pos pos);

size_t bbm_sizeof(size_t memory_size) {
	if ((memory_size % BBM_ALIGN) != 0) {
		return 0; /* invalid */
	}
	if (memory_size == 0) {
		return 0; /* invalid */
	}
	size_t bbt_order = bbt_order_for_memory(memory_size);
	if (bbt_order < 2) {
		return 0;
	}
	size_t bbt_size = bbt_sizeof(bbt_order) + bbt_sizeof(bbt_order-1);
	return sizeof(struct bbm) + bbt_size;
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
	size_t bbt_order = bbt_order_for_memory(memory_size);

	/* TODO check for overlap between bbm metadata and main block */
	struct bbm *bbm = (struct bbm *) at;
	bbm->main = main;
	bbm->memory_size = memory_size;
	/*
	 * Initialize two boolean binary trees, one of the full required depth
	 * and another of a depth minus one.
	 *
	 * The first tree will track a FREE or FULL bit.
	 * The second tree will track a NON-PARTIAL or PARTIAL bit.
	 *
	 * As leaf nodes cannot be partial the second tree is smaller.
	 *
	 * The PARTIAL bit allows for defined traversal and ensures that
	 * a allocate operation can finish in a bounded ammount of checks.
	 */
	bbt_init(bbm->bbt_backing, bbt_order);
	bbt_init(bbm->bbt_backing + bbt_sizeof(bbt_order), bbt_order - 1);
	return bbm;
}

static size_t bbt_order_for_memory(size_t memory_size) {
	size_t blocks = memory_size / BBM_ALIGN;
	size_t bbt_order = 1;
	while (blocks >>= 1u) {
		bbt_order++;
	}
	return bbt_order;
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

	bbt_pos pos = find_free_slot(bbm, bbt_left_pos_at_depth(bbm_free_full_bbt(bbm), 0), target_depth);
	if (!bbt_pos_valid(bbm_free_full_bbt(bbm), pos)) {
		return NULL;
	}
	allocate_slot(bbm, pos);

	/* Find and return the actual memory address */
	size_t block_size = size_for_depth(bbm, target_depth);
	size_t addr = block_size * bbt_pos_index(bbm_free_full_bbt(bbm), pos);
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
	struct bbt* free_full = bbm_free_full_bbt(bbm);
	struct bbt* partial = bbm_partial_bbt(bbm);
	bbt_pos pos = bbt_left_pos_at_depth(free_full, bbt_order(free_full)-1);
	while (index > 0) {
		pos = bbt_pos_right_adjacent(free_full, pos);
		index--;
	}
	/* Find the actual allocated position tracking this address */
	while ((! bbt_pos_test(free_full, pos)) && bbt_pos_valid(free_full, pos)) {
		pos = bbt_pos_parent(free_full, pos);
	}
	/* Free it and update the parent chain */
	bbt_pos_clear(free_full, pos);
	bbt_pos_clear(partial, pos);
	update_parent_chain(bbm, bbt_pos_parent(free_full, pos));
}

static size_t depth_for_size(struct bbm *bbm, size_t requested_size) {
	size_t depth = 0;
	size_t memory_size = bbm->memory_size;
	while ((memory_size / requested_size) >> 1u) {
		depth++;
		memory_size >>= 1u;
	}
	return depth;
}

static size_t size_for_depth(struct bbm *bbm, size_t depth) {
	size_t result = bbm->memory_size >> depth;
	return result;
}

static struct bbt *bbm_free_full_bbt(struct bbm *bbm) {
	struct bbt* free_full_bbt = (struct bbt*) bbm->bbt_backing;
	return free_full_bbt;
}

static struct bbt *bbm_partial_bbt(struct bbm *bbm) {
	struct bbt* free_full_bbt = (struct bbt*) bbm->bbt_backing;
	unsigned char free_full_order = bbt_order(free_full_bbt);
	struct bbt* partial_bbt = (struct bbt*) (bbm->bbt_backing + bbt_sizeof(free_full_order));
	return partial_bbt;
}

static bbt_pos find_free_slot(struct bbm *bbm, bbt_pos pos, size_t target_depth) {
	/* Note that positions coincide on the trees */
	struct bbt* free_full = bbm_free_full_bbt(bbm);
	struct bbt* partial = bbm_partial_bbt(bbm);

	size_t current_depth = bbt_pos_depth(free_full, pos);
	if (current_depth == target_depth) {
		if (bbt_pos_test(partial, pos)) {
			/* this position is in partial use and cannot be used for a new allocation */
			return 0;
		}
		if (bbt_pos_test(free_full, pos)) {
			/* this position is fully-used and cannot be used for a new allocation */
			return 0;
		}
		/* this position is free */
		return pos;
	}
	/* else - current depth has not yet reached target depth */
	if (bbt_pos_test(free_full, pos)) {
		/* current positon is fully-used and cannot be used to descent */
		return 0;
	}
	/* else - current position is either free or partially used and can be used to descent */
	bbt_pos next = find_free_slot(bbm, bbt_pos_left_child(free_full, pos), target_depth);
	if (bbt_pos_valid(free_full, next)) {
		return next;
	}
	return find_free_slot(bbm, bbt_pos_right_child(free_full, pos), target_depth);
}

static void allocate_slot(struct bbm *bbm, bbt_pos pos) {
	struct bbt* free_full = bbm_free_full_bbt(bbm);
	struct bbt* partial = bbm_partial_bbt(bbm);
	/*
	 * The allocate_slot function will set the indicated
	 * position to 'full' and trigger a parent chain update.
	 */
	bbt_pos_set(free_full, pos);
	bbt_pos_clear(partial, pos);
	update_parent_chain(bbm, bbt_pos_parent(free_full, pos));
}

static void update_parent_chain(struct bbm *bbm, bbt_pos pos) {
	struct bbt* free_full = bbm_free_full_bbt(bbm);
	struct bbt* partial = bbm_partial_bbt(bbm);
	/*
	 * The following matrix shows the parent status function
	 * of its child nodes statuses
	 *
	 *         |   free  | partial |   full
	 * --------+---------+---------+---------+
	 *    free |   free  | partial | partial |
	 * --------+---------+---------+---------+
	 * partial | partial | partial | partial |
	 * --------+---------+---------+---------+
	 *    full | partial | partial |   full  |
	 * --------+---------+---------+---------+
	 */
	if (!bbt_pos_valid(free_full, pos)) {
		return;
	}
	if (bbt_pos_test(partial, bbt_pos_left_child(partial, pos)) ||
			bbt_pos_test(partial, bbt_pos_right_child(partial, pos))) {
		bbt_pos_clear(free_full, pos);
		bbt_pos_set(partial, pos);
	} else {
		/* neither child node is partial */
		size_t result = bbt_pos_test(free_full, bbt_pos_left_child(free_full, pos) << 1u);
		result |= bbt_pos_test(free_full, bbt_pos_right_child(free_full, pos));
		switch(result) {
			case 0:
			/* both are free */
				bbt_pos_clear(free_full, pos);
				bbt_pos_clear(partial, pos);
			case 3:
			/* both are full */
				bbt_pos_set(free_full, pos);
				bbt_pos_clear(partial, pos);
			default:
			/* one is free and one is full */
				bbt_pos_clear(free_full, pos);
				bbt_pos_set(partial, pos);
		}
	}
	/* Continue up the parent chain */
	update_parent_chain(bbm, bbt_pos_parent(free_full, pos));
}

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
#include <string.h>
#include <stdio.h>

struct buddy {
	size_t memory_size;
	size_t virtual_slots;
	union {
		unsigned char *main;
		ptrdiff_t main_offset;
	};
	_Bool relative_mode;
	unsigned char buddy_tree[];
};

struct buddy_embed_check {
	_Bool can_fit;
	size_t offset;
	size_t buddy_size;
};

static size_t buddy_tree_order_for_memory(size_t memory_size);
static size_t depth_for_size(struct buddy *buddy, size_t requested_size);
static size_t size_for_depth(struct buddy *buddy, size_t depth);
static void *address_for_position(struct buddy *buddy, buddy_tree_pos pos);
static buddy_tree_pos position_for_address(struct buddy *buddy, const unsigned char *addr);
static unsigned char *buddy_main(struct buddy *buddy);
static struct buddy_tree *buddy_tree(struct buddy *buddy);
static void buddy_toggle_virtual_slots(struct buddy *buddy, _Bool state);
static struct buddy *buddy_resize_standard(struct buddy *buddy, size_t new_memory_size);
static struct buddy *buddy_resize_embedded(struct buddy *buddy, size_t new_memory_size);
static _Bool buddy_is_free(struct buddy *buddy, size_t from);
static struct buddy_embed_check buddy_embed_offset(size_t memory_size);

size_t buddy_sizeof(size_t memory_size) {
	if (memory_size < BUDDY_ALIGN) {
		return 0; /* invalid */
	}
	size_t buddy_tree_order = buddy_tree_order_for_memory(memory_size);
	return sizeof(struct buddy) + buddy_tree_sizeof(buddy_tree_order);
}

struct buddy *buddy_init(unsigned char *at, unsigned char *main, size_t memory_size) {
	if ((at == NULL) || (main == NULL)) {
		return NULL;
	}
	size_t at_alignment = ((uintptr_t) at) % alignof(struct buddy);
	if (at_alignment != 0) {
		return NULL;
	}
	size_t main_alignment = ((uintptr_t) main) % alignof(size_t);
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
	buddy->relative_mode = 0;
	buddy_tree_init(buddy->buddy_tree, buddy_tree_order);
	buddy_toggle_virtual_slots(buddy, 1);
	return buddy;
}

struct buddy *buddy_embed(unsigned char *main, size_t memory_size) {
	struct buddy_embed_check result = buddy_embed_offset(memory_size);
	if (! result.can_fit) {
		return NULL;
	}

	struct buddy *buddy = buddy_init(main+result.offset, main, result.offset);
	if (! buddy) { /* regular initialization failed */
		return NULL;
	}

	buddy->relative_mode = 1;
	buddy->main_offset = (unsigned char *)buddy - main;
	return buddy;
}

struct buddy *buddy_resize(struct buddy *buddy, size_t new_memory_size) {
	if (new_memory_size == buddy->memory_size) {
		return buddy;
	}

	size_t current_effective_memory_size = ceiling_power_of_two(buddy->memory_size);
	size_t new_effective_memory_size = ceiling_power_of_two(new_memory_size);

	if (current_effective_memory_size == new_effective_memory_size) {
		/* As the tree is not resized we need to check if any allocated slots
		will be impacted by reducing the memory */
		if ((new_memory_size < buddy->memory_size) &&
				! buddy_is_free(buddy, new_memory_size)) {
			return NULL;
		}

		/* Release the virtual slots */
		buddy_toggle_virtual_slots(buddy, 0);

		/* Store the new memory size and reconstruct any virtual slots */
		buddy->memory_size = new_memory_size;
		buddy_toggle_virtual_slots(buddy, 1);

		return buddy;
	}

	if (buddy->relative_mode) {
		return buddy_resize_embedded(buddy, new_memory_size);
	} else {
		return buddy_resize_standard(buddy, new_memory_size);
	}
}

static struct buddy *buddy_resize_standard(struct buddy *buddy, size_t new_memory_size) {
	/* Release the virtual slots */
	buddy_toggle_virtual_slots(buddy, 0);

	/* Calculate new tree size and adjust it */
	size_t old_buddy_tree_order = buddy_tree_order_for_memory(new_memory_size);
	size_t new_buddy_tree_order = buddy_tree_order_for_memory(new_memory_size);
	/* Upsizing the buddy tree can always go through (vs downsizing) */
	buddy_tree_resize(buddy_tree(buddy), new_buddy_tree_order);
	if (buddy_tree_order(buddy_tree(buddy)) != new_buddy_tree_order) {
		/* Resizing failed, restore the tree and virtual blocks */
		buddy_tree_resize(buddy_tree(buddy), old_buddy_tree_order);
		buddy_toggle_virtual_slots(buddy, 1);
		return NULL;
	}

	/* Store the new memory size and reconstruct any virtual slots */
	buddy->memory_size = new_memory_size;
	buddy_toggle_virtual_slots(buddy, 1);

	/* Resize successful */
	return buddy;
}

static struct buddy *buddy_resize_embedded(struct buddy *buddy, size_t new_memory_size) {
	/* Ensure that the embedded allocator can fit */
	struct buddy_embed_check result = buddy_embed_offset(new_memory_size);
	if (! result.can_fit) {
		return NULL;
	}

	/* Resize the allocator in the normal way */
	struct buddy *resized = buddy_resize_standard(buddy, result.offset);
	if (resized == NULL) {
		return NULL;
	}

	/* Get the absolute main address. The relative will be invalid after relocation. */
	unsigned char *main = buddy_main(buddy);

	/* Relocate the allocator */
	unsigned char *buddy_destination = buddy_main(buddy) + result.offset;
	memmove(buddy_destination, resized, result.buddy_size);

	/* Update the main offset in the allocator */
	struct buddy *relocated = (struct buddy *) buddy_destination;
	relocated->main_offset = buddy_destination - main;

	return relocated;
}

_Bool buddy_can_shrink(struct buddy *buddy) {
	if (buddy == NULL) {
		return 0;
	}
	buddy_toggle_virtual_slots(buddy, 0);
	_Bool result = buddy_tree_can_shrink(buddy_tree(buddy));
	buddy_toggle_virtual_slots(buddy, 1);
	return result;
}

size_t buddy_arena_size(struct buddy *buddy) {
	if (buddy == NULL) {
		return 0;
	}
	return buddy->memory_size;
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
	return address_for_position(buddy, pos);
}

void *buddy_calloc(struct buddy *buddy, size_t members_count, size_t member_size) {
	if (members_count == 0) {
		return NULL;
	}
	if (member_size == 0) {
		return NULL;
	}
	/* Check for overflow */
	if ((members_count * member_size)/members_count != member_size) {
		return NULL;
	}
	size_t total_size = members_count * member_size;
	void *result = buddy_malloc(buddy, total_size);
	if (result) {
		memset(result, 0, total_size);
	}
	return result;
}

void *buddy_realloc(struct buddy *buddy, void *ptr, size_t requested_size) {
	/*
	 * realloc is a joke:
	 * - NULL ptr degrades into malloc
	 * - Zero size degrades into free
	 * - Same size as previous malloc/calloc/realloc is a no-op or a rellocation
	 * - Smaller size than previous *alloc decrease the allocated size with an optional rellocation
	 * - If the new allocation cannot be satisfied NULL is returned BUT the slot
	 * - Larger size than previous *alloc increase tha allocated size with an optional rellocation
	 */
	if (ptr == NULL) {
		return buddy_malloc(buddy, requested_size);
	}
	if (requested_size == 0) {
		buddy_free(buddy, ptr);
		return NULL;
	}
	if (requested_size > buddy->memory_size) {
		return NULL;
	}

	/* Find the position tracking this address */
	buddy_tree_pos origin = position_for_address(buddy, ptr);
	size_t current_depth = buddy_tree_depth(origin);
	size_t target_depth = depth_for_size(buddy, requested_size);

	/* If the new size fits in the same slot do nothing */
	if (current_depth == target_depth) {
		return ptr;
	}

	/* Release the position and perform a search */
	buddy_tree_release(buddy_tree(buddy), origin);
	buddy_tree_pos new_pos = buddy_tree_find_free(buddy_tree(buddy), target_depth);

	if (buddy_tree_valid(buddy_tree(buddy), new_pos)) {
		/* Copy the content */
		void *source = address_for_position(buddy, origin);
		void *destination = address_for_position(buddy, new_pos);
		memmove(destination, source, size_for_depth(buddy,
			current_depth < target_depth ? current_depth : target_depth));
		/* Allocate and return */
		buddy_tree_mark(buddy_tree(buddy), new_pos);
		return destination;
	}

	/* allocation failure, restore mark and return null */
	buddy_tree_mark(buddy_tree(buddy), origin);
	return NULL;
}

void *buddy_reallocarray(struct buddy *buddy, void *ptr,
		size_t members_count, size_t member_size) {
	if (members_count == 0 || member_size == 0) {
		return buddy_realloc(buddy, ptr, 0);
	}
	/* Check for overflow */
	if ((members_count * member_size)/members_count != member_size) {
		return NULL;
	}
	return buddy_realloc(buddy, ptr, members_count * member_size);
}

void buddy_free(struct buddy *buddy, void *ptr) {
	if (buddy == NULL) {
		return;
	}
	if (ptr == NULL) {
		return;
	}
	unsigned char *dst = (unsigned char *)ptr;
	unsigned char *main = buddy_main(buddy);
	if ((dst < main) || (dst > (main + buddy->memory_size))) {
		return;
	}

	/* Find the position tracking this address */
	buddy_tree_pos pos = position_for_address(buddy, dst);

	/* Release the position */
	buddy_tree_release(buddy_tree(buddy), pos);
}

static size_t depth_for_size(struct buddy *buddy, size_t requested_size) {
	if (requested_size < BUDDY_ALIGN) {
		requested_size = BUDDY_ALIGN;
	}
	size_t depth = 1;
	size_t effective_memory_size = ceiling_power_of_two(buddy->memory_size);
	while ((effective_memory_size / requested_size) >> 1u) {
		depth++;
		effective_memory_size >>= 1u;
	}
	return depth;
}

static size_t size_for_depth(struct buddy *buddy, size_t depth) {
	size_t result = ceiling_power_of_two(buddy->memory_size) >> (depth-1);
	return result;
}

static struct buddy_tree *buddy_tree(struct buddy *buddy) {
	return (struct buddy_tree*) buddy->buddy_tree;
}

static void *address_for_position(struct buddy *buddy, buddy_tree_pos pos) {
	size_t block_size = size_for_depth(buddy, buddy_tree_depth(pos));
	size_t addr = block_size * buddy_tree_index(buddy_tree(buddy), pos);
	return buddy_main(buddy) + addr;
}

static buddy_tree_pos position_for_address(struct buddy *buddy, const unsigned char *addr) {
	/* Find the deepest position tracking this address */
	unsigned char *main = buddy_main(buddy);
	ptrdiff_t offset = addr - main;
	if (offset % BUDDY_ALIGN) {
		return 0; /* invalid alignment */
	}
	size_t index = offset / BUDDY_ALIGN;

	buddy_tree_pos pos = buddy_tree_leftmost_child(buddy_tree(buddy));
	while (index > 0) {
		pos = buddy_tree_right_adjacent(buddy_tree(buddy), pos);
		index--;
	}

	/* Find the actual allocated position tracking this address */
	while ((! buddy_tree_status(buddy_tree(buddy), pos)) && buddy_tree_valid(buddy_tree(buddy), pos)) {
		pos = buddy_tree_parent(buddy_tree(buddy), pos);
	}

	return pos;
}

static unsigned char *buddy_main(struct buddy *buddy) {
	if (buddy->relative_mode) {
		return (unsigned char *)buddy - buddy->main_offset;
	}
	return buddy->main;
}

static void buddy_toggle_virtual_slots(struct buddy *buddy, _Bool state) {
	size_t memory_size = buddy->memory_size;
	/* Mask/unmask the virtual space if memory is not a power of two */
	size_t effective_memory_size = ceiling_power_of_two(memory_size);
	if (effective_memory_size != memory_size) {
		size_t delta = effective_memory_size - memory_size;
		size_t delta_count = delta / BUDDY_ALIGN;
		if (delta % BUDDY_ALIGN) {
			delta_count += 1;
		}

		/* Update the virtual slot count */
		buddy->virtual_slots = state ? delta_count : 0;

		buddy_tree_pos pos = buddy_tree_rightmost_child(buddy_tree(buddy));
		while (delta_count) {
			if (state) {
				buddy_tree_mark(buddy_tree(buddy), pos);
			} else {
				buddy_tree_release(buddy_tree(buddy), pos);
			}
			pos = buddy_tree_left_adjacent(buddy_tree(buddy), pos);
			delta_count--;
		}
	} else {
		/* Update the virtual slot count */
		buddy->virtual_slots = 0;
	}
}

/* Internal function that checks if there are any allocations
 after the indicated relative memory index. Used to check if
 the arena can be downsized. */
static _Bool buddy_is_free(struct buddy *buddy, size_t from) {
	size_t delta = buddy->memory_size - from;
	size_t delta_count = delta / BUDDY_ALIGN;

	buddy_tree_pos pos = buddy_tree_rightmost_child(buddy_tree(buddy));

	/* Account for virtual slots */
	if (buddy->virtual_slots) {
		size_t virtual_slots = buddy->virtual_slots;
		while (virtual_slots) {
			pos = buddy_tree_left_adjacent(buddy_tree(buddy), pos);
			virtual_slots--;
		}
	}

	/* Check for use */
	while (delta_count) {
		if (! buddy_tree_is_free(buddy_tree(buddy), pos)) {
			return 0;
		}
		pos = buddy_tree_left_adjacent(buddy_tree(buddy), pos);
		delta_count--;
	}

	/* Blocks are free */
	return 1;
}

static struct buddy_embed_check buddy_embed_offset(size_t memory_size) {
	struct buddy_embed_check result = {0};
	result.can_fit = 1;

	size_t buddy_size = buddy_sizeof(memory_size);
	if (buddy_size >= memory_size) {
		result.can_fit = 0;
	}

	size_t offset = memory_size - buddy_size;
	if (offset % alignof(struct buddy) != 0) {
		buddy_size += offset % alignof(struct buddy);
		if (buddy_size >= memory_size) {
			result.can_fit = 0;
		}
		offset = memory_size - buddy_size;
	}

	if (result.can_fit) {
		result.offset = offset;
		result.buddy_size = buddy_size;
	}
	return result;
}

void buddy_debug(struct buddy *buddy) {
	printf("buddy allocator at: %p arena at: %p\n", (void *)buddy, (void *)buddy_main(buddy));
	printf("memory size: %zu\n", buddy->memory_size);
	printf("mode: ");printf(buddy->relative_mode ? "embedded" : "standard");printf("\n");
	printf("virtual slots: %zu\n", buddy->virtual_slots);
	printf("allocator tree follows:\n");
	buddy_tree_debug(buddy_tree(buddy), buddy_tree_root());
}

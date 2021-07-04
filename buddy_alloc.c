/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include "bits.h"
#include "buddy_alloc.h"

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
static buddy_tree_pos deepest_position_for_offset(struct buddy *buddy, size_t offset);

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
	 * - If the new allocation cannot be satisfied NULL is returned BUT the slot is preserved
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

	if (! buddy_tree_valid(buddy_tree(buddy), new_pos)) {
        /* allocation failure, restore mark and return null */
        buddy_tree_mark(buddy_tree(buddy), origin);
        return NULL;
	}

    /* Copy the content */
    void *source = address_for_position(buddy, origin);
    void *destination = address_for_position(buddy, new_pos);
    memmove(destination, source, size_for_depth(buddy,
        current_depth < target_depth ? current_depth : target_depth));
    /* Allocate and return */
    buddy_tree_mark(buddy_tree(buddy), new_pos);
    return destination;
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
	size_t addr = block_size * buddy_tree_index(pos);
	return buddy_main(buddy) + addr;
}

static buddy_tree_pos deepest_position_for_offset(struct buddy *buddy, size_t offset) {
    size_t index = offset / BUDDY_ALIGN;
    buddy_tree_pos pos = buddy_tree_leftmost_child(buddy_tree(buddy));
    while (index > 0) {
        pos = buddy_tree_right_adjacent(pos);
        index--;
    }
    return pos;
}

static buddy_tree_pos position_for_address(struct buddy *buddy, const unsigned char *addr) {
	/* Find the deepest position tracking this address */
	unsigned char *main = buddy_main(buddy);
	ptrdiff_t offset = addr - main;
	if (offset % BUDDY_ALIGN) {
		return 0; /* invalid alignment */
	}
	buddy_tree_pos pos = deepest_position_for_offset(buddy, offset);

	/* Find the actual allocated position tracking this address */
	while ((! buddy_tree_status(buddy_tree(buddy), pos)) && buddy_tree_valid(buddy_tree(buddy), pos)) {
		pos = buddy_tree_parent(pos);
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
	if (effective_memory_size == memory_size) {
        /* Update the virtual slot count */
        buddy->virtual_slots = 0;
        return;
    }

    /* Get the area that we need to mask and pad it to alignment */
    size_t delta = effective_memory_size - memory_size;
    if (delta < BUDDY_ALIGN) {
        delta = BUDDY_ALIGN;
    }
    if (delta % BUDDY_ALIGN) {
        delta += BUDDY_ALIGN - (delta % BUDDY_ALIGN);
    }

    /* Update the virtual slot count */
    buddy->virtual_slots = state ? (delta / BUDDY_ALIGN) : 0;

    /* Determine whether to mark or release */
    void (*toggle)(struct buddy_tree *, buddy_tree_pos) =
        state ? &buddy_tree_mark : &buddy_tree_release;

    struct buddy_tree *tree = buddy_tree(buddy);
    buddy_tree_pos pos = buddy_tree_right_child(tree, buddy_tree_root());
    while (delta) {
        size_t current_pos_size = size_for_depth(buddy, buddy_tree_depth(pos));
        if (delta == current_pos_size) {
            // toggle current pos
            (*toggle)(tree, pos);
            break;
        }
        if (delta == (current_pos_size / 2)) {
            // toggle right child
            (*toggle)(tree, buddy_tree_right_child(tree, pos));
            break;
        }
        if (delta > current_pos_size / 2) {
            // toggle right child
            (*toggle)(tree, buddy_tree_right_child(tree, pos));
            // reduce delta
            delta -= current_pos_size / 2;
            // re-run for left child
            pos = buddy_tree_left_child(tree, pos);
            continue;
        }
        if (delta < current_pos_size / 2) {
            // re-run for right child
            pos = buddy_tree_right_child(tree, pos);
            continue;
        }
    }
}

/* Internal function that checks if there are any allocations
 after the indicated relative memory index. Used to check if
 the arena can be downsized. */
static _Bool buddy_is_free(struct buddy *buddy, size_t from) {
    /* Adjust from for alignment */
    if (from % BUDDY_ALIGN) {
        from -= (from % BUDDY_ALIGN);
    }

    size_t effective_memory_size = ceiling_power_of_two(buddy->memory_size);
    size_t to = effective_memory_size - (buddy->virtual_slots * BUDDY_ALIGN);

    struct buddy_tree *t = buddy_tree(buddy);

    struct buddy_tree_interval query_range = {0};
    query_range.from = deepest_position_for_offset(buddy, from);
    query_range.to = deepest_position_for_offset(buddy, to);

    buddy_tree_pos pos = deepest_position_for_offset(buddy, from);
    while(pos < query_range.to) {
        struct buddy_tree_interval current_test_range = {0};
        struct buddy_tree_interval parent_test_range = buddy_tree_interval(t, buddy_tree_parent(pos));
        while(buddy_tree_interval_contains(query_range, parent_test_range)) {
            pos = buddy_tree_parent(pos);
            current_test_range = parent_test_range;
            parent_test_range = buddy_tree_interval(t, buddy_tree_parent(pos));
        }
        /* pos is now tracking an overlapping segment */
        if (! buddy_tree_is_free(t, pos)) {
            return 0;
        }
        /* Advance check */
        pos = buddy_tree_right_adjacent(current_test_range.to);
    }
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

/*
 buddy alloc tree (pending merge)
*/


/*
 * A buddy allocation tree
 */
#include "bitset.h"
#include "bits.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct buddy_tree {
	uint8_t order;
	size_t upper_pos_bound;
	unsigned char bits[];
};

struct internal_position {
	size_t max_value;
	size_t local_offset;
	size_t local_index;
	size_t bitset_location;
};

static size_t size_for_order(uint8_t order, uint8_t to);
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
	p.max_value = tree_order - buddy_tree_depth(pos) + 1;
	size_t total_offset = size_for_order(tree_order, p.max_value);
	p.local_offset = highest_bit_position(p.max_value);
	p.local_index = buddy_tree_index_internal(pos);
	p.bitset_location = total_offset + (p.local_offset*p.local_index);
	return p;
}

size_t buddy_tree_sizeof(uint8_t order) {
	size_t result = 0;
	result = size_for_order(order, 0);
	result /= 8; /* convert bits to bytes */
	result += 1; /* any bits not landing on byte boundary */
	result += sizeof(struct buddy_tree);
	return result;
}

struct buddy_tree *buddy_tree_init(unsigned char *at, uint8_t order) {
	size_t size = buddy_tree_sizeof(order);
	struct buddy_tree *t = (struct buddy_tree*) at;
	memset(at, 0, size);
	t->order = order;
	t->upper_pos_bound = 1u << t->order;
	return t;
}

void buddy_tree_resize(struct buddy_tree *t, uint8_t desired_order) {
	if (t->order == desired_order) {
		return;
	}
	if (t->order < desired_order) {
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
		size_t new_depth = buddy_tree_depth(next_pos);

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
		t->upper_pos_bound = 1u << t->order;
		update_parent_chain(t, buddy_tree_root());
	}
}

static void buddy_tree_shrink(struct buddy_tree *t, uint8_t desired_order) {
	while (desired_order < t->order) {
		if (!buddy_tree_can_shrink(t)) {
			return;
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
			size_t current_depth = buddy_tree_depth(current_pos);
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
		t->upper_pos_bound = 1u << t->order;
	}	
}

_Bool buddy_tree_valid(struct buddy_tree *t, buddy_tree_pos pos) {
	return pos && (pos < t->upper_pos_bound);
}

uint8_t buddy_tree_order(struct buddy_tree *t) {
	return t->order;
}

buddy_tree_pos buddy_tree_root(void) {
	return 1;
}

buddy_tree_pos buddy_tree_leftmost_child(struct buddy_tree *t) {
	return buddy_tree_leftmost_child_internal(t->order);
}

static buddy_tree_pos buddy_tree_leftmost_child_internal(size_t tree_order) {
	return 1u << (tree_order - 1u);
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

size_t buddy_tree_depth(buddy_tree_pos pos) {
	return highest_bit_position(pos);
}

buddy_tree_pos buddy_tree_left_child(struct buddy_tree *t, buddy_tree_pos pos) {
	if (buddy_tree_depth(pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	return pos;
}

buddy_tree_pos buddy_tree_right_child(struct buddy_tree *t, buddy_tree_pos pos) {
	if (buddy_tree_depth(pos) == t->order) {
		return 0;
	}
	pos = 2 * pos;
	pos += 1;
	return pos;
}

buddy_tree_pos buddy_tree_parent(buddy_tree_pos pos) {
	size_t parent = pos / 2;
	if ((parent != pos) && parent != 0) {
		return parent;
	}
	return 0; /* root node has no parent node */
}

buddy_tree_pos buddy_tree_right_adjacent(buddy_tree_pos pos) {
	if (highest_bit_position(pos) != highest_bit_position(pos + 1)) {
		return 0;
	}
	pos += 1;
	return pos;
}

size_t buddy_tree_index(buddy_tree_pos pos) {
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

struct buddy_tree_interval buddy_tree_interval(struct buddy_tree *t, buddy_tree_pos pos) {
	struct buddy_tree_interval result = {0};
	result.from = pos;
	result.to = pos;
	size_t depth = buddy_tree_depth(pos);
	while (depth != t->order) {
		result.from = buddy_tree_left_child(t, result.from);
		result.to = buddy_tree_right_child(t, result.to);
		depth += 1;
	}
	return result;
}

_Bool buddy_tree_interval_contains(struct buddy_tree_interval outer,
		struct buddy_tree_interval inner) {
	return (inner.from >= outer.from)
		&& (inner.from <= outer.to)
		&& (inner.to >= outer.from)
		&& (inner.to <= outer.to);
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

	/*
	 * Calling mark on a used position is likely a bug in caller.
	 * Caller should handle such check if needed.
	 */

	/* Mark the node as used */
	write_to_internal_position(t->bits, internal, internal.max_value);

	/* Update the tree upwards */
	update_parent_chain(t, buddy_tree_parent(pos));
}

void buddy_tree_release(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return;
	}

	struct internal_position internal = buddy_tree_internal_position(t->order, pos);

	/*
	 * Calling release on an unused or a partially-used position is
	 * likely a bug in caller. Caller should handle such check if needed.
	 */

	/* Mark the node as unused */
	write_to_internal_position(t->bits, internal, 0);

	/* Update the tree upwards */
	update_parent_chain(t, buddy_tree_parent(pos));
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
		pos = buddy_tree_parent(pos);
	}
}

buddy_tree_pos buddy_tree_find_free(struct buddy_tree *t, uint8_t depth) {
	return buddy_tree_find_free_internal(t, buddy_tree_root(), depth, depth-1);
}

static buddy_tree_pos buddy_tree_find_free_internal(struct buddy_tree *t, buddy_tree_pos start,
		uint8_t target_depth, uint8_t target_status) {
	size_t current_depth = buddy_tree_depth(start);
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

_Bool buddy_tree_is_free(struct buddy_tree *t, buddy_tree_pos pos) {
	if (buddy_tree_status(t, pos)) {
		return 0;
	}
	pos = buddy_tree_parent(pos);
	while(buddy_tree_valid(t, pos)) {
		struct internal_position internal = buddy_tree_internal_position(t->order, pos);
		size_t value = read_from_internal_position(t->bits, internal);
		if (value) {
			return value != internal.max_value;
		}
		pos = buddy_tree_parent(pos);
	}
	return 1;
}

_Bool buddy_tree_can_shrink(struct buddy_tree *t) {
	if (buddy_tree_status(t, buddy_tree_right_child(t, buddy_tree_root())) != 0) {
		return 0; /* Refusing to shrink with right subtree still used! */
	}
	struct internal_position root_internal =
		buddy_tree_internal_position(t->order, buddy_tree_root());
	size_t root_value = read_from_internal_position(t->bits, root_internal);
	if (root_value == root_internal.max_value) {
		return 0; /* Refusing to shrink with the root fully-allocated! */
	}
	return 1;
}

void buddy_tree_debug(struct buddy_tree *t, buddy_tree_pos pos) {
	if (!buddy_tree_valid(t, pos)) {
		return;
	}
	for (size_t j = buddy_tree_depth(pos); j > 0; j--) {
		printf(" ");
	}
	printf("pos: %zu status: %zu\n", pos, buddy_tree_status(t, pos));
	buddy_tree_debug(t, buddy_tree_left_child(t, pos));
	buddy_tree_debug(t, buddy_tree_right_child(t, pos));
	fflush(stdout);
}

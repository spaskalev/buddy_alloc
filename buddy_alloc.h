/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 *
 * To include and use it in your project do the following
 * 1. Add buddy_alloc.h (this file) to your include directory
 * 2. Include the header in places where you need to use the allocator
 * 3. In one of your source files #define BUDDY_ALLOC_IMPLEMENTATION
 *    and then import the header. This will insert the implementation.
 */

#ifndef BUDDY_ALLOC_H
#define BUDDY_ALLOC_H

#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

struct buddy;

/* Returns the size of a buddy required to manage of block of the specified size */
size_t buddy_sizeof(size_t memory_size);

/* Initializes a binary buddy memory allocator at the specified location */
struct buddy *buddy_init(unsigned char *at, unsigned char *main, size_t memory_size);

/*
 * Initializes a binary buddy memory allocator embedded in the specified arena.
 * The arena's capacity is reduced to account for the allocator metadata.
 */
struct buddy *buddy_embed(unsigned char *main, size_t memory_size);

/* Resizes the arena and metadata to a new size. */
struct buddy *buddy_resize(struct buddy *buddy, size_t new_memory_size);

/* Tests if the allocation can be shrank in half */
_Bool buddy_can_shrink(struct buddy *buddy);

/* Reports the arena size */
size_t buddy_arena_size(struct buddy *buddy);

/*
 * Allocation functions
 */

/* Use the specified buddy to allocate memory. See malloc. */
void *buddy_malloc(struct buddy *buddy, size_t requested_size);

/* Use the specified buddy to allocate zeroed memory. See calloc. */
void *buddy_calloc(struct buddy *buddy, size_t members_count, size_t member_size);

/* Realloc semantics are a joke. See realloc. */
void *buddy_realloc(struct buddy *buddy, void *ptr, size_t requested_size);

/* Realloc-like behavior that checks for overflow. See reallocarray*/
void *buddy_reallocarray(struct buddy *buddy, void *ptr,
	size_t members_count, size_t member_size);

/* Use the specified buddy to free memory. See free. */
void buddy_free(struct buddy *buddy, void *ptr);

#endif /* BUDDY_ALLOC_H */

#ifdef BUDDY_ALLOC_IMPLEMENTATION
#undef BUDDY_ALLOC_IMPLEMENTATION

#ifndef BUDDY_ALLOC_ALIGN
#define BUDDY_ALLOC_ALIGN (sizeof(size_t) * CHAR_BIT)
#endif

/*
 * Debug functions
 */

/* Implementation defined */
static void buddy_debug(FILE *stream, struct buddy *buddy);

struct buddy_tree;

typedef size_t buddy_tree_pos;

struct buddy_tree_interval {
    buddy_tree_pos from;
    buddy_tree_pos to;    
};

/*
 * Initialization functions
 */

/* Returns the size of a buddy allocation tree of the desired order*/
static size_t buddy_tree_sizeof(uint8_t order);

/* Initializes a buddy allocation tree at the specified location */
static struct buddy_tree *buddy_tree_init(unsigned char *at, uint8_t order);

/* Indicates whether this is a valid position for the tree */
static _Bool buddy_tree_valid(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the order of the specified buddy allocation tree */
static uint8_t buddy_tree_order(struct buddy_tree *t);

/* Resize the tree to the new order. When downsizing the left subtree is picked. */
/* Caller must ensure enough space for the new order. */
static void buddy_tree_resize(struct buddy_tree *t, uint8_t desired_order);

/*
 * Navigation functions
 */

/* Returns a position at the root of a buddy allocation tree */
static buddy_tree_pos buddy_tree_root(void);

/* Returns the leftmost child node */
static buddy_tree_pos buddy_tree_leftmost_child(struct buddy_tree *t);

/* Returns the tree depth of the indicated position */
static inline size_t buddy_tree_depth(buddy_tree_pos pos);

/* Returns the left child node position. Does not check if that is a valid position */
static inline buddy_tree_pos buddy_tree_left_child(buddy_tree_pos pos);

/* Returns the right child node position. Does not check if that is a valid position */
static inline buddy_tree_pos buddy_tree_right_child(buddy_tree_pos pos);

/* Returns the parent node position or an invalid position if there is no parent node */
static inline buddy_tree_pos buddy_tree_parent(buddy_tree_pos pos);

/* Returns the right adjacent node position or an invalid position if there is no right adjacent node */
static buddy_tree_pos buddy_tree_right_adjacent(buddy_tree_pos pos);

/* Returns the at-depth index of the indicated position */
static size_t buddy_tree_index(buddy_tree_pos pos);

/* Return the interval of the deepest positions spanning the indicated position */
static struct buddy_tree_interval buddy_tree_interval(struct buddy_tree *t, buddy_tree_pos pos);

/* Checks if one interval contains another */
static _Bool buddy_tree_interval_contains(struct buddy_tree_interval outer,
    struct buddy_tree_interval inner);

/*
 * Allocation functions
 */

/* Returns the free capacity at or underneath the indicated position */
static size_t buddy_tree_status(struct buddy_tree *t, buddy_tree_pos pos);

/* Marks the indicated position as allocated and propagates the change */
static void buddy_tree_mark(struct buddy_tree *t, buddy_tree_pos pos);

/* Marks the indicated position as free and propagates the change */
static void buddy_tree_release(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns a free position at the specified depth or an invalid position */
static buddy_tree_pos buddy_tree_find_free(struct buddy_tree *t, uint8_t depth);

/* Tests if the incidated position is available for allocation */
static _Bool buddy_tree_is_free(struct buddy_tree *t, buddy_tree_pos pos);

/* Tests if the tree can be shrank in half */
static _Bool buddy_tree_can_shrink(struct buddy_tree *t);

/*
 * Debug functions
 */

/* Implementation defined */
static void buddy_tree_debug(FILE *stream, struct buddy_tree *t, buddy_tree_pos pos, size_t start_size);

/* Implementation defined */
static _Bool buddy_tree_check_invariant(struct buddy_tree *t, buddy_tree_pos pos);

/*
 * A char-backed bitset implementation
 */

static size_t bitset_sizeof(size_t elements);

static void bitset_set_range(unsigned char *bitset, size_t from_pos, size_t to_pos);

static void bitset_clear_range(unsigned char *bitset, size_t from_pos, size_t to_pos);

static size_t bitset_count_range(unsigned char *bitset, size_t from_pos, size_t to_pos);

static inline void bitset_set(unsigned char *bitset, size_t pos);

static inline void bitset_clear(unsigned char *bitset, size_t pos);

static inline _Bool bitset_test(const unsigned char *bitset, size_t pos);

static void bitset_shift_left(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by);

static void bitset_shift_right(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by);

/*
 * Debug functions
 */

/* Implementation defined */
static void bitset_debug(FILE *stream, unsigned char *bitset, size_t length);

/*
 Bits
*/

/* Returns the index of the highest bit set (1-based) */
static inline size_t highest_bit_position(size_t value);

/* Returns the nearest larger or equal power of two */
static inline size_t ceiling_power_of_two(size_t value);

/*
 Implementation
*/

/*
 * A binary buddy memory allocator
 */

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
static inline size_t size_for_depth(struct buddy *buddy, size_t depth);
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
    if (memory_size < BUDDY_ALLOC_ALIGN) {
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
    /* Trim down memory to alignment */
    if (memory_size % BUDDY_ALLOC_ALIGN) {
        memory_size -= (memory_size % BUDDY_ALLOC_ALIGN);
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
    if (! main) {
        return NULL;
    }
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

    if (buddy->relative_mode) {
        return buddy_resize_embedded(buddy, new_memory_size);
    } else {
        return buddy_resize_standard(buddy, new_memory_size);
    }
}

static struct buddy *buddy_resize_standard(struct buddy *buddy, size_t new_memory_size) {
    /* Trim down memory to alignment */
    if (new_memory_size % BUDDY_ALLOC_ALIGN) {
        new_memory_size -= (new_memory_size % BUDDY_ALLOC_ALIGN);
    }

    /* Account for tree use */
    if (!buddy_is_free(buddy, new_memory_size)) {
        return NULL;
    }

    /* Release the virtual slots */
    buddy_toggle_virtual_slots(buddy, 0);

    /* Calculate new tree order and resize it */
    size_t new_buddy_tree_order = buddy_tree_order_for_memory(new_memory_size);
    buddy_tree_resize(buddy_tree(buddy), new_buddy_tree_order);

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
    if (! resized) {
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
    return buddy_is_free(buddy, buddy->memory_size / 2);
}

size_t buddy_arena_size(struct buddy *buddy) {
    if (buddy == NULL) {
        return 0;
    }
    return buddy->memory_size;
}

static size_t buddy_tree_order_for_memory(size_t memory_size) {
    size_t blocks = memory_size / BUDDY_ALLOC_ALIGN;
    return highest_bit_position(ceiling_power_of_two(blocks));
}

void *buddy_malloc(struct buddy *buddy, size_t requested_size) {
    if (buddy == NULL) {
        return NULL;
    }
    if (requested_size == 0) {
        /*
         * Batshit crazy code exists that calls malloc(0) and expects
         * a result that can be safely passed to free().
         * And even though this allocator will safely handle a free(NULL)
         * the particular batshit code will expect a non-NULL malloc(0) result!
         *
         * See also https://wiki.sei.cmu.edu/confluence/display/c/MEM04-C.+Beware+of+zero-length+allocations
         */
        requested_size = 1;
    }
    if (requested_size > buddy->memory_size) {
        return NULL;
    }

    size_t target_depth = depth_for_size(buddy, requested_size);
    buddy_tree_pos pos = buddy_tree_find_free(buddy_tree(buddy), target_depth);

    if (!pos) {
        return NULL; /* no slot found */
    }

    /* Allocate the slot */
    buddy_tree_mark(buddy_tree(buddy), pos);

    /* Find and return the actual memory address */
    return address_for_position(buddy, pos);
}

void *buddy_calloc(struct buddy *buddy, size_t members_count, size_t member_size) {
    if (members_count == 0 || member_size == 0) {
        /* See the gleeful remark in malloc */
        members_count = 1;
        member_size = 1;
    }
    /* Check for overflow */
    if (((members_count * member_size)/members_count) != member_size) {
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
        current_depth > target_depth ? current_depth : target_depth));
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
    if ((dst < main) || (dst >= (main + buddy->memory_size))) {
        return;
    }

    /* Find the position tracking this address */
    buddy_tree_pos pos = position_for_address(buddy, dst);

    /* Release the position */
    buddy_tree_release(buddy_tree(buddy), pos);
}

static size_t depth_for_size(struct buddy *buddy, size_t requested_size) {
    if (requested_size < BUDDY_ALLOC_ALIGN) {
        requested_size = BUDDY_ALLOC_ALIGN;
    }
    size_t depth = 1;
    size_t effective_memory_size = ceiling_power_of_two(buddy->memory_size);
    while ((effective_memory_size / requested_size) >> 1u) {
        depth++;
        effective_memory_size >>= 1u;
    }
    return depth;
}

static inline size_t size_for_depth(struct buddy *buddy, size_t depth) {
    depth += !depth; /* Silences a clang warning about undefined right shift */
    return ceiling_power_of_two(buddy->memory_size) >> (depth-1);
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
    size_t index = offset / BUDDY_ALLOC_ALIGN;
    buddy_tree_pos pos = buddy_tree_leftmost_child(buddy_tree(buddy));
    return pos + index;
}

static buddy_tree_pos position_for_address(struct buddy *buddy, const unsigned char *addr) {
    /* Find the deepest position tracking this address */
    unsigned char *main = buddy_main(buddy);
    ptrdiff_t offset = addr - main;

#ifdef BUDDY_ALLOC_SAFETY
    if (offset % BUDDY_ALLOC_ALIGN) {
        return 0; /* invalid alignment */
    }
#endif

    buddy_tree_pos pos = deepest_position_for_offset(buddy, offset);

    /* Find the actual allocated position tracking this address */
    while (pos && !buddy_tree_status(buddy_tree(buddy), pos)) {
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
    /* Node memory size is already aligned to BUDDY_ALLOC_ALIGN */
    size_t delta = effective_memory_size - memory_size;

    /* Update the virtual slot count */
    buddy->virtual_slots = state ? (delta / BUDDY_ALLOC_ALIGN) : 0;

    /* Determine whether to mark or release */
    void (*toggle)(struct buddy_tree *, buddy_tree_pos) =
        state ? &buddy_tree_mark : &buddy_tree_release;

    struct buddy_tree *tree = buddy_tree(buddy);
    buddy_tree_pos pos = buddy_tree_right_child(buddy_tree_root());
    while (delta) {
        size_t current_pos_size = size_for_depth(buddy, buddy_tree_depth(pos));
        if (delta == current_pos_size) {
            // toggle current pos
            (*toggle)(tree, pos);
            break;
        }
        if (delta <= (current_pos_size / 2)) {
            // re-run for right child
            pos = buddy_tree_right_child(pos);
            continue;
        } else {
            // toggle right child
            (*toggle)(tree, buddy_tree_right_child(pos));
            // reduce delta
            delta -= current_pos_size / 2;
            // re-run for left child
            pos = buddy_tree_left_child(pos);
            continue;
        }
    }
}

/* Internal function that checks if there are any allocations
 after the indicated relative memory index. Used to check if
 the arena can be downsized. */
static _Bool buddy_is_free(struct buddy *buddy, size_t from) {
    /* from is already adjusted for alignment */

    size_t effective_memory_size = ceiling_power_of_two(buddy->memory_size);
    size_t to = effective_memory_size -
        ((buddy->virtual_slots ? buddy->virtual_slots : 1) * BUDDY_ALLOC_ALIGN);

    struct buddy_tree *t = buddy_tree(buddy);

    struct buddy_tree_interval query_range = {0};
    query_range.from = deepest_position_for_offset(buddy, from);
    query_range.to = deepest_position_for_offset(buddy, to);

    buddy_tree_pos pos = deepest_position_for_offset(buddy, from);
    while(buddy_tree_valid(t, pos) && (pos < query_range.to)) {
        struct buddy_tree_interval current_test_range = buddy_tree_interval(t, pos);
        struct buddy_tree_interval parent_test_range =
            buddy_tree_interval(t, buddy_tree_parent(pos));
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

static void buddy_debug(FILE *stream, struct buddy *buddy) {
    fprintf(stream, "buddy allocator at: %p arena at: %p\n", (void *)buddy, (void *)buddy_main(buddy));
    fprintf(stream, "memory size: %zu\n", buddy->memory_size);
    fprintf(stream, "mode: ");
    fprintf(stream, buddy->relative_mode ? "embedded" : "standard");
    fprintf(stream, "\n");
    fprintf(stream, "virtual slots: %zu\n", buddy->virtual_slots);
    fprintf(stream, "allocator tree follows:\n");
    buddy_tree_debug(stream, buddy_tree(buddy), buddy_tree_root(), ceiling_power_of_two(buddy->memory_size));
}

/*
 * A buddy allocation tree
 */

struct buddy_tree {
    uint8_t order;
    size_t upper_pos_bound;
    size_t size_for_order_offset;
    size_t data[];
};

struct internal_position {
    size_t local_offset;
    size_t bitset_location;
};

static inline size_t size_for_order(uint8_t order, uint8_t to);
static inline size_t buddy_tree_index_internal(buddy_tree_pos pos);
static buddy_tree_pos buddy_tree_leftmost_child_internal(size_t tree_order);
static struct internal_position buddy_tree_internal_position_order(
    size_t tree_order, buddy_tree_pos pos);
static struct internal_position buddy_tree_internal_position_tree(
    struct buddy_tree *t, buddy_tree_pos pos);
static void buddy_tree_grow(struct buddy_tree *t, uint8_t desired_order);
static void buddy_tree_shrink(struct buddy_tree *t, uint8_t desired_order);
static void update_parent_chain(struct buddy_tree *t, buddy_tree_pos pos);
static inline unsigned char *buddy_tree_bits(struct buddy_tree *t);
static void buddy_tree_populate_size_for_order(struct buddy_tree *t);
static inline size_t buddy_tree_size_for_order(struct buddy_tree *t, uint8_t to);
static void write_to_internal_position(unsigned char *bitset, struct internal_position pos, size_t value);
static size_t read_from_internal_position(unsigned char *bitset, struct internal_position pos);

static inline size_t size_for_order(uint8_t order, uint8_t to) {
    size_t result = 0;
    size_t multi = 1u;
    while (order != to) {
        result += order * multi;
        order--;
        multi *= 2;
    }
    return result;
}

static inline struct internal_position buddy_tree_internal_position_order(
        size_t tree_order, buddy_tree_pos pos) {
    struct internal_position p = {0};
    p.local_offset = tree_order - buddy_tree_depth(pos) + 1;
    size_t total_offset = size_for_order(tree_order, p.local_offset);
    size_t local_index = buddy_tree_index_internal(pos);
    p.bitset_location = total_offset + (p.local_offset * local_index);
    return p;
}

static inline struct internal_position buddy_tree_internal_position_tree(
        struct buddy_tree *t, buddy_tree_pos pos) {
    struct internal_position p = {0};
    p.local_offset = t->order - buddy_tree_depth(pos) + 1;
    size_t total_offset = buddy_tree_size_for_order(t, p.local_offset);
    size_t local_index = buddy_tree_index_internal(pos);
    p.bitset_location = total_offset + (p.local_offset * local_index);
    return p;
}

static size_t buddy_tree_sizeof(uint8_t order) {
    size_t tree_size = sizeof(struct buddy_tree);
    /* Account for the bitset */
    size_t bitset_size = bitset_sizeof(size_for_order(order, 0));
    if (bitset_size % sizeof(size_t)) {
        bitset_size += (bitset_size % sizeof(size_t));
    }
    /* Account for the size_for_order memoization */
    size_t size_for_order_size = ((order+2) * sizeof(size_t));
    return tree_size + bitset_size + size_for_order_size;
}

static struct buddy_tree *buddy_tree_init(unsigned char *at, uint8_t order) {
    size_t size = buddy_tree_sizeof(order);
    struct buddy_tree *t = (struct buddy_tree*) at;
    memset(at, 0, size);
    t->order = order;
    t->upper_pos_bound = 1u << t->order;
    buddy_tree_populate_size_for_order(t);
    return t;
}

static void buddy_tree_resize(struct buddy_tree *t, uint8_t desired_order) {
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
        buddy_tree_pos current_pos = buddy_tree_leftmost_child_internal(current_order);
        buddy_tree_pos next_pos = buddy_tree_leftmost_child_internal(current_order + 1u);
        while(current_order) {
            /* Get handles into the rows at the tracked depth */
            struct internal_position current_internal = buddy_tree_internal_position_order(
                t->order, current_pos);
            struct internal_position next_internal = buddy_tree_internal_position_order(
                t->order + 1u, next_pos);

            /* There are this many nodes at the current level */
            size_t node_count = 1u << (current_order - 1u);

            /* Transfer the bits*/
            bitset_shift_right(buddy_tree_bits(t),
                current_internal.bitset_location /* from here */,
                current_internal.bitset_location + (current_internal.local_offset * node_count) /* up to here */,
                next_internal.bitset_location - current_internal.bitset_location /* by */);

            /* Clear right section */
            bitset_clear_range(buddy_tree_bits(t),
                next_internal.bitset_location + (next_internal.local_offset * node_count),
                next_internal.bitset_location + (next_internal.local_offset * node_count * 2) - 1);

            /* Handle the upper level */
            current_order -= 1u;
            current_pos = buddy_tree_parent(current_pos);
            next_pos = buddy_tree_parent(next_pos);
        }
        /* Advance the order and refrest the root */
        t->order += 1u;
        t->upper_pos_bound = 1u << t->order;
        buddy_tree_populate_size_for_order(t);

        /* Update the root */
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

        buddy_tree_pos left_start = buddy_tree_left_child(buddy_tree_root());
        while(buddy_tree_valid(t, left_start)) {
            /* Get handles into the rows at the tracked depth */
            struct internal_position current_internal = buddy_tree_internal_position_order(
                current_order, left_start);
            struct internal_position next_internal = buddy_tree_internal_position_order(
                next_order, buddy_tree_parent(left_start));

            /* There are this many nodes at the current level */
            size_t node_count = 1u << (buddy_tree_depth(left_start) - 1u);

            /* Transfer the bits*/
            bitset_shift_left(buddy_tree_bits(t),
                current_internal.bitset_location /* from here */,
                current_internal.bitset_location + (current_internal.local_offset * node_count / 2) /* up to here */,
                current_internal.bitset_location - next_internal.bitset_location/* at here */);

            /* Handle the lower level */
            left_start = buddy_tree_left_child(left_start);
        }

        /* Advance the order */
        t->order = next_order;
        t->upper_pos_bound = 1u << t->order;
        buddy_tree_populate_size_for_order(t);
    }   
}

static _Bool buddy_tree_valid(struct buddy_tree *t, buddy_tree_pos pos) {
    return pos && (pos < t->upper_pos_bound);
}

static uint8_t buddy_tree_order(struct buddy_tree *t) {
    return t->order;
}

static buddy_tree_pos buddy_tree_root(void) {
    return 1;
}

static buddy_tree_pos buddy_tree_leftmost_child(struct buddy_tree *t) {
    return buddy_tree_leftmost_child_internal(t->order);
}

static buddy_tree_pos buddy_tree_leftmost_child_internal(size_t tree_order) {
    assert(tree_order);
    return 1u << (tree_order - 1u);
}

static inline size_t buddy_tree_depth(buddy_tree_pos pos) {
    return highest_bit_position(pos);
}

static inline buddy_tree_pos buddy_tree_left_child(buddy_tree_pos pos) {
    return pos * 2;
}

static inline buddy_tree_pos buddy_tree_right_child(buddy_tree_pos pos) {
    return (pos * 2) + 1;
}

static inline buddy_tree_pos buddy_tree_parent(buddy_tree_pos pos) {
    return pos/2;
}

static buddy_tree_pos buddy_tree_right_adjacent(buddy_tree_pos pos) {
    if (highest_bit_position(pos) != highest_bit_position(pos + 1)) {
        return 0;
    }
    pos += 1;
    return pos;
}

static size_t buddy_tree_index(buddy_tree_pos pos) {
    return buddy_tree_index_internal(pos);
}

static inline size_t buddy_tree_index_internal(buddy_tree_pos pos) {
    /* Clear out the highest bit, this gives us the index
     * in a row of sibling nodes */
    /* % ((sizeof(size_t) * CHAR_BIT)-1) ensures we don't shift into
     * undefined behavior and stops clang from barking :)
     * Hopefully clang also optimizes it away :) */
    size_t mask = 1u << (highest_bit_position(pos) - 1) % ((sizeof(size_t) * CHAR_BIT)-1);
    size_t result = pos & ~mask;
    return result;
}

static inline unsigned char *buddy_tree_bits(struct buddy_tree *t) {
    return (unsigned char *) t->data;
}

static void buddy_tree_populate_size_for_order(struct buddy_tree *t) {
    size_t bitset_offset = bitset_sizeof(size_for_order(t->order, 0));
    if (bitset_offset % sizeof(size_t)) {
        bitset_offset += (bitset_offset % sizeof(size_t));
    }
    t->size_for_order_offset = bitset_offset / sizeof(size_t);
    t->size_for_order_offset++;
    for (size_t i = 0; i <= t->order; i++) {
        *(t->data+t->size_for_order_offset+i) = size_for_order(t->order, i);
    }
}

static inline size_t buddy_tree_size_for_order(struct buddy_tree *t,
         uint8_t to) {
     return *(t->data+t->size_for_order_offset+to);
}

static void write_to_internal_position(unsigned char *bitset, struct internal_position pos, size_t value) {
    bitset_clear_range(bitset, pos.bitset_location,
        pos.bitset_location+pos.local_offset-1);
    if (value) {
        bitset_set_range(bitset, pos.bitset_location,
            pos.bitset_location+value-1);
    }
}

static size_t read_from_internal_position(unsigned char *bitset, struct internal_position pos) {
    if (! bitset_test(bitset, pos.bitset_location)) {
        return 0; /* Fast test without complete extratioc */
    }
    return bitset_count_range(bitset, pos.bitset_location, pos.bitset_location+pos.local_offset-1);
}

static struct buddy_tree_interval buddy_tree_interval(struct buddy_tree *t, buddy_tree_pos pos) {
    struct buddy_tree_interval result = {0};
    result.from = pos;
    result.to = pos;
    size_t depth = buddy_tree_depth(pos);
    while (depth != t->order) {
        result.from = buddy_tree_left_child(result.from);
        result.to = buddy_tree_right_child(result.to);
        depth += 1;
    }
    return result;
}

static _Bool buddy_tree_interval_contains(struct buddy_tree_interval outer,
        struct buddy_tree_interval inner) {
    return (inner.from >= outer.from)
        && (inner.from <= outer.to)
        && (inner.to >= outer.from)
        && (inner.to <= outer.to);
}

static size_t buddy_tree_status(struct buddy_tree *t, buddy_tree_pos pos) {
    if (!pos) {
        return 0;
    }

    struct internal_position internal = buddy_tree_internal_position_tree(t, pos);
    return read_from_internal_position(buddy_tree_bits(t), internal);
}

static void buddy_tree_mark(struct buddy_tree *t, buddy_tree_pos pos) {
    if (!pos) {
        return;
    }

    /* Calling mark on a used position is a bug in caller */
    struct internal_position internal = buddy_tree_internal_position_tree(t, pos);

    /* Mark the node as used */
    write_to_internal_position(buddy_tree_bits(t), internal, internal.local_offset);

    /* Update the tree upwards */
    update_parent_chain(t, buddy_tree_parent(pos));
}

static void buddy_tree_release(struct buddy_tree *t, buddy_tree_pos pos) {
    if (!pos) {
        return;
    }

    /* Calling release on an unused or a partially-used position a bug in caller */
    struct internal_position internal = buddy_tree_internal_position_tree(t, pos);

#ifdef BUDDY_ALLOC_SAFETY
    if (read_from_internal_position(buddy_tree_bits(t), internal) != internal.local_offset) {
        return;
    }
#endif

    /* Mark the node as unused */
    write_to_internal_position(buddy_tree_bits(t), internal, 0);

    /* Update the tree upwards */
    update_parent_chain(t, buddy_tree_parent(pos));
}

static void update_parent_chain(struct buddy_tree *t, buddy_tree_pos pos) {
    if (! pos) {
        return;
    }

    unsigned char *bits = buddy_tree_bits(t);

    struct internal_position pos_internal =
        buddy_tree_internal_position_tree(t, buddy_tree_left_child(pos));
    size_t size_a = read_from_internal_position(bits, pos_internal);

    pos_internal.bitset_location += pos_internal.local_offset;
    size_t size_b = read_from_internal_position(bits, pos_internal);

    while (1) {
        size_t free = 0;
        if (size_a || size_b) {
            free = (size_a <= size_b ? size_a : size_b) + 1;
        }

        pos_internal = buddy_tree_internal_position_tree(t, pos);
        size_t current = read_from_internal_position(bits, pos_internal);
        if (free == current) {
            return; /* short the parent chain update */
        }

        /* Update the node */
        write_to_internal_position(bits, pos_internal, free);

        if (pos == 1) {
            return;
        }

        size_a = free; /* A remains the current position */
        /* B is the sibling position */
        if (pos & 1u) {
            pos_internal.bitset_location -= pos_internal.local_offset;
        } else {
            pos_internal.bitset_location += pos_internal.local_offset;
        }
        size_b = read_from_internal_position(bits, pos_internal);

        /* Advance upwards */
        pos = buddy_tree_parent(pos);
    }
}

static buddy_tree_pos buddy_tree_find_free(struct buddy_tree *t, uint8_t target_depth) {
    assert(target_depth <= t->order);
    buddy_tree_pos start = buddy_tree_root();
    uint8_t target_status = target_depth - 1;
    size_t current_depth = buddy_tree_depth(start);
    size_t current_status = buddy_tree_status(t, start);
    while (1) {
        if (current_depth == target_depth) {
            return current_status == 0 ? start : 0;
        }
        if (current_status > target_status) {
            return 0; /* No position available down the tree */
        }

        /* Advance criteria */
        target_status -= 1;
        current_depth += 1;

        /* Do an optimal fit followed by left-first fit */
        buddy_tree_pos left_pos = buddy_tree_left_child(start);
        buddy_tree_pos right_pos = left_pos + 1;
        struct internal_position internal = buddy_tree_internal_position_tree(t, left_pos);
        size_t left_status = read_from_internal_position(buddy_tree_bits(t), internal);
        internal.bitset_location += internal.local_offset; /* advance to the right */
        size_t right_status = read_from_internal_position(buddy_tree_bits(t), internal);

        if (left_status > target_status) { /* left branch is busy, pick right */
            start = right_pos;
            current_status = right_status;
            continue;
        }

        if (right_status > target_status) { /* right branch is busy, pick left */
            start = left_pos;
            current_status = left_status;
            continue;
        }

        /* Both branches are good, pick the more-used one */
        if (left_status >= right_status) {
            start = left_pos;
            current_status = left_status;
        } else {
            start = right_pos;
            current_status = right_status;
        }
    }
}

static _Bool buddy_tree_is_free(struct buddy_tree *t, buddy_tree_pos pos) {
    if (buddy_tree_status(t, pos)) {
        return 0;
    }
    pos = buddy_tree_parent(pos);
    while(buddy_tree_valid(t, pos)) {
        struct internal_position internal = buddy_tree_internal_position_tree(t, pos);
        size_t value = read_from_internal_position(buddy_tree_bits(t), internal);
        if (value) {
            return value != internal.local_offset;
        }
        pos = buddy_tree_parent(pos);
    }
    return 1;
}

static _Bool buddy_tree_can_shrink(struct buddy_tree *t) {
    if (buddy_tree_status(t, buddy_tree_right_child(buddy_tree_root())) != 0) {
        return 0; /* Refusing to shrink with right subtree still used! */
    }
    struct internal_position root_internal =
        buddy_tree_internal_position_tree(t, buddy_tree_root());
    size_t root_value = read_from_internal_position(buddy_tree_bits(t), root_internal);
    if (root_value == root_internal.local_offset) {
        return 0; /* Refusing to shrink with the root fully-allocated! */
    }
    return 1;
}

static void buddy_tree_debug(FILE *stream, struct buddy_tree *t, buddy_tree_pos pos,
        size_t start_size) {
    if (!buddy_tree_valid(t, pos)) {
        return;
    }

    buddy_tree_pos start = pos;
    _Bool going_up = 0;
    while (1) {
        if (going_up) {
            if (pos == start) {
                break;
            }
            if (! (pos & 1u /* bit-wise */) /* left node */) {
                pos = buddy_tree_right_adjacent(pos);
                going_up = 0;
            } else {
                pos = buddy_tree_parent(pos);
            }
        } else {
            struct internal_position pos_internal =
                buddy_tree_internal_position_tree(t, pos);
            size_t pos_status = read_from_internal_position(buddy_tree_bits(t), pos_internal);
            size_t pos_size = start_size 
                >> ((buddy_tree_depth(pos) - 1u)
                    % ((sizeof(size_t) * CHAR_BIT)-1));
            fprintf(stream, "%.*s",
                (int) buddy_tree_depth(pos),
                "                                                               ");
            fprintf(stream, "pos: %zu status: %zu bitset-len: %zu bitset-at: %zu",
                pos, pos_status, pos_internal.local_offset, pos_internal.bitset_location);
            if (pos_status == pos_internal.local_offset) {
                fprintf(stream, " size: %zu\n", pos_size);
            } else {
                fprintf(stream, "\n");
            }
            if (buddy_tree_valid(t, buddy_tree_left_child(pos))) {
                pos = buddy_tree_left_child(pos);
            } else {
                going_up = 1;
            }
        }
    }

    fflush(stdout);
}

static _Bool buddy_tree_check_invariant(struct buddy_tree *t, buddy_tree_pos pos) {
    buddy_tree_pos start = pos;
    _Bool going_up = 0;
    _Bool fail = 0;
    while (1) {
        if (going_up) {
            if (pos == start) {
                break;
            }
            if (! (pos & 1u /* bit-wise */) /* left node */) {
                pos = buddy_tree_right_adjacent(pos);
                going_up = 0;
            } else {
                pos = buddy_tree_parent(pos);
            }
        } else {
            if (buddy_tree_valid(t, buddy_tree_left_child(pos))) {

                struct internal_position current_internal = buddy_tree_internal_position_tree(t, pos);
                size_t current_status = read_from_internal_position(buddy_tree_bits(t), current_internal);

                size_t left_child_status = buddy_tree_status(t, buddy_tree_left_child(pos));
                size_t right_child_status = buddy_tree_status(t, buddy_tree_right_child(pos));

                _Bool violated = 0;

                if (left_child_status || right_child_status) {
                    size_t min = left_child_status <= right_child_status
                        ? left_child_status : right_child_status;
                    if (current_status != (min + 1)) {
                        violated = 1;
                    }
                } else {
                    if ((current_status > 0) && (current_status < current_internal.local_offset)) {
                        violated = 1;
                    }
                }

                if (violated) {
                    fail = 1;
                    fprintf(stdout, "invariant violation at position [ %zu ]!\n", pos);
                    fprintf(stdout, "current: %zu left %zu right %zu max %zu\n",
                        current_status, left_child_status, right_child_status, current_internal.local_offset);
                }

                pos = buddy_tree_left_child(pos);
            } else {
                going_up = 1;
            }
        }
    }
    return fail;
}

/*
 * A char-backed bitset implementation
 */

size_t bitset_sizeof(size_t elements) {
    return ((elements) + CHAR_BIT - 1u) / CHAR_BIT;
}

static uint8_t bitset_index_mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

static inline void bitset_set(unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    bitset[bucket] |= bitset_index_mask[index];
}

static inline void bitset_clear(unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    bitset[bucket] &= ~bitset_index_mask[index];
}

static inline _Bool bitset_test(const unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    return bitset[bucket] & bitset_index_mask[index];
}

static uint8_t bitset_char_mask[8][8] = {
    {1, 3, 7, 15, 31, 63, 127, 255},
    {0, 2, 6, 14, 30, 62, 126, 254},
    {0, 0, 4, 12, 28, 60, 124, 252},
    {0, 0, 0,  8, 24, 56, 120, 248},
    {0, 0, 0,  0, 16, 48, 112, 240},
    {0, 0, 0,  0,  0, 32,  96, 224},
    {0, 0, 0,  0,  0,  0,  64, 192},
    {0, 0, 0,  0,  0,  0,   0, 128},
};

static void bitset_clear_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
    size_t from_bucket = from_pos / CHAR_BIT;
    size_t to_bucket = to_pos / CHAR_BIT;

    size_t from_index = from_pos % CHAR_BIT;
    size_t to_index = to_pos % CHAR_BIT;

    if (from_bucket == to_bucket) {
        bitset[from_bucket] &= ~bitset_char_mask[from_index][to_index];
    } else {
        bitset[from_bucket] &= ~bitset_char_mask[from_index][7];
        from_bucket++;
        while(from_bucket != to_bucket) {
            bitset[from_bucket] = 0;
            from_bucket++;
        }
        bitset[to_bucket] &= ~bitset_char_mask[0][to_index];
    }
}

static void bitset_set_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
    size_t from_bucket = from_pos / CHAR_BIT;
    size_t to_bucket = to_pos / CHAR_BIT;

    size_t from_index = from_pos % CHAR_BIT;
    size_t to_index = to_pos % CHAR_BIT;

    if (from_bucket == to_bucket) {
        bitset[from_bucket] |= bitset_char_mask[from_index][to_index];
    } else {
        bitset[from_bucket] |= bitset_char_mask[from_index][7];
        from_bucket++;
        while(from_bucket != to_bucket) {
            bitset[from_bucket] = 255u;
            from_bucket++;
        }
        bitset[to_bucket] |= bitset_char_mask[0][to_index];
    }
}

static size_t bitset_count_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
    size_t from_bucket = from_pos / CHAR_BIT;
    size_t to_bucket = to_pos / CHAR_BIT;

    size_t from_index = from_pos % CHAR_BIT;
    size_t to_index = to_pos % CHAR_BIT;

    if (from_bucket == to_bucket) {
        return __builtin_popcount(bitset[from_bucket] & bitset_char_mask[from_index][to_index]);
    }

    size_t result = __builtin_popcount(bitset[from_bucket] & bitset_char_mask[from_index][7]);
    from_bucket++;
    while(from_bucket != to_bucket) {
        result += __builtin_popcount(bitset[from_bucket]);
        from_bucket++;
    }
    result += __builtin_popcount(bitset[to_bucket]  & bitset_char_mask[0][to_index]);
    return result;
}

static void bitset_shift_left(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by) {
    size_t length = to_pos - from_pos;
    for(size_t i = 0; i < length; i++) {
        size_t at = from_pos + i;
        if (bitset_test(bitset, at)) {
            bitset_set(bitset, at-by);
        } else {
            bitset_clear(bitset, at-by);
        }
    }
    bitset_clear_range(bitset, length, length+by-1);

}

static void bitset_shift_right(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by) {
    ssize_t length = to_pos - from_pos;
    while (length >= 0) {
        size_t at = from_pos + length;
        if (bitset_test(bitset, at)) {
            bitset_set(bitset, at+by);
        } else {
            bitset_clear(bitset, at+by);
        }
        length -= 1;
    }
    bitset_clear_range(bitset, from_pos, from_pos+by-1);
}

static void bitset_debug(FILE *stream, unsigned char *bitset, size_t length) {
    for (size_t i = 0; i < length; i++) {
        fprintf(stream, "%zu: %d\n", i, bitset_test(bitset, i));
    }
}

/*
 Bits
*/

/* Returns the higest set bit position for the given value. Do not call with zero. */
static inline size_t highest_bit_position(size_t value) {
    assert(value);
    return ((sizeof(size_t) * CHAR_BIT) - __builtin_clzl(value));
}

static inline size_t ceiling_power_of_two(size_t value) {
    value += !value; /* branchless x -> { 1 for 0, x for x } */
    return 1u << ((sizeof(size_t) * CHAR_BIT) - __builtin_clzl(value + value - 1)-1);
}

#endif /* BUDDY_ALLOC_IMPLEMENTATION */

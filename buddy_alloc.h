/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include <limits.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define BUDDY_ALIGN (sizeof(size_t) * CHAR_BIT)

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

/*
 * Debug functions
 */

/* Implementation defined */
void buddy_debug(struct buddy *buddy);

/*
 buddy alloc tree (pending merge)
*/


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
size_t buddy_tree_sizeof(uint8_t order);

/* Initializes a buddy allocation tree at the specified location */
struct buddy_tree *buddy_tree_init(unsigned char *at, uint8_t order);

/* Indicates whether this is a valid position for the tree */
_Bool buddy_tree_valid(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the order of the specified buddy allocation tree */
uint8_t buddy_tree_order(struct buddy_tree *t);

/* Resize the tree to the new order. When downsizing the left subtree is picked. */
/* Caller must ensure enough space for the new order. */
void buddy_tree_resize(struct buddy_tree *t, uint8_t desired_order);

/*
 * Navigation functions
 */

/* Returns a position at the root of a buddy allocation tree */
buddy_tree_pos buddy_tree_root(void);

/* Returns the leftmost child node */
buddy_tree_pos buddy_tree_leftmost_child(struct buddy_tree *t);

/* Returns the tree depth of the indicated position */
size_t buddy_tree_depth(buddy_tree_pos pos);

/* Returns the left child node position or an invalid position if there is no left child node */
buddy_tree_pos buddy_tree_left_child(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the right child node position or an invalid position if there is no right child node */
buddy_tree_pos buddy_tree_right_child(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the parent node position or an invalid position if there is no parent node */
buddy_tree_pos buddy_tree_parent(buddy_tree_pos pos);

/* Returns the right adjacent node position or an invalid position if there is no right adjacent node */
buddy_tree_pos buddy_tree_right_adjacent(buddy_tree_pos pos);

/* Returns the at-depth index of the indicated position */
size_t buddy_tree_index(buddy_tree_pos pos);

/* Return the interval of the deepest positions spanning the indicated position */
struct buddy_tree_interval buddy_tree_interval(struct buddy_tree *t, buddy_tree_pos pos);

/* Checks if one interval contains another */
_Bool buddy_tree_interval_contains(struct buddy_tree_interval outer,
    struct buddy_tree_interval inner);

/*
 * Allocation functions
 */

/* Returns the free capacity at or underneath the indicated position */
size_t buddy_tree_status(struct buddy_tree *t, buddy_tree_pos pos);

/* Marks the indicated position as allocated and propagates the change */
void buddy_tree_mark(struct buddy_tree *t, buddy_tree_pos pos);

/* Marks the indicated position as free and propagates the change */
void buddy_tree_release(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns a free position at the specified depth or an invalid position */
buddy_tree_pos buddy_tree_find_free(struct buddy_tree *t, uint8_t depth);

/* Tests if the incidated position is available for allocation */
_Bool buddy_tree_is_free(struct buddy_tree *t, buddy_tree_pos pos);

/* Tests if the tree can be shrank in half */
_Bool buddy_tree_can_shrink(struct buddy_tree *t);

/*
 * Debug functions
 */

/* Implementation defined */
void buddy_tree_debug(struct buddy_tree *t, buddy_tree_pos pos);

/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A buddy allocation tree
 */
#include <stddef.h>
#include <stdint.h>

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

/* Returns the rightmost child node */
buddy_tree_pos buddy_tree_rightmost_child(struct buddy_tree *t);

/* Returns the tree depth of the indicated position */
size_t buddy_tree_depth(buddy_tree_pos pos);

/* Returns the left child node position or an invalid position if there is no left child node */
buddy_tree_pos buddy_tree_left_child(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the right child node position or an invalid position if there is no right child node */
buddy_tree_pos buddy_tree_right_child(struct buddy_tree *t, buddy_tree_pos pos);

/* Returns the parent node position or an invalid position if there is no parent node */
buddy_tree_pos buddy_tree_parent(buddy_tree_pos pos);

/* Returns the left adjacent node position or an invalid position if there is no left adjacent node */
buddy_tree_pos buddy_tree_left_adjacent(buddy_tree_pos pos);

/* Returns the right adjacent node position or an invalid position if there is no right adjacent node */
buddy_tree_pos buddy_tree_right_adjacent(buddy_tree_pos pos);

/* Returns the at-depth index of the indicated position */
size_t buddy_tree_index(buddy_tree_pos pos);

/* Return the interval of the deepest positions spanning the indicated position */
struct buddy_tree_interval buddy_tree_interval(struct buddy_tree *t, buddy_tree_pos pos);

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

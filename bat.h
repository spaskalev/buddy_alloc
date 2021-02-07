/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A buddy allocation tree
 */
#include <stddef.h>
#include <stdint.h>

struct bat;

typedef size_t bat_pos;

/*
 * Initialization functions
 */

/* Returns the size of a buddy allocation tree of the desired order*/
size_t bat_sizeof(uint8_t order);

/* Initializes a buddy allocation tree at the specified location */
struct bat *bat_init(unsigned char *at, uint8_t order);

/* Indicates whether this is a valid position for the tree */
_Bool bat_valid(struct bat *t, bat_pos pos);

/* Returns the order of the specified buddy allocation tree */
uint8_t bat_order(struct bat *t);

/*
 * Navigation functions
 */

/* Returns a position at the root of the buddy allocation tree */
bat_pos bat_root(struct bat *t);

/* Returns the depth at the indicated position */
size_t bat_depth(struct bat *t, bat_pos pos);

/* Returns the left child node position or an invalid position if there is no left child node */
bat_pos bat_left_child(struct bat *t, bat_pos pos);

/* Returns the right child node position or an invalid position if there is no right child node */
bat_pos bat_right_child(struct bat *t, bat_pos pos);

/* Returns the parent node position or an invalid position if there is no parent node */
bat_pos bat_parent(struct bat *t, bat_pos pos);

/* Returns the sibling node position or an invalid position if there is no sibling node */
bat_pos bat_sibling(struct bat *t, bat_pos pos);

/* Returns the left adjacent node position or an invalid position if there is no left adjacent node */
bat_pos bat_left_adjacent(struct bat *t, bat_pos pos);

/* Returns the right adjacent node position or an invalid position if there is no right adjacent node */
bat_pos bat_right_adjacent(struct bat *t, bat_pos pos);

/* Returns the at-depth index of the indicated position */
size_t bat_index(struct bat *t, bat_pos pos);

/*
 * Allocation functions
 */

/* Returns the free capacity at or underneath the indicated position */
uint8_t bat_status(struct bat *t, bat_pos pos);

/* Marks the indicated position as allocated and propagates the change */
void bat_mark(struct bat *t, bat_pos pos);

/* Marks the indicated position as free and propagates the change */
void bat_release(struct bat *t, bat_pos pos);

/* Returns a free position at the specified depth or an invalid position */
bat_pos bat_find_free(struct bat *t, uint8_t depth);

/*
 * Debug functions
 */

/* Implementation defined */
void bat_debug(struct bat *t, bat_pos pos);

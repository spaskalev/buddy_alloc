/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A boolean perfect binary tree
 */
#include <stddef.h>

struct bbt;

typedef size_t bbt_pos;

/* Returns the sizeof of a bbt of the desired order. Keep order small. */
size_t bbt_sizeof(unsigned char order);

/* Initializes a boolean binary tree at the specified location. Keep order small. */
struct bbt *bbt_init(unsigned char *at, unsigned char order);

/* Returns the order of the specified boolean binary tree */
unsigned char bbt_order(struct bbt *t);

/* Returns a leftward position at the specified depth */
bbt_pos bbt_left_pos_at_depth(struct bbt *t, size_t depth);

/* Returns the left child node position or an invalid position if there is no left child node */
bbt_pos bbt_pos_left_child(struct bbt *t, bbt_pos pos);

/* Returns the right child node position or an invalid position if there is no right child node */
bbt_pos bbt_pos_right_child(struct bbt *t, bbt_pos pos);

/* Returns the left adjacent node position or an invalid position if there is no left adjacent node */
bbt_pos bbt_pos_left_adjacent(struct bbt *t, bbt_pos pos);

/* Returns the right adjacent node position or an invalid position if there is no right adjacent node */
bbt_pos bbt_pos_right_adjacent(struct bbt *t, bbt_pos pos);

/* Returns the sibling node position or an invalid position if there is no sibling node */
bbt_pos bbt_pos_sibling(struct bbt *t, bbt_pos pos);

/* Returns the parent node position or an invalid position if there is no parent node */
bbt_pos bbt_pos_parent(struct bbt *t, bbt_pos pos);

/* Indicates whether this is a valid position for the tree */
_Bool bbt_pos_valid(struct bbt *t, bbt_pos pos);

/* Set the value at the indicated position */
void bbt_pos_set(struct bbt *t, bbt_pos pos);

/* Clear the value at the indicated position */
void bbt_pos_clear(struct bbt *t, bbt_pos pos);

/* Flips the value at the indicated position */
void bbt_pos_flip(struct bbt *t, bbt_pos pos);

/* Returns the value at the indicated position */
_Bool bbt_pos_test(struct bbt *t, bbt_pos pos);

/* Returns the depth at the indicated position */
size_t bbt_pos_depth(struct bbt *t, bbt_pos pos);

/* Returns the at-depth index of the indicated position */
size_t bbt_pos_index(struct bbt *t, bbt_pos pos);

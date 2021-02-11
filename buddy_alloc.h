/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include <stdalign.h>
#include <stddef.h>

#define BUDDY_ALIGN (sizeof(max_align_t))

struct buddy;

/* Returns the size of a buddy required to manage of block of the specified size */
size_t buddy_sizeof(size_t memory_size);

/* Initializes the binary buddy memory allocator at the specified location */
struct buddy *buddy_init(unsigned char *at, unsigned char *main, size_t memory_size);

/* Use the specified buddy to allocate memory. See malloc. */
void *buddy_malloc(struct buddy *buddy, size_t requested_size);

/* Use the specified buddy to free memory. See free. */
void buddy_free(struct buddy *buddy, void *ptr);

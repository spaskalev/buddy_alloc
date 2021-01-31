/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A binary buddy memory allocator
 */

#include <stdalign.h>
#include <stddef.h>

#define BBM_ALIGN (sizeof(max_align_t))

struct bbm;

/* Returns the size of a bbm required to manage of block of the specified size */
size_t bbm_sizeof(size_t memory_size);

/* Initializes the binary buddy memory allocator at the specified location */
struct bbm *bbm_init(unsigned char *at, unsigned char *main, size_t memory_size);

/* Use the specified bbm to allocate memory. See malloc. */
void *bbm_malloc(struct bbm *bbm, size_t requested_size);

/* Use the specified bbm to free memory. See free. */
void bbm_free(struct bbm *bbm, void *ptr);

/*
 * Copyright 2023 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUDDY_ALLOC_ALIGN 64
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc.h"
#undef BUDDY_ALLOC_IMPLEMENTATION

double test_malloc(size_t alloc_size);
double test_malloc_firstfit(size_t alloc_size);
void *freeing_callback(void *ctx, void *addr, size_t slot_size, size_t allocated);

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);

	double total = 0;
	for (size_t i = 0; i <= 6; i++) {
		total += test_malloc(64 << i);
	}
	printf("Total malloc runtime was %f seconds.\n\n", total);
}

double test_malloc(size_t alloc_size) {

	size_t arena_size = 1 << 30;
	unsigned char *buddy_buf = malloc(buddy_sizeof(arena_size));
	unsigned char *data_buf = malloc(arena_size);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, arena_size);

	printf("Starting test with alloc size [%zu].\n", alloc_size);
	time_t start_time = time(NULL);

	while (buddy_malloc(buddy, alloc_size)) {
		// fill it up
	}
	time_t alloc_time = time(NULL);

	assert(buddy_is_full(buddy));

	buddy_walk(buddy, freeing_callback, buddy);
	assert(buddy_is_empty(buddy));

	time_t end_time = time(NULL);
	double delta = difftime(end_time, start_time);
	printf("Test took %.f seconds in total. Allocation: %.f freeing: %.f\n", delta,
		difftime(alloc_time, start_time), difftime(end_time, alloc_time));

	free(data_buf);
	free(buddy_buf);

	return delta;
}

void *freeing_callback(void *ctx, void *addr, size_t slot_size, size_t allocated) {
	if (! allocated) {
		return NULL;
	}
	struct buddy *buddy = (struct buddy*) ctx;
	buddy_free(buddy, addr);
	return NULL;
}
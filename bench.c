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

double test(size_t alloc_size, size_t iterations);

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);

	double total = 0;
	for (size_t i = 0; i <= 6; i++) {
		total += test(64 << i, 1);
	}
	
	printf("Total runtime was %f seconds.\n", total);	
}

double test(size_t alloc_size, size_t iterations) {

	unsigned char *buddy_buf = malloc(buddy_sizeof(1 << 30));
	unsigned char *data_buf = malloc(1 << 30);
	struct buddy *buddy = buddy_init(buddy_buf, data_buf, 1 << 30);

	printf("Starting test with alloc size [%zu] and [%zu] iterations.\n", alloc_size, iterations);
	time_t start_time = time(NULL);
	for (size_t i = 0; i < iterations; i++) {
		while (buddy_malloc(buddy, alloc_size)) {
			// fill it up
		}
	}
	time_t end_time = time(NULL);
	double delta = difftime(end_time, start_time);
	printf("Test took %f seconds.\n", delta);

	free(data_buf);
	free(buddy_buf);

	return delta;
}

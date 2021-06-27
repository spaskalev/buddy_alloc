/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <unistd.h>
#include <errno.h>

#include "buddy_alloc.h"
#include "buddy_global.h"
#include "buddy_brk.h"

struct buddy *global_buddy;
size_t global_break_size;

void buddy_brk_init() {
    if (global_buddy) {
        return;
    }

    /* Get the system break */
    void *break_start = sbrk(0);
    if (break_start == NULL && errno == ENOMEM) {
        return;
    }

    /* Bump the break to the default initial size */
    void *break_bump = sbrk(BUDDY_BRK_DEFAULT_SIZE);
    if (break_bump == NULL && errno == ENOMEM) {
        return;
    }

    global_buddy = buddy_embed(break_start, BUDDY_BRK_DEFAULT_SIZE);
    global_break_size = BUDDY_BRK_DEFAULT_SIZE;
}

void *malloc(size_t size) {
    buddy_brk_init();
    return buddy_malloc(global_buddy, size);
}

void free(void *ptr) {
    buddy_brk_init();
    buddy_free(global_buddy, ptr);
}

void *calloc(size_t nmemb, size_t size) {
    buddy_brk_init();
    return buddy_calloc(global_buddy, nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    buddy_brk_init();
    return buddy_realloc(global_buddy, ptr, size);
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    buddy_brk_init();
    return buddy_reallocarray(global_buddy, ptr, nmemb, size);
}

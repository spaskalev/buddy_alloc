/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUDDY_ALLOC_IMPLEMENTATION
#define BUDDY_ALLOC_ALIGN sizeof(size_t)
#include "../buddy_alloc.h"

#include "buddy_global.h"
#include "buddy_brk.h"

static struct {
    unsigned char *break_start;
    unsigned char *break_at;
    struct buddy *buddy;
} global_brk = {0};

static void buddy_brk_init() {
    if (global_brk.buddy) {
        return;
    }

    /* Get the system break */
    global_brk.break_start = sbrk(0);
    if (errno == ENOMEM) {
        return;
    }

    /* Bump the break to the default initial size */
    (void) sbrk(BUDDY_BRK_DEFAULT_SIZE);
    if (errno == ENOMEM) {
        return;
    }
    /*
     * sbrk will return the start of the new space
     * so to get the actual break we must ask again
     *
     * sbrk can also increase the break beyond what's asked
     * which is another reason to double-check it
     */
    global_brk.break_at = sbrk(0);
    if (errno == ENOMEM) {
        return;
    }

    /* Initialize the allocator */
    global_brk.buddy = buddy_embed(global_brk.break_start,
        global_brk.break_at - global_brk.break_start);
}

static void buddy_bump_brk() {
    /* Double the break */
    (void) sbrk(global_brk.break_at - global_brk.break_start);
    if (errno == ENOMEM) {
        return;
    }
    global_brk.break_at = sbrk(0);
    if (errno == ENOMEM) {
        return;
    }

    global_brk.buddy = buddy_resize(global_brk.buddy,
        global_brk.break_at - global_brk.break_start);
}

static void buddy_lower_brk(void) {

}

void *malloc(size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_malloc(global_brk.buddy, size))) {
        buddy_bump_brk();
    }
    return result;
}

void free(void *ptr) {
    buddy_brk_init();
    buddy_free(global_brk.buddy, ptr);
}

void *calloc(size_t nmemb, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_calloc(global_brk.buddy, nmemb, size))) {
        assert(buddy_arena_size(global_brk.buddy));
        buddy_bump_brk();
    }
    assert(buddy_arena_size(global_brk.buddy));
    return result;
}

void *realloc(void *ptr, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_realloc(global_brk.buddy, ptr, size))) {
        assert(buddy_arena_size(global_brk.buddy));
        buddy_bump_brk();
    }
    return result;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_reallocarray(global_brk.buddy, ptr, nmemb, size))) {
        assert(buddy_arena_size(global_brk.buddy));
        buddy_bump_brk();
    }
    return result;
}

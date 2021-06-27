/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "buddy_alloc.h"
#include "buddy_global.h"
#include "buddy_brk.h"

unsigned char *global_break_start;
unsigned char *global_break;
struct buddy *global_buddy;

void buddy_brk_init() {
    if (global_buddy) {
        return;
    }

    /* Get the system break */
    global_break_start = sbrk(0);
    if (global_break_start == NULL && errno == ENOMEM) {
        return;
    }

    /* Bump the break to the default initial size */
    void *break_bump = sbrk(BUDDY_BRK_DEFAULT_SIZE
        + buddy_sizeof(BUDDY_BRK_DEFAULT_SIZE));
    if (break_bump == NULL && errno == ENOMEM) {
        return;
    }

    global_buddy = buddy_init(
        global_break_start + BUDDY_BRK_DEFAULT_SIZE,
        global_break_start,
        BUDDY_BRK_DEFAULT_SIZE
    );
    global_break = break_bump;
}

void buddy_bump_brk() {
    /* Get the current sizes */
    size_t old_arena_size = buddy_arena_size(global_buddy);
    size_t old_buddy_size = buddy_sizeof(old_arena_size);
    size_t old_total_size = old_arena_size + old_buddy_size;

    /* Get the new sizes */
    size_t new_arena_size = old_arena_size * 2;
    size_t new_buddy_size = buddy_sizeof(new_arena_size);
    size_t new_total_size = new_arena_size + new_buddy_size;

    /* Raise the brk */
    ssize_t diff = new_total_size - old_total_size;
    global_break = sbrk(diff);

    /* Resize and relocate the allocator */
    buddy_resize(global_buddy, new_arena_size);
    memmove(global_break_start+new_arena_size,
        global_break_start+old_arena_size, new_buddy_size);
    global_buddy = (struct buddy *) (global_break_start+new_arena_size);
}

void *malloc(size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_malloc(global_buddy, size))) {
        buddy_bump_brk();
    }
    return result;
}

void free(void *ptr) {
    buddy_brk_init();
    buddy_free(global_buddy, ptr);
    if (buddy_can_shrink(global_buddy)) {
        /* Get the current sizes */
        size_t old_arena_size = buddy_arena_size(global_buddy);
        if (old_arena_size == BUDDY_BRK_DEFAULT_SIZE) {
            return;
        }
        size_t old_buddy_size = buddy_sizeof(old_arena_size);
        size_t old_total_size = old_arena_size + old_buddy_size;

        /* Get the new sizes */
        size_t new_arena_size = old_arena_size / 2;
        size_t new_buddy_size = buddy_sizeof(new_arena_size);
        size_t new_total_size = new_arena_size + new_buddy_size;

        /* Attempt to resize the allocator */
        struct buddy *resized = buddy_resize(global_buddy, new_arena_size);
        if (! resized) {
            return; // failed to resize
        }

        /* Move it back */
        memmove(global_break_start+new_arena_size,
            global_break_start+old_arena_size, new_buddy_size);
        global_buddy = (struct buddy *) (global_break_start+new_arena_size);

        /* Lower the brk */
        ssize_t diff = old_total_size - new_total_size;
        global_break = sbrk(diff * -1);
    }
}

void *calloc(size_t nmemb, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_calloc(global_buddy, nmemb, size))) {
        buddy_bump_brk();
    }
    return result;
}

void *realloc(void *ptr, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_realloc(global_buddy, ptr, size))) {
        buddy_bump_brk();
    }
    return result;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    buddy_brk_init();
    void *result = NULL;
    while (!(result = buddy_reallocarray(global_buddy, ptr, nmemb, size))) {
        buddy_bump_brk();
    }
    return result;
}

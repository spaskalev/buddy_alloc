/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <unistd.h>
#include <errno.h>

#include "buddy_alloc.h"
#include "buddy_global.h"
#include "buddy_brk.h"

unsigned char *buddy_brk_start;
unsigned char *buddy_brk_at;
struct buddy *global_buddy;

void buddy_brk_init(size_t global_buddy_size) {
    /*
    Get the system break
    Bump to default initial size
    Place buddy metadata at the end.
    */
    buddy_brk_start = sbrk(0);
    if (errno == ENOMEM) {
        return;
    }
    size_t initial_size = global_buddy_size
        + buddy_sizeof(global_buddy_size);

    buddy_brk_at = sbrk(initial_size);
    if (errno == ENOMEM) {
        return;
    }

    global_buddy = buddy_init(buddy_brk_start + global_buddy_size,
        buddy_brk_start, global_buddy_size);
}

void buddy_brk_init_default_size() {
    buddy_brk_init(BUDDY_BRK_DEFAULT_SIZE);
}

void *malloc(size_t size) {
    if (global_buddy == NULL) {
        buddy_brk_init_default_size();
        if (global_buddy == NULL) {
            return NULL;
        }
    }
    return buddy_malloc(global_buddy, size);
}

void free(void *ptr) {
    if (global_buddy == NULL) {
        buddy_brk_init_default_size();
        if (global_buddy == NULL) {
            return;
        }
    }
    buddy_free(global_buddy, ptr);
}

void *calloc(size_t nmemb, size_t size) {
    if (global_buddy == NULL) {
        buddy_brk_init_default_size();
        if (global_buddy == NULL) {
            return NULL;
        }
    }
    return buddy_calloc(global_buddy, nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    if (global_buddy == NULL) {
        buddy_brk_init_default_size();
        if (global_buddy == NULL) {
            return NULL;
        }
    }
    return buddy_realloc(global_buddy, ptr, size);
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    if (global_buddy == NULL) {
        buddy_brk_init_default_size();
        if (global_buddy == NULL) {
            return NULL;
        }
    }
    return buddy_reallocarray(global_buddy, ptr, nmemb, size);
}

/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <unistd.h>

#define BUDDY_BRK_DEFAULT_SIZE 16777216UL

void buddy_brk_init(size_t global_buddy_size);

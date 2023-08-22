/*
 * Copyright 2020-2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

// Just include buddy_alloc from C++
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc.h"
#undef BUDDY_ALLOC_IMPLEMENTATION

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}
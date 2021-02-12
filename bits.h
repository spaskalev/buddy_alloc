/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include <stddef.h>
#include <stdint.h>

/* Returns the index of the highest bit set */
size_t highest_bit_position(size_t value);

/* Returns the the highest bit set */
size_t highest_bit(size_t value);

/* Returns the nearest larger or equal power of two */
size_t ceiling_power_of_two(size_t value);

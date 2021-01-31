/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A char-backed bitset implementation
 */

#include <limits.h>
#include <stddef.h>

#include "bitset.h"

size_t bitset_size(size_t elements) {
	return ((elements) + CHAR_BIT - 1u) / CHAR_BIT;
}

void bitset_set_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
	const unsigned char cbits[8] = {1, 3, 7, 15, 31, 63, 127, 255};
    size_t from_bucket = from_pos / CHAR_BIT;
    size_t from_index = from_pos % CHAR_BIT;
    size_t to_bucket = to_pos / CHAR_BIT;
    size_t to_index = to_pos % CHAR_BIT;
    if (from_bucket == to_bucket) {
		bitset[from_bucket] |= (size_t)(cbits[to_index - from_index]) << from_index;
	} else {
		bitset[from_bucket] |= (size_t)(cbits[7u - from_index]) << from_index;
		for (size_t i = from_bucket+1; i < to_bucket; i++) {
			bitset[i] = 255u;
		}
		bitset[to_bucket] |= cbits[to_index];
	}
}

void bitset_clear_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
	const unsigned char cbits[8] = {1, 3, 7, 15, 31, 63, 127, 255};
    size_t from_bucket = from_pos / CHAR_BIT;
    size_t from_index = from_pos % CHAR_BIT;
    size_t to_bucket = to_pos / CHAR_BIT;
    size_t to_index = to_pos % CHAR_BIT;
    if (from_bucket == to_bucket) {
		bitset[from_bucket] &= ~(size_t)(cbits[to_index - from_index]) << from_index;
	} else {
		bitset[from_bucket] &= ~(size_t)(cbits[7u - from_index]) << from_index;
		for (size_t i = from_bucket+1; i < to_bucket; i++) {
			bitset[i] = 0u;
		}
		bitset[to_bucket] &= ~(size_t)cbits[to_index];
	}
}

void bitset_set(unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    bitset[bucket] |= (1u << index);
}

void bitset_clear(unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    bitset[bucket] &= ~(1u << index);
}

void bitset_flip(unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    bitset[bucket] ^= (1u << index);
}

_Bool bitset_test(const unsigned char *bitset, size_t pos) {
    size_t bucket = pos / CHAR_BIT;
    size_t index = pos % CHAR_BIT;
    return (_Bool)(bitset[bucket] & (1u << index));
}

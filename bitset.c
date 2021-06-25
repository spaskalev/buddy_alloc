/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

/*
 * A char-backed bitset implementation
 */

#include <limits.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>

#include "bitset.h"

size_t bitset_size(size_t elements) {
	return ((elements) + CHAR_BIT - 1u) / CHAR_BIT;
}

void bitset_set_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
    if (to_pos < from_pos) {
        return;
    }
    while (from_pos <= to_pos) {
        bitset_set(bitset, from_pos);
        from_pos += 1;
    }
}

void bitset_clear_range(unsigned char *bitset, size_t from_pos, size_t to_pos) {
    if (to_pos < from_pos) {
        return;
    }
    while (from_pos <= to_pos) {
        bitset_clear(bitset, from_pos);
        from_pos += 1;
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

void bitset_shift_left(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by) {
    size_t length = to_pos - from_pos;
    for(size_t i = 0; i < length; i++) {
        size_t at = from_pos + i;
        if (bitset_test(bitset, at)) {
            bitset_set(bitset, at-by);
        } else {
            bitset_clear(bitset, at-by);
        }
    }
    bitset_clear_range(bitset, length, length+by-1);

}

void bitset_shift_right(unsigned char *bitset, size_t from_pos, size_t to_pos, size_t by) {
    ssize_t length = to_pos - from_pos;
    while (length >= 0) {
        size_t at = from_pos + length;
        if (bitset_test(bitset, at)) {
            bitset_set(bitset, at+by);
        } else {
            bitset_clear(bitset, at+by);
        }
        length -= 1;
    }
    bitset_clear_range(bitset, from_pos, from_pos+by-1);
}

void bitset_debug(unsigned char *bitset, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%zu: %d\n", i, bitset_test(bitset, i));
    }
}

/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 */

#include "bits.h"

size_t highest_bit_position(size_t value) {
	size_t pos = ((_Bool) value) & 1u;
	while ( value >>= 1u ) {
		pos += 1;
	}
	return pos;
}

size_t highest_bit(size_t value) {
	size_t count = 0;
	while ( value >>= 1u ) {
		count += 1;
	}
	return 1u << count;
}

size_t ceiling_power_of_two(size_t value) {
	size_t hb = highest_bit(value);
	if (hb == value) {
		return hb;
	}
	return hb << 1u;
}

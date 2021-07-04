# buddy_alloc
A buddy memory allocator for C

## Licensing

This project is licensed under the 0BSD license. See the LICENSE.md file for details.

## Overview

This is a simple buddy memory allocator that might be suitable for use in applications that require predictable allocation and deallocation behavior. The allocator's metadata is kept separate from the arena and its size is a function of the arena's size.

## Usage

Here is an example of initializing and using the buddy allocator with metadata external to the arena.

```c
size_t arena_size = 65536;
/* You need space for the metadata and for the arena */
void *buddy_metadata = malloc(buddy_sizeof(arena_size));
void *buddy_arena = malloc(arena_size);
struct buddy *buddy = buddy_init(buddy_metadata, buddy_arena, arena_size);

/* Allocate using the buddy allocator */
void *data = buddy_malloc(buddy, 2048);
/* Free using the buddy allocator */
buddy_free(buddy, data);

free(buddy_metadata);
free(buddy_arena);
```

Here is an example of initializing and using the buddy allocator with metadata internal to the arena.

```c
size_t arena_size = 65536;
/* You need space for arena and builtin metadata */
void *buddy_arena = malloc(arena_size);
struct buddy *buddy = buddy_embed(buddy_arena, arena_size);

/* Allocate using the buddy allocator */
void *data = buddy_malloc(buddy, 2048);
/* Free using the buddy allocator */
buddy_free(buddy, data);

free(buddy_arena);
```

## Design

The allocator was designed with the following requirements in mind.

- Allocation and deallocation operations should behave in a similar and preditable way regardless of the state of the allocator
- The allocator's metadata size should be predictable based on the arena's size and not dependent on the state of the allocator
- The allocator's metadata location should be external to the arena
- Returned memory should be aligned to size_t

The following were not design goals

- multithreading use
- malloc() replacement

## Implementation

### Metadata

The allocator uses a bitset-backed perfect binary tree to track allocations. The tree is fixed in size and remains outside of the main arena. This allows for better cache performance in the arena as the cache is not loading allocator metadata when processing application data.

### Allocation and deallocation

The binary tree nodes are labeled with the largest allocation slot available under them. This allows allocation to happen with a limited number of operations. Allocations that cannot be satisfied are fast to fail. Once a free node of the desired size is found it is marked as used and the nodes leading to root of the tree are updated to account for any difference in the largest available size. Deallocation works in a similar way - the smallest used block size for the given address is found, marked as a free and the same node update as with allocation is used to update the tree. 

### Space requirements

The tree is stored in a bitset with each node using just enough bits to store the maximum allocation slot available under it. For leaf nodes this is a single bit. Other nodes sizes depend on the height of the tree.

### Non-power-of-two arena sizes

The perfect binary tree always tracks an arena which size is a power-of-two. When the allocator is initialized or resized with an arena that is not a perfect fit the binary tree is updated to mask out the virtual arena complement to next power-of-two.

### Resizing

Resizing is available for both split and embedded allocator modes and supports both growing the arena and shrinking it. Checks are present that prevent shrinking the arena when memory that is to be reduced is still allocated.

### Resiliency

- Calling free in a wrong way (double free, out of arena, unaligned) will not corrupt the allocator's metadata.

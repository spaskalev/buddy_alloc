# buddy_alloc
A buddy memory allocator for C

## Licensing

This project is licensed under the 0BSD license. See the LICENSE.md file for details.

## Overview

This is a simple buddy memory allocator that might be suitable for use in applications that require predictable allocation and deallocation behavior. The allocator's metadata is kept separate from the arena and its size is a function of the arena's size.

## Design

The allocator was designed with the following requirements in mind.

- Allocation and deallocation operations should behave in a similar and preditable way regardless of the state of the allocator
- The allocator's metadata size should be predictable based on the arena's size and not dependent on the state of the allocator
- The allocator's metadata location should be external to the arena
- Returned memory should be aligned to max_align_t

The following were not design goals

- multithreading use
- malloc() replacement

## Implementation

### Metadata

The allocator uses a pair of bitset-backed boolean perfect binary trees to track allocations. The first binary tree leaf nodes represent the smallest-possible allocations and the tree is used to track a FREE or FULL state for any block. The second binary tree is used to track a NON-PARTIAL or PARTIAL state and its height is one less than the first tree since the smallest-possible allocations can only be fully-allocated. No other metadata is used and the trees are kept out of the managed arena. This allows for better cache performance in the arena as the cache is not loading allocator metadata when processing application data.

### Allocation and deallocation

Allocation will traverse the trees following the PARTIAL or FREE states until a block of the desired size is found and marked as FULL (or is not found and allocation fails). Afterwards the parent nodes' state is updated to account for the new use. Deallocation will find the smallest-possible block for the given address and traverse upwards until a FULL block is found. The full block is then set to FREE and the parent nodes' state is again updated to account for the new use. This algorithm ensures that the allocate and deallocate operations will perform a predictable number of operations based primarily on the size of the arena and not on the allocation state of the arena.

# buddy_alloc
A buddy memory allocator for C

# Status

- [![cicd](https://github.com/spaskalev/buddy_alloc/workflows/cicd/badge.svg)](https://github.com/spaskalev/buddy_alloc/actions)
- [Latest release](https://github.com/spaskalev/buddy_alloc/releases/latest)
- [Changes](CHANGES.md)

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

- Allocation and deallocation operations should behave in a similar and predictable way regardless of the state of the allocator
- The allocator's metadata size should be predictable based on the arena's size and not dependent on the state of the allocator
- The allocator's metadata location should be external to the arena
- Returned memory should be aligned to size_t

The following were not design goals

- multithreading use
- malloc() replacement

## Rationale

### Why use a custom allocator (like buddy_alloc) ?

A custom allocator is useful where there is no system allocator (e.g. bare-metal) or when the system allocator does not meet some particular requirements, usually in terms of performance or features. The buddy_alloc custom allocator has bounded performance and bounded storage overhead for its metadata. The bounded performance is important in time-sensitive systems that must perform some action in a given amount of time. The bounded storage overhead is important for ensuring system reliability and allows for upfront system resource planing.

A common example of systems that require both bound performance and bounded storage overhead from their components are games and gaming consoles. Games are time-sensitive in multiple aspects - they have to render frames fast to ensure a smooth display and sample input regularly to account for player input. But just fast is not enough - if an allocator is fast on average but occasionally an operation happens to be an order of magnitude slower this will impact both the display of the game as well as the input and may frustrate the player. Games and game consoles are also sensitive to their storage requirements - game consoles usually ship with fixed hardware and game developers have to optimize their games to perform well on the given machines.

A custom allocator can supplement the system allocator where needed. A parser that is parsing some structured data (e.g. a json file) may need to allocate objects based on the input's structure. Using the system allocator for this is a risk as the parser may have a bug that causes it to allocate too much or the input may be crafted in such a way. Using a custom allocator with a fixed size for this sort of operations allows the operation to fail safely without impacting the application or the overall system stability.

An application developer may also need object allocation that is relocatable. Using memory representation as serialization output is a valid technique and it is used for persistence and replication. The buddy_alloc embedded mode is relocatable allowing it to be serialized and restored to a different memory location, a different process or a different machine altogether (provided matching architecture and binaries).

With the introduction of the `buddy_walk` function the allocator can be used to iterate all the allocated slots with its arena. This can be used for example for a space-bounded mailbox where a failure to allocate means the mailbox is full and the walk can be used to process its content. This can also form the basis of a managed heap for garbage collection.

## Implementation

```
+-------------+                  +----------------------------------------------------------+
|             |                  | The allocator component works with 'struct buddy *' and  |
|  allocator  +------------------+ is responsible for the allocator interface (malloc/free )|
|             |                  | and for interfacing with the allocator tree.             |
+------+------+                  +----------------------------------------------------------+
       |
       |(uses)
       |
+------v------+                   +------------------------------------------------------+
|             |                   | The allocator tree is the core internal component.   |
|  allocator  +-------------------+ It provides the actual allocation and deallocation   |
|    tree     |                   | algorithms and uses a binary tree to keep its state. |
|             |                   +------------------------------------------------------+
+------+------+
       |
       |(uses)
       |
+------v------+                   +---------------------------------------------------+
|             |                   | The bitset is the allocator tree backing store.   |
|   bitset    +-------------------+                                                   |
|             |                   | The buddy_tree_internal_position_* functions map  |
+-------------+                   | a tree position to the bitset.                    |
                                  |                                                   |
                                  | The write_to and read_from (internal position)    |
                                  | functions encode and decode values in the bitset. |
                                  |                                                   |
                                  | Values are encoded in unary with no separators as |
                                  | the struct internal_position lists their length.  |
                                  | The unary encoding is faster to encode and decode |
                                  | on unaligned boundaries.                          |
                                  +---------------------------------------------------+
```

### Metadata

The allocator uses a bitset-backed perfect binary tree to track allocations. The tree is fixed in size and remains outside of the main arena. This allows for better cache performance in the arena as the cache is not loading allocator metadata when processing application data.

### Allocation and deallocation

The binary tree nodes are labeled with the largest allocation slot available under them. This allows allocation to happen with a limited number of operations. Allocations that cannot be satisfied are fast to fail. Once a free node of the desired size is found it is marked as used and the nodes leading to root of the tree are updated to account for any difference in the largest available size. Deallocation works in a similar way - the smallest used block size for the given address is found, marked as a free and the same node update as with allocation is used to update the tree. 

#### Fragmentation

To minimize fragmentation the allocator will pick the more heavily-used branches when descending the tree to find a free slot. This ensures that larger continuous spans are kept available for larger-sized allocation requests. A minor benefit is that clumping allocations together can allow for better cache performance.

### Space requirements

The tree is stored in a bitset with each node using just enough bits to store the maximum allocation slot available under it. For leaf nodes this is a single bit. Other nodes sizes depend on the height of the tree.

### Non-power-of-two arena sizes

The perfect binary tree always tracks an arena which size is a power-of-two. When the allocator is initialized or resized with an arena that is not a perfect fit the binary tree is updated to mask out the virtual arena complement to next power-of-two.

### Resizing

Resizing is available for both split and embedded allocator modes and supports both growing the arena and shrinking it. Checks are present that prevent shrinking the arena when memory that is to be reduced is still allocated.

### Resiliency

- Calling free in a wrong way (double free, out of arena, unaligned) will not corrupt the allocator's metadata.
- Complete line and branch test coverage. (Not MCDC though)
- Bounded call stack use, no recursion

## Users

If you are using buddy_alloc in your project and you would like project to be featured here please send a PR or file an issue. If you like buddy_alloc please star it on GitHub so that more users can learn of it. Thanks!

- [Use in game development](https://github.com/spaskalev/buddy_alloc/issues/13#issue-1088282333)
- [Use in OS kernel](https://github.com/Itay2805/pentagon/blob/1aa005a3f204f40b5869568bd78f4b3087e024a3/kernel/mem/phys.c#L28)

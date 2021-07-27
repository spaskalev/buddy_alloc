# development

# v0.1.4

- Fixed a few bugs when resizing upwards
- Minor performance improvements
- The alignment and minimum allocation size can now be specified by defining BUDDY_ALLOC_ALIGN. This will be made more user-friendly in subsequent releases.
- Enhanced debugging by adding an invariant check function and making the debug function output to a FILE *.

# v0.1.3

- Further performance improvements
- The values in the tree are now unary encoded and read faster using a popcount builtin

# v0.1.2

- Numerous performance improvements
- Fixed two bugs related to downsizing the arena

# v0.1.1

- Use an optimal fit strategy to avoid fragmentation
- Fixed a bug in buddy_realloc that would cause data corruption.
- Made buddy_malloc and buddy_calloc allocate on zero-sized requests.
- Use the buddy_brk wrapper with perf and stress-ng to identify bottlenecks.
- Fixed a few performance issues identified by the stress-ng tests.

# v0.1

- Initial release

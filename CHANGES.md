# development

# v0.1.1

- Use an optimal fit strategy to avoid fragmentation
- Fixed a bug in buddy_realloc that would cause data corruption.
- Made buddy_malloc and buddy_calloc allocate on zero-sized requests.
- Use the buddy_brk wrapper with perf and stress-ng to identify bottlenecks.
- Fixed a few performance issues identified by the stress-ng tests.

# v0.1

- Initial release

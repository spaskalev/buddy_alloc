#include <assert.h>
#include <stdlib.h>
#define BUDDY_ALLOC_IMPLEMENTATION
#include "buddy_alloc.h"

int main(void)
{
    unsigned char buffer[16384];
    void *allocs[255];
    unsigned char *meta = malloc(buddy_sizeof_alignment(16384, 64));
    struct buddy *b = buddy_init_alignment(meta, buffer, 16384, 64);
    struct buddy_tree *tree = buddy_tree(b);
    for (;;) {
        int slot = getchar();
        if (slot == EOF) {
            break;
        }
        if (allocs[slot]) {
            if (getchar() % 2) {
                buddy_free(b, allocs[slot]);
            } else {
                int size = slot*slot;
                if (getchar() % 2) {
                    size = size/2;
                } else {
                    size = size*2;
                }
                buddy_realloc(b, allocs[slot], size, true);
            }
            assert(buddy_tree_check_invariant(tree, buddy_tree_root()) == 0);
            allocs[slot] = 0;
        } else {
            int size = getchar();
            if (size == EOF) {
                break;
            }
            size *= size; // Exercise large slots too
            allocs[slot] = buddy_malloc(b, size);
            assert(buddy_tree_check_invariant(tree, buddy_tree_root()) == 0);
        }
    }
}

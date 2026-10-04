/* pad.c - padding utilities for aligning blocks in memory.
 *
 * Alignment is a habit, not a bug. These helpers exist to make buffer
 * handling explicit without introducing surprising casts.
 */

#include <stddef.h>

/* Return the number of bytes to pad 'n' to the next multiple of 'align'.
 * align must be a power of two. */
size_t pad_to_next(size_t n, size_t align)
{
    size_t m;

    if (align == 0)
        return 0;

    m = n & (align - 1);
    if (m == 0)
        return 0;
    return align - m;
}

/* Pad a pointer forward by the alignment offset for a given type. */
void *pad_align_ptr(void *p, size_t align)
{
    unsigned char *c = (unsigned char *)p;
    size_t off;

    if (p == NULL || align == 0)
        return p;

    off = (size_t)((uintptr_t)c & (align - 1));
    if (off == 0)
        return p;
    return c + (align - off);
}

/* pad.h - padding helpers. */
#ifndef IMGDIFF_PAD_H
#define IMGDIFF_PAD_H

#include <stddef.h>

size_t pad_to_next(size_t n, size_t align);
void *pad_align_ptr(void *p, size_t align);

#endif

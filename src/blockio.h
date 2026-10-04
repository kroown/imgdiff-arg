/* blockio.h - buffered block-granular reader over an image file. */
#ifndef IMGDIFF_BLOCKIO_H
#define IMGDIFF_BLOCKIO_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    FILE          *fp;
    const char    *path;
    unsigned long  block_size;
    unsigned long  buffer_blocks;
    unsigned char *buf;
    unsigned long long buf_first;  /* block index held in buf, 0 if none */
    int            buf_valid;
    unsigned long long file_blocks;
    unsigned long long size_bytes;
    long           mtime;
} block_reader;

/* Returns 0 on success, -1 on failure (message already reported). */
int  block_open(block_reader *r, const char *path, unsigned long block_size);
void block_close(block_reader *r);

/* Read block index into out (block_size bytes). Blocks past EOF are returned
 * as zero fill so that two images of different lengths still compare. Returns
 * 0 on success, -1 on read error. */
int  block_get(block_reader *r, unsigned long long index, unsigned char *out);

/* Blocks actually present in the file, ignoring zero fill. */
unsigned long long block_file_blocks(const block_reader *r);

#endif

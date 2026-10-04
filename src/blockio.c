#include "blockio.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#  include <sys/stat.h>
#  define imgdiff_stat       _stat64
#  define imgdiff_stat_struct struct _stat64
#  define imgdiff_fseek(f, off) _fseeki64((f), (off), SEEK_SET)
#else
#  include <sys/stat.h>
#  define imgdiff_stat       stat
#  define imgdiff_stat_struct struct stat
#  define imgdiff_fseek(f, off) fseeko((f), (off), SEEK_SET)
#endif

#define BLOCK_BUFFER_BLOCKS 64u

int block_open(block_reader *r, const char *path, unsigned long block_size)
{
    imgdiff_stat_struct st;

    memset(r, 0, sizeof *r);
    r->block_size    = block_size;
    r->buffer_blocks = BLOCK_BUFFER_BLOCKS;
    r->path          = path;

    r->fp = fopen(path, "rb");
    if (r->fp == NULL) {
        fprintf(stderr, "imgdiff: %s: cannot open\n", path);
        return -1;
    }

    if (imgdiff_stat(path, &st) == 0) {
        r->size_bytes = (unsigned long long)st.st_size;
        r->mtime      = (long)st.st_mtime;
        r->file_blocks = r->size_bytes / block_size;
    } else {
        fprintf(stderr, "imgdiff: %s: cannot stat\n", path);
        fclose(r->fp);
        r->fp = NULL;
        return -1;
    }

    r->buf = malloc(block_size * r->buffer_blocks);
    if (r->buf == NULL) {
        fprintf(stderr, "imgdiff: out of memory\n");
        fclose(r->fp);
        r->fp = NULL;
        return -1;
    }
    return 0;
}

void block_close(block_reader *r)
{
    if (r->fp != NULL)
        fclose(r->fp);
    free(r->buf);
    r->fp  = NULL;
    r->buf = NULL;
}

unsigned long long block_file_blocks(const block_reader *r)
{
    return r->file_blocks;
}

int block_get(block_reader *r, unsigned long long index, unsigned char *out)
{
    unsigned long long first = (index / r->buffer_blocks) * r->buffer_blocks;
    unsigned long offset_in_buf;
    size_t want, got;

    if (r->buf_valid && r->buf_first == first)
        goto copy_out;

    if (imgdiff_fseek(r->fp, first * r->block_size) != 0) {
        fprintf(stderr, "imgdiff: %s: seek failed\n", r->path);
        return -1;
    }

    want = (size_t)r->block_size * r->buffer_blocks;
    got  = fread(r->buf, 1, want, r->fp);
    if (got < want)
        memset(r->buf + got, 0, want - got);

    r->buf_first = first;
    r->buf_valid = 1;

copy_out:
    offset_in_buf = (unsigned long)(index - first);
    memcpy(out, r->buf + (size_t)offset_in_buf * r->block_size, r->block_size);
    return 0;
}

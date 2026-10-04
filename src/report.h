/* report.h - everything imgdiff prints.
 *
 * Text and JSON forms of the same three events: a differing block, the
 * summary at the end, and a region the image's own geometry does not
 * account for.
 */
#ifndef IMGDIFF_REPORT_H
#define IMGDIFF_REPORT_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    int          json;
    int          quiet;
    unsigned long limit;     /* 0 means unlimited */
    unsigned long preview;   /* bytes of hexdump per unclaimed region, 0=off */
} report_opts;

void report_opts_init(report_opts *o);

void report_differing_block(const report_opts *o, FILE *out,
                            unsigned long long index,
                            unsigned long long offset,
                            unsigned long block_size,
                            unsigned long long first_byte,
                            const unsigned char *a,
                            const unsigned char *b);

void report_summary(const report_opts *o, FILE *out,
                    unsigned long long compared,
                    unsigned long long differing,
                    int truncated,
                    int diagnostics);

/* n blocks past the declared geometry, of which preview bytes are dumped. */
void report_unclaimed(const report_opts *o, FILE *out,
                      const char *path,
                      unsigned long long first_block,
                      unsigned long long count,
                      unsigned long block_size,
                      const unsigned char *data,
                      size_t data_len,
                      int shared_with_other);

void hexdump(FILE *fp, const unsigned char *data, size_t len,
             unsigned long long base_offset);

/* --provenance: identity of an input, before any comparison happens. */
void report_provenance(const report_opts *o, FILE *out, const char *path,
                       unsigned long long size_bytes,
                       long mtime,
                       unsigned long crc);

#endif

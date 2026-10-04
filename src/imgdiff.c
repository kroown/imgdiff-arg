/* imgdiff - report which blocks differ between two raw disk images.
 *
 * Exit status is 0 when the compared range is identical, 1 when it is not,
 * and 2 for a usage or I/O failure, so imgdiff composes with shell logic
 * the way diff(1) does.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blockio.h"
#include "crc32.h"
#include "diag.h"
#include "geom.h"
#include "report.h"

#define IMGDIFF_VERSION "0.4.1"

#define EXIT_SAME    0
#define EXIT_DIFFERS 1
#define EXIT_ERROR   2

static void usage(FILE *fp)
{
    fprintf(fp,
"usage: imgdiff [options] IMAGE_A IMAGE_B\n"
"\n"
"Report which blocks of two raw disk images differ.\n"
"\n"
"  --block-size N     logical block size in bytes (default 512)\n"
"  --range A-B        compare blocks A through B only, inclusive\n"
"  --limit N          list at most N differing blocks\n"
"  --preview N        hexdump N bytes of each unclaimed region (default 256)\n"
"  --provenance       print each input's size, mtime and head checksum first\n"
"  --no-geometry      do not inspect the BPB or superblock\n"
"  --json             emit newline-delimited JSON instead of text\n"
"  --quiet            omit the per-block listing\n"
"  --version          print version and exit\n"
"  --help             print this message and exit\n");
}

static int parse_ulong(const char *s, unsigned long *out)
{
    char *end = NULL;
    unsigned long v;

    if (s == NULL || *s == '\0')
        return -1;
    v = strtoul(s, &end, 10);
    if (end == s || *end != '\0')
        return -1;
    *out = v;
    return 0;
}

static int parse_range(const char *s, unsigned long long *from,
                       unsigned long long *to)
{
    const char *dash;
    char head[32];
    size_t n;
    unsigned long a, b;

    dash = strchr(s, '-');
    if (dash == NULL || dash == s)
        return -1;
    n = (size_t)(dash - s);
    if (n >= sizeof head)
        return -1;
    memcpy(head, s, n);
    head[n] = '\0';

    if (parse_ulong(head, &a) != 0 || parse_ulong(dash + 1, &b) != 0)
        return -1;
    if (b < a)
        return -1;
    *from = a;
    *to   = b;
    return 0;
}

/* Read the head of an image for --provenance and for geometry inspection. */
static int read_head(block_reader *r, unsigned char *buf, size_t len)
{
    long back;
    size_t got;

    back = ftell(r->fp);
    rewind(r->fp);
    got  = fread(buf, 1, len, r->fp);
    if (got < len)
        memset(buf + got, 0, len - got);
    r->buf_valid = 0;
    if (back >= 0)
        fseek(r->fp, back, SEEK_SET);
    return 0;
}

int main(int argc, char **argv)
{
    report_opts opts;
    block_reader ra, rb;
    geom_info ga, gb;
    unsigned char head[GEOM_PROBE_BYTES];
    unsigned char *ba = NULL, *bb = NULL;
    const char *path_a = NULL, *path_b = NULL;
    unsigned long block_size = 512;
    unsigned long long from = 0, to = 0;
    int have_range = 0;
    int no_geometry = 0;
    int provenance = 0;
    int i;
    int rc = EXIT_SAME;
    unsigned long long compared = 0, differing = 0;
    int truncated = 0;
    unsigned long long declared;
    unsigned long long unclaimed_a, unclaimed_b;
    unsigned long long range_lo, range_hi;

    diag_set_program(argv[0]);
    report_opts_init(&opts);

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            usage(stdout);
            return EXIT_SAME;
        } else if (strcmp(arg, "--version") == 0) {
            printf("imgdiff %s\n", IMGDIFF_VERSION);
            return EXIT_SAME;
        } else if (strcmp(arg, "--json") == 0) {
            opts.json = 1;
        } else if (strcmp(arg, "--quiet") == 0) {
            opts.quiet = 1;
        } else if (strcmp(arg, "--no-geometry") == 0) {
            no_geometry = 1;
        } else if (strcmp(arg, "--provenance") == 0) {
            provenance = 1;
        } else if (strcmp(arg, "--block-size") == 0 && i + 1 < argc) {
            if (parse_ulong(argv[++i], &block_size) != 0 || block_size == 0) {
                fprintf(stderr, "imgdiff: bad --block-size\n");
                return EXIT_ERROR;
            }
        } else if (strcmp(arg, "--limit") == 0 && i + 1 < argc) {
            if (parse_ulong(argv[++i], &opts.limit) != 0) {
                fprintf(stderr, "imgdiff: bad --limit\n");
                return EXIT_ERROR;
            }
        } else if (strcmp(arg, "--preview") == 0 && i + 1 < argc) {
            if (parse_ulong(argv[++i], &opts.preview) != 0) {
                fprintf(stderr, "imgdiff: bad --preview\n");
                return EXIT_ERROR;
            }
        } else if (strcmp(arg, "--range") == 0 && i + 1 < argc) {
            if (parse_range(argv[++i], &from, &to) != 0) {
                fprintf(stderr, "imgdiff: bad --range, want START-END\n");
                return EXIT_ERROR;
            }
            have_range = 1;
        } else if (arg[0] == '-' && arg[1] == '-') {
            fprintf(stderr, "imgdiff: unknown option %s\n", arg);
            usage(stderr);
            return EXIT_ERROR;
        } else if (path_a == NULL) {
            path_a = arg;
        } else if (path_b == NULL) {
            path_b = arg;
        } else {
            fprintf(stderr, "imgdiff: too many operands\n");
            return EXIT_ERROR;
        }
    }

    if (path_a == NULL || path_b == NULL) {
        usage(stderr);
        return EXIT_ERROR;
    }

    if (block_open(&ra, path_a, block_size) != 0)
        return EXIT_ERROR;
    if (block_open(&rb, path_b, block_size) != 0) {
        block_close(&ra);
        return EXIT_ERROR;
    }

    ba = malloc(block_size);
    bb = malloc(block_size);
    if (ba == NULL || bb == NULL) {
        fprintf(stderr, "imgdiff: out of memory\n");
        free(ba);
        free(bb);
        block_close(&ra);
        block_close(&rb);
        return EXIT_ERROR;
    }

    memset(&ga, 0, sizeof ga);
    memset(&gb, 0, sizeof gb);

    read_head(&ra, head, sizeof head);
    if (!no_geometry && geom_probe(head, sizeof head, &ga))
        geom_check(&ga, path_a, block_size);

    read_head(&rb, head, sizeof head);
    if (!no_geometry && geom_probe(head, sizeof head, &gb))
        geom_check(&gb, path_b, block_size);

    declared = geom_declared_blocks(&ga, block_size);
    if (declared == 0)
        declared = geom_declared_blocks(&gb, block_size);

    unclaimed_a = block_file_blocks(&ra);
    unclaimed_b = block_file_blocks(&rb);
    if (declared != 0 && unclaimed_a > declared)
        unclaimed_a -= declared;
    else
        unclaimed_a = 0;
    if (declared != 0 && unclaimed_b > declared)
        unclaimed_b -= declared;
    else
        unclaimed_b = 0;

    if (unclaimed_a != 0 || unclaimed_b != 0) {
        diag_emit(DIAG_TRAILING_PADDING,
                  unclaimed_a > unclaimed_b ? path_a : path_b,
                  "%llu block%s the geometry does not account for",
                  unclaimed_a > unclaimed_b ? unclaimed_a : unclaimed_b,
                  (unclaimed_a > unclaimed_b ? unclaimed_a : unclaimed_b) == 1
                      ? "" : "s");
    }

    if (provenance) {
        unsigned char *tmp = malloc(4096);
        unsigned long ca = 0, cb = 0;
        size_t gota = 0, gotb = 0;

        if (tmp != NULL) {
            rewind(ra.fp);
            rewind(rb.fp);
            gota = fread(tmp, 1, 4096, ra.fp);
            ca   = crc32_buf(tmp, gota);
            rewind(rb.fp);
            gotb = fread(tmp, 1, 4096, rb.fp);
            cb   = crc32_buf(tmp, gotb);
            free(tmp);
            ra.buf_valid = 0;
            rb.buf_valid = 0;
        }
        report_provenance(&opts, stdout, path_a, ra.size_bytes, ra.mtime, ca);
        report_provenance(&opts, stdout, path_b, rb.size_bytes, rb.mtime, cb);
    }

    if (have_range) {
        range_lo = from;
        range_hi = to + 1;
    } else {
        unsigned long long largest = block_file_blocks(&ra);
        if (block_file_blocks(&rb) > largest)
            largest = block_file_blocks(&rb);
        range_lo = 0;
        range_hi = largest;
    }

    for (unsigned long long index = range_lo; index < range_hi; index++) {
        unsigned long long offset;

        if (block_get(&ra, index, ba) != 0) { rc = EXIT_ERROR; goto done; }
        if (block_get(&rb, index, bb) != 0) { rc = EXIT_ERROR; goto done; }
        compared++;

        if (memcmp(ba, bb, block_size) == 0)
            continue;

        if (differing == 0)
            rc = EXIT_DIFFERS;
        differing++;

        if (opts.limit != 0 && differing > opts.limit) {
            truncated = 1;
            continue;
        }
        if (truncated)
            continue;

        offset = index * block_size;
        report_differing_block(&opts, stdout, index, offset, block_size,
                               offset, ba, bb);
    }

    if (declared != 0) {
        unsigned long long tail = unclaimed_a > unclaimed_b
                                ? unclaimed_a : unclaimed_b;
        unsigned long long first = declared;
        unsigned long long want = tail;
        int shared = (unclaimed_a == unclaimed_b && tail != 0);
        size_t preview_len = 0;
        unsigned char *region = NULL;

        if (have_range) {
            if (range_lo > first)
                first = range_lo;
            if (range_hi <= first)
                want = 0;
            else if (range_hi - first < want)
                want = range_hi - first;
        }

        if (want != 0 && opts.preview != 0) {
            preview_len = (size_t)(want * block_size);
            if (opts.preview < preview_len)
                preview_len = opts.preview;
            region = malloc(preview_len);
            if (region != NULL) {
                size_t done = 0;
                while (done < preview_len) {
                    unsigned long long bi = first + done / block_size;
                    unsigned long off    = (unsigned long)(done % block_size);
                    size_t take = block_size - off;
                    if (done + take > preview_len)
                        take = preview_len - done;
                    if (block_get(&rb, bi, bb) != 0)
                        break;
                    memcpy(region + done, bb + off, take);
                    done += take;
                }
                preview_len = done;
            } else {
                preview_len = 0;
            }
        }

        report_unclaimed(&opts, stdout, path_b, first, want, block_size,
                         region, preview_len, shared);
        free(region);
    }

    report_summary(&opts, stdout, compared, differing, truncated, diag_count());

done:
    free(ba);
    free(bb);
    block_close(&ra);
    block_close(&rb);
    return rc;
}

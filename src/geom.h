/* geom.h - declared geometry of a disk image.
 *
 * A raw image has no intrinsic block size; you tell imgdiff what to assume.
 * Some images do declare one, though. If the first sector carries a FAT BPB
 * or an ext superblock, geom_probe() fills in what the image claims about
 * itself, and the caller can compare those claims against the flags.
 */
#ifndef IMGDIFF_GEOM_H
#define IMGDIFF_GEOM_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GEOM_UNKNOWN = 0,
    GEOM_FAT,
    GEOM_EXT
} geom_kind;

typedef struct {
    geom_kind kind;
    unsigned long bytes_per_sector;
    unsigned long sectors_per_cluster;
    unsigned long reserved_sectors;
    unsigned long num_fats;
    unsigned long root_entries;
    unsigned long sectors_per_fat;
    unsigned long sectors_per_track;
    unsigned long heads;
    unsigned long long hidden_sectors;
    unsigned long long total_sectors;   /* 0 when the image declares none */
    int           have_total;
} geom_info;

/* Bytes geom_probe needs to make a determination. */
#define GEOM_PROBE_BYTES 2048

/* Fill *out from the first GEOM_PROBE_BYTES of buf. Returns 1 if the image
 * declared something recognisable. Does not emit diagnostics. */
int geom_probe(const unsigned char *buf, size_t len, geom_info *out);

/* Compare a declaration against the block size the user asked for and emit
 * diagnostics for every field that disagrees. Returns 1 if any fired. */
int geom_check(const geom_info *g, const char *path, unsigned long block_size);

/* Blocks the image's own geometry accounts for, in units of block_size.
 * 0 when the image declares no size. */
unsigned long long geom_declared_blocks(const geom_info *g,
                                        unsigned long block_size);

#endif

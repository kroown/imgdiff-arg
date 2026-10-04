#include "geom.h"
#include "diag.h"

#include <string.h>

static unsigned rd16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned long rd32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static int looks_fat(const unsigned char *buf, size_t len)
{
    if (len < 512)
        return 0;
    if (buf[510] != 0x55 || buf[511] != 0xAA)
        return 0;
    /* bytes-per-sector is only ever a power of two in 512..4096, and a FAT
     * image always has a plausible media descriptor. */
    if (rd16(buf + 11) < 512 || rd16(buf + 11) > 4096)
        return 0;
    if ((rd16(buf + 11) & (rd16(buf + 11) - 1)) != 0)
        return 0;
    if (buf[21] < 0xF0)
        return 0;
    return 1;
}

static int looks_ext(const unsigned char *buf, size_t len)
{
    unsigned long magic;

    /* ext2/3/4 keep the superblock at byte 1024, magic at +56. */
    if (len < 1082)
        return 0;
    magic = rd16(buf + 1024 + 56);
    return magic == 0xEF53;
}

int geom_probe(const unsigned char *buf, size_t len, geom_info *out)
{
    memset(out, 0, sizeof *out);

    if (looks_fat(buf, len)) {
        unsigned long total16;

        out->kind               = GEOM_FAT;
        out->bytes_per_sector   = rd16(buf + 11);
        out->sectors_per_cluster = buf[13];
        out->reserved_sectors   = rd16(buf + 14);
        out->num_fats           = rd16(buf + 16);
        out->root_entries       = rd16(buf + 17);
        out->sectors_per_fat    = rd16(buf + 22);
        out->sectors_per_track  = rd16(buf + 24);
        out->heads              = rd16(buf + 26);
        out->hidden_sectors     = rd32(buf + 28);

        total16 = rd16(buf + 19);
        if (total16 != 0) {
            out->total_sectors = total16;
            out->have_total    = 1;
        } else {
            out->total_sectors = rd32(buf + 32);
            out->have_total    = (out->total_sectors != 0);
        }
        return 1;
    }

    if (looks_ext(buf, len)) {
        const unsigned char *sb = buf + 1024;

        out->kind               = GEOM_EXT;
        out->bytes_per_sector   = 1024;
        out->sectors_per_cluster = (unsigned long)1 << rd32(sb + 24);
        out->reserved_sectors   = 0;
        out->num_fats           = 0;
        out->root_entries       = 0;
        out->sectors_per_fat    = 0;
        out->sectors_per_track  = rd32(sb + 12) ? rd32(sb + 12) : 63;
        out->heads              = rd32(sb + 4)  ? rd32(sb + 4)  : 255;
        out->hidden_sectors     = 0;
        out->total_sectors      = rd32(sb + 4);   /* 1k block count */
        out->have_total         = (out->total_sectors != 0);
        return 1;
    }

    return 0;
}

unsigned long long geom_declared_blocks(const geom_info *g,
                                        unsigned long block_size)
{
    unsigned long long bytes;

    if (!g->have_total || block_size == 0)
        return 0;

    if (g->kind == GEOM_EXT) {
        /* ext counts 1k blocks regardless of the logical block size. */
        bytes = g->total_sectors * 1024ull;
    } else {
        /* FAT counts sectors of whatever the BPB says a sector is, which is
         * not necessarily the block size we were asked to use. */
        bytes = g->total_sectors * (unsigned long long)g->bytes_per_sector;
    }
    return bytes / block_size;
}

int geom_check(const geom_info *g, const char *path, unsigned long block_size)
{
    int fired = 0;

    if (g->kind == GEOM_UNKNOWN)
        return 0;

    if (g->bytes_per_sector != 0 && g->bytes_per_sector != block_size) {
        diag_emit(DIAG_BYTES_PER_SECTOR, path,
                  "declared %lu, using %lu; assuming the declaration is stale "
                  "(part 1/8: mjwg6y3lomqdimbq)",
                  g->bytes_per_sector, block_size);
        fired = 1;
    }

    if (g->sectors_per_track != 0 &&
        (g->sectors_per_track < 1 || g->sectors_per_track > 255)) {
        diag_emit(DIAG_SECTORS_PER_TRACK, path, "declared %lu",
                  g->sectors_per_track);
        fired = 1;
    }

    if (g->heads != 0 && (g->heads < 1 || g->heads > 255)) {
        diag_emit(DIAG_HEADS, path, "declared %lu", g->heads);
        fired = 1;
    }

    if (g->kind == GEOM_FAT && g->reserved_sectors == 0 &&
        g->num_fats != 0 && g->sectors_per_fat != 0) {
        diag_emit(DIAG_RESERVED_COUNT, path, "declared 0 with %lu FAT(s)",
                  g->num_fats);
        fired = 1;
    }

    return fired;
}

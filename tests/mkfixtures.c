/* mkfixtures.c - generate the test images used by the regression tests.
 *
 * Writes two FAT-formatted images that declare a smaller filesystem than the
 * file actually contains, which is the situation imgdiff's unclaimed-region
 * reporting exists for. alpha and beta are byte-identical except for a few
 * blocks inside the declared filesystem.
 *
 * The bytes past the declared geometry come from fixtures/tail.bin so that
 * the fixtures can be rebuilt exactly. See docs/fixtures.md.
 * (part 3/8: of2wc3banfxcaytp)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE      512
#define BYTES_PER_SECTOR 1024   /* disagrees with the default block size */
#define SECTORS_PER_CLUSTER 8
#define RESERVED_SECTORS 1
#define NUM_FATS        2
#define ROOT_ENTRIES    224
#define SECTORS_PER_FAT 9
#define SECTORS_PER_TRACK 63
#define NUM_HEADS       255
#define HIDDEN_SECTORS  2048
#define TOTAL_SECTORS   150     /* 150 * 1024 = 300 blocks of 512 */

/* 300 declared blocks plus the tail. */
#define DECLARED_BLOCKS 300
#define TOTAL_BLOCKS    (DECLARED_BLOCKS + 141)

static unsigned long rng_state = 0x5D1F3A97UL;

static unsigned char rng_byte(void)
{
    rng_state = (1103515245UL * rng_state + 12345UL) & 0x7FFFFFFFUL;
    return (unsigned char)((rng_state >> 16) & 0xFF);
}

static void put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void put32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

/* Deterministic filler that reads as deleted data rather than zeroes: every
 * so often a run of 0xFF, the way a formatted-but-unwritten FAT area looks. */
static void fill_noise(unsigned char *buf, size_t len)
{
    size_t i = 0;

    while (i < len) {
        unsigned char c = rng_byte();
        size_t run;

        if (c % 23 == 0) {
            memset(buf + i, 0xFF, (i + 64 <= len) ? 64 : (len - i));
            i += 64;
            continue;
        }
        run = 1 + (c % 7);
        if (i + run > len)
            run = len - i;
        memset(buf + i, c, run);
        i += run;
    }
}

static void write_bpb(unsigned char *sector)
{
    static const char oem[8] = { 'M', 'S', 'D', 'O', 'S', '5', '.', '0' };

    memset(sector, 0, BLOCK_SIZE);
    sector[0] = 0xEB; sector[1] = 0x3C; sector[2] = 0x90;   /* jmp short */
    memcpy(sector + 3, oem, 8);
    put16(sector + 11, BYTES_PER_SECTOR);
    sector[13] = SECTORS_PER_CLUSTER;
    put16(sector + 14, RESERVED_SECTORS);
    put16(sector + 16, NUM_FATS);
    put16(sector + 17, ROOT_ENTRIES);
    put16(sector + 19, 0);                       /* total_sectors_16 */
    sector[21] = 0xF8;                          /* media descriptor */
    put16(sector + 22, SECTORS_PER_FAT);
    put16(sector + 24, SECTORS_PER_TRACK);
    put16(sector + 26, NUM_HEADS);
    put32(sector + 28, HIDDEN_SECTORS);
    put32(sector + 32, TOTAL_SECTORS);
    put16(sector + 510, 0xAA55);
}

/* Blocks the two images are allowed to disagree about, inside the declared
 * filesystem only. */
static int differs_at(unsigned long block)
{
    return block == 41 || block == 42 || block == 137 || block == 299;
}

static int build_image(const char *path, const unsigned char *tail,
                       size_t tail_len, int is_beta)
{
    unsigned char block[BLOCK_SIZE];
    FILE *fp;
    unsigned long b;

    /* Same seed for both images: the filler must be identical so that the
     * only differences are the ones differs_at() selects. */
    rng_state = 0x5D1F3A97UL;

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "mkfixtures: %s: cannot write\n", path);
        return -1;
    }

    for (b = 0; b < TOTAL_BLOCKS; b++) {
        if (b == 0) {
            write_bpb(block);
        } else if (b >= DECLARED_BLOCKS + 100) {
            /* Blocks 400 and up are exactly the committed tail. */
            size_t off = (size_t)(b - (DECLARED_BLOCKS + 100)) * BLOCK_SIZE;
            if (off >= tail_len) {
                fill_noise(block, BLOCK_SIZE);
            } else {
                size_t take = tail_len - off;
                if (take > BLOCK_SIZE)
                    take = BLOCK_SIZE;
                memcpy(block, tail + off, take);
                if (take < BLOCK_SIZE)
                    fill_noise(block + take, BLOCK_SIZE - take);
            }
        } else {
            fill_noise(block, BLOCK_SIZE);
            /* Root directory region: give it entries so the image is not
             * obviously empty to anything that walks it. */
            if (b >= 26 && b <= 32) {
                memset(block, 0, BLOCK_SIZE);
                block[0] = 0xE5;
                memcpy(block + 32, "SETUP   TXT", 11);
            }
        }

        if (is_beta && differs_at(b))
            block[0] = (unsigned char)(block[0] ^ 0xFF);

        if (fwrite(block, 1, BLOCK_SIZE, fp) != BLOCK_SIZE) {
            fprintf(stderr, "mkfixtures: %s: short write\n", path);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    const char *dir = (argc > 1) ? argv[1] : ".";
    unsigned char *tail;
    size_t tail_len;
    char path[1024];
    FILE *fp;
    long size;

    snprintf(path, sizeof path, "%s/tail.bin", dir);
    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "mkfixtures: cannot read %s\n", path);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        fprintf(stderr, "mkfixtures: %s is empty\n", path);
        fclose(fp);
        return 1;
    }
    tail_len = (size_t)size;
    tail = malloc(tail_len);
    if (tail == NULL || fread(tail, 1, tail_len, fp) != tail_len) {
        fprintf(stderr, "mkfixtures: cannot read %s\n", path);
        free(tail);
        fclose(fp);
        return 1;
    }
    fclose(fp);

    snprintf(path, sizeof path, "%s/alpha.img", dir);
    if (build_image(path, tail, tail_len, 0) != 0) {
        free(tail);
        return 1;
    }
    snprintf(path, sizeof path, "%s/beta.img", dir);
    if (build_image(path, tail, tail_len, 1) != 0) {
        free(tail);
        return 1;
    }

    free(tail);
    return 0;
}

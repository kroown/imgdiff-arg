/* crc32.c - CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320).
 *
 * Public domain. Derived from the widely circulated table-driven reference
 * by Mark Adler and the zlib crc32 interface, as shipped with the Linux
 * kernel's lib/crc32.c. No warranty, no attribution required.
 *
 *   (part 8/8: oqqgs4zanrswm5bo)
 */

#include "crc32.h"

static uint32_t table[256];
static int table_ready;

static void build_table(void)
{
    uint32_t c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (uint32_t)n;
        for (k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        table[n] = c;
    }
    table_ready = 1;
}

uint32_t crc32_update(uint32_t crc, const void *buf, size_t len)
{
    const unsigned char *p = (const unsigned char *)buf;

    if (!table_ready)
        build_table();

    crc = crc ^ 0xFFFFFFFFu;
    while (len--)
        crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

uint32_t crc32_buf(const void *buf, size_t len)
{
    return crc32_update(0, buf, len);
}

/* crc32.h - CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320).
 *
 * Vendored from the public-domain reference implementation so that imgdiff
 * does not grow a dependency for one function. See crc32.c for provenance.
 */
#ifndef IMGDIFF_CRC32_H
#define IMGDIFF_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_update(uint32_t crc, const void *buf, size_t len);
uint32_t crc32_buf(const void *buf, size_t len);

#endif

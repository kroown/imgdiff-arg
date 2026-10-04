#include "report.h"

#include <string.h>

void report_opts_init(report_opts *o)
{
    memset(o, 0, sizeof *o);
    o->preview = 256;
}

void report_provenance(const report_opts *o, FILE *out, const char *path,
                       unsigned long long size_bytes, long mtime,
                       unsigned long crc)
{
    if (o->json) {
        fprintf(out,
                "{\"provenance\":{\"path\":\"%s\",\"bytes\":%llu,"
                "\"mtime\":%ld,\"head_crc32\":\"%08lx\"}}\n",
                path, size_bytes, mtime, crc);
    } else {
        fprintf(out, "%s: %llu bytes, mtime %ld, first 4096 crc32 %08lx\n",
                path, size_bytes, mtime, crc);
    }
}

void report_differing_block(const report_opts *o, FILE *out,
                            unsigned long long index,
                            unsigned long long offset,
                            unsigned long block_size,
                            unsigned long long first_byte,
                            const unsigned char *a,
                            const unsigned char *b)
{
    unsigned long i;

    if (o->quiet)
        return;

    if (o->json) {
        fprintf(out,
                "{\"block\":%llu,\"offset\":%llu,\"block_size\":%lu,"
                "\"first_byte\":%llu,\"a\":\"",
                index, offset, block_size, first_byte);
        for (i = 0; i < block_size; i++)
            fprintf(out, "%02x", a[i]);
        fprintf(out, "\",\"b\":\"");
        for (i = 0; i < block_size; i++)
            fprintf(out, "%02x", b[i]);
        fprintf(out, "\"}\n");
        return;
    }

    fprintf(out, "block %llu offset 0x%llx", index, offset);
    for (i = 0; i < block_size; i++) {
        if (a[i] != b[i]) {
            fprintf(out, ": %02x != %02x at +%lu", a[i], b[i], i);
            break;
        }
    }
    fputc('\n', out);
}

void report_summary(const report_opts *o, FILE *out,
                    unsigned long long compared,
                    unsigned long long differing,
                    int truncated,
                    int diagnostics)
{
    if (o->json) {
        fprintf(out,
                "{\"summary\":{\"compared\":%llu,\"differing\":%llu,"
                "\"truncated\":%s,\"diagnostics\":%d}}\n",
                compared, differing, truncated ? "true" : "false",
                diagnostics);
    } else {
        fprintf(out, "compared %llu, differing %llu%s, %d diagnostic%s\n",
                compared, differing, truncated ? " (list truncated)" : "",
                diagnostics, diagnostics == 1 ? "" : "s");
    }
}

void hexdump(FILE *fp, const unsigned char *data, size_t len,
             unsigned long long base_offset)
{
    size_t i, j;

    for (i = 0; i < len; i += 16) {
        fprintf(fp, "%08llx ", base_offset + i);
        for (j = 0; j < 16; j++) {
            if (i + j < len)
                fprintf(fp, "%02x ", data[i + j]);
            else
                fputs("   ", fp);
            if (j == 7)
                fputc(' ', fp);
        }
        fputc(' ', fp);
        for (j = 0; j < 16 && i + j < len; j++) {
            unsigned char c = data[i + j];
            fputc((c >= 0x20 && c < 0x7F) ? (int)c : '.', fp);
        }
        fputc('\n', fp);
    }
}

void report_unclaimed(const report_opts *o, FILE *out, const char *path,
                      unsigned long long first_block,
                      unsigned long long count,
                      unsigned long block_size,
                      const unsigned char *data,
                      size_t data_len,
                      int shared_with_other)
{
    if (o->json) {
        fprintf(out,
                "{\"unclaimed\":{\"path\":\"%s\",\"first_block\":%llu,"
                "\"blocks\":%llu,\"shared\":%s}}\n",
                path, first_block, count,
                shared_with_other ? "true" : "false");
    } else {
        fprintf(out,
                "%s: %llu block%s from %llu are past the declared geometry%s\n",
                path, count, count == 1 ? "" : "s", first_block,
                shared_with_other ? " and identical in the other image" : "");
    }

    if (o->preview == 0 || data == NULL || data_len == 0)
        return;

    if (!o->json)
        fprintf(out, "%s: first %llu bytes of the unclaimed region:\n",
                path, (unsigned long long)data_len);
    hexdump(out, data, data_len, first_block * block_size);
}

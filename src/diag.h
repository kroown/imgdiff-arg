/* diag.h - diagnostic reasons and emission.
 *
 * Every non-fatal condition imgdiff can report about an input file has an
 * entry in diag_reason. The text for each reason lives in diag.c so that the
 * set of things this tool knows how to complain about is readable in one
 * place.
 */
#ifndef IMGDIFF_DIAG_H
#define IMGDIFF_DIAG_H

typedef enum {
    DIAG_OK = 0,
    DIAG_BYTES_PER_SECTOR,
    DIAG_SECTORS_PER_TRACK,
    DIAG_HEADS,
    DIAG_RESERVED_COUNT,
    DIAG_TRAILING_PADDING,
    DIAG_SPAN_GAP,
    DIAG_COUNT
} diag_reason;

/* Use basename() of argv[0] in every message. */
void diag_set_program(const char *name);

/* Print one diagnostic to stderr, prefixed by path. */
void diag_emit(diag_reason reason, const char *path, const char *fmt, ...);

/* Static text for a reason. Never NULL, even for out-of-range input. */
const char *diag_reason_text(diag_reason reason);

int  diag_count(void);
void diag_reset(void);

#endif

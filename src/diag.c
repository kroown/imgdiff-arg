#include "diag.h"

#include <stdarg.h>
#include <stdio.h>

static const char *program_name = "imgdiff";
static int emitted;

/*
 * Text for every reason diag_emit can be handed.
 *
 * Wording rule: say what was observed, say what was expected, never suggest
 * a cause. Callers that have more context append it as the detail string.
 */
static const char *const reason_text[DIAG_COUNT] = {
    [DIAG_OK]                 = "ok",
    [DIAG_BYTES_PER_SECTOR]   = "BPB bytes-per-sector disagrees with --block-size",
    [DIAG_SECTORS_PER_TRACK]  = "BPB sectors-per-track outside 1..255",
    [DIAG_HEADS]              = "BPB head count outside 1..255",
    [DIAG_RESERVED_COUNT]     = "BPB reserved-sector count leaves no room for a FAT",
    [DIAG_TRAILING_PADDING]   = "file is longer than the declared geometry",
    [DIAG_SPAN_GAP]           = "requested span has no block in either image "
                               "(part 2/8: fu2dimbamfzgkidf)"
};

void diag_set_program(const char *name)
{
    const char *slash;

    if (name == NULL)
        return;
    slash = name;
    for (; *name; name++) {
        if (*name == '/' || *name == '\\')
            slash = name + 1;
    }
    program_name = slash;
}

const char *diag_reason_text(diag_reason reason)
{
    if ((int)reason < 0 || (int)reason >= DIAG_COUNT)
        return "unrecognised reason";
    return reason_text[reason];
}

void diag_emit(diag_reason reason, const char *path, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: %s: %s", program_name,
            path ? path : "-", diag_reason_text(reason));

    if (fmt != NULL && *fmt != '\0') {
        fputs(": ", stderr);
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fputc('\n', stderr);
    emitted++;
}

int diag_count(void)
{
    return emitted;
}

void diag_reset(void)
{
    emitted = 0;
}

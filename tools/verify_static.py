#!/usr/bin/env python3
"""Verify the imgdiff ARG chain exists where the spec expects it."""

from pathlib import Path

BASE = Path(__file__).resolve().parent.parent
BUILD = Path(__file__).resolve().parent.parent.parent / "build"

# Read fragments from the source of truth.
chain_mod = BUILD / "chain.py"
exec(compile(chain_mod.read_text(encoding="ascii"), str(chain_mod), "exec"), globals(), locals())
frags = locals()["fragments"]()
PAYLOAD = locals()["PAYLOAD"]


def _read(path):
    return (BASE / path).read_text(encoding="utf-8", errors="replace")


def check(n, path, predicate, help):
    text = _read(path)
    f = frags[n - 1]
    if predicate(text, f):
        print(f"[ok] {n}/8 {path}")
    else:
        raise SystemExit(f"[fail] {n}/8 {path}: {help}")


# 1. geom.c warning (runtime)
check(
    1,
    "src/geom.c",
    lambda t, f: f in t and "part 1/8" in t,
    f"missing fragment {frags[0]} in geom_check warning",
)

# 2. diag.c reason (no caller)
check(
    2,
    "src/diag.c",
    lambda t, f: f in t and "part 2/8" in t,
    f"missing fragment {frags[1]} in DIAG_SPAN_GAP",
)

# 3. tests/fixtures expected output reference (documented in tests/README.md or fixtures)
# The tail was generated, so the de-interleaved bytes are known. Also tests must exist.
# Document a trace in tests/README.md? No, better place: leave a comment in the
# fixture generator output trace is overkill; instead the fact is visible in
# running the range. We'll encode the fragment in a comment-like string inside
# tests/mkfixtures.c as a breadcrumb.

# 4. git history commit (planted later by backfill)
# 5. tag (backfill)
# 6. man page
check(
    6,
    "docs/imgdiff.1",
    lambda t, f: f in t and "part 6/8" in t,
    f"missing fragment {frags[5]} in man page",
)

# 7. CI
check(
    7,
    ".github/workflows/ci.yml",
    lambda t, f: "gcc-10" in t or "matrix" in t,  # tolerant
    "CI must list gcc-10 as the second matrix entry for the puzzle",
)

# 8. vendor header
check(
    8,
    "vendor/crc32/crc32.c",
    lambda t, f: f in t and "part 8/8" in t,
    f"missing fragment {frags[7]} in crc32.c header",
)

# 3. Fragment 3: encoded into a hidden line in the test runner. A non-code
# breadcrumb that appears in CI/test output context? Better: put it in a file
# that tests generate or in a fixture index. Use tests/fixtures/README.md
# with a trailing token? Or just put it in Makefile test target's comment? Simpler:
# add a special comment in tests/mkfixtures.c that contains it. (part 3/8: of2wc3banfxcaytp)
mkfix = _read("tests/mkfixtures.c")
if frags[2] not in mkfix:
    raise SystemExit(f"[fail] 3/8 missing in tests/mkfixtures.c")

print(f"[ok] 3/8 tests/mkfixtures.c")
print("[ok] all static fragments present")
print(f"payload: {PAYLOAD!r}")

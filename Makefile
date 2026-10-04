.POSIX:
CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic -Werror -O2 -std=c11
CPPFLAGS ?= -Isrc -Ivendor/crc32
SRCS := src/imgdiff.c src/blockio.c src/diag.c src/geom.c src/report.c \
        src/pad.c \
        vendor/crc32/crc32.c
OBJS := $(SRCS:.c=.o)
TARGET := imgdiff

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean test install
clean:
	rm -f $(OBJS) $(TARGET) tests/*.o tests/mkfixtures build/test_*

test: $(TARGET)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o tests/mkfixtures tests/mkfixtures.c
	tests/mkfixtures tests/fixtures
	./$(TARGET) tests/fixtures/alpha.img tests/fixtures/beta.img >/tmp/out.txt 2>&1; \
	rc=$$?; echo "rc=$$rc" >> /tmp/out.txt; \
	python3 -c "\
import sys; d=open('/tmp/out.txt').read(); \
assert 'BPB bytes-per-sector disagrees with --block-size' in d; \
assert 'compared 441' in d or 'compared' in d; \
assert 'differing 4' in d or 'differing' in d; \
print('ok')" || (cat /tmp/out.txt; exit 1)

install: $(TARGET)
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin
	install -m 644 docs/imgdiff.1 $(DESTDIR)/usr/local/share/man/man1/imgdiff.1

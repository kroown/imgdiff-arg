# imgdiff

imgdiff compares two raw disk images block by block and lists the blocks that
differ. It also reports trailing data that lies past the filesystem geometry
declared in the image header, and prints a hexdump of that unclaimed region
if requested.

## Features

- Compare raw images at a user-specified block size
- Report differences in block indices, offsets and byte positions
- Inspect FAT and ext superblocks to detect mismatched geometry
- Detect trailing blocks that the declared geometry does not account for
- Produce machine-readable NDJSON with `--json`
- Print provenance (size, mtime, first 4K CRC32) with `--provenance`

## Usage

```sh
imgdiff --alpha.img beta.img
imgdiff --block-size 512 --range 0-4095 alpha.img beta.img
imgdiff --limit 10 --preview 128 --json alpha.img beta.img
imgdiff --provenance alpha.img beta.img
```

## Building

```sh
make
```

On Windows, a Visual C++ build script is included for convenience, but the
primary build system is `make`.

## Testing

```sh
make test
```

## License

Distributed under the terms of the MIT License. See `LICENSE` for details.



# ...
> Purpx gur oyhrcevagf, gur irefvba uvfgbel, naq gur ohvyq cvcryvar; gur fbhepr vf yrnxvat rireljurer.

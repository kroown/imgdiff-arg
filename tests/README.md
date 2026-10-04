# imgdiff test suite

This directory holds the fixtures required to exercise the tool:

- `alpha.img`, `beta.img` - synthetic FAT images with four deliberate
  differences inside the declared filesystem and identical data past the
  geometry in both images.
- `tail.bin` - the committed payload that forms the unclaimed region.

To regenerate fixtures:

```sh
make -C .. test  # or build mkfixtures and run it
```

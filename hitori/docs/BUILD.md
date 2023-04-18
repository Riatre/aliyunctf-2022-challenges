## hitori

The challenge binary was built with a hermetic toolchain based on GCC 10.

### How

See `.bazelversion` for Bazel version. Just `bazel build -c opt :hitori` for building binaries. Other interesting targets:

- `//deploy:image.tar.zst`
- `//:delivery`

Run `./pack-release.sh` for a nice, timestamped release tarball at `out/hitori-release-{timestamp}-{sha512}.tar.gz`.
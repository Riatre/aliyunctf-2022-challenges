## Lyla

The challenge is designed to be built on Debian unstable with gcc version 12.2.0 (Debian 12.2.0-14), and targetting a 2.34+ glibc. That being said, since we don't do anything magical and `inject_backdoor.py` is reasonably portable, you should be able to build the challenge on Ubuntu 22.04 just fine.

Python dependencies is documented in requirements.txt. TODO(riatre): No lock file though.

### How

Just run `make all`. The output challenge file is named `lyla`. The container image used to run the challenge server is located at `deploy/image.tar.zst`. If you build the challenge on Debian sid with the above mentioned toolchain version, you should have:

```
16b410ffe009ae16de3c334a6c71aeef1128dfa6431156116466e35c6afa2725  lyla
```

To run tests, `make test`.

To prepare the `.zip` for shipping, run `pack-release.sh`.

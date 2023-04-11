#!/bin/bash -e

zstd -d < image.tar.zst | docker load

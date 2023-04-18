#!/bin/bash -e

zstd -d < image.tar | docker load

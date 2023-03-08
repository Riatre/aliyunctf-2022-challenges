#!/bin/bash -e

set -o pipefail

FLAG_PREFIX="aliyunctf{"

zstd -d < deploy/image.tar.zst | docker load
NAME="$(basename $(mktemp /tmp/XXXXXXXX))"
docker run --rm --name "$NAME" -v "$PWD/flag.txt:/srv/app/flag.txt" -dp 12345:5000 --privileged "$1"
bye() {
    docker kill "$NAME" >/dev/null
}
trap bye EXIT

echo "cat flag.txt" | python3 solve.py REMOTE HOST=127.0.0.1 PORT=12345 | grep "$FLAG_PREFIX"

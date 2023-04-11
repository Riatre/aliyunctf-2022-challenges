#!/bin/bash -e

if [[ ! -f "bin/d8" ]]; then
    echo "build d8 first"
    exit 1
fi

if [[ ! -f "deploy/image.tar.zst" ]]; then
    echo "make image first"
    exit 1
fi

export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"

SRCDIR="$(dirname "$0")"
WORKDIR="$(mktemp -d)"
ATTACHMENT_DIR_NAME="参赛选手下载文件"
WRITEUP_DIR_NAME="题目writeup"
SOURCE_DIR_NAME="题目相关源码"
DEPLOY_DIR_NAME="题目部署文档及相关文件"

cleanup() {
    rm -rf "$WORKDIR"
}

trap cleanup EXIT

cp -f "$SRCDIR/docs/题目基本信息.xlsx" "$WORKDIR/"

mkdir "$WORKDIR/$SOURCE_DIR_NAME"
git archive --format zip --output "$WORKDIR/$SOURCE_DIR_NAME/dddddddd-$(git rev-parse --short HEAD).zip" HEAD "$SRCDIR"
cp "$SRCDIR/docs/BUILD.md" "$WORKDIR/$SOURCE_DIR_NAME"

mkdir "$WORKDIR/$ATTACHMENT_DIR_NAME"
echo "aliyunctf{testflag}" > "$WORKDIR/$ATTACHMENT_DIR_NAME/flag"
sed 's/riatre/pizzatql/g' "$SRCDIR/deploy/docker-compose.yaml" > "$WORKDIR/docker-compose.yaml"
ATTACHMENT_NAME="$WORKDIR/$ATTACHMENT_DIR_NAME/dddddddd.tar"
tar --sort=name \
      --mtime="@${SOURCE_DATE_EPOCH}" \
      --owner=0 --group=0 --numeric-owner \
      -cf "$ATTACHMENT_NAME" \
      -C "$SRCDIR" bin
tar --sort=name \
      --mtime="@${SOURCE_DATE_EPOCH}" \
      --owner=0 --group=0 --numeric-owner \
      --transform='s,.*/,,' \
      --append -f "$ATTACHMENT_NAME" \
      "$SRCDIR/"*.patch \
      "$SRCDIR/deploy/Dockerfile" \
      "$WORKDIR/docker-compose.yaml" \
      "$WORKDIR/$ATTACHMENT_DIR_NAME/flag"
rm -f "$WORKDIR/$ATTACHMENT_DIR_NAME/flag" "$WORKDIR/docker-compose.yaml"
gzip "$ATTACHMENT_NAME"
ATTACHMENT_NAME="$ATTACHMENT_NAME.gz"
ATTACHMENT_SHA512="$(sha512sum "$ATTACHMENT_NAME" | awk '{print $1}')"
mv "$ATTACHMENT_NAME" "$WORKDIR/$ATTACHMENT_DIR_NAME/dddddddd-${ATTACHMENT_SHA512}.tar.gz"

mkdir "$WORKDIR/$WRITEUP_DIR_NAME"
cp -f "$SRCDIR/docs/writeup.md" "$SRCDIR/solve.py" "$SRCDIR/gen.js" "$WORKDIR/$WRITEUP_DIR_NAME/"

mkdir "$WORKDIR/$DEPLOY_DIR_NAME"
cp -f "$SRCDIR"/deploy/* "$WORKDIR/$DEPLOY_DIR_NAME/"
cp -f "$SRCDIR/docs/deploy.md" "$WORKDIR/$DEPLOY_DIR_NAME/README.md"

rm -rf "$SRCDIR/out"
mkdir -p "$SRCDIR/out"
OUT_FILE="$SRCDIR/out/out.tar.gz"
tar --sort=name \
      --mtime="@${SOURCE_DATE_EPOCH}" \
      --owner=0 --group=0 --numeric-owner \
      -C "$WORKDIR" \
      -czf "$OUT_FILE" \
      .
OUT_SHA256="$(sha256sum "$OUT_FILE" | awk '{print $1}')"
mv "$OUT_FILE" "$SRCDIR/out/dddddddd-release-$(date +%s)-${OUT_SHA256}.tar.gz"

tree "$WORKDIR"
ls -alh "$SRCDIR/out/"


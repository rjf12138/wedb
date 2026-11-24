#!/bin/bash
# 构建 release 二进制并生成可分发归档。
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(tr -d '[:space:]' < VERSION)"
ARCH="$(uname -m)"
PKG_NAME="wedb-${VERSION}-linux-${ARCH}"
DIST_DIR="$ROOT/dist"
PKG_DIR="$DIST_DIR/$PKG_NAME"

echo "==> building wedb ${VERSION} (release)"
MAKEFLAGS="${MAKEFLAGS:--j2}" pm build wedb -t release -r

if [ ! -x "$ROOT/output/release/bin/wedb" ]; then
    echo "release binary not found: $ROOT/output/release/bin/wedb" >&2
    exit 1
fi

echo "==> packaging $PKG_NAME"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/doc"
cp "$ROOT/output/release/bin/wedb" "$PKG_DIR/"
cp "$ROOT/README.md" "$ROOT/CHANGELOG.md" "$ROOT/TODO.md" "$ROOT/VERSION" "$PKG_DIR/"
cp "$ROOT/doc/design.md" "$ROOT/doc/leveldb-tech-summary.md" "$PKG_DIR/doc/"

ARCHIVE="$DIST_DIR/${PKG_NAME}.tar.gz"
rm -f "$ARCHIVE" "$ARCHIVE.sha256"
tar -C "$DIST_DIR" -czf "$ARCHIVE" "$PKG_NAME"
( cd "$DIST_DIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256" )

echo "==> done"
echo "archive:  $ARCHIVE"
echo "checksum: $ARCHIVE.sha256"

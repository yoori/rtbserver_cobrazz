#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 OLLAMA_VERSION" >&2
  exit 1
fi

OLLAMA_VERSION=$1
SOURCE_ROOT=$(cd "$(dirname "$0")/.." && pwd)
TOPDIR=${TMPDIR:-/tmp}/foros-ollama-rpmbuild-$OLLAMA_VERSION
OUTPUT_DIR=${OUTPUT_DIR:-$SOURCE_ROOT/build/RPMS}
OLLAMA_ARCHIVE=${OLLAMA_ARCHIVE:-}
OLLAMA_URL=${OLLAMA_URL:-https://github.com/ollama/ollama/releases/download/v$OLLAMA_VERSION/ollama-linux-amd64.tar.zst}
SOURCE_ARCHIVE=$TOPDIR/SOURCES/ollama-linux-amd64.tar.zst

if [[ ! "$OLLAMA_VERSION" =~ ^[0-9][A-Za-z0-9._+-]*$ ]]; then
  echo "Unsupported Ollama version: $OLLAMA_VERSION" >&2
  exit 1
fi

rm -rf "$TOPDIR"
mkdir -p "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

if [ -n "$OLLAMA_ARCHIVE" ]; then
  cp "$OLLAMA_ARCHIVE" "$SOURCE_ARCHIVE"
else
  curl --fail --location --retry 3 \
    --output "$SOURCE_ARCHIVE" \
    "$OLLAMA_URL"
fi

if [ -n "${OLLAMA_SHA256:-}" ]; then
  echo "$OLLAMA_SHA256  $SOURCE_ARCHIVE" | sha256sum --check
fi

cp "$SOURCE_ROOT/RPM/SPECS/ollama.spec" "$TOPDIR/SPECS/"

rpmbuild \
  --define "_topdir $TOPDIR" \
  --define "_tmppath /tmp" \
  --define "_binary_payload w3.zstdio" \
  --define "ollama_version $OLLAMA_VERSION" \
  -bb "$TOPDIR/SPECS/ollama.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/*/foros-ollama-*.rpm "$OUTPUT_DIR/"
rm -rf "$TOPDIR"

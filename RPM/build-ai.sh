#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 VERSION" >&2
  exit 1
fi

VERSION=$1
SOURCE_ROOT=$(cd "$(dirname "$0")/.." && pwd)
TOPDIR=${TMPDIR:-/tmp}/foros-server-ai-rpmbuild-$VERSION
OUTPUT_DIR=${OUTPUT_DIR:-$SOURCE_ROOT/build/RPMS}

rm -rf "$TOPDIR"
mkdir -p "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

tar \
  --exclude='*/__pycache__' \
  --exclude='*.py[co]' \
  --transform "s,^,foros-server-ai-$VERSION/," \
  -C "$SOURCE_ROOT" \
  -czf "$TOPDIR/SOURCES/foros-server-ai-$VERSION.tar.gz" \
  AI \
  DACS/AICluster

cp "$SOURCE_ROOT/RPM/SPECS/server-ai.spec" "$TOPDIR/SPECS/"

rpmbuild \
  --define "_topdir $TOPDIR" \
  --define "_tmppath /tmp" \
  --define "version $VERSION" \
  -ba "$TOPDIR/SPECS/server-ai.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/*/foros-server-ai-*.rpm "$OUTPUT_DIR/"

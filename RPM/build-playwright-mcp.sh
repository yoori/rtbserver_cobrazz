#!/bin/bash

set -euo pipefail

if [ "$#" -gt 2 ]; then
  echo "Usage: $0 [PLAYWRIGHT_MCP_VERSION [NODE_VERSION]]" >&2
  exit 1
fi

PLAYWRIGHT_MCP_VERSION=${1:-0.0.79}
NODE_VERSION=${2:-22.18.0}
SOURCE_ROOT=$(cd "$(dirname "$0")/.." && pwd)
TOPDIR=${TMPDIR:-/tmp}/foros-playwright-mcp-rpmbuild-$PLAYWRIGHT_MCP_VERSION
OUTPUT_DIR=${OUTPUT_DIR:-$SOURCE_ROOT/build/RPMS}
RUNTIME_ROOT=$TOPDIR/runtime
NODE_ARCHIVE=$TOPDIR/node-v$NODE_VERSION-linux-x64.tar.xz
SOURCE_ARCHIVE=$TOPDIR/SOURCES/playwright-mcp-runtime.tar.zst

for value in "$PLAYWRIGHT_MCP_VERSION" "$NODE_VERSION"; do
  if [[ ! "$value" =~ ^[0-9][A-Za-z0-9._+-]*$ ]]; then
    echo "Unsupported version: $value" >&2
    exit 1
  fi
done

rm -rf "$TOPDIR"
mkdir -p \
  "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS} \
  "$RUNTIME_ROOT/node" \
  "$RUNTIME_ROOT/lib"

curl --fail --location --retry 3 \
  --output "$NODE_ARCHIVE" \
  "https://nodejs.org/dist/v$NODE_VERSION/node-v$NODE_VERSION-linux-x64.tar.xz"
tar -xJf "$NODE_ARCHIVE" \
  --strip-components=1 \
  -C "$RUNTIME_ROOT/node"

PATH=$RUNTIME_ROOT/node/bin:$PATH \
PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1 \
  "$RUNTIME_ROOT/node/bin/npm" install \
    --prefix "$RUNTIME_ROOT/lib" \
    --omit=dev \
    --ignore-scripts \
    --no-audit \
    --no-fund \
    "@playwright/mcp@$PLAYWRIGHT_MCP_VERSION"

PLAYWRIGHT_BROWSERS_PATH=0 \
  "$RUNTIME_ROOT/node/bin/node" \
    "$RUNTIME_ROOT/lib/node_modules/playwright/cli.js" \
    install --only-shell chromium

PLAYWRIGHT_BROWSERS_PATH=0 \
  "$RUNTIME_ROOT/node/bin/node" \
    "$RUNTIME_ROOT/lib/node_modules/@playwright/mcp/cli.js" \
    --help >/dev/null

rm -f \
  "$RUNTIME_ROOT/node/bin/corepack" \
  "$RUNTIME_ROOT/node/bin/npm" \
  "$RUNTIME_ROOT/node/bin/npx"
rm -rf \
  "$RUNTIME_ROOT/node/include" \
  "$RUNTIME_ROOT/node/lib/node_modules" \
  "$RUNTIME_ROOT/node/share"

tar -C "$RUNTIME_ROOT" -cf - . \
  | zstd -T0 -1 -o "$SOURCE_ARCHIVE"

cp "$SOURCE_ROOT/RPM/playwright-mcp/playwright-mcp" "$TOPDIR/SOURCES/"
cp "$SOURCE_ROOT/RPM/SPECS/playwright-mcp.spec" "$TOPDIR/SPECS/"

rpmbuild \
  --define "_topdir $TOPDIR" \
  --define "_tmppath ${TMPDIR:-/tmp}" \
  --define "_binary_payload w3.zstdio" \
  --define "playwright_mcp_version $PLAYWRIGHT_MCP_VERSION" \
  -bb "$TOPDIR/SPECS/playwright-mcp.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/*/foros-playwright-mcp-*.rpm "$OUTPUT_DIR/"
rm -rf "$TOPDIR"

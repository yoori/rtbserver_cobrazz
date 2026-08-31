#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 TORCH_VERSION" >&2
  exit 1
fi

TORCH_VERSION=$1
TORCH_VARIANT=${TORCH_VARIANT:-cu124}
PYTHON=${PYTHON:-python3.12}
SOURCE_ROOT=$(cd "$(dirname "$0")/.." && pwd)
TOPDIR=${TMPDIR:-/tmp}/foros-python3.12-torch-rpmbuild-$TORCH_VERSION-$TORCH_VARIANT
OUTPUT_DIR=${OUTPUT_DIR:-$SOURCE_ROOT/build/RPMS}
SITE_PACKAGES=$TOPDIR/site-packages
SOURCE_ARCHIVE=$TOPDIR/SOURCES/python3.12-torch-site-packages.tar.zst
TORCH_INDEX_URL=${TORCH_INDEX_URL:-https://download.pytorch.org/whl/$TORCH_VARIANT}
TORCH_WHEEL=${TORCH_WHEEL:-}
TORCH_SITE_PACKAGES_SOURCE=${TORCH_SITE_PACKAGES_SOURCE:-}

for value in "$TORCH_VERSION" "$TORCH_VARIANT"; do
  if [[ ! "$value" =~ ^[A-Za-z0-9._+-]+$ ]]; then
    echo "Unsupported value: $value" >&2
    exit 1
  fi
done

if ! "$PYTHON" -m pip --version >/dev/null 2>&1; then
  echo "Python pip is not available: $PYTHON" >&2
  exit 1
fi

rm -rf "$TOPDIR"
mkdir -p \
  "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS} \
  "$SITE_PACKAGES"

if [ -n "$TORCH_SITE_PACKAGES_SOURCE" ]; then
  if [ ! -d "$TORCH_SITE_PACKAGES_SOURCE/torch" ]; then
    echo "Invalid PyTorch site-packages source: $TORCH_SITE_PACKAGES_SOURCE" >&2
    exit 1
  fi
  cp -a "$TORCH_SITE_PACKAGES_SOURCE"/. "$SITE_PACKAGES"/
else
  torch_requirement="torch==$TORCH_VERSION+$TORCH_VARIANT"
  if [ -n "$TORCH_WHEEL" ]; then
    if [ ! -r "$TORCH_WHEEL" ]; then
      echo "Torch wheel is not readable: $TORCH_WHEEL" >&2
      exit 1
    fi
    torch_requirement=$TORCH_WHEEL
  fi

  "$PYTHON" -m pip install \
    --disable-pip-version-check \
    --no-cache-dir \
    --target "$SITE_PACKAGES" \
    --index-url "$TORCH_INDEX_URL" \
    --extra-index-url https://pypi.org/simple \
    "$torch_requirement"
fi

find "$SITE_PACKAGES" -type d -name __pycache__ -prune -exec rm -rf {} +
find "$SITE_PACKAGES" -type f \( -name '*.pyc' -o -name '*.pyo' \) -delete

PYTHONPATH=$SITE_PACKAGES "$PYTHON" -c \
  'import torch; print(torch.__version__, torch.version.cuda)'

tar -C "$SITE_PACKAGES" -cf - . \
  | zstd -T0 -1 -o "$SOURCE_ARCHIVE"

cp "$SOURCE_ROOT/RPM/SPECS/python3.12-torch.spec" "$TOPDIR/SPECS/"

rpmbuild \
  --define "_topdir $TOPDIR" \
  --define "_tmppath ${TMPDIR:-/tmp}" \
  --define "_binary_payload w3.zstdio" \
  --define "torch_version $TORCH_VERSION" \
  --define "torch_variant $TORCH_VARIANT" \
  -bb "$TOPDIR/SPECS/python3.12-torch.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/*/foros-python3.12-torch-*.rpm "$OUTPUT_DIR/"
rm -rf "$TOPDIR"

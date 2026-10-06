#!/bin/bash
set -euo pipefail

VERSION=2.0.2
RELEASE=ssv4
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
TOPDIR="$PROJECT_DIR/build/xgboost-rpm/$VERSION-$RELEASE"
SOURCE_DIR="$TOPDIR/SOURCES/xgboost-$VERSION"
ARCHIVE="$TOPDIR/SOURCES/xgboost-$VERSION.tar.gz"
JOBS=${JOBS:-4}

mkdir -p "$TOPDIR"/{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS,tmp}
mkdir -p "$PROJECT_DIR/build/RPMS" "$PROJECT_DIR/build/SRPMS"
cp "$SCRIPT_DIR/SPECS/xgboost.spec" "$TOPDIR/SPECS/"
cp "$SCRIPT_DIR/foros-cxx-abi.macros" "$SCRIPT_DIR"/tests/xgboost-*.cpp "$TOPDIR/SOURCES/"

if [[ ! -f "$ARCHIVE" ]]; then
  if [[ ! -d "$SOURCE_DIR" ]]; then
    git clone --branch "v$VERSION" --depth 1 --recurse-submodules --shallow-submodules \
      https://github.com/dmlc/xgboost.git "$SOURCE_DIR"
  fi
  [[ $(git -C "$SOURCE_DIR" describe --tags --exact-match) == "v$VERSION" ]]
  if git -C "$SOURCE_DIR" submodule status --recursive | grep -Eq '^[-+U]'; then
    echo "XGBoost submodules do not match the release; refresh $SOURCE_DIR" >&2
    exit 1
  fi
  tar --exclude=.git -czf "$ARCHIVE.tmp" -C "$TOPDIR/SOURCES" "xgboost-$VERSION"
  mv "$ARCHIVE.tmp" "$ARCHIVE"
fi

# rpmbuild checks BuildRequires, including the selected GCC Toolset, before compiling.
rpmbuild -ba --define "_topdir $TOPDIR" --define "xgboost_version $VERSION" \
  --define "_tmppath $TOPDIR/tmp" --define "xgboost_release $RELEASE" \
  --define "_smp_mflags -j$JOBS" \
  "$TOPDIR/SPECS/xgboost.spec"

find "$TOPDIR/RPMS" -name "xgboost*-$VERSION-$RELEASE*.rpm" \
  -exec cp -t "$PROJECT_DIR/build/RPMS" {} +
cp "$TOPDIR"/SRPMS/xgboost-"$VERSION-$RELEASE"*.src.rpm "$PROJECT_DIR/build/SRPMS/"

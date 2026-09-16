#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
VERSION=1.38.1
UPSTREAM_RELEASE=1
ARCH=x86_64
SOURCE_FILE="telegraf-$VERSION-$UPSTREAM_RELEASE.$ARCH.rpm"
SOURCE_PATH="/repos/oracle/8.6/REPO_EPEL8/$ARCH/Packages/$SOURCE_FILE"
DEFAULT_SOURCE_URL="http://192.168.10.1$SOURCE_PATH"
SOURCE_URL="${TELEGRAF_SOURCE_URL:-$DEFAULT_SOURCE_URL}"
SOURCE_SHA256=3c16153a5db1d609b3daaf36a6f2ee2ad074d4f919cc51727d7b379560748097

RPM_SOURCES_DIR=$(rpm -E '%_sourcedir')
RPM_SPECS_DIR=$(rpm -E '%_specdir')
BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
RESULT_RPMS="$SCRIPT_DIR/../build/RPMS"

mkdir -p "$RPM_SOURCES_DIR" "$RPM_SPECS_DIR" "$BIN_RPM_FOLDER" "$RESULT_RPMS"

curl --fail --location --retry 3 --retry-delay 2 --output "$RPM_SOURCES_DIR/$SOURCE_FILE" \
  "$SOURCE_URL"
echo "$SOURCE_SHA256  $RPM_SOURCES_DIR/$SOURCE_FILE" | sha256sum --check --strict

cp "$SCRIPT_DIR/SPECS/telegraf.spec" "$RPM_SPECS_DIR/foros-telegraf.spec"
rpmbuild --force -bb \
  --define "version $VERSION" \
  --define "telegraf_upstream_release $UPSTREAM_RELEASE" \
  "$RPM_SPECS_DIR/foros-telegraf.spec"

cp "$BIN_RPM_FOLDER"/foros-telegraf-"$VERSION"-*."$ARCH".rpm "$RESULT_RPMS/"

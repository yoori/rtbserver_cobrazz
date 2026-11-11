#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
DEPENDENCIES_DIR=${DEPENDENCIES_DIR:-"$SCRIPT_DIR/../../foros-dependencies"}
exec bash "$DEPENDENCIES_DIR/rocksdb-build.sh" "$@"

#!/bin/bash
#
# SyncLogs helper: primary rsync must succeed; optional side-copy is
# best-effort and must not fail the primary route (Predictor / RIM).
#
# USAGE:
#   synclogs_rsync_side_copy.sh <SRC_PATH> <PRIMARY_DST> [SIDE_DST]
#
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "USAGE: synclogs_rsync_side_copy.sh <SRC_PATH> <PRIMARY_DST> [SIDE_DST]" >&2
  exit 1
fi

SRC_PATH=$1
PRIMARY_DST=$2
SIDE_DST=${3:-}

/usr/bin/rsync -az -t --timeout=55 --log-format=%f "$SRC_PATH" "$PRIMARY_DST"
PRIMARY_RC=$?
if [ "$PRIMARY_RC" -ne 0 ]; then
  exit "$PRIMARY_RC"
fi

if [ -n "$SIDE_DST" ]; then
  /usr/bin/rsync -az -t --timeout=55 --log-format=%f "$SRC_PATH" "$SIDE_DST" || true
fi

exit 0

#!/bin/bash
#
# SyncLogs helper: primary rsync must succeed; optional ClickHouse inbox
# side-copy is best-effort and must not fail the primary route.
#
# USAGE:
#   synclogs_rsync_adv_action_ch.sh <SRC_PATH> <PRIMARY_DST> [CH_DST_DIR]
#
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "USAGE: synclogs_rsync_adv_action_ch.sh <SRC_PATH> <PRIMARY_DST> [CH_DST_DIR]" >&2
  exit 1
fi

SRC_PATH=$1
PRIMARY_DST=$2
CH_DST=${3:-}

/usr/bin/rsync -t -z --timeout=55 --log-format=%f "$SRC_PATH" "$PRIMARY_DST"
PRIMARY_RC=$?
if [ "$PRIMARY_RC" -ne 0 ]; then
  exit "$PRIMARY_RC"
fi

if [ -n "$CH_DST" ]; then
  # Never fail SyncLogs / RIM delivery because of the analytics side-copy.
  /usr/bin/rsync -t -z --timeout=55 --log-format=%f "$SRC_PATH" "$CH_DST" || true
fi

exit 0

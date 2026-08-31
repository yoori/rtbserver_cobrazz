#!/bin/bash

set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 CONFIG_FILE WORKSPACE_ROOT" >&2
  exit 1
fi

CONFIG_FILE=$1
WORKSPACE_ROOT=$2

if [ ! -r "$CONFIG_FILE" ]; then
  echo "AIAgent config is not readable: $CONFIG_FILE" >&2
  exit 1
fi

. "$CONFIG_FILE"

: "${AI_OLLAMA_BIN:?AI_OLLAMA_BIN is not set in $CONFIG_FILE}"
: "${AI_MODEL_DIR:?AI_MODEL_DIR is not set in $CONFIG_FILE}"
: "${AI_LISTEN_HOST:?AI_LISTEN_HOST is not set in $CONFIG_FILE}"
: "${AI_PORT:?AI_PORT is not set in $CONFIG_FILE}"

if [ ! -x "$AI_OLLAMA_BIN" ]; then
  echo "Ollama executable is not available: $AI_OLLAMA_BIN" >&2
  exit 1
fi

if [ ! -d "$AI_MODEL_DIR" ]; then
  echo "Ollama model directory is not available: $AI_MODEL_DIR" >&2
  exit 1
fi

RUN_DIR=$WORKSPACE_ROOT/run
HOME_DIR=$WORKSPACE_ROOT/AIAgent
PID_FILE=$RUN_DIR/AIAgent.pid
LOCK_FILE=$RUN_DIR/AIAgent.lock

mkdir -p "$RUN_DIR" "$HOME_DIR"
exec 9>"$LOCK_FILE"
flock -n 9 || {
  echo "AIAgent is already starting or running" >&2
  exit 1
}

if [ -e "$PID_FILE" ]; then
  old_pid=$(cat "$PID_FILE")
  if kill -0 "$old_pid" 2>/dev/null; then
    echo "AIAgent is already running: pid=$old_pid" >&2
    exit 1
  fi
  rm -f "$PID_FILE"
fi

temporary_pid_file=$PID_FILE.$$
printf '%s\n' "$$" >"$temporary_pid_file"
mv -f "$temporary_pid_file" "$PID_FILE"

remove_pid_file()
{
  if [ -e "$PID_FILE" ] && [ "$(cat "$PID_FILE")" = "$$" ]; then
    rm -f "$PID_FILE"
  fi
}

server_pid=
stop_server()
{
  if [ -n "$server_pid" ]; then
    kill -TERM "$server_pid" 2>/dev/null || true
  fi
}

trap remove_pid_file EXIT
trap stop_server INT TERM HUP

export HOME=$HOME_DIR
export NO_PROXY=127.0.0.1,localhost${NO_PROXY:+,$NO_PROXY}
export no_proxy=$NO_PROXY
export OLLAMA_HOST=$AI_LISTEN_HOST:$AI_PORT
export OLLAMA_MODELS=$AI_MODEL_DIR
export OLLAMA_NO_CLOUD=true
export OLLAMA_NOPRUNE=true

"$AI_OLLAMA_BIN" serve &
server_pid=$!

status=0
while kill -0 "$server_pid" 2>/dev/null; do
  wait "$server_pid" || status=$?
done
wait "$server_pid" 2>/dev/null || true
exit "$status"

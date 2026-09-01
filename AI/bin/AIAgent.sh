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
: "${AI_AGENT_LISTEN_HOST:?AI_AGENT_LISTEN_HOST is not set in $CONFIG_FILE}"
: "${AI_AGENT_PORT:?AI_AGENT_PORT is not set in $CONFIG_FILE}"
: "${AI_CLUSTER_AGENT_LISTEN_HOST:?AI_CLUSTER_AGENT_LISTEN_HOST is not set in $CONFIG_FILE}"
: "${AI_CLUSTER_AGENT_PORT:?AI_CLUSTER_AGENT_PORT is not set in $CONFIG_FILE}"
: "${AI_MCP_COMMAND:?AI_MCP_COMMAND is not set in $CONFIG_FILE}"
: "${AI_WEB_MCP_COMMAND:?AI_WEB_MCP_COMMAND is not set in $CONFIG_FILE}"
: "${AI_CLUSTER_MCP_COMMAND:?AI_CLUSTER_MCP_COMMAND is not set in $CONFIG_FILE}"

if [ ! -x "$AI_OLLAMA_BIN" ]; then
  echo "Ollama executable is not available: $AI_OLLAMA_BIN" >&2
  exit 1
fi

if [ ! -d "$AI_MODEL_DIR" ]; then
  echo "Ollama model directory is not available: $AI_MODEL_DIR" >&2
  exit 1
fi

if [ ! -x "$AI_MCP_COMMAND" ]; then
  echo "MCP executable is not available: $AI_MCP_COMMAND" >&2
  exit 1
fi

if [ ! -x "$AI_WEB_MCP_COMMAND" ]; then
  echo "Web MCP executable is not available: $AI_WEB_MCP_COMMAND" >&2
  exit 1
fi

if [ ! -x "$AI_CLUSTER_MCP_COMMAND" ]; then
  echo "Cluster MCP executable is not available: $AI_CLUSTER_MCP_COMMAND" >&2
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

ollama_pid=
agent_pid=
stop_servers()
{
  if [ -n "$agent_pid" ]; then
    kill -TERM "$agent_pid" 2>/dev/null || true
  fi
  if [ -n "$ollama_pid" ]; then
    kill -TERM "$ollama_pid" 2>/dev/null || true
  fi
}

trap remove_pid_file EXIT
trap stop_servers INT TERM HUP

export HOME=$HOME_DIR
export NO_PROXY=127.0.0.1,localhost${NO_PROXY:+,$NO_PROXY}
export no_proxy=$NO_PROXY
export OLLAMA_HOST=$AI_LISTEN_HOST:$AI_PORT
export OLLAMA_MODELS=$AI_MODEL_DIR
export OLLAMA_NO_CLOUD=true
export OLLAMA_NOPRUNE=true
export AI_OLLAMA_URL=http://127.0.0.1:$AI_PORT

"$AI_OLLAMA_BIN" serve &
ollama_pid=$!

ollama_ready=false
for unused in $(seq 1 120); do
  if ! kill -0 "$ollama_pid" 2>/dev/null; then
    break
  fi
  if OLLAMA_HOST=$AI_OLLAMA_URL "$AI_OLLAMA_BIN" show "$AI_MODEL" >/dev/null 2>&1; then
    ollama_ready=true
    break
  fi
  sleep 1
done

if [ "$ollama_ready" != true ]; then
  echo "Ollama did not become ready" >&2
  stop_servers
  wait "$ollama_pid" 2>/dev/null || true
  exit 1
fi

AIAgent.py --host="$AI_AGENT_LISTEN_HOST" --port="$AI_AGENT_PORT" &
agent_pid=$!

status=0
while kill -0 "$ollama_pid" 2>/dev/null && kill -0 "$agent_pid" 2>/dev/null; do
  sleep 1
done

if ! kill -0 "$ollama_pid" 2>/dev/null; then
  wait "$ollama_pid" || status=$?
else
  wait "$agent_pid" || status=$?
fi
stop_servers
wait "$agent_pid" 2>/dev/null || true
wait "$ollama_pid" 2>/dev/null || true
exit "$status"

#!/usr/bin/env bash
set -euo pipefail

repo="${REPO:-/home/jurij_kuznecov/projects/rtbserver_cobrazz}"
build="${BUILD:-$repo/build}"
endpoint="${ENDPOINT:-0.0.0.0:26528}"
out="${OUT:-$repo/tmp/UserBindGrpcPerf.blackhole.out}"
err="${ERR:-$repo/tmp/UserBindGrpcPerf.blackhole.err}"
server_log="${SERVER_LOG:-$repo/tmp/MockUserBindServer.blackhole.log}"
drop_seconds="${DROP_SECONDS:-10}"
count="${COUNT:-10000000}"

cleanup_firewall() {
  sudo iptables -D OUTPUT -o lo -p tcp -m tcp --dport 26528 -j DROP 2>/dev/null || true
}

start_server() {
  cd "$build"
  ./bin/MockUserBindServer --grpc-endpoint="$endpoint" >>"$server_log" 2>&1 &
  echo "$!"
}

wait_port() {
  for _ in $(seq 1 100); do
    if ss -ltnp 2>/dev/null | grep -q ':26528'; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

wait_successful_perf() {
  for _ in $(seq 1 60); do
    if awk '
      /^[0-9][0-9]:/ {
        done=$2
        gsub(/,/, "", done)
        errors=$3
        sub(/^errors=/, "", errors)
        gsub(/,/, "", errors)
        writes=0
        for (i = 1; i <= NF; ++i) {
          if ($i ~ /^writes=/) {
            writes=$i
            sub(/^writes=/, "", writes)
            gsub(/,/, "", writes)
          }
        }

        if (done > 0 && errors == 0 && writes > 0) {
          found=1
        }
      }
      END {exit found ? 0 : 1}
    ' "$out" 2>/dev/null; then
      return 0
    fi
    sleep 1
  done
  return 1
}

kill_server() {
  local pid
  pid="$(pgrep -f "MockUserBindServer --grpc-endpoint=$endpoint" | head -1 || true)"
  if [ -n "$pid" ]; then
    kill -TERM "$pid" || true
    for _ in $(seq 1 100); do
      if ! kill -0 "$pid" 2>/dev/null; then
        break
      fi
      sleep 0.1
    done
    if kill -0 "$pid" 2>/dev/null; then
      kill -KILL "$pid" || true
    fi
  fi
}

cleanup() {
  cleanup_firewall
  if [ -n "${perf_pid:-}" ] && kill -0 "$perf_pid" 2>/dev/null; then
    kill -TERM "$perf_pid" || true
    wait "$perf_pid" || true
  fi
  kill_server
}
trap cleanup EXIT

mkdir -p "$(dirname "$out")" "$(dirname "$err")" "$(dirname "$server_log")"
rm -f "$out" "$err" "$server_log"
cleanup_firewall
kill_server

server_pid="$(start_server)"
echo "$(date +%H:%M:%S) mock server pid=$server_pid endpoint=$endpoint"
wait_port

cd "$build"
env -u http_proxy -u https_proxy -u HTTP_PROXY -u HTTPS_PROXY -u all_proxy -u ALL_PROXY \
  ./bin/UserBindGrpcPerf \
    -g 127.0.0.1:26528 \
    -c "$count" \
    -t 16 \
    --client-threads=4 \
    --grpc-compression=1 \
    --mode=async-batch \
    --max-batch-size=2000 \
    --max-inflight=2000000 \
    --max-batch-delay-us=3000 \
    --max-queue-wait-us=100000 \
    --max-streams=32 \
    --hot-buckets-count=16 \
    >"$out" 2>"$err" &
perf_pid=$!

wait_successful_perf
if ! kill -0 "$perf_pid" 2>/dev/null; then
  echo "UserBindGrpcPerf finished before blackhole stage" >&2
  tail -20 "$out" || true
  exit 1
fi
echo "$(date +%H:%M:%S) warmup ok"
tail -5 "$out" || true

sudo iptables -I OUTPUT 1 -p tcp -o lo --dport 26528 -j DROP
echo "$(date +%H:%M:%S) blackhole enabled for ${drop_seconds}s"
sleep "$drop_seconds"
cleanup_firewall
echo "$(date +%H:%M:%S) blackhole disabled"

for _ in $(seq 1 30); do
  if ! kill -0 "$perf_pid" 2>/dev/null; then
    break
  fi
  sleep 1
done

if kill -0 "$perf_pid" 2>/dev/null; then
  kill -TERM "$perf_pid" || true
fi
wait "$perf_pid" || true

echo "--- perf out tail ---"
tail -40 "$out" || true
echo "--- perf err tail ---"
tail -40 "$err" || true

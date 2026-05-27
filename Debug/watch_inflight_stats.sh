#!/usr/bin/env bash

set -u

threshold=10000
sleep_sec=1

declare -A bad_hosts

hosts=()
for host_id in {100..111} {200..211}; do
  hosts+=("adfe${host_id}")
done

ports=(10277 10296)

while true; do
  for host in "${hosts[@]}"; do
    host_bad=0
    host_data=""

    for port in "${ports[@]}"; do
      json="$(
        curl -sS \
          --connect-timeout 1 \
          --max-time 3 \
          "http://${host}:${port}/stats" 2>/dev/null || true
      )"
      [[ -z "${json}" ]] && continue

      inflight_lines="$(
        jq -r '
          to_entries[]
          | select((.key | contains("_inflight_")) or .key == "call_inflight")
          | "\(.key)=\(.value)"
        ' <<<"${json}" 2>/dev/null
      )"
      [[ -z "${inflight_lines}" ]] && continue

      while IFS='=' read -r key value; do
        [[ -z "${key:-}" ]] && continue
        if [[ "${value}" =~ ^[0-9]+$ ]] && (( value > threshold )); then
          host_bad=1
        fi
      done <<<"${inflight_lines}"

      host_data+=$'\n'"${host}:${port}"$'\n'"${inflight_lines}"
    done

    if (( host_bad )); then
      if [[ -z "${bad_hosts[${host}]:-}" ]]; then
        bad_hosts["${host}"]=1
        {
          date '+%F %T'
          echo "ERROR: ${host} has *_inflight_* > ${threshold}"
          echo "${host_data}"
          echo
        } >&2
      fi
    else
      unset "bad_hosts[${host}]"
    fi
  done

  sleep "${sleep_sec}"
done

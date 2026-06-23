#!/usr/bin/env bash

set -u

export LC_ALL=C

usage()
{
  cat <<'EOF'
Usage:
  GetPerfStats.sh '<url>' '<request body>' [timeout]

Example:
  GetPerfStats.sh 'http://host/openrtb?...' $'{"id":"...","imp":[...]}' 0.060

The script sends the request in a loop until interrupted with Ctrl-C.
Set CHECK_PERF_LIMIT=<count> to stop after a fixed number of requests.
EOF
}

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  usage >&2
  exit 1
fi

URL=$1
REQUEST_BODY=$2
TIMEOUT=${3:-0.060}
LIMIT=${CHECK_PERF_LIMIT:-0}

case "$TIMEOUT" in
  ''|*[!0-9.]*)
    echo "Invalid timeout: $TIMEOUT" >&2
    exit 1
    ;;
esac

TMP_STATS=$(mktemp "${TMPDIR:-/tmp}/get-perf-stats.XXXXXX")
TMP_RESPONSE=$(mktemp "${TMPDIR:-/tmp}/get-perf-response.XXXXXX")
STOP=0

cleanup()
{
  rm -f "$TMP_STATS" "$TMP_RESPONSE"
}

finish()
{
  print_stats
  cleanup
}

on_interrupt()
{
  STOP=1
}

trap on_interrupt INT TERM
trap finish EXIT

parse_response()
{
  awk -v timeout="$TIMEOUT" '
    BEGIN {
      steps[1] = "user_resolving"
      steps[2] = "trigger_match"
      steps[3] = "history_match"
      steps[4] = "creative_selection"

      titles["user_resolving"] = "user resolving"
      titles["trigger_match"] = "trigger matching"
      titles["history_match"] = "history matching"
      titles["creative_selection"] = "campaign selection"

      order["user_resolving"] = 1
      order["trigger_match"] = 2
      order["history_match"] = 3
      order["creative_selection"] = 4

      interrupted_alias["user resolving"] = "user_resolving"
      interrupted_alias["trigger matching"] = "trigger_match"
      interrupted_alias["history matching"] = "history_match"
      interrupted_alias["campaign selection"] = "creative_selection"
      interrupted_alias["campaign selection considering"] = "creative_selection"
    }

    function trim(value) {
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
      return value
    }

    function normalize_step(value) {
      value = trim(value)
      gsub(/;$/, "", value)
      return interrupted_alias[value]
    }

    function is_number(value) {
      return value ~ /^[0-9]+([.][0-9]+)?$/
    }

    function parse_number(value, parts) {
      value = trim(value)
      sub(/[[:space:]]*:.*/, "", value)
      sub(/[[:space:]]*=>.*/, "", value)
      gsub(/;$/, "", value)
      return value
    }

    function round_10ms(value) {
      if (value < 0) {
        value = 0
      }
      return int(value * 100 + 0.5) / 100
    }

    /^interrupted_step[[:space:]]*=/ {
      interrupted_step = normalize_step(substr($0, index($0, "=") + 1))
      next
    }

    /^=== Time Metering ===/ {
      has_time_metering = 1
      next
    }

    /^[a-z_]+_started_at[[:space:]]*=/ {
      name = $1
      sub(/_started_at$/, "", name)
      value = parse_number(substr($0, index($0, "=") + 1))
      if (name in order && is_number(value)) {
        started_at[name] = value + 0
      }
      next
    }

    /^[a-z_]+_time[[:space:]]*=/ {
      name = $1
      sub(/_time$/, "", name)
      value = parse_number(substr($0, index($0, "=") + 1))
      if (name in order && is_number(value)) {
        time[name] = value + 0
      }
      next
    }

    END {
      print "REQUEST|" (has_time_metering ? 1 : 0)

      if (!has_time_metering) {
        exit
      }

      interrupted_order = interrupted_step in order ? order[interrupted_step] : 0

      for (i = 1; i <= 4; ++i) {
        step = steps[i]

        if (interrupted_order && i >= interrupted_order) {
          continue
        }

        if (step in time) {
          print "STEP|" step "|" titles[step] "|" time[step]
        }
      }

      if (interrupted_order && (interrupted_step in started_at)) {
        print "INTERRUPT|" interrupted_step "|" \
          titles[interrupted_step] "|" round_10ms(timeout - started_at[interrupted_step])
      }
    }
  ' "$TMP_RESPONSE" >> "$TMP_STATS"
}

print_stats()
{
  awk -F'|' '
    $1 == "REQUEST" {
      ++requests
      if ($2 == 1) {
        ++metered_requests
      } else {
        ++unmetered_requests
      }
      next
    }

    $1 == "STEP" {
      step = $2
      title[step] = $3
      sum[step] += $4
      count[step] += 1
      reached[step] += 1
      next
    }

    $1 == "INTERRUPT" {
      step = $2
      title[step] = $3
      bucket = $4
      interrupt_count[step, bucket] += 1
      bucket_seen[step, bucket] = 1
      reached[step] += 1
      next
    }

    END {
      steps[1] = "user_resolving"
      steps[2] = "trigger_match"
      steps[3] = "history_match"
      steps[4] = "creative_selection"

      default_title["user_resolving"] = "user resolving"
      default_title["trigger_match"] = "trigger matching"
      default_title["history_match"] = "history matching"
      default_title["creative_selection"] = "campaign selection"

      print ""
      print "requests: " requests
      print "metered requests: " metered_requests
      if (unmetered_requests) {
        print "without time metering: " unmetered_requests
      }

      for (i = 1; i <= 4; ++i) {
        step = steps[i]
        name = title[step] ? title[step] : default_title[step]
        print ""
        print name ":"
        if (count[step]) {
          printf "average time: %.6f sec\n", sum[step] / count[step]
          print "completed: " count[step]
        } else {
          print "average time: n/a"
          print "completed: 0"
        }

        total = reached[step]
        if (!total) {
          continue
        }

        for (key in bucket_seen) {
          split(key, parts, SUBSEP)
          if (parts[1] == step) {
            buckets[parts[2]] = 1
          }
        }

        for (bucket in buckets) {
          sorted[++bucket_count] = bucket
        }

        for (left = 1; left <= bucket_count; ++left) {
          for (right = left + 1; right <= bucket_count; ++right) {
            if ((sorted[left] + 0) > (sorted[right] + 0)) {
              tmp = sorted[left]
              sorted[left] = sorted[right]
              sorted[right] = tmp
            }
          }
        }

        for (idx = 1; idx <= bucket_count; ++idx) {
          bucket = sorted[idx]
          percent = interrupt_count[step, bucket] * 100 / total
          printf "%%%0.0f interrupted at %.2f sec\n", percent, bucket
        }

        delete buckets
        delete sorted
        bucket_count = 0
      }
    }
  ' "$TMP_STATS"
}

echo "Running request loop. Press Ctrl-C to print stats." >&2

REQUESTS_DONE=0
while [ "$STOP" -eq 0 ]; do
  curl "$URL" \
    -H 'Content-Type: application/json' \
    --data "$REQUEST_BODY" \
    -v > "$TMP_RESPONSE" 2>&1
  parse_response

  REQUESTS_DONE=$((REQUESTS_DONE + 1))
  if [ "$((REQUESTS_DONE % 100))" -eq 0 ]; then
    echo "processed $REQUESTS_DONE requests" >&2
  fi

  if [ "$LIMIT" -gt 0 ] && [ "$REQUESTS_DONE" -ge "$LIMIT" ]; then
    break
  fi
done

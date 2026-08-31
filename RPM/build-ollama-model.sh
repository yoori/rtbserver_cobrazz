#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 VERSION" >&2
  exit 1
fi

VERSION=$1
MODEL=${MODEL:-qwen3}
OLLAMA_VERSION=${OLLAMA_VERSION:-0.33.2}
MODEL_PACKAGE_SUFFIX=${MODEL_PACKAGE_SUFFIX:-${MODEL//[:\/]/-}}
SOURCE_ROOT=$(cd "$(dirname "$0")/.." && pwd)
PACKAGE_NAME=foros-ollama-model-$MODEL_PACKAGE_SUFFIX
TOPDIR=${TMPDIR:-/tmp}/$PACKAGE_NAME-rpmbuild-$VERSION
OUTPUT_DIR=${OUTPUT_DIR:-$SOURCE_ROOT/build/RPMS}
BUILD_PORT=${OLLAMA_BUILD_PORT:-41134}
MODEL_ROOT=$TOPDIR/model
BUILD_HOME=$TOPDIR/home
OLLAMA_ROOT=$TOPDIR/ollama
SOURCE_ARCHIVE=$TOPDIR/SOURCES/ollama-model.tar.zst
MODEL_ENV=$TOPDIR/SOURCES/ollama-model.env
MODEL_CHECKSUMS=$TOPDIR/SOURCES/ollama-model.sha256
OLLAMA_LOG=$TOPDIR/ollama-build.log
MODEL_SOURCE_DIR=${MODEL_SOURCE_DIR:-}

for value in \
  "$VERSION" \
  "$MODEL" \
  "$OLLAMA_VERSION" \
  "$MODEL_PACKAGE_SUFFIX"; do
  if [[ ! "$value" =~ ^[A-Za-z0-9_./:@+-]+$ ]]; then
    echo "Unsupported value: $value" >&2
    exit 1
  fi
done

rm -rf "$TOPDIR"
mkdir -p \
  "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS} \
  "$MODEL_ROOT" \
  "$BUILD_HOME" \
  "$OLLAMA_ROOT"

if [ -n "$MODEL_SOURCE_DIR" ]; then
  if [ ! -d "$MODEL_SOURCE_DIR/blobs" ] || \
     [ ! -d "$MODEL_SOURCE_DIR/manifests" ]; then
    echo "Invalid Ollama model source directory: $MODEL_SOURCE_DIR" >&2
    exit 1
  fi
  cp -a "$MODEL_SOURCE_DIR"/. "$MODEL_ROOT"/
else
  if [ -n "${OLLAMA_BIN:-}" ]; then
    ollama_bin=$OLLAMA_BIN
  else
    ollama_archive=$TOPDIR/ollama-linux-amd64.tar.zst
    if [ -n "${OLLAMA_ARCHIVE:-}" ]; then
      cp "$OLLAMA_ARCHIVE" "$ollama_archive"
    else
      curl --fail --location --retry 3 \
        --output "$ollama_archive" \
        "https://github.com/ollama/ollama/releases/download/v$OLLAMA_VERSION/ollama-linux-amd64.tar.zst"
    fi
    tar --use-compress-program=unzstd \
      -xf "$ollama_archive" \
      -C "$OLLAMA_ROOT"
    ollama_bin=$OLLAMA_ROOT/bin/ollama
  fi

  if [ ! -x "$ollama_bin" ]; then
    echo "Ollama executable is not available: $ollama_bin" >&2
    exit 1
  fi

  export HOME=$BUILD_HOME
  export NO_PROXY=127.0.0.1,localhost${NO_PROXY:+,$NO_PROXY}
  export no_proxy=$NO_PROXY
  export OLLAMA_HOST=127.0.0.1:$BUILD_PORT
  export OLLAMA_MODELS=$MODEL_ROOT

  "$ollama_bin" serve >"$OLLAMA_LOG" 2>&1 &
  server_pid=$!
  cleanup()
  {
    kill -TERM "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  }
  trap cleanup EXIT

  ready=0
  for attempt in $(seq 1 120); do
    if ! kill -0 "$server_pid" 2>/dev/null; then
      cat "$OLLAMA_LOG" >&2
      exit 1
    fi
    if curl --fail --silent --noproxy '*' \
      "http://127.0.0.1:$BUILD_PORT/api/tags" >/dev/null; then
      ready=1
      break
    fi
    sleep 1
  done

  if [ "$ready" != 1 ]; then
    cat "$OLLAMA_LOG" >&2
    exit 1
  fi

  "$ollama_bin" pull "$MODEL"
  "$ollama_bin" show "$MODEL" >/dev/null

  cleanup
  trap - EXIT
fi

(
  cd "$MODEL_ROOT"
  find . -type f -print0 \
    | sort -z \
    | xargs -0 -r sha256sum
) >"$MODEL_CHECKSUMS"

tar -C "$MODEL_ROOT" -cf - . \
  | zstd -T0 -1 -o "$SOURCE_ARCHIVE"

printf "OLLAMA_MODEL='%s'\n" "$MODEL" >"$MODEL_ENV"
printf "OLLAMA_MODEL_PACKAGE='%s'\n" "$PACKAGE_NAME" >>"$MODEL_ENV"
printf "OLLAMA_MODEL_DIR='%s'\n" \
  "/u01/foros/server-ai/models/ollama/$MODEL_PACKAGE_SUFFIX" >>"$MODEL_ENV"
printf 'export OLLAMA_MODEL OLLAMA_MODEL_PACKAGE OLLAMA_MODEL_DIR\n' \
  >>"$MODEL_ENV"

cp "$SOURCE_ROOT/RPM/SPECS/ollama-model.spec" "$TOPDIR/SPECS/"

rpmbuild \
  --define "_topdir $TOPDIR" \
  --define "_tmppath /tmp" \
  --define "_binary_payload w3.zstdio" \
  --define "version $VERSION" \
  --define "model $MODEL" \
  --define "model_package_suffix $MODEL_PACKAGE_SUFFIX" \
  --define "ollama_version $OLLAMA_VERSION" \
  -bb "$TOPDIR/SPECS/ollama-model.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/*/"$PACKAGE_NAME"-*.rpm "$OUTPUT_DIR/"
rm -rf "$TOPDIR"

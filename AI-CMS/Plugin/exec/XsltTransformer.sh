#!/bin/bash

XSL_FILE=
APP_XML=
OUT_FILE=
PARAMS=

while [ "$#" -gt 0 ]; do
  case "$1" in
    --xsl)
      shift
      XSL_FILE=$1
      ;;
    --app-xml)
      shift
      APP_XML=$1
      ;;
    --out-file)
      shift
      OUT_FILE=$1
      ;;
    --var)
      shift
      PARAMS="$PARAMS --stringparam $1 \"$2\""
      shift
      ;;
  esac
  shift
done

if [ -z "$XSL_FILE" ] || [ -z "$APP_XML" ] || [ -z "$OUT_FILE" ]; then
  echo "XsltTransformer.sh: --xsl, --app-xml and --out-file are required" >&2
  exit 1
fi

if [ "$OUT_FILE" = - ]; then
  eval xsltproc $PARAMS "\"$XSL_FILE\"" "\"$APP_XML\""
else
  mkdir -p "$(dirname "$OUT_FILE")"
  eval xsltproc $PARAMS -o "\"$OUT_FILE\"" "\"$XSL_FILE\"" "\"$APP_XML\""
fi

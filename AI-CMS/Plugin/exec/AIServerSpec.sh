#!/bin/bash

if [ "$#" -lt 8 ]; then
  echo "Usage: $0 APP_XML APP_XPATH PLUGIN_ROOT SPEC_PATH PACKAGE_NAME BUILD_ROOT VERSION RELEASE" >&2
  exit 1
fi

APP_XML=$1
APP_XPATH=$2
PLUGIN_ROOT=$3
SPEC_PATH=$4
PACKAGE_NAME=$5
BUILD_ROOT=$6
VERSION=$7
RELEASE=$8

. "$PLUGIN_ROOT/exec/ColocationName.sh" "$APP_XML" "$PLUGIN_ROOT"

"$PLUGIN_ROOT/exec/XsltTransformer.sh" \
  --var XPATH "$APP_XPATH" \
  --var PACKAGE_NAME "$PACKAGE_NAME" \
  --var BUILD_ROOT "$BUILD_ROOT" \
  --var VERSION "$VERSION" \
  --var RELEASE "$RELEASE" \
  --var COLOCATION_NAME "$COLOCATION_NAME" \
  --app-xml "$APP_XML" \
  --xsl "$PLUGIN_ROOT/xslt/specs/AIServerSpec.xsl" \
  --out-file "$SPEC_PATH"

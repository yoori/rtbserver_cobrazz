#!/bin/bash

if [ "$#" -lt 6 ]; then
  echo "Usage: $0 APP_XML APP_XPATH BUILD_ROOT PLUGIN_ROOT VERSION RELEASE" >&2
  exit 1
fi

APP_XML=$1
APP_XPATH=$2
BUILD_ROOT=$3
PLUGIN_ROOT=$4
VERSION=$5
RELEASE=$6
EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

. "$EXEC/ColocationName.sh" "$APP_XML" "$PLUGIN_ROOT"

CLUSTER_XPATH="$APP_XPATH/serviceGroup[@descriptor = 'AICluster']"
CONFIG_OUT_DIR="$BUILD_ROOT/opt/foros/server-ai/etc/$COLOCATION_NAME/aicluster"
MANAGER_OUT_DIR="$BUILD_ROOT/opt/foros/manager/etc/config.d"
PRODUCT_IDENTIFIER="$COLOCATION_NAME-$VERSION-$RELEASE"

mkdir -p "$CONFIG_OUT_DIR" "$MANAGER_OUT_DIR"
mkdir -p "$BUILD_ROOT/u01/foros/server-ai/var/run"
mkdir -p "$BUILD_ROOT/opt/foros/server-ai/manager/$PRODUCT_IDENTIFIER"
mkdir -p "$BUILD_ROOT/opt/foros/manager/var/state/server-$COLOCATION_NAME"

"$EXEC/XsltTransformer.sh" \
  --var XPATH "$CLUSTER_XPATH" \
  --var COLOCATION_NAME "$COLOCATION_NAME" \
  --app-xml "$APP_XML" \
  --xsl "$XSLT_ROOT/Environment.xsl" \
  --out-file "$CONFIG_OUT_DIR/environment.sh" || exit $?
test -s "$CONFIG_OUT_DIR/environment.sh" || {
  echo "Generated environment.sh is empty" >&2
  exit 1
}

for SERVICE in AIAgent SegmentGenerator; do
  SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = 'AICluster/$SERVICE']"
  SERVICE_COUNT=$(
    "$EXEC/XPathGetValue.sh" \
      --xml "$APP_XML" \
      --xpath "count($SERVICE_XPATH)" \
      --plugin-root "$PLUGIN_ROOT")
  for ((i = 1; i <= SERVICE_COUNT; ++i)); do
    CURRENT_XPATH="$SERVICE_XPATH[$i]"
    HOSTS=$(
      "$EXEC/XPathGetValue.sh" \
        --xml "$APP_XML" \
        --xpath "$CURRENT_XPATH/@host" \
        --plugin-root "$PLUGIN_ROOT")
    for HOST in $HOSTS; do
      HOST_OUT_DIR="$CONFIG_OUT_DIR/$HOST"
      mkdir -p "$HOST_OUT_DIR"
      if [ "$SERVICE" = AIAgent ]; then
        XSL=AIAgent.xsl
        OUT=AIAgentConfig.sh
      else
        XSL=SegmentGenerator.xsl
        OUT=SegmentGeneratorConfig.json
      fi
      "$EXEC/XsltTransformer.sh" \
        --var XPATH "$CURRENT_XPATH" \
        --var HOST "$HOST" \
        --app-xml "$APP_XML" \
        --xsl "$XSLT_ROOT/$XSL" \
        --out-file "$HOST_OUT_DIR/$OUT" || exit $?
      test -s "$HOST_OUT_DIR/$OUT" || {
        echo "Generated $HOST_OUT_DIR/$OUT is empty" >&2
        exit 1
      }
    done
  done
done

"$EXEC/XsltTransformer.sh" \
  --var XPATH "$APP_XPATH" \
  --var COLOCATION_NAME "$COLOCATION_NAME" \
  --var VERSION "$VERSION" \
  --var RELEASE "$RELEASE" \
  --app-xml "$APP_XML" \
  --xsl "$XSLT_ROOT/Manager.xsl" \
  --out-file "$MANAGER_OUT_DIR/server-$PRODUCT_IDENTIFIER.xml" || exit $?
test -s "$MANAGER_OUT_DIR/server-$PRODUCT_IDENTIFIER.xml" || {
  echo "Generated manager configuration is empty" >&2
  exit 1
}

echo "AI cluster configuration completed successfully"

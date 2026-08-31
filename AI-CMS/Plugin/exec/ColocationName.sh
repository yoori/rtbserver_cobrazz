#!/bin/bash

APP_XML=$1
PLUGIN_ROOT=$2
COLOCATION_NAME=$(
  "$PLUGIN_ROOT/exec/XPathGetValue.sh" \
    --xml "$APP_XML" \
    --xpath "/colo:colocation/@name" \
    --plugin-root "$PLUGIN_ROOT" |
  tr '[:upper:] ' '[:lower:]_')

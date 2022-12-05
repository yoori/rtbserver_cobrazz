#!/bin/bash

APP_XML=$1
PLUGIN_ROOT=$2
EXEC=$PLUGIN_ROOT/exec

COLOCATION_NAME_XPATH="/colo:colocation/@name"
COLOCATION_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$COLOCATION_NAME_XPATH" \
  --plugin-root $PLUGIN_ROOT | tr '[:upper:] ' [:lower:]_)


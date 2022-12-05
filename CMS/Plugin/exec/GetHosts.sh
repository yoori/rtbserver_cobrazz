#!/bin/bash

APP_XML=''
PLUGIN_ROOT=''
HOST_SERVICE_XPATH=''
XSL_NAME=/xslt/GetHosts.xsl

while [ "$#" -gt "0" ]
do
  case $1 in
  --plugin-root)
    shift
    PLUGIN_ROOT=$1
    ;;
  --app-xml)
    shift
    APP_XML=$1
    ;;
  --host-service-xpath)
    shift
    HOST_SERVICE_XPATH=$1
    ;;
  --xsl-name)
    shift
    XSL_NAME=$1
    ;;
  esac
  shift
done

EXEC=$PLUGIN_ROOT/exec

$EXEC/XsltTransformer.sh \
  --var XPATH "$HOST_SERVICE_XPATH" \
  --app-xml $APP_XML \
  --xsl $PLUGIN_ROOT$XSL_NAME \
  --out-file -

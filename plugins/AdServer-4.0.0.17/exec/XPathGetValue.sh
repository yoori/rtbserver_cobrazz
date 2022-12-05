#!/bin/bash

XML=''
XPATH=''
PLUGIN_ROOT=''

while [ "$#" -gt "0" ]
do
  case $1 in
  --plugin-root)
    shift
    PLUGIN_ROOT=$1
    ;;
  --xml)
    shift
    XML=$1
    ;;
  --xpath)
    shift
    XPATH=$1
    ;;
  esac
  shift
done

$PLUGIN_ROOT/exec/XsltTransformer.sh \
  --var XPATH "$XPATH" \
  --app-xml $XML \
  --xsl $PLUGIN_ROOT/xslt/XPathGetValue.xsl \
  --out-file - || \
  echo "error on xpath: $XPATH"

exit $?

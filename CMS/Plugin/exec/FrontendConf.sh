#!/bin/bash

TRACE=1

if [ $# -lt 7 ]
then
  echo "$0 : number of argument must be equal 8."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
FRONTEND_XPATH=$2
PLUGIN_ROOT=$3
OUT_DIR=$4
OUT_DIR_SUFFIX=$5
CONF_TYPE=$6
FRONTEND_COUNT=$7

EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

CONFIG_NAME="FeConfig.xml"
CONF_SUFFIX=""

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Frontend/AdFrontend.xsl \
  --out-file $CONFIG_NAME \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --var CONF_TYPE $CONF_TYPE

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --xsl $XSLT_ROOT/Frontend/nginx/nginx.conf.xsl \
  --out-file conf1/nginx.conf \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --var NGINX_INDEX 1 \
  --var CONF_TYPE $CONF_TYPE

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --xsl $XSLT_ROOT/Frontend/nginx/nginx.conf.xsl \
  --out-file conf2/nginx.conf \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --var NGINX_INDEX 2 \
  --var CONF_TYPE $CONF_TYPE

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --xsl $XSLT_ROOT/Frontend/nginx/nginx.conf.xsl \
  --out-file conf3/nginx.conf \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --var NGINX_INDEX 3 \
  --var CONF_TYPE $CONF_TYPE

let "EXIT_CODE|=$?"

for (( i=1; i<=FRONTEND_COUNT; i++ ))
do
  FRONTEND_HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$FRONTEND_XPATH[$i]" --plugin-root $PLUGIN_ROOT`

  for host in $FRONTEND_HOSTS
  do
    echo -e "\
  <cross-domain-policy>\n\
    <site-control permitted-cross-domain-policies=\"all\"/>\n\
    <allow-access-from domain=\"*\" secure=\"false\"/>\n\
    <allow-http-request-headers-from domain=\"*\" headers=\"*\" secure=\"false\"/>\n\
  </cross-domain-policy>" > $OUT_DIR/$host/"$CONF_SUFFIX"conf1/crossdomain.xml

  cp "$OUT_DIR/$host/"$CONF_SUFFIX"conf1/crossdomain.xml" "$OUT_DIR/$host/"$CONF_SUFFIX"conf2/crossdomain.xml"
  cp "$OUT_DIR/$host/"$CONF_SUFFIX"conf1/crossdomain.xml" "$OUT_DIR/$host/"$CONF_SUFFIX"conf3/crossdomain.xml"
  done
done

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/PageSense/PSConfig.xsl \
  --var PROTOCOL "http" \
  --out-file "$CONF_SUFFIX"PS/ps_http_vars \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$FRONTEND_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/PageSense/PSConfig.xsl \
  --var PROTOCOL "https" \
  --out-file "$CONF_SUFFIX"PS/ps_https_vars \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
   --services-xpath "$FRONTEND_XPATH" \
   --app-xml $APP_XML \
   --plugin-root $PLUGIN_ROOT \
   --out-dir $OUT_DIR \
   --cmd "mkdir -p \$HOST_DIR/$CONF_SUFFIX" \
   --cmd "cp -r $PLUGIN_ROOT/data/FrontendSubCluster/Frontend/* \$HOST_DIR/$CONF_SUFFIX" \
   --cmd "mkdir -p \$HOST_DIR/\"$CONF_SUFFIX\"conf1" \
   --cmd "touch \$HOST_DIR/\"$CONF_SUFFIX\"conf1/empty" \
   --cmd "mkdir -p \$HOST_DIR/\"$CONF_SUFFIX\"conf2" \
   --cmd "touch \$HOST_DIR/\"$CONF_SUFFIX\"conf2/empty" \
   --cmd "mkdir -p \$HOST_DIR/\"$CONF_SUFFIX\"conf3" \
   --cmd "touch \$HOST_DIR/\"$CONF_SUFFIX\"conf3/empty" \
   --cmd "cp $OUT_DIR/frontend_cert/* \$HOST_DIR/cert"
let "EXIT_CODE|=$?"

if [ $EXIT_CODE -eq 0 ]
then
  echo "config for Frontend $CONF_TYPE completed successfully"
else
  echo "config for Frontend $CONF_TYPE contains errors" >2
fi

exit $EXIT_CODE

#!/bin/bash

if [ $# -lt 5 ]
then
  echo "$0 : number of argument must be equal 5."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
OUT_DIR=$3
PLUGIN_ROOT=$4
OUT_DIR_SUFFIX=$5

EXEC=$PLUGIN_ROOT/exec

XSLT_ROOT=$PLUGIN_ROOT/xslt

DACS_LOCAL_PROXY_NAME=be.sh

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

LOCAL_PROXY_DESCR=AdCluster/BackendSubCluster/LocalProxy
CHANNEL_PROXY_DESCR=$LOCAL_PROXY_DESCR/ChannelProxy
STUNNEL_CLIENT_DESCR=$LOCAL_PROXY_DESCR/STunnelClient

## configure ChannelProxy
CHANNEL_PROXY_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_PROXY_DESCR']"
$EXEC/ServiceConf.sh \
  --services-xpath "$CHANNEL_PROXY_XPATH" \
  --var MODE REMOTE-MODE \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ChannelServing/ChannelProxy.xsl \
  --out-file ChannelProxy.xml \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "channel_proxy" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:channelProxy" \
  --services-xpath "$CHANNEL_PROXY_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$DACS_LOCAL_PROXY_NAME

let "EXIT_CODE|=$?"

## configure STunnelClient
STUNNEL_CLIENT_XPATH="$CLUSTER_XPATH/service[@descriptor = '$STUNNEL_CLIENT_DESCR']"
$EXEC/ServiceConf.sh \
  --services-xpath "$STUNNEL_CLIENT_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/STunnelClient.xsl \
  --out-file conf/stunnel_client.conf \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --services-xpath "$STUNNEL_CLIENT_XPATH" \
  --app-xml $APP_XML \
  --plugin-root $PLUGIN_ROOT \
  --out-dir $OUT_DIR \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $PLUGIN_ROOT/data/ProxyCluster/cert/npca.pem $HOST_DIR/cert' \
  --cmd 'cp $PLUGIN_ROOT/data/ProxyCluster/cert/npcert.pem $HOST_DIR/cert' \
  --cmd 'cp $PLUGIN_ROOT/data/ProxyCluster/cert/npkey.pem $HOST_DIR/cert'

let "EXIT_CODE|=$?"

if [ $EXIT_CODE -eq 0 ]
then
  echo config for LocalProxySubCluster completed successfully
else
  echo config for LocalProxySubCluster contains errors >2
fi

exit $EXIT_CODE

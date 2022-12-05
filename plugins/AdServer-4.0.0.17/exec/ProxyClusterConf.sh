#!/bin/bash

if [ $# -lt 6 ]
then
  echo "$0 : number of argument must be equal 6."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
BUILD_ROOT=$3
PLUGIN_ROOT=$4
CLUSTER_INDEX=$5
CLUSTER_TYPE=$6

ROOT_OUT_DIR=$BUILD_ROOT/opt/foros/server/etc
EXEC=$PLUGIN_ROOT/exec
. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT
. $EXEC/functions

CLUSTER_DESCR_XPATH="$CLUSTER_XPATH/@descriptor"
CLUSTER_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CLUSTER_DESCR_XPATH" \
               --plugin-root $PLUGIN_ROOT | tr "[:upper:] " "[:lower:]_")

OUT_DIR_SUFFIX=$COLOCATION_NAME/$CLUSTER_NAME-$CLUSTER_INDEX
OUT_DIR=$ROOT_OUT_DIR/$OUT_DIR_SUFFIX
START_SCRIPT_NAME="$COLOCATION_NAME-$CLUSTER_NAME-$CLUSTER_INDEX"


XSLT_ROOT=$PLUGIN_ROOT/xslt
DACS_PBE_NAME=pbe.sh

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

USER_INFO_EXCHANGER_DESCR=AdProxyCluster/UserInfoExchanger
PROXY_CAMPAIGN_SERVER_DESCR=AdProxyCluster/CampaignServer
PROXY_CHANNEL_PROXY_DESCR=AdProxyCluster/ChannelProxy
STUNNEL_SERVER_DESCR=AdProxyCluster/STunnelServer

mkdir -p ${OUT_DIR}

## configure CleanupLogs
. $EXEC/functions
SERVICES_XPATH="$CLUSTER_XPATH//service"
SERVICES_HOSTS=`get_hosts_by_xpath "$APP_XML" "$PLUGIN_ROOT" "$EXEC" "$SERVICES_XPATH"`

$EXEC/ServiceConf.sh \
  --services-xpath "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $PLUGIN_ROOT/xslt/LogProcessing/CleanupLogs.xsl \
  --out-file conf/cleanup_logs.conf \
  --service-hosts "$SERVICES_HOSTS" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

## configure UserInfoExchanger
USER_INFO_EXCHANGER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_INFO_EXCHANGER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$USER_INFO_EXCHANGER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/UserInfoProfiling/UserInfoExchanger.xsl \
  --out-file UserInfoExchanger.xml \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE += $?"

START_SCRIPT_NAME="$COLOCATION_NAME-$CLUSTER_NAME"

$EXEC/CurrentEnvGen.sh \
  --service-name "user_info_exchanger" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:userInfoExchanger" \
  --services-xpath "$USER_INFO_EXCHANGER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$DACS_PBE_NAME

let "EXIT_CODE += $?"

## configure CampaignServer in proxy mode
CAMPAIGN_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$PROXY_CAMPAIGN_SERVER_DESCR']"
$EXEC/ServiceConf.sh \
 --services-xpath "$CAMPAIGN_SERVER_XPATH" \
 --var MODE PROXY-MODE \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/CampaignManagement/CampaignServer.xsl \
 --out-file CampaignServerConfig.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

let "EXIT_CODE += $?"

$EXEC/CurrentEnvGen.sh \
  --def-port-add-prefix "proxy-" \
  --service-name "campaign_server" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:campaignServer" \
  --services-xpath "$CAMPAIGN_SERVER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$DACS_PBE_NAME


let "EXIT_CODE += $?"

## configure ChannelProxy
CHANNEL_PROXY_XPATH="$CLUSTER_XPATH/service[@descriptor = '$PROXY_CHANNEL_PROXY_DESCR']"
$EXEC/ServiceConf.sh \
  --services-xpath "$CHANNEL_PROXY_XPATH" \
  --var MODE PROXY-MODE \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ChannelServing/ChannelProxy.xsl \
  --out-file ChannelProxy.xml \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT


let "EXIT_CODE += $?"

$EXEC/CurrentEnvGen.sh \
  --service-name "channel_proxy" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:channelProxy" \
  --services-xpath "$CHANNEL_PROXY_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$DACS_PBE_NAME

let "EXIT_CODE += $?"

## configure SyncLogs
SYNC_PROCESSING_XPATH="$CLUSTER_XPATH/service["\
"@descriptor = '$PROXY_CHANNEL_PROXY_DESCR' or "\
"@descriptor = '$PROXY_CAMPAIGN_SERVER_DESCR' or "\
"@descriptor = '$STUNNEL_SERVER_DESCR']"

SYNC_SERVICE_HOSTS=`get_hosts_by_xpath "$APP_XML" "$PLUGIN_ROOT" "$EXEC" "$SYNC_PROCESSING_XPATH"`

$EXEC/ServiceConf.sh \
  --services-xpath "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/ProxySyncLogs.xsl \
  --out-file SyncLogsConfig.xml \
  --service-hosts "$SYNC_SERVICE_HOSTS" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE += $?"

$EXEC/CurrentEnvGen.sh \
  --def-port-add-prefix "proxy-" \
  --service-name "sync_logs" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:cluster/cfg:syncLogs" \
  --services-xpath "$CLUSTER_XPATH" \
  --service-hosts "$SYNC_SERVICE_HOSTS" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$DACS_PBE_NAME


let "EXIT_CODE += $?"

## configure STunnelServer
STUNNEL_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$STUNNEL_SERVER_DESCR']"
$EXEC/ServiceConf.sh \
  --services-xpath "$STUNNEL_SERVER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/STunnelServer.xsl \
  --out-file conf/stunnel_server.conf \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE += $?"

## configure RSyncServer
STUNNEL_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$STUNNEL_SERVER_DESCR']"
$EXEC/ServiceConf.sh \
  --services-xpath "$STUNNEL_SERVER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/RSyncServer.xsl \
  --out-file conf/rsync_server.conf \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE += $?"

$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --var OUT_DIR "$OUT_DIR_SUFFIX" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ProxyAdCluster/Environment.xsl \
  --out-file $OUT_DIR/environment.sh

let "EXIT_CODE += $?"

if [ $EXIT_CODE -eq 0 ]; then

cp $EXEC/copy_and_backup.sh $OUT_DIR/../copy_and_backup.sh

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --services-xpath "$USER_INFO_EXCHANGER_XPATH" \
  --services-xpath "$CHANNEL_PROXY_XPATH" \
  --services-xpath "$CAMPAIGN_SERVER_XPATH" \
  --services-xpath "$STUNNEL_SERVER_XPATH" \
  --app-xml $APP_XML \
  --plugin-root $PLUGIN_ROOT \
  --out-dir $OUT_DIR \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd "cp -f \$HOST_DIR/CurrentEnv/$DACS_PBE_NAME \$HOST_DIR/CurrentEnv/cluster" \
  --cmd 'cp $PLUGIN_ROOT/data/ProxyCluster/cert/* $HOST_DIR/cert' \
  --cmd 'chmod og-wr $HOST_DIR/cert/* 2>/dev/null'

let "EXIT_CODE|=$?"

fi

if [ $EXIT_CODE -eq 0 ]
then
  echo config for ProxyCluster completed successfully
else
  echo config for ProxyCluster contains errors >2
fi

exit $EXIT_CODE

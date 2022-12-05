#!/bin/bash

if [ $# -lt 7 ]
then
  echo "$0 : number of argument must be equal 7."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
BUILD_ROOT=$3
PLUGIN_ROOT=$4
OUT_DIR=$5
START_SCRIPT_NAME=$6
OUT_DIR_SUFFIX=$7

ROOT_OUT_DIR=$BUILD_ROOT/opt/foros/server/etc
XSLT_ROOT=$PLUGIN_ROOT/xslt
DATA=$PLUGIN_ROOT/data

BACKEND_CLUSTER_DESCR=AdCluster/BackendSubCluster
REQUEST_INFO_MANAGER_DESCR=$BACKEND_CLUSTER_DESCR/RequestInfoManager
FRONTEND_CLUSTER_DESCR=AdCluster/FrontendSubCluster
CHANNEL_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelServer
LOCAL_PROXY_CLUSTER_DESCR=$BACKEND_CLUSTER_DESCR/LocalProxy
LOG_GENERALIZER_DESCR=$BACKEND_CLUSTER_DESCR/LogGeneralizer
CAMPAIGN_SERVER_DESCR=$BACKEND_CLUSTER_DESCR/CampaignServer

EXEC=$PLUGIN_ROOT/exec

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

mkdir -p ${OUT_DIR}

. $EXEC/functions
#### configure DACS

FE_CLUSTERS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$FRONTEND_CLUSTER_DESCR']"

$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --var OUT_DIR "$OUT_DIR_SUFFIX" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Environment.xsl \
  --out-file $OUT_DIR/environment.sh

let "EXIT_CODE|=$?"

DB_ACCESS_XPATH="$CLUSTER_XPATH//service[@descriptor = '$LOG_GENERALIZER_DESCR' or
  @descriptor = '$CHANNEL_SERVER_DESCR' or @descriptor = '$CAMPAIGN_SERVER_DESCR']"
DB_ACCESS_HOSTS=`get_hosts_by_xpath "$APP_XML" "$PLUGIN_ROOT" "$EXEC" "$DB_ACCESS_XPATH"`

$EXEC/ServiceConf.sh \
  --services-xpath "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/DBAccess.xsl \
  --out-file DBAccess.pm \
  --service-hosts "$DB_ACCESS_HOSTS" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"
$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ErrorsCheck.xslt \
  --out-file -

let "EXIT_CODE|=$?"

cp $EXEC/copy_and_backup.sh $OUT_DIR/../copy_and_backup.sh

let "EXIT_CODE|=$?"

cp $EXEC/rsync_immutable.sh $OUT_DIR/../rsync_immutable.sh

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --plugin-root $PLUGIN_ROOT \
  --app-xml $APP_XML \
  --out-dir $OUT_DIR \
  --services-xpath "$CLUSTER_XPATH//service[@descriptor = 'AdCluster/FrontendSubCluster/Frontend' or \
     @descriptor = '$REQUEST_INFO_MANAGER_DESCR' or @descriptor = 'AdCluster/FrontendSubCluster/NginxFrontend']" \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $OUT_DIR/frontend_cert/* $HOST_DIR/cert' \
  --cmd 'rm $HOST_DIR/cert/SignedUids.pm'

let "EXIT_CODE|=$?"

chmod og-r $OUT_DIR/cert/stunnel.crt 2>/dev/null

if [ $EXIT_CODE -eq 0 ]
then
  echo config for DACS config completed successfully
else
  echo config for DACS config contains errors
fi

exit $EXIT_CODE

#!/bin/bash

TRACE=1

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
OUT_DIR_SUFFIX=$6
CLUSTER_ID=$7

EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

DACS_FE_NAME=fe.sh

WORKSPACE_OUT_DIR=$BUILD_ROOT/u01/foros/server/var

FRONTEND_CLUSTER_DESCR=AdCluster/FrontendSubCluster

USER_INFO_MANAGER_DESCR=$FRONTEND_CLUSTER_DESCR/UserInfoManager
USER_INFO_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/UserInfoController

CHANNEL_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelServer
CHANNEL_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelController
CHANNEL_SEARCH_SERVICE_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelSearchService

USER_BIND_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/UserBindServer
USER_BIND_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/UserBindController

BILLING_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/BillingServer

CAMPAIGN_MANAGER_DESCR=$FRONTEND_CLUSTER_DESCR/CampaignManager
FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/Frontend

HTTP_FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/HttpFrontend

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

let "EXIT_CODE|=$?"

### UserInfoProfiling

## configure UserInfoManager
USER_INFO_MANAGER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_INFO_MANAGER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$USER_INFO_MANAGER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/UserInfoProfiling/UserInfoManager.xsl \
  --out-file UserInfoManagerConfig.xml \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --cluster-id $CLUSTER_ID

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "user_info_manager" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML  \
  --search-xpath "configuration/cfg:userInfoManager" \
  --services-xpath "$USER_INFO_MANAGER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/fe.sh

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --services-xpath "$USER_INFO_MANAGER_XPATH" \
  --app-xml $APP_XML \
  --plugin-root $PLUGIN_ROOT \
  --out-dir $OUT_DIR \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $PLUGIN_ROOT/data/BackendSubCluster/cert/* $HOST_DIR/cert'

let "EXIT_CODE|=$?"

## configure UserInfoController
USER_INFO_CONTROLLER_XPATH="$CLUSTER_XPATH/service[
  @descriptor = '$USER_INFO_CONTROLLER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$USER_INFO_CONTROLLER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/UserInfoProfiling/UserInfoController.xsl \
  --out-file UserInfoController.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

### ChannelServing

## configure ChannelServer
CHANNEL_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SERVER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$CHANNEL_SERVER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ChannelServing/ChannelServer.xsl \
  --out-file ChannelServer.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

## configure ChannelController
CHANNEL_CONTROLLER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_CONTROLLER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$CHANNEL_CONTROLLER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ChannelServing/ChannelController.xsl \
  --out-file ChannelController.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --cluster-id $CLUSTER_ID

let "EXIT_CODE|=$?"

CHANNEL_SEARCH_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SEARCH_SERVICE_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$CHANNEL_SEARCH_SERVICE_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/ChannelServing/ChannelSearchService.xsl \
  --out-file ChannelSearchServiceConfig.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

### UserBindServer
USER_BIND_SERVER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_BIND_SERVER_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$USER_BIND_SERVER_SERVICE_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/UserInfoProfiling/UserBindServer.xsl \
 --out-file UserBindServer.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT \
 --cluster-id $CLUSTER_ID

let "EXIT_CODE|=$?"

### UserBindController
USER_BIND_CONTROLLER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_BIND_CONTROLLER_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$USER_BIND_CONTROLLER_SERVICE_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/UserInfoProfiling/UserBindController.xsl \
 --out-file UserBindController.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

### BillingServer
BILLING_SERVER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$BILLING_SERVER_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$BILLING_SERVER_SERVICE_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/CampaignManagement/BillingServer.xsl \
 --out-file BillingServer.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT \
 --cluster-id $CLUSTER_ID

let "EXIT_CODE|=$?"

## configure CampaignManager
CAMPAIGN_MANAGER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CAMPAIGN_MANAGER_DESCR']"

CAMPAIGN_MANAGER_COUNT_XPATH="count($CAMPAIGN_MANAGER_XPATH)"
CAMPAIGN_MANAGER_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CAMPAIGN_MANAGER_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`

if [ $CAMPAIGN_MANAGER_COUNT -ne 0 ]
then
  $EXEC/ServiceConf.sh \
    --services-xpath "$CAMPAIGN_MANAGER_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/CampaignManagement/CampaignManager.xsl \
    --out-file CampaignManagerConfig.xml \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT \
    --cluster-id $CLUSTER_ID \
    --var MODE AD

  let "EXIT_CODE|=$?"
  $EXEC/ProcessHostFiles.sh \
      --services-xpath "$CAMPAIGN_MANAGER_XPATH" \
      --app-xml $APP_XML \
      --plugin-root $PLUGIN_ROOT \
      --out-dir $OUT_DIR \
      --cmd 'mkdir -p $HOST_DIR' \
      --cmd 'cp -r $PLUGIN_ROOT/data/FrontendSubCluster/CampaignManager/* $HOST_DIR'

  let "EXIT_CODE|=$?"
fi

### configure Frontend
HTTP_FRONTEND_XPATH="$CLUSTER_XPATH/service[@descriptor = '$HTTP_FRONTEND_DESCR']"
HTTP_FRONTEND_COUNT_XPATH="count($HTTP_FRONTEND_XPATH)"

HTTP_FRONTEND_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$HTTP_FRONTEND_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`

if [ $CAMPAIGN_MANAGER_COUNT -ne 0 ] || [ $HTTP_FRONTEND_COUNT -ne 0 ]
then
  $EXEC/ServiceConf.sh \
    --services-xpath "$CAMPAIGN_MANAGER_XPATH | $HTTP_FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/CampaignManagement/DomainConfig.xsl \
    --out-file DomainConfig.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT \
    --var MODE AD

  let "EXIT_CODE|=$?"
fi

if [ $HTTP_FRONTEND_COUNT -ne 0 ]
then
  # RTB Ad server
  $EXEC/ServiceConf.sh \
    --services-xpath "$HTTP_FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Frontend/FCGIAdServer.xsl \
    --out-file FCGIAdServerConfig.xml \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "fcgi_adserver" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --relative-port-xpath "configuration/cfg:frontend/cfg:adFCGINetworkParams/@port" \
    --services-xpath "$HTTP_FRONTEND_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/$DACS_FE_NAME

  let "EXIT_CODE|=$?"

  # FCGI Track server
  for TRACK_INDEX in $(seq 1 2)
  do
    $EXEC/ServiceConf.sh \
      --macro "{FCGITRACKSERVER_INDEX}" "$TRACK_INDEX" \
      --services-xpath "$HTTP_FRONTEND_XPATH" \
      --app-xml "$APP_XML" \
      --xsl "$XSLT_ROOT/Frontend/FCGITrackServer.xsl" \
      --out-file FCGITrackServer${TRACK_INDEX}Config.xml \
      --out-dir-suffix "$OUT_DIR_SUFFIX" \
      --out-dir "$OUT_DIR" \
      --plugin-root "$PLUGIN_ROOT"

    let "EXIT_CODE|=$?"

    $EXEC/CurrentEnvGen.sh \
      --service-name "fcgi_trackserver$TRACK_INDEX" \
      --plugin-root "$PLUGIN_ROOT" \
      --app-xml "$APP_XML" \
      --relative-port-xpath "configuration/cfg:frontend/cfg:trackFCGI${TRACK_INDEX}NetworkParams/@port" \
      --services-xpath "$HTTP_FRONTEND_XPATH" \
      --out-dir "$OUT_DIR" \
      --out-file "CurrentEnv/$DACS_FE_NAME"

    let "EXIT_CODE|=$?"
  done

  # RTB FCGI server
  for RTB_INDEX in $(seq 1 2)
  do
    $EXEC/ServiceConf.sh \
      --macro "{FCGIRTBSERVER_INDEX}" "$RTB_INDEX" \
      --services-xpath "$HTTP_FRONTEND_XPATH" \
      --app-xml "$APP_XML" \
      --xsl "$XSLT_ROOT/Frontend/FCGIRtbServer.xsl" \
      --out-file "FCGIRtbServer${RTB_INDEX}Config.xml" \
      --out-dir-suffix "$OUT_DIR_SUFFIX" \
      --out-dir "$OUT_DIR" \
      --plugin-root "$PLUGIN_ROOT"

    let "EXIT_CODE|=$?"

    $EXEC/CurrentEnvGen.sh \
      --service-name "fcgi_rtbserver$RTB_INDEX" \
      --plugin-root "$PLUGIN_ROOT" \
      --app-xml "$APP_XML" \
      --relative-port-xpath "configuration/cfg:frontend/cfg:rtbFCGI${RTB_INDEX}NetworkParams/@port" \
      --services-xpath "$HTTP_FRONTEND_XPATH" \
      --out-dir "$OUT_DIR" \
      --out-file "CurrentEnv/$DACS_FE_NAME"

    let "EXIT_CODE|=$?"
  done

  # UserBind FCGI server
  $EXEC/ServiceConf.sh \
    --services-xpath "$HTTP_FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Frontend/FCGIUserBindServer.xsl \
    --out-file FCGIUserBindServerConfig.xml \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "fcgi_userbindserver" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --relative-port-xpath "configuration/cfg:frontend/cfg:userBindFCGI1NetworkParams/@port" \
    --services-xpath "$HTTP_FRONTEND_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/$DACS_FE_NAME

  let "EXIT_CODE|=$?"

  # http server
  $EXEC/FrontendConf.sh $APP_XML \
    "$HTTP_FRONTEND_XPATH" \
    "$PLUGIN_ROOT" \
    "$OUT_DIR" \
    "$OUT_DIR_SUFFIX" \
    "nginx" \
    "$HTTP_FRONTEND_COUNT"

  let "EXIT_CODE|=$?"

fi

$EXEC/FrontendSubClusterCurrentEnvConf.sh \
  "$APP_XML" \
  "$CLUSTER_XPATH" \
  "$OUT_DIR" \
  "$PLUGIN_ROOT"

let "EXIT_CODE|=$?"

mkdir -p $WORKSPACE_OUT_DIR
cp -r $PLUGIN_ROOT/data/FrontendSubClusterWorkspace/* $WORKSPACE_OUT_DIR

let "EXIT_CODE|=$?"

if [ $EXIT_CODE -eq 0 ]
then
  echo config for FrontendSubCluster completed successfully
else
  echo config for FrontendSubCluster contains errors >2
fi

exit $EXIT_CODE

#!/bin/bash

TRACE=1

if [ $# -lt 4 ]
then
  echo "$0 : number of argument must be equal 4."
  exit 1
fi

XIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
OUT_DIR=$3
PLUGIN_ROOT=$4

EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

FE_CURRENT_NAME=fe.sh

FRONTEND_CLUSTER_DESCR=AdCluster/FrontendSubCluster

CHANNEL_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelServer
CHANNEL_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelController
CHANNEL_SEARCH_SERVICE_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelSearchService

USER_BIND_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/UserBindServer
USER_BIND_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/UserBindController

CAMPAIGN_MANAGER_DESCR=$FRONTEND_CLUSTER_DESCR/CampaignManager
CONV_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/ConvServer
BILLING_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/BillingServer
FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/Frontend
NGINX_FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/NginxFrontend

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

let "EXIT_CODE|=$?"

CHANNEL_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SERVER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "channel_server" \
  --cluster "ad" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:channelServer" \
  --services-xpath "$CHANNEL_SERVER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME



let "EXIT_CODE|=$?"

CHANNEL_CONTROLLER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_CONTROLLER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "channel_controller" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:channelController" \
  --services-xpath "$CHANNEL_CONTROLLER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

CHANNEL_SEARCH_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SEARCH_SERVICE_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "channel_search_service" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:channelSearchService" \
  --services-xpath "$CHANNEL_SEARCH_SERVICE_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

USER_BIND_SERVER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_BIND_SERVER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "user_bind_server" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:userBindServer" \
  --services-xpath "$USER_BIND_SERVER_SERVICE_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

USER_BIND_CONTROLLER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_BIND_CONTROLLER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "user_bind_controller" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:userBindController" \
  --services-xpath "$USER_BIND_CONTROLLER_SERVICE_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

##
CAMPAIGN_MANAGER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CAMPAIGN_MANAGER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "campaign_manager" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:campaignManager" \
  --services-xpath "$CAMPAIGN_MANAGER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

##
CONV_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CONV_SERVER_DESCR']"

$EXEC/CurrentEnvGen.sh \
    --service-name "conv_server" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:convServer" \
  --services-xpath "$CONV_SERVER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

##
BILLING_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$BILLING_SERVER_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "billing_server" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:billingServer" \
  --services-xpath "$BILLING_SERVER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

##
FRONTEND_XPATH="$CLUSTER_XPATH/service[@descriptor = '$FRONTEND_DESCR']"

$EXEC/CurrentEnvGen.sh \
  --service-name "ad_frontend" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --port-xpath "$CLUSTER_XPATH/../configuration/cfg:cluster/cfg:coloParams/cfg:virtualServer[1]/@internal_port" \
  --services-xpath "$FRONTEND_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

##
NGINX_FRONTEND_XPATH="$CLUSTER_XPATH/service[@descriptor = '$NGINX_FRONTEND_DESCR']"
ALL_NGINX_VIRTUAL_SERVERS="$CLUSTER_XPATH/../configuration/cfg:cluster/cfg:coloParams/cfg:virtualServer
  [(count(@enable) = 0 or @enable='true') and @type = 'nginx' ]"

$EXEC/CurrentEnvGen.sh \
  --service-name "nginx_frontend" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --port-xpath "$ALL_NGINX_VIRTUAL_SERVERS[1]/@internal_port" \
  --services-xpath "$NGINX_FRONTEND_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/$FE_CURRENT_NAME

let "EXIT_CODE|=$?"

exit $EXIT_CODE

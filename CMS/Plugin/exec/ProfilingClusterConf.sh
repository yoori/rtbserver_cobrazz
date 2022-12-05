#!/bin/bash

if [ $# -lt 6 ]
then
  echo "$0 : number of argument must be equal 6."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
PLUGIN_ROOT=$4
CLUSTER_NAME=$5
PRODUCT_IDENTIFIER=$6

EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT

CLUSTER_DESCR_XPATH="$CLUSTER_XPATH/@descriptor"
CLUSTER_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CLUSTER_DESCR_XPATH" \
               --plugin-root $PLUGIN_ROOT | tr "[:upper:] " "[:lower:]_")

BACKEND_CLUSTER_DESCR=AdProfilingCluster/BackendSubCluster
CAMPAIGN_SERVER_DESCR=$BACKEND_CLUSTER_DESCR/CampaignServer
DICTIONARY_PROVIDER_DESCR=$BACKEND_CLUSTER_DESCR/DictionaryProvider

BE_CLUSTER_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$BACKEND_CLUSTER_DESCR']"

BUILD_DIR=$3
OUT_DIR_SUFFIX=$COLOCATION_NAME/$CLUSTER_NAME
OUT_DIR=$3/opt/foros/server/etc/$OUT_DIR_SUFFIX

WORKSPACE_OUT_DIR=$BUILD_DIR/u01/foros/server/var

mkdir -p $OUT_DIR
mkdir -p $WORKSPACE_OUT_DIR

# DACS configuration
DACS_FE_NAME=fe.sh
START_SCRIPT_NAME="$COLOCATION_NAME-$CLUSTER_NAME"

$EXEC/FrontendCertificateGen.sh \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml "$APP_XML" \
  --cluster-xpath "$CLUSTER_XPATH" \
  --certificate-root "$OUT_DIR/frontend_cert/"

let "EXIT_CODE|=$?"


#### configure DACS
$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --var OUT_DIR "$OUT_DIR_SUFFIX" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Environment.xsl \
  --out-file $OUT_DIR/environment.sh

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --plugin-root $PLUGIN_ROOT \
  --app-xml $APP_XML \
  --out-dir $OUT_DIR \
  --services-xpath "$CLUSTER_XPATH//service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']" \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $OUT_DIR/frontend_cert/* $HOST_DIR/cert'

$EXEC/ProcessHostFiles.sh \
  --plugin-root $PLUGIN_ROOT \
  --app-xml $APP_XML \
  --out-dir $OUT_DIR \
  --services-xpath "$CLUSTER_XPATH//service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']" \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $OUT_DIR/frontend_cert/* $HOST_DIR/cert' \
  --cmd 'rm $HOST_DIR/cert/SignedUids.pm'

let "EXIT_CODE|=$?"

chmod og-r $OUT_DIR/cert/stunnel.crt 2>/dev/null

# All subclusters service configuration
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

let "EXIT_CODE|=$?"

# Backend subcluster configuration

### CampaignServer
CAMPAIGN_SERVER_XPATH="$BE_CLUSTER_XPATH/service[@descriptor = '$CAMPAIGN_SERVER_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$CAMPAIGN_SERVER_XPATH" \
 --var MODE CENTRAL-OR-REMOTE-MODE \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/CampaignManagement/CampaignServer.xsl \
 --out-file CampaignServerConfig.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
 --service-name "campaign_server" \
 --plugin-root "$PLUGIN_ROOT" \
 --app-xml $APP_XML \
 --search-xpath "configuration/cfg:campaignServer" \
 --services-xpath "$CAMPAIGN_SERVER_XPATH" \
 --out-dir $OUT_DIR \
 --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

### DictionaryProvider
DICTIONARY_PROVIDER_SERVICE_XPATH="$BE_CLUSTER_XPATH/service[@descriptor = '$DICTIONARY_PROVIDER_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$DICTIONARY_PROVIDER_SERVICE_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/ChannelServing/DictionaryProvider.xsl \
 --out-file DictionaryProvider.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

$EXEC/CurrentEnvGen.sh \
  --service-name "dictionary_provider" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:dictionaryProvider" \
  --services-xpath "$DICTIONARY_PROVIDER_SERVICE_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --services-xpath "$CAMPAIGN_SERVER_XPATH" \
  --app-xml $APP_XML \
  --plugin-root $PLUGIN_ROOT \
  --out-dir $OUT_DIR \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $PLUGIN_ROOT/data/BackendSubCluster/cert/* $HOST_DIR/cert'

let "EXIT_CODE|=$?"

# Local proxy subcluster configuration
LOCAL_PROXY_CLUSTER_DESCR=AdProfilingCluster/BackendSubCluster/LocalProxy
LOCAL_PROXY_CLUSTER_XPATH="$BE_CLUSTER_XPATH/serviceGroup[@descriptor = '$LOCAL_PROXY_CLUSTER_DESCR']"
LOCAL_PROXY_CLUSTER_COUNT="count($LOCAL_PROXY_CLUSTER_XPATH)"
LOCAL_PROXY_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$LOCAL_PROXY_CLUSTER_COUNT" --plugin-root $PLUGIN_ROOT`
if [ $LOCAL_PROXY_COUNT -ne 0 ]
then
  ## configure ChannelProxy
  CHANNEL_PROXY_XPATH="$LOCAL_PROXY_CLUSTER_XPATH/service[@descriptor = '$LOCAL_PROXY_CLUSTER_DESCR/ChannelProxy']"
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
    --out-file CurrentEnv/be.sh

  let "EXIT_CODE|=$?"
fi

# Frontend subcluster configuration
FRONTEND_CLUSTER_DESCR=AdProfilingCluster/FrontendSubCluster
CAMPAIGN_MANAGER_DESCR=$FRONTEND_CLUSTER_DESCR/CampaignManager
CHANNEL_SERVER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelServer
CHANNEL_CONTROLLER_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelController
CHANNEL_SEARCH_SERVICE_DESCR=$FRONTEND_CLUSTER_DESCR/ChannelSearchService

FE_CLUSTERS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$FRONTEND_CLUSTER_DESCR']"
FE_CLUSTERS_COUNT="count($FE_CLUSTERS_XPATH)"
FE_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$FE_CLUSTERS_COUNT" --plugin-root $PLUGIN_ROOT`
for (( CLUSTER_ID=1; CLUSTER_ID<=FE_COUNT; CLUSTER_ID++ ))
do
  FE_CLUSTER_XPATH="${FE_CLUSTERS_XPATH}[$CLUSTER_ID]"

  ### ChannelServing
  ## configure ChannelServer
  CHANNEL_SERVER_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SERVER_DESCR']"

  $EXEC/ServiceConf.sh \
    --services-xpath "$CHANNEL_SERVER_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/ChannelServing/ChannelServer.xsl \
    --out-file ChannelServer.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "channel_server" \
    --cluster "ad" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --search-xpath "configuration/cfg:channelServer" \
    --services-xpath "$CHANNEL_SERVER_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/fe.sh

  let "EXIT_CODE|=$?"

  ## configure ChannelController
  CHANNEL_CONTROLLER_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$CHANNEL_CONTROLLER_DESCR']"

  $EXEC/ServiceConf.sh \
    --services-xpath "$CHANNEL_CONTROLLER_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/ChannelServing/ChannelController.xsl \
    --out-file ChannelController.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT \
    --cluster-id $CLUSTER_ID

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "channel_controller" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --search-xpath "configuration/cfg:channelController" \
    --services-xpath "$CHANNEL_CONTROLLER_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/fe.sh

  ## configure ChannelSearchServer
  CHANNEL_SEARCH_SERVICE_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$CHANNEL_SEARCH_SERVICE_DESCR']"

  $EXEC/ServiceConf.sh \
    --services-xpath "$CHANNEL_SEARCH_SERVICE_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/ChannelServing/ChannelSearchService.xsl \
    --out-file ChannelSearchServiceConfig.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "channel_search_service" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --search-xpath "configuration/cfg:channelSearchService" \
    --services-xpath "$CHANNEL_SEARCH_SERVICE_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/fe.sh

  let "EXIT_CODE|=$?"

  ## configure CampaignManager
  CAMPAIGN_MANAGER_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$CAMPAIGN_MANAGER_DESCR']"

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
      --var MODE PROFILING

    let "EXIT_CODE|=$?"
    $EXEC/ProcessHostFiles.sh \
        --services-xpath "$CAMPAIGN_MANAGER_XPATH" \
        --app-xml $APP_XML \
        --plugin-root $PLUGIN_ROOT \
        --out-dir $OUT_DIR \
        --cmd 'mkdir -p $HOST_DIR' \
        --cmd 'cp -r $PLUGIN_ROOT/data/FrontendSubCluster/CampaignManager/* $HOST_DIR'
  
    let "EXIT_CODE|=$?"
    
    $EXEC/CurrentEnvGen.sh \
      --service-name "campaign_manager" \
      --plugin-root "$PLUGIN_ROOT" \
      --app-xml $APP_XML \
      --search-xpath "configuration/cfg:campaignManager" \
      --services-xpath "$CAMPAIGN_MANAGER_XPATH" \
      --out-dir $OUT_DIR \
      --out-file CurrentEnv/fe.sh
  fi

  ### configure Frontend
  FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/Frontend
  FRONTEND_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$FRONTEND_DESCR']"

  $EXEC/ServiceConf.sh \
    --services-xpath "$FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/CampaignManagement/DomainConfig.xsl \
    --out-file DomainConfig.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/ServiceConf.sh \
    --services-xpath "$FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Frontend/AdFrontend.xsl \
    --out-file FeConfig.xml \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT \
    --var CONF_TYPE "base"

  let "EXIT_CODE|=$?"

  $EXEC/ServiceConf.sh \
    --services-xpath "$FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --xsl $XSLT_ROOT/Frontend/httpd/httpd.conf.xsl \
    --out-file conf/httpd.conf \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT \
    --var CONF_TYPE "base"

  let "EXIT_CODE|=$?"

  $EXEC/ServiceConf.sh \
    --services-xpath "$FRONTEND_XPATH" \
    --out-dir-suffix "$OUT_DIR_SUFFIX" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Frontend/httpd/ad_profiling_virtual_server.conf.xsl \
    --out-file conf/ad_virtual_server.conf \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT

  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "ad_frontend" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --port-xpath "$FE_CLUSTER_XPATH/../configuration/cfg:cluster/cfg:coloParams/cfg:virtualServer[1]/@internal_port" \
    --services-xpath "$FRONTEND_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/fe.sh

  let "EXIT_CODE|=$?"

  $EXEC/ProcessHostFiles.sh \
    --services-xpath "$FRONTEND_XPATH" \
    --app-xml $APP_XML \
    --plugin-root $PLUGIN_ROOT \
    --out-dir $OUT_DIR \
    --cmd 'mkdir -p $HOST_DIR' \
    --cmd 'cp -r $PLUGIN_ROOT/data/FrontendSubCluster/Frontend/* $HOST_DIR' \
    --cmd 'chmod +x $HOST_DIR/http/bin/apachectl' \
    --cmd 'mkdir -p $HOST_DIR/conf' \
    --cmd 'touch $HOST_DIR/conf/empty'

  let "EXIT_CODE|=$?"

  cp -r $PLUGIN_ROOT/data/FrontendSubClusterWorkspace/* $WORKSPACE_OUT_DIR
  let "EXIT_CODE|=$?"

  ## Configure SubAgent
  SA_CONFIG_XPATH="$FE_CLUSTER_XPATH/../configuration/cfg:cluster/cfg:snmpStats/@enable"
  SUBAGENT_ENABLE_XPATH="$SA_CONFIG_XPATH = 'true' or $SA_CONFIG_XPATH='1'"
  SUBAGENT_ENABLE=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SUBAGENT_ENABLE_XPATH" --plugin-root $PLUGIN_ROOT`

  if [ $SUBAGENT_ENABLE == "true" ]
  then
    # Services that needs special functions-conf.xml, this XPath use to determine hosts which get it.
    SUBAGENT_NEEDS_XPATH="$FE_CLUSTER_XPATH/service[@descriptor = '$FRONTEND_DESCR']"

    $EXEC/ServiceConf.sh \
      --services-xpath "$SUBAGENT_NEEDS_XPATH" \
      --app-xml $APP_XML \
      --xsl $XSLT_ROOT/Zenoss/SubAgentShellFunctions.xsl \
      --out-file subagent/subagent-shell-AdServer-functions-conf.xml \
      --out-dir $OUT_DIR \
      --plugin-root $PLUGIN_ROOT \
      --var CONNECTIONS_ONLY 0 \
      --var CLUSTER_XPATH "$FE_CLUSTER_XPATH/.."

    let "EXIT_CODE|=$?"

    $EXEC/ProcessHostFiles.sh \
      --services-xpath "$SUBAGENT_NEEDS_XPATH" \
      --app-xml $APP_XML \
      --plugin-root $PLUGIN_ROOT \
      --out-dir $OUT_DIR \
      --cmd 'cp $PLUGIN_ROOT/data/SubAgentShell/SubAgent-Shell-Apache.functions $HOST_DIR/subagent/' \
      --cmd 'cp $PLUGIN_ROOT/data/SubAgentShell/SubAgent-Shell-AdCluster.functions $HOST_DIR/subagent/' \
      --cmd 'cp $OUT_DIR/frontend_cert/SignedUids.pm $HOST_DIR/subagent/'

    let "EXIT_CODE|=$?"
  fi

done

# cleanup temporary directories
rm -r $OUT_DIR/frontend_cert

if [ $EXIT_CODE -eq 0 ]
then
  echo config for AdCluster config completed successfully
else
  echo config for AdCluster config contains errors
fi

exit $EXIT_CODE

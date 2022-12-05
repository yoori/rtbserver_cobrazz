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
XSLT_ROOT=${PLUGIN_ROOT}xslt

. $EXEC/functions

BACKEND_CLUSTER=AdCluster/BackendSubCluster
CAMPAIGN_SERVER_DESCR=$BACKEND_CLUSTER/CampaignServer
DICTIONARY_PROVIDER_DESCR=$BACKEND_CLUSTER/DictionaryProvider
EXPRESSION_MATCHER_DESCR=$BACKEND_CLUSTER/ExpressionMatcher
LOG_GENERALIZER_DESCR=$BACKEND_CLUSTER/LogGeneralizer
LOG_PROCESSING_DESCR=$BACKEND_CLUSTER/LogProcessing
REQUEST_INFO_MANAGER_DESCR=$BACKEND_CLUSTER/RequestInfoManager
STAT_RECEIVER_DESCR=$BACKEND_CLUSTER/StatReceiver
STATS_COLLECTOR_DESCR=$BACKEND_CLUSTER/StatsCollector
USER_OPERATION_GENERATOR_DESCR=$BACKEND_CLUSTER/UserOperationGenerator

LOCAL_PROXY_DESCR=AdCluster/BackendSubCluster/LocalProxy
STUNNEL_CLIENT_DESCR=$LOCAL_PROXY_DESCR/STunnelClient

FRONTEND_SUB_CLUSTER_DESCR=AdCluster/FrontendSubCluster
USER_INFO_MANAGER_DESCR=AdCluster/FrontendSubCluster/UserInfoManager

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

mkdir -p ${OUT_DIR}

### CampaignServer
CAMPAIGN_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$CAMPAIGN_SERVER_DESCR']"

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

### LogProcessing

## configure LogGeneralizer
LOG_GENERALIZER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$LOG_GENERALIZER_DESCR']"

FRONTEND_CLUSTER_DESCR=AdCluster/FrontendSubCluster
CAMPAIGN_MANAGER_DESCR=$FRONTEND_CLUSTER_DESCR/CampaignManager
FRONTEND_DESCR=$FRONTEND_CLUSTER_DESCR/Frontend

SYNC_PROCESSING_XPATH="$CLUSTER_XPATH/..//service["\
"@descriptor = '$CAMPAIGN_MANAGER_DESCR' or "\
"@descriptor = '$FRONTEND_DESCR' or "\
"@descriptor = '$LOG_GENERALIZER_DESCR' or "\
"@descriptor = '$EXPRESSION_MATCHER_DESCR' or "\
"@descriptor = '$REQUEST_INFO_MANAGER_DESCR' or "\
"@descriptor = '$USER_INFO_MANAGER_DESCR' or "\
"@descriptor = '$STAT_RECEIVER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$LOG_GENERALIZER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/LogGeneralizer.xsl \
  --out-file LogGeneralizerConfig.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "log_generalizer" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:logGeneralizer" \
  --services-xpath "$LOG_GENERALIZER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## configure RequestInfoManager
REQUEST_INFO_MANAGER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$REQUEST_INFO_MANAGER_DESCR']"

$EXEC/ProcessHostFiles.sh \
  --plugin-root $PLUGIN_ROOT \
  --app-xml $APP_XML \
  --out-dir $OUT_DIR \
  --services-xpath "$REQUEST_INFO_MANAGER_XPATH" \
  --cmd 'mkdir -p $HOST_DIR/cert' \
  --cmd 'cp $OUT_DIR/frontend_cert/* $HOST_DIR/cert'

let "EXIT_CODE|=$?"

$EXEC/ServiceConf.sh \
  --services-xpath "$REQUEST_INFO_MANAGER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/RequestInfoManager.xsl \
  --out-file RequestInfoManagerConfig.xml \
  --out-dir-suffix "$OUT_DIR_SUFFIX" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

## configure RequestInfoChunksConfigurator
$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/RequestInfoChunksConfigurator.xsl \
  --out-file $OUT_DIR/RIChunksConfig

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "request_info_manager" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:requestInfoManager" \
  --services-xpath "$REQUEST_INFO_MANAGER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## configure ExpressionMatcher
EXPRESSION_MATCHER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$EXPRESSION_MATCHER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$EXPRESSION_MATCHER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/ExpressionMatcher.xsl \
  --out-file ExpressionMatcherConfig.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "expression_matcher" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:expressionMatcher" \
  --services-xpath "$EXPRESSION_MATCHER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## configure Predictor
PREDICTOR_DESCR=$BACKEND_CLUSTER/Predictor
PREDICTOR_XPATH="$CLUSTER_XPATH/service[@descriptor = '$PREDICTOR_DESCR']"
PREDICTOR_COUNT_EXP="count($PREDICTOR_XPATH)"
PREDICTOR_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$PREDICTOR_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $PREDICTOR_COUNT -ne 0 ]
then
  $EXEC/ServiceConf.sh \
    --services-xpath "$PREDICTOR_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Predictor/SyncLogsServer.xsl \
    --out-file conf/predictor_synclogs_server.conf \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT
  let "EXIT_CODE|=$?"

  $EXEC/ServiceConf.sh \
    --services-xpath "$PREDICTOR_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Predictor/Merger.xsl \
    --out-file PredictorMergerConfig.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT
  let "EXIT_CODE|=$?"

  $EXEC/ServiceConf.sh \
    --services-xpath "$PREDICTOR_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Predictor/SVMGenerator.xsl \
    --out-file CTRPredictorSVMGeneratorConfig.xml \
    --out-dir $OUT_DIR \
    --plugin-root $PLUGIN_ROOT
  let "EXIT_CODE|=$?"

  $EXEC/CurrentEnvGen.sh \
    --service-name "svm_generator" \
    --plugin-root "$PLUGIN_ROOT" \
    --app-xml $APP_XML \
    --search-xpath "configuration/cfg:predictor/cfg:svmGenerator" \
    --services-xpath "$PREDICTOR_XPATH" \
    --out-dir $OUT_DIR \
    --out-file CurrentEnv/be.sh
  let "EXIT_CODE|=$?"
fi

## configure SyncLogsServer
SYNC_SERVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$LOG_GENERALIZER_DESCR' or "\
"@descriptor = '$EXPRESSION_MATCHER_DESCR' or "\
"@descriptor = '$REQUEST_INFO_MANAGER_DESCR'] | "\
"$CLUSTER_XPATH/../serviceGroup[@descriptor = '$FRONTEND_SUB_CLUSTER_DESCR']/"\
"service[@descriptor = '$USER_INFO_MANAGER_DESCR'] | "\
"$CLUSTER_XPATH/serviceGroup[@descriptor = '$LOCAL_PROXY_DESCR']/"\
"service[@descriptor = '$STUNNEL_CLIENT_DESCR']"

SYNC_SERVER_HOSTS=`get_hosts_by_xpath "$APP_XML" "$PLUGIN_ROOT" "$EXEC" "$SYNC_SERVER_XPATH"`

$EXEC/ServiceConf.sh \
  --services-xpath "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/SyncLogsServer.xsl \
  --out-file conf/synclogs_server.conf \
  --service-hosts "$SYNC_SERVER_HOSTS" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

## configure SyncLogs
SYNC_SERVICE_HOSTS=`get_hosts_by_xpath "$APP_XML" "$PLUGIN_ROOT" "$EXEC" "$SYNC_PROCESSING_XPATH"`

$EXEC/ServiceConf.sh \
  --services-xpath "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/SyncLogs.xsl \
  --out-file SyncLogsConfig.xml \
  --service-hosts "$SYNC_SERVICE_HOSTS" \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "sync_logs" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "/serviceGroup[@descriptor = '$LOG_PROCESSING_DESCR']/configuration/cfg:logProcessing/cfg:syncLogs" \
  --services-xpath "$CLUSTER_XPATH" \
  --out-dir $OUT_DIR \
  --service-hosts "$SYNC_SERVICE_HOSTS" \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## configure StatReceiver
STAT_RECEIVER_XPATH="$CLUSTER_XPATH/service[@descriptor = '$STAT_RECEIVER_DESCR']"

$EXEC/ServiceConf.sh \
  --services-xpath "$STAT_RECEIVER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/LogProcessing/StatReceiver.xsl \
  --out-file StatReceiver.conf \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

$EXEC/CurrentEnvGen.sh \
  --service-name "stat_receiver" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:statReceiver" \
  --services-xpath "$STAT_RECEIVER_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## configure Controlling
STATS_COLLECTOR_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$STATS_COLLECTOR_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$STATS_COLLECTOR_SERVICE_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/Controlling/StatsCollector.xsl \
 --out-file StatsCollector.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

let "EXIT_CODE|=$?"

### StatsCollector
$EXEC/CurrentEnvGen.sh \
  --service-name "stats_collector" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:statsCollector" \
  --services-xpath "$STATS_COLLECTOR_SERVICE_XPATH" \
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

### DictionaryProvider
DICTIONARY_PROVIDER_SERVICE_XPATH="$CLUSTER_XPATH/service[@descriptor = '$DICTIONARY_PROVIDER_DESCR']"

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

### UserOperationGenerator
USER_OPERATION_GENERATOR_XPATH="$CLUSTER_XPATH/service[@descriptor = '$USER_OPERATION_GENERATOR_DESCR']"

$EXEC/ServiceConf.sh \
 --services-xpath "$USER_OPERATION_GENERATOR_XPATH" \
 --app-xml $APP_XML \
 --xsl $XSLT_ROOT/UserInfoProfiling/UserOperationGenerator.xsl \
 --out-file UserOperationGenerator.xml \
 --out-dir-suffix "$OUT_DIR_SUFFIX" \
 --out-dir $OUT_DIR \
 --plugin-root $PLUGIN_ROOT

$EXEC/CurrentEnvGen.sh \
  --service-name "user_operation_generator" \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --search-xpath "configuration/cfg:userOperationGenerator" \
  --services-xpath "$USER_OPERATION_GENERATOR_XPATH" \
  --out-dir $OUT_DIR \
  --out-file CurrentEnv/be.sh

let "EXIT_CODE|=$?"

## Configure SubAgent
SA_CONFIG_XPATH="$CLUSTER_XPATH/../configuration/cfg:cluster/cfg:snmpStats/@enable"
SUBAGENT_ENABLE_XPATH="$SA_CONFIG_XPATH = 'true' or $SA_CONFIG_XPATH='1'"
SUBAGENT_ENABLE=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SUBAGENT_ENABLE_XPATH" --plugin-root $PLUGIN_ROOT`

if [ $SUBAGENT_ENABLE == "true" ]
then
SUBAGENT_NEEDS_XPATH="
  $EXPRESSION_MATCHER_XPATH |
  $LOG_GENERALIZER_XPATH |
  $REQUEST_INFO_MANAGER_XPATH"

$EXEC/ServiceConf.sh \
  --services-xpath "$SUBAGENT_NEEDS_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Zenoss/SubAgentShellFunctions.xsl \
  --out-file subagent/subagent-shell-AdServer-functions-conf.xml \
  --out-dir $OUT_DIR \
  --plugin-root $PLUGIN_ROOT \
  --var CONNECTIONS_ONLY 0 \
  --var CLUSTER_XPATH "$CLUSTER_XPATH/.."

let "EXIT_CODE|=$?"

$EXEC/ProcessHostFiles.sh \
  --services-xpath "$SUBAGENT_NEEDS_XPATH" \
  --app-xml $APP_XML \
  --plugin-root $PLUGIN_ROOT \
  --out-dir $OUT_DIR \
  --cmd 'cp $PLUGIN_ROOT/data/SubAgentShell/SubAgent-Shell-Apache.functions $HOST_DIR/subagent/' \
  --cmd 'cp $PLUGIN_ROOT/data/SubAgentShell/SubAgent-Shell-AdCluster.functions $HOST_DIR/subagent/' \
  --cmd 'cp $OUT_DIR/frontend_cert/SignedUids.pm $HOST_DIR/subagent'

let "EXIT_CODE|=$?"
fi

if [ $EXIT_CODE -eq 0 ]
then
  echo config for BackendSubCluster completed successfully
else
  echo config for BackendSubCluster contains errors
fi

exit $EXIT_CODE

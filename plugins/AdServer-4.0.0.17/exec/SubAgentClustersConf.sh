#!/bin/bash
# Configure SubAgent for connections of services

if [ $# -lt 4 ]
then
  echo "$0 : number of argument must be equal 4."
  exit 1
fi

APP_XML=$1
CLUSTER_XPATH=$2
OUT_DIR=$3
PLUGIN_ROOT=$4

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi
EXIT_CODE=0

function subagent_setup()
{
  if [ -d $HOST_DIR ];
  then
    for f in `find $HOST_DIR -name "*.port" -type f`; do DATASET=$DATASET`cat $f`"\n"; done &&
    mkdir -p $HOST_DIR/subagent &&
    cp $PLUGIN_ROOT/data/SubAgentShell/subagent-shell-conf.xml $HOST_DIR/subagent/ &&
    cp $PLUGIN_ROOT/data/SubAgentShell/ConnectionsPorts.pm $HOST_DIR/subagent/ &&
    cp $PLUGIN_ROOT/data/SubAgentShell/SubAgent-Shell-Common.functions $HOST_DIR/subagent/ &&
    sed -i -e "s#__SERVICENAME_PORT_VALUES__#$DATASET#g" $HOST_DIR/subagent/ConnectionsPorts.pm
    let "EXIT_CODE|=$?"
    ALL_DATASET=$ALL_DATASET$DATASET
    FUNCTION_CONFIG=$HOST_DIR/subagent/subagent-shell-AdServer-functions-conf.xml
    if [ ! -e $FUNCTION_CONFIG ];
    then
      echo $DEFAULT_FUNCTION > $FUNCTION_CONFIG
    fi
    DATASET=""
  fi
}

ROOT_OUT_DIR=${OUT_DIR}opt/foros/server/etc
EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt
DATA_ROOT=$PLUGIN_ROOT/data

. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT

# AdCluster configuration
ADCLUSTER_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = 'AdCluster' or @descriptor = 'AdProfilingCluster']"
ADCLUSTER_DESCR_XPATH="$ADCLUSTER_XPATH/@descriptor"
ADCLUSTER_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$ADCLUSTER_DESCR_XPATH" \
               --plugin-root $PLUGIN_ROOT | tr "[:upper:] " "[:lower:]_")
ADCLUSTER_OUT_DIR=$ROOT_OUT_DIR/$COLOCATION_NAME/$ADCLUSTER_NAME
SA_CONFIG_XPATH="$ADCLUSTER_XPATH/configuration/cfg:cluster/cfg:snmpStats/@enable"
SUBAGENT_ENABLE_XPATH="count($ADCLUSTER_XPATH) > 0 and \
  ($SA_CONFIG_XPATH = 'true' or $SA_CONFIG_XPATH='1')"
ADCLUSTER_SNMP_ENABLE=`$EXEC/XPathGetValue.sh --xml $APP_XML \
  --xpath "$SUBAGENT_ENABLE_XPATH" --plugin-root $PLUGIN_ROOT`
let "EXIT_CODE|=$?"

if [ $ADCLUSTER_SNMP_ENABLE == "true" ]
then
  ADCLUSTER_DEFAULT_FUNCTION=`$EXEC/XsltTransformer.sh \
  --var XPATH "$ADCLUSTER_XPATH/serviceGroup/service" \
  --var CONNECTIONS_ONLY 1 --var CLUSTER_XPATH "$ADCLUSTER_XPATH" --app-xml $APP_XML \
  --xsl $PLUGIN_ROOT/xslt/Zenoss/SubAgentShellFunctions.xsl --out-file -`
  let "EXIT_CODE|=$?"

# get frontends hosts list
  FRONTEND_DESCR=AdCluster/FrontendSubCluster/Frontend
  FRONTEND_XPATH="$ADCLUSTER_XPATH/serviceGroup[@descriptor = 'AdCluster/FrontendSubCluster']/service[@descriptor = '$FRONTEND_DESCR']"
  
  FRONTEND_COUNT_XPATH="count($FRONTEND_XPATH)"
  FRONTEND_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$FRONTEND_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`
  
  if [ $FRONTEND_COUNT -ne 0 ]
  then
    for (( i=1; i<=FRONTEND_COUNT; i++ ))
    do
      HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$FRONTEND_XPATH[$i]" --plugin-root $PLUGIN_ROOT`
      FRONTEND_HOSTS="$FRONTEND_HOSTS $HOSTS"
    done
  fi
  FRONTEND_HOSTS=`echo $FRONTEND_HOSTS | xargs -n1 | sort -u | tr '\n' ','`
fi

# AdProxyCluster configuration
ADPROXYCLUSTERS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = 'AdProxyCluster']"
ADPROXYCLUSTERS_COUNT_XPATH="count($ADPROXYCLUSTERS_XPATH)"
ADPROXYCLUSTERS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML \
  --xpath "$ADPROXYCLUSTERS_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`
let "EXIT_CODE|=$?"

for HOST in $($EXEC/XsltTransformer.sh --app-xml $APP_XML --xsl $PLUGIN_ROOT/xslt/GetAllHosts.xsl --out-file -)
do
  if [ $ADCLUSTER_SNMP_ENABLE == "true" ]
  then
    HOST_DIR=$ADCLUSTER_OUT_DIR/$HOST
    DEFAULT_FUNCTION=$ADCLUSTER_DEFAULT_FUNCTION
    subagent_setup
  fi

  for (( CLUSTER_INDEX=1; CLUSTER_INDEX<=ADPROXYCLUSTERS_COUNT; CLUSTER_INDEX++ ))
  do
    AD_PROXYCLUSTER_XPATH="${ADPROXYCLUSTERS_XPATH}[$CLUSTER_INDEX]"
    AD_PROXYCLUSTER_DESCR_XPATH="$AD_PROXYCLUSTER_XPATH/@descriptor"
    AD_PROXYCLUSTER_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$AD_PROXYCLUSTER_DESCR_XPATH" \
               --plugin-root $PLUGIN_ROOT | tr "[:upper:] " "[:lower:]_")
    AD_PROXYCLUSTER_OUT_DIR=$ROOT_OUT_DIR/$COLOCATION_NAME/$AD_PROXYCLUSTER_NAME-$CLUSTER_INDEX

    SA_CONFIG_XPATH="$AD_PROXYCLUSTER_XPATH/configuration/cfg:cluster/cfg:snmpStats/@enable"
    SUBAGENT_ENABLE_XPATH="$SA_CONFIG_XPATH = 'true' or $SA_CONFIG_XPATH='1'"
    AD_PROXYCLUSTER_SNMP_ENABLE=`$EXEC/XPathGetValue.sh --xml $APP_XML \
      --xpath "$SUBAGENT_ENABLE_XPATH" --plugin-root $PLUGIN_ROOT`
    let "EXIT_CODE|=$?"

    if [ $AD_PROXYCLUSTER_SNMP_ENABLE == "true" ]
    then
      HOST_DIR=$AD_PROXYCLUSTER_OUT_DIR/$HOST
      DEFAULT_FUNCTION=`$EXEC/XsltTransformer.sh \
        --var XPATH "$AD_PROXYCLUSTER_XPATH/service" \
        --var CONNECTIONS_ONLY 1 --var CLUSTER_XPATH "$AD_PROXYCLUSTER_XPATH" --app-xml $APP_XML \
        --xsl $PLUGIN_ROOT/xslt/Zenoss/SubAgentShellFunctions.xsl --out-file -`
      let "EXIT_CODE|=$?"
      subagent_setup
    fi
  done
done

find $ROOT_OUT_DIR -name '*.port' -type f -exec rm -f {} \;

# Zenoss configuration
ZENOSS_DIR=`$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/GetZenossFolder.xsl \
  --out-file -`
let "EXIT_CODE|=$?"

# Snmp must be enabled somewhere to config ZenOSS
if [ -n "$ZENOSS_DIR" ] && [ -n "$ALL_DATASET" ]
then

ADCLUSTER_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML \
  --xpath "count($CLUSTER_XPATH/serviceGroup[@descriptor = 'AdCluster' or @descriptor = 'AdProfilingCluster'])" --plugin-root $PLUGIN_ROOT)
let "EXIT_CODE|=$?"
mkdir -p "${OUT_DIR}${ZENOSS_DIR}/mibs"

if [ $ADCLUSTER_PRESENTED -ne 0 ]
then
  ### create Zenoss config file
  $EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --var CLUSTER_NAME "adcluster" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Zenoss/Zenoss.xsl \
  --out-file ${OUT_DIR}${ZENOSS_DIR}/adserver-config.xml
  let "EXIT_CODE|=$?"

  echo -e "$ALL_DATASET" | sort -u | $EXEC/bin/AdServerTemplateFiller.pl \
    -f $FRONTEND_HOSTS \
    ${DATA_ROOT}/Zenoss/adserver-template.xml "${OUT_DIR}${ZENOSS_DIR}"
  rsync -r --exclude=.svn ${DATA_ROOT}/mibs "${OUT_DIR}${ZENOSS_DIR}"
else
  ### create Zenoss config file
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CLUSTER_XPATH" \
    --var CLUSTER_NAME "adcontentcluster" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Zenoss/Zenoss.xsl \
    --out-file ${OUT_DIR}${ZENOSS_DIR}/adserver-config.xml
  let "EXIT_CODE|=$?"

  echo -e "$ALL_DATASET" | sort | uniq | $EXEC/bin/AdServerTemplateFiller.pl \
    ${DATA_ROOT}/AdContentCluster/Zenoss/adserver-template.xml "${OUT_DIR}${ZENOSS_DIR}"
  cp -p ${DATA_ROOT}/mibs/AdServer.mib "${OUT_DIR}${ZENOSS_DIR}/mibs/"
  cp -p ${DATA_ROOT}/mibs/SubAgent-Shell-AdServer.mib "${OUT_DIR}${ZENOSS_DIR}/mibs/"
  cp -p ${DATA_ROOT}/mibs/SubAgent-Shell-Connections.mib "${OUT_DIR}${ZENOSS_DIR}/mibs/"
  cp -p ${DATA_ROOT}/mibs/SubAgent-Shell-AdServer-Apache.mib "${OUT_DIR}${ZENOSS_DIR}/mibs/"
fi
fi

if [ $EXIT_CODE -eq 0 ]
then
  echo config for SubAgent services completed successfully
else
  echo config for SubAgent services contains errors
fi

exit $EXIT_CODE

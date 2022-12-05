#!/bin/bash

if [ $# -lt 6 ]
then
  echo "$0 : number of argument must be equal 6."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
OUT_DIR=$3
PLUGIN_ROOT=$4
APP_VERSION=$5
APP_RELEASE=$6

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

MANAGMENT_CONFIG_OUT_DIR=$OUT_DIR/opt/foros/manager/etc/config.d
EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT

WORKSPACE_OUT_DIR=$OUT_DIR/u01/foros/server/var
PRODUCT_IDENTIFIER=$COLOCATION_NAME-$APP_VERSION-$APP_RELEASE

mkdir -p $WORKSPACE_OUT_DIR
mkdir -p $OUT_DIR/opt/foros/manager/var/server-$COLOCATION_NAME

# AdCluster configuration
ADCLUSTER_DESCR=AdCluster
ADCLUSTER_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$ADCLUSTER_DESCR']"
ADCLUSTER_COUNT_XPATH="count($ADCLUSTER_XPATH)"
ADCLUSTER_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$ADCLUSTER_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`

if [ $ADCLUSTER_COUNT -ne 0 ]
then
  $EXEC/AdClusterConf.sh $APP_XML \
   "$ADCLUSTER_XPATH" \
   "$OUT_DIR" \
   "$PLUGIN_ROOT" \
   "$ADCLUSTER_DESCR" \
   "$PRODUCT_IDENTIFIER"

  let "EXIT_CODE|=$?"
fi


# AdProxyCluster configuration
ADPROXYCLUSTERS_DESCR=AdProxyCluster
ADPROXYCLUSTERS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$ADPROXYCLUSTERS_DESCR']"
ADPROXYCLUSTERS_COUNT_XPATH="count($ADPROXYCLUSTERS_XPATH)"
ADPROXYCLUSTERS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$ADPROXYCLUSTERS_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`
for (( i=1; i<=ADPROXYCLUSTERS_COUNT; i++ ))
do
  AD_PROXYCLUSTER_XPATH="${ADPROXYCLUSTERS_XPATH}[$i]"
  $EXEC/ProxyClusterConf.sh $APP_XML \
    "$AD_PROXYCLUSTER_XPATH" \
    "$OUT_DIR" \
    "$PLUGIN_ROOT" \
    "$i" \
    "$ADPROXYCLUSTERS_DESCR"

  let "EXIT_CODE|=$?"
done

# AdProfilingCluster configuration
ADPROFILING_CLUSTER_DESCR=AdProfilingCluster
ADPROFILING_CLUSTER_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$ADPROFILING_CLUSTER_DESCR']"
ADPROFILING_CLUSTERS_COUNT="count($ADPROFILING_CLUSTER_XPATH)"
ADPROFILING_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$ADPROFILING_CLUSTERS_COUNT" --plugin-root $PLUGIN_ROOT`

if [ $ADPROFILING_COUNT -ne 0 ]
then
  $EXEC/ProfilingClusterConf.sh $APP_XML \
   "$ADPROFILING_CLUSTER_XPATH" \
   "$OUT_DIR" \
   "$PLUGIN_ROOT" \
   "$ADPROFILING_CLUSTER_DESCR" \
   "$PRODUCT_IDENTIFIER"
  let "EXIT_CODE|=$?"
fi

# OCM configuration
$EXEC/OCMConf.sh $APP_XML \
  "$CLUSTER_XPATH" \
  "$PLUGIN_ROOT" \
  "$MANAGMENT_CONFIG_OUT_DIR" \
  "$COLOCATION_NAME" \
  "$APP_VERSION" \
  "$APP_RELEASE" \
  "$ADCLUSTER_COUNT" \
  "$ADPROFILING_COUNT" \
  "$OUT_DIR"

let "EXIT_CODE|=$?"

$EXEC/SubAgentClustersConf.sh $APP_XML \
 "$CLUSTER_XPATH" \
 "$OUT_DIR" \
 "$PLUGIN_ROOT"

let "EXIT_CODE|=$?"

if [ $EXIT_CODE -eq 0 ]
then
  echo config for AdServer config completed successfully
else
  echo config for AdServer config contains errors
fi

exit $EXIT_CODE

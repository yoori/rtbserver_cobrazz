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

. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT

CLUSTER_DESCR_XPATH="$CLUSTER_XPATH/@descriptor"
CLUSTER_NAME=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CLUSTER_DESCR_XPATH" \
               --plugin-root $PLUGIN_ROOT | tr "[:upper:] " "[:lower:]_")

BUILD_DIR=$3
OUT_DIR_SUFFIX=$COLOCATION_NAME/$CLUSTER_NAME
OUT_DIR=$3/opt/foros/server/etc/$OUT_DIR_SUFFIX

WORKSPACE_OUT_DIR=$BUILD_DIR/u01/foros/server/var

mkdir -p $WORKSPACE_OUT_DIR/sync
mkdir -p $WORKSPACE_OUT_DIR/www

# DACS configuration
START_SCRIPT_NAME="$COLOCATION_NAME-$CLUSTER_NAME"

$EXEC/FrontendCertificateGen.sh \
  --plugin-root "$PLUGIN_ROOT" \
  --app-xml "$APP_XML" \
  --cluster-xpath "$CLUSTER_XPATH" \
  --certificate-root "$OUT_DIR/frontend_cert/"

let "EXIT_CODE|=$?"

$EXEC/DACSConf.sh $APP_XML \
"$CLUSTER_XPATH" \
"$BUILD_DIR" \
"$PLUGIN_ROOT" \
"$OUT_DIR" \
"$START_SCRIPT_NAME" \
"$OUT_DIR_SUFFIX"

let "EXIT_CODE|=$?"

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
BACKEND_CLUSTER_DESCR=AdCluster/BackendSubCluster
BE_CLUSTER_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$BACKEND_CLUSTER_DESCR']"
$EXEC/BackendSubClusterConf.sh $APP_XML \
"$BE_CLUSTER_XPATH" \
"$OUT_DIR" \
"$PLUGIN_ROOT" \
"$OUT_DIR_SUFFIX"

let "EXIT_CODE|=$?"

# Local proxy subcluster configuration
LOCAL_PROXY_CLUSTER_DESCR=AdCluster/BackendSubCluster/LocalProxy
LOCAL_PROXY_CLUSTER_XPATH="$BE_CLUSTER_XPATH/serviceGroup[@descriptor = '$LOCAL_PROXY_CLUSTER_DESCR']"
LOCAL_PROXY_CLUSTER_COUNT="count($LOCAL_PROXY_CLUSTER_XPATH)"
LOCAL_PROXY_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$LOCAL_PROXY_CLUSTER_COUNT" --plugin-root $PLUGIN_ROOT`
if [ $LOCAL_PROXY_COUNT -ne 0 ]
then
  $EXEC/LocalProxySubClusterConf.sh $APP_XML \
    "$LOCAL_PROXY_CLUSTER_XPATH" \
     "$OUT_DIR" \
     "$PLUGIN_ROOT" \
     "$OUT_DIR_SUFFIX"
  let "EXIT_CODE|=$?"
fi

# Frontend subcluster configuration
FRONTEND_CLUSTER_DESCR=AdCluster/FrontendSubCluster
FE_CLUSTERS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$FRONTEND_CLUSTER_DESCR']"
FE_CLUSTERS_COUNT="count($FE_CLUSTERS_XPATH)"
FE_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$FE_CLUSTERS_COUNT" --plugin-root $PLUGIN_ROOT`
for (( i=1; i<=FE_COUNT; i++ ))
do
  FE_CLUSTER_XPATH="${FE_CLUSTERS_XPATH}[$i]"
  $EXEC/FrontendSubClusterConf.sh $APP_XML \
    "$FE_CLUSTER_XPATH" \
    "$BUILD_DIR" \
    "$PLUGIN_ROOT" \
    "$OUT_DIR" \
    "$OUT_DIR_SUFFIX" \
    "$i"

  let "EXIT_CODE|=$?"
done

# cleanup temporary directories
rm -r $OUT_DIR/frontend_cert

# Tests configuration
TESTS_DESCR=AdCluster/Tests
TESTS_XPATH="$CLUSTER_XPATH/serviceGroup[@descriptor = '$TESTS_DESCR']"
TESTS_CLUSTERS_COUNT="count($TESTS_XPATH)"
TESTS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$TESTS_CLUSTERS_COUNT" --plugin-root $PLUGIN_ROOT`
if [ $TESTS_COUNT -ne 0 ]
then
  $EXEC/Tests/TestsConf.sh $APP_XML \
    "$TESTS_XPATH" \
    "$BUILD_DIR" \
    "$PLUGIN_ROOT" \
    "$PRODUCT_IDENTIFIER"
  let "EXIT_CODE|=$?"
fi

if [ $EXIT_CODE -eq 0 ]
then
  echo config for AdCluster config completed successfully
else
  echo config for AdCluster config contains errors
fi

exit $EXIT_CODE

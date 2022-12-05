#!/bin/bash

if [ $# -lt 10 ]
then
  echo "$0 : number of argument must be equal 10."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
CLUSTER_XPATH=$2
PLUGIN_ROOT=$3
OUT_DIR=$4
COLOCATION_NAME=$5
APP_VERSION=$6
APP_RELEASE=$7
ADCLUSTER_COUNT=$8
ADPROFILINGCLUSTER_COUNT=$9
ROOT_OUT_DIR=${10}

EXEC=$PLUGIN_ROOT/exec
APP_ENV_XPATH=$CLUSTER_XPATH/configuration/cfg:environment
SEPARATE_ISP_ZONE=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "$APP_ENV_XPATH/cfg:ispZoneManagement/@separate_isp_zone" --plugin-root $PLUGIN_ROOT)

if [ "$SEPARATE_ISP_ZONE" != "true" ]
then
PRODUCT_PHORM_IDENTIFIER=$COLOCATION_NAME-$APP_VERSION-$APP_RELEASE
PRODUCT_ISP_IDENTIFIER=$COLOCATION_NAME-$APP_VERSION-$APP_RELEASE
else
PRODUCT_PHORM_IDENTIFIER=$COLOCATION_NAME-foros-$APP_VERSION-$APP_RELEASE
PRODUCT_ISP_IDENTIFIER=$COLOCATION_NAME-isp-$APP_VERSION-$APP_RELEASE
fi

XSLT_ROOT=$PLUGIN_ROOT/xslt
DATA=$PLUGIN_ROOT/data

EXEC=$PLUGIN_ROOT/exec

MGR_PHORM_EXT_ROOT_DIR=$ROOT_OUT_DIR/opt/foros/server/manager/$PRODUCT_PHORM_IDENTIFIER
MGR_PHORM_EXT_BIN_DIR=$MGR_PHORM_EXT_ROOT_DIR/bin/
MGR_PHORM_EXT_LIB_DIR=$MGR_PHORM_EXT_ROOT_DIR/lib/
MGR_PHORM_EXT_VAR_DIR=$MGR_PHORM_EXT_ROOT_DIR/var/

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

mkdir -p ${OUT_DIR}
mkdir -p $MGR_PHORM_EXT_ROOT_DIR

rsync -r --exclude=.svn $EXEC/bin/ConfigureChunks.pl $MGR_PHORM_EXT_BIN_DIR
rsync -r --exclude=.svn $PLUGIN_ROOT/lib/ $MGR_PHORM_EXT_LIB_DIR
mkdir -p $MGR_PHORM_EXT_VAR_DIR

TIMESTAMP=`date '+%F %T'`

if [ $ADPROFILINGCLUSTER_COUNT -ne 0 ]
then
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CLUSTER_XPATH" \
    --var CURRENT_TIME "$TIMESTAMP" \
    --var PRODUCT_IDENTIFIER "$PRODUCT_PHORM_IDENTIFIER" \
    --var ISP_ZONE "0" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/OCM/AdProfilingServer.xsl \
    --out-file $OUT_DIR/server-$PRODUCT_PHORM_IDENTIFIER.xml
else
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CLUSTER_XPATH" \
    --var CURRENT_TIME "$TIMESTAMP" \
    --var PRODUCT_IDENTIFIER "$PRODUCT_PHORM_IDENTIFIER" \
    --var ISP_ZONE "0" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/OCM/AdServer.xsl \
    --out-file $OUT_DIR/server-$PRODUCT_PHORM_IDENTIFIER.xml
fi

let "EXIT_CODE|=$?"

if [ $ADCLUSTER_COUNT -ne 0 ] || [ $ADPROFILINGCLUSTER_COUNT -ne 0 ]
then

MGR_ISP_EXT_ROOT_DIR=$ROOT_OUT_DIR/opt/foros/server/manager/$PRODUCT_ISP_IDENTIFIER
MGR_ISP_EXT_BIN_DIR=$MGR_ISP_EXT_ROOT_DIR/bin/
MGR_ISP_EXT_LIB_DIR=$MGR_ISP_EXT_ROOT_DIR/lib/
MGR_ISP_EXT_VAR_DIR=$MGR_ISP_EXT_ROOT_DIR/var/

  mkdir -p $MGR_ISP_EXT_ROOT_DIR
  mkdir -p $MGR_ISP_EXT_LIB_DIR
  rsync -r --exclude=.svn $EXEC/bin/PreStartChecker.pl $MGR_ISP_EXT_BIN_DIR
  rsync -r --exclude=.svn $PLUGIN_ROOT/lib/Args.pm $MGR_ISP_EXT_LIB_DIR
  mkdir -p $MGR_ISP_EXT_VAR_DIR

  if [ "$SEPARATE_ISP_ZONE" = "true" ]
  then
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CLUSTER_XPATH" \
    --var CURRENT_TIME "$TIMESTAMP" \
    --var PRODUCT_IDENTIFIER "$PRODUCT_ISP_IDENTIFIER" \
    --var ISP_ZONE "1" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/OCM/AdServer.xsl \
    --out-file $OUT_DIR/server-$PRODUCT_ISP_IDENTIFIER.xml

  let "EXIT_CODE|=$?"
  fi
fi

if [ $EXIT_CODE -eq 0 ]
then
  echo config for OCM completed successfully
else
  echo config for OCM contains errors
fi

exit $EXIT_CODE

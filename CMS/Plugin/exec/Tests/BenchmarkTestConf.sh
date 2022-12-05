#!/bin/bash

if [ $# -lt 4 ]
then
  echo "$0 : number of argument must be equal 5."
  exit 1
fi

EXIT_CODE=0

APP_XML=$1
CONFIG_XPATH=$2
BUILD_ROOT=$3
ROOT_OUT_DIR=$BUILD_ROOT/opt/foros/server/etc
OUT_DIR=$ROOT_OUT_DIR/TestConfig
PLUGIN_ROOT=$4
TESTS_XPATH=$5
EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

CONFIGNAME_XPATH="$CONFIG_XPATH/@name"
CONFIGNAME=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CONFIGNAME_XPATH" --plugin-root $PLUGIN_ROOT`

PERF_OUT_DIR=$OUT_DIR/BenchmarkTest-$CONFIGNAME
mkdir -p $PERF_OUT_DIR


[ x$CONFIGNAME = "x" ] && echo "Invalid performance test configuration name" >&2 && exit 1

# process campaign config
CAMPAIGNS_XPATH="$CONFIG_XPATH/cfg:campaigns/cfg:campaign"
CAMPAIGNS_COUNT_EXP="count($CAMPAIGNS_XPATH)"
CAMPAIGNS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CAMPAIGNS_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $CAMPAIGNS_COUNT -ne 0 ]
then
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CONFIG_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/Campaigns.xsl \
    --out-file $PERF_OUT_DIR/campaigns.cfg
  let "EXIT_CODE |= $?"
fi

# process channel config
CHANNELS_XPATH="$CONFIG_XPATH/cfg:channels/cfg:channel"
CHANNELS_COUNT_EXP="count($CHANNELS_XPATH)"
CHANNELS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$CHANNELS_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $CHANNELS_COUNT -ne 0 ]
then
  $EXEC/XsltTransformer.sh \
    --var XPATH "$CONFIG_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/Channels.xsl \
    --out-file $PERF_OUT_DIR/channels.cfg
  let "EXIT_CODE |= $?"
fi

# process performance test
$EXEC/XsltTransformer.sh \
  --var XPATH "$CONFIG_XPATH" \
  --var TEST_TYPE "optin" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Tests/BenchmarkTestSh.xsl \
  --out-file $ROOT_OUT_DIR/benchmark-test-$CONFIGNAME-optin.sh

let "EXIT_CODE |= $?"

chmod +x $ROOT_OUT_DIR/benchmark-test-$CONFIGNAME-optin.sh

$EXEC/XsltTransformer.sh \
  --var XPATH "$CONFIG_XPATH" \
  --var TEST_TYPE "optout" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Tests/BenchmarkTestSh.xsl \
  --out-file $ROOT_OUT_DIR/benchmark-test-$CONFIGNAME-optout.sh

let "EXIT_CODE |= $?"

chmod +x $ROOT_OUT_DIR/benchmark-test-$CONFIGNAME-optout.sh

if [ $EXIT_CODE -eq 0 ]
then
  echo config for BenchmarkTest \"$CONFIGNAME\" completed successfully
else
  echo config for BenchmarkTest \"$CONFIGNAME\" contains errors >2
fi

exit $EXIT_CODE

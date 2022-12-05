#!/bin/bash

if [ $# -lt 5 ]
then
  echo "$0 : number of argument must be equal 5."
  exit 1
fi

EXIT_CODE=0
APP_XML=$1
TESTS_XPATH=$2
BUILD_ROOT=$3
ROOT_OUT_DIR=$BUILD_ROOT/opt/foros/server/etc
OUT_DIR=$ROOT_OUT_DIR/TestConfig
PLUGIN_ROOT=$4
PRODUCT_IDENTIFIER=$5
EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

if [ ! -r ${APP_XML} ]
then
  echo "$0 : application xml ''${APP_XML}'' not found"
  exit 1
fi

AUTO_TEST_DESCR=AdCluster/Tests/AutoTest
PERFORMANCE_TEST_DESCR=AdCluster/Tests/PerformanceTest
BENCHMARK_TEST_DESCR=AdCluster/Tests/BenchmarkTest

AUTO_TEST_XPATH="$TESTS_XPATH/service[@descriptor = '$AUTO_TEST_DESCR']"
AUTO_TEST_COUNT_EXP="count($AUTO_TEST_XPATH)"
AUTO_TEST_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$AUTO_TEST_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $AUTO_TEST_COUNT -ne 0 ]
then
  $EXEC/XsltTransformer.sh \
    --var XPATH "$AUTO_TEST_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/AutoTestsConfig.xsl \
    --out-file $OUT_DIR/AutoTests/AutoTestsConfig.xml

  let "EXIT_CODE |= $?"

  $EXEC/XsltTransformer.sh \
    --var XPATH "$AUTO_TEST_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/AutoTestExec.xsl \
    --out-file $ROOT_OUT_DIR/autotest.sh

  $EXEC/XsltTransformer.sh \
    --var XPATH "$AUTO_TEST_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/RunTests.xsl \
    --out-file $ROOT_OUT_DIR/runTests.sh

  let "EXIT_CODE |= $?"

  chmod +x $ROOT_OUT_DIR/autotest.sh

  let "EXIT_CODE|=$?"

  chmod +x $ROOT_OUT_DIR/runTests.sh

  let "EXIT_CODE|=$?"

  # Copy autotests script
  cp $EXEC/Tests/testsCommons.sh $OUT_DIR/testsCommons.sh
  let "EXIT_CODE |= $?"

  cp $EXEC/Tests/prepareDB.sh $ROOT_OUT_DIR/prepareDB.sh
  let "EXIT_CODE |= $?"

  # Process results

  PROCESS_RESULT_XPATH="$TESTS_XPATH/configuration/cfg:testsCommon/cfg:testResultProcessing"
  PROCESS_RESULT_COUNT_EXP="count($PROCESS_RESULT_XPATH)"
  PROCESS_RESULT_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$PROCESS_RESULT_COUNT_EXP" --plugin-root $PLUGIN_ROOT`
  if [ $PROCESS_RESULT_COUNT -ne 0 ]
  then
      cp $EXEC/Tests/processNBTest.sh $ROOT_OUT_DIR/process-test-result.sh
      let "EXIT_CODE |= $?"

      cp $EXEC/Tests/getlogs.sh $ROOT_OUT_DIR/getlogs.sh
      let "EXIT_CODE |= $?"
  else
      TASKBOT_XPATH="$TESTS_XPATH/configuration/cfg:testsCommon/cfg:taskbot"
      TASKBOT_COUNT_EXP="count($TASKBOT_XPATH)"
      TASKBOT_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$TASKBOT_COUNT_EXP" --plugin-root $PLUGIN_ROOT`
      if [ $TASKBOT_COUNT -ne 0 ]
      then
          cp $EXEC/Tests/processTest.sh $ROOT_OUT_DIR/process-test-result.sh
          let "EXIT_CODE |= $?"

          $EXEC/XsltTransformer.sh \
              --var XPATH "$TESTS_XPATH" \
              --app-xml $APP_XML \
              --xsl $XSLT_ROOT/Tests/TaskbotUserConfig.xsl \
              --out-file $OUT_DIR/AutoTests/user.config
          let "EXIT_CODE |= $?"

          $EXEC/XsltTransformer.sh \
              --var XPATH "$TESTS_XPATH" \
              --app-xml $APP_XML \
              --xsl $XSLT_ROOT/Tests/TaskbotDBConfig.xsl \
             --out-file $OUT_DIR/AutoTests/mysql.cnf
          let "EXIT_CODE |= $?"
      fi
  fi

fi


# Performance test configurations
PERFORMANCE_TEST_XPATH="$TESTS_XPATH/service[@descriptor = '$PERFORMANCE_TEST_DESCR']"
PERFORMANCE_TEST_COUNT_EXP="count($PERFORMANCE_TEST_XPATH)"
PERFORMANCE_TEST_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$PERFORMANCE_TEST_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $PERFORMANCE_TEST_COUNT -ne 0 ]
then

PERFORMANCE_CONFIGS_XPATH="$PERFORMANCE_TEST_XPATH/configuration/cfg:performanceTest/cfg:test"
PERFORMANCE_CONFIGS_COUNT_EXP="count($PERFORMANCE_CONFIGS_XPATH)"
PERFORMANCE_CONFIGS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$PERFORMANCE_CONFIGS_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

  for (( i=1; i<=PERFORMANCE_CONFIGS_COUNT; i++ ))
  do
    CONFIG_XPATH="${PERFORMANCE_CONFIGS_XPATH}[$i]"
    $EXEC/Tests/PerformanceTestConf.sh $APP_XML \
    "$CONFIG_XPATH" \
    "$BUILD_ROOT" \
    "$PLUGIN_ROOT" \
    "$TESTS_XPATH"
    let "EXIT_CODE|=$?"

  done

  # process make-config
  $EXEC/XsltTransformer.sh \
    --var XPATH "$PERFORMANCE_CONFIGS_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/MakeConfig.xsl \
    --out-file $OUT_DIR/make-config.sh

  let "EXIT_CODE |= $?"

  $EXEC/XsltTransformer.sh \
    --var XPATH "$PERFORMANCE_CONFIGS_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/PerformanceTestSh.xsl \
    --out-file $ROOT_OUT_DIR/performance-test.sh

  let "EXIT_CODE|=$?"

  chmod +x $ROOT_OUT_DIR/performance-test.sh

  let "EXIT_CODE|=$?"

fi

# Benchmark test configurations
BENCHMARK_TEST_XPATH="$TESTS_XPATH/service[@descriptor = '$BENCHMARK_TEST_DESCR']"
BENCHMARK_TEST_COUNT_EXP="count($BENCHMARK_TEST_XPATH)"
BENCHMARK_TEST_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$BENCHMARK_TEST_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

if [ $BENCHMARK_TEST_COUNT -ne 0 ]
then

BENCHMARK_CONFIGS_XPATH="$BENCHMARK_TEST_XPATH/configuration/cfg:benchmarkTest/cfg:test"
BENCHMARK_CONFIGS_COUNT_EXP="count($BENCHMARK_CONFIGS_XPATH)"
BENCHMARK_CONFIGS_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$BENCHMARK_CONFIGS_COUNT_EXP" --plugin-root $PLUGIN_ROOT`

  for (( i=1; i<=BENCHMARK_CONFIGS_COUNT; i++ ))
  do
    CONFIG_XPATH="${BENCHMARK_CONFIGS_XPATH}[$i]"
    $EXEC/Tests/BenchmarkTestConf.sh $APP_XML \
    "$CONFIG_XPATH" \
    "$BUILD_ROOT" \
    "$PLUGIN_ROOT" \
    "$TESTS_XPATH"
    let "EXIT_CODE|=$?"
  done

  # process make-config
  $EXEC/XsltTransformer.sh \
    --var XPATH "$BENCHMARK_CONFIGS_XPATH" \
    --app-xml $APP_XML \
    --xsl $XSLT_ROOT/Tests/MakeBenchmarkConfig.xsl \
    --out-file $OUT_DIR/make-benchmark-config.sh

  let "EXIT_CODE |= $?"

  
fi

cp -r $PLUGIN_ROOT/data/Tests/tests.css $OUT_DIR/tests.css
let "EXIT_CODE |= $?"

$EXEC/XsltTransformer.sh \
  --var XPATH "$AUTO_TEST_XPATH" \
  --var PRODUCT_IDENTIFIER "$PRODUCT_IDENTIFIER" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/Tests/EnvParams.xsl \
  --out-file $OUT_DIR/envparams.sh

let "EXIT_CODE |= $?"


if [ $EXIT_CODE -eq 0 ]
then
  echo config for Tests completed successfully
else
  echo config for Tests contains errors >2
fi

exit $EXIT_CODE  

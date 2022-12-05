#!/bin/bash

export auto_tests_log_path=$test_log_path/AutoTests
export auto_tests_config_root=$test_config_root/AutoTests
export additional_logs="$auto_tests_log_path/server-logs $auto_tests_log_path/server-output $auto_tests_log_path/server-configs $auto_tests_log_path/autotests-configs"

function check_error ()
{
 [ $1 -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
}

function stop_service
{
  echo "Stop service $1..."
  ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no \
         $adserver_host "(cd $config_root; $server_start_prefix ./$colocation_name.sh stop $1)"
  check_error $? "./$colocation_name.sh stop $1"
  echo "Stop service $1...ok"
}

function start_service
{
  echo "Start service $1..."
  ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no \
         $adserver_host "(cd $config_root; $server_start_prefix ./$colocation_name.sh start $1)"
  check_error $? "./$colocation_name.sh start $1"
  echo "Start service $1...ok"
}


function process_test_result
{
echo "Processing test results $1..."
test_date=`date "+%04Y-%02m-%02e-%02k-%02M"`
report_name=$1
if [ "x$vg_report_suffix" != "x" ] 
then
report_name="$vg_report_suffix-$report_name"
fi
./process-test-result.sh $test_date $auto_tests_log_path \
                         foros/server/automated-tests-$report_name \
                         "Report-$report_name"
check_error $? "process-test-result.sh"
echo "Processing test results $1...ok"
}

# run tests ts
function run_tests
{
mkdir -p $auto_tests_log_path
rm -rf $auto_tests_log_path/*.out $auto_tests_log_path/*.err

run_name=$1
shift

echo "Run tests $run_name..."
echo "$@"
AutoTests "$@"
test_result=$?
echo "Run tests $run_name...status=$test_result"
echo "Test logs stored in $auto_tests_log_path."
# Result processing
if [ $test_result == 0 ] || [ $test_result == 1 ] 
then
 process_test_result $run_name
fi
}


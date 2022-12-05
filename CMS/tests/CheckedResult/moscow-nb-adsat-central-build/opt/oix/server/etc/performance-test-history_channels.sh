

#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/make-config.sh

function check_error ()
{
 [ $1 -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
}

 

name=PerformanceTest
log_path=$workspace_root/log/$name
mkdir -p $log_path

mkdir -p ${test_config_root}

# Clear old logs
rm -f $log_path/*.out
rm -f $log_path/*.err


db_clean_history_channels
make_config_history_channels

read -p "Restart adserver and press any key to continue..." -t 600



echo "Run tests history_channels..."
AdServerTest 6 \
  ${test_config_root}/$name-history_channels/Config.xml \
  1>$log_path/$name-history_channels.out \
  2>$log_path/$name-history_channels.err
exit_code=$?
check_error $exit_code "Run tests history_channels"
echo "Run tests history_channels...status=$exit_code"

echo "Processing performance test results..."
test_date=`date "+%04Y-%02m-%02e-%02k-%02M"`
report_name="performance-test"
./process-test-result.sh $test_date $log_path \
                         foros/server/$report_name \
                         "Report-$report_name"
check_error $? "process-test-result.sh"
echo "Processing performance test results...ok"

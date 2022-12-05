

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


db_clean_size_100
db_clean_size_500
db_clean_size_1000
db_clean_history_channels
db_clean_idle
make_config_size_100
make_config_size_500
make_config_size_1000
make_config_history_channels
make_config_idle

read -p "Restart adserver and press any key to continue..." -t 600



echo "Run tests size_100..."
AdServerTest 6 \
  ${test_config_root}/$name-size_100/Config.xml \
  1>$log_path/$name-size_100.out \
  2>$log_path/$name-size_100.err
exit_code=$?
check_error $exit_code "Run tests size_100"
echo "Run tests size_100...status=$exit_code"


echo "Run tests size_500..."
AdServerTest 6 \
  ${test_config_root}/$name-size_500/Config.xml \
  1>$log_path/$name-size_500.out \
  2>$log_path/$name-size_500.err
exit_code=$?
check_error $exit_code "Run tests size_500"
echo "Run tests size_500...status=$exit_code"


echo "Run tests size_1000..."
AdServerTest 6 \
  ${test_config_root}/$name-size_1000/Config.xml \
  1>$log_path/$name-size_1000.out \
  2>$log_path/$name-size_1000.err
exit_code=$?
check_error $exit_code "Run tests size_1000"
echo "Run tests size_1000...status=$exit_code"


echo "Run tests history_channels..."
AdServerTest 6 \
  ${test_config_root}/$name-history_channels/Config.xml \
  1>$log_path/$name-history_channels.out \
  2>$log_path/$name-history_channels.err
exit_code=$?
check_error $exit_code "Run tests history_channels"
echo "Run tests history_channels...status=$exit_code"


echo "Run tests idle..."
AdServerTest 6 \
  ${test_config_root}/$name-idle/Config.xml \
  1>$log_path/$name-idle.out \
  2>$log_path/$name-idle.err
exit_code=$?
check_error $exit_code "Run tests idle"
echo "Run tests idle...status=$exit_code"

echo "Processing performance test results..."
test_date=`date "+%04Y-%02m-%02e-%02k-%02M"`
report_name="performance-test"
./process-test-result.sh $test_date $log_path \
                         foros/server/$report_name \
                         "Report-$report_name"
check_error $? "process-test-result.sh"
echo "Processing performance test results...ok"

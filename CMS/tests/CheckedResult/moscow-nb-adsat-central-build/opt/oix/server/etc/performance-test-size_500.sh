

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


db_clean_size_500
make_config_size_500

read -p "Restart adserver and press any key to continue..." -t 600



echo "Run tests size_500..."
AdServerTest 6 \
  ${test_config_root}/$name-size_500/Config.xml \
  1>$log_path/$name-size_500.out \
  2>$log_path/$name-size_500.err
exit_code=$?
check_error $exit_code "Run tests size_500"
echo "Run tests size_500...status=$exit_code"

echo "Processing performance test results..."
test_date=`date "+%04Y-%02m-%02e-%02k-%02M"`
report_name="performance-test"
./process-test-result.sh $test_date $log_path \
                         foros/server/$report_name \
                         "Report-$report_name"
check_error $? "process-test-result.sh"
echo "Processing performance test results...ok"

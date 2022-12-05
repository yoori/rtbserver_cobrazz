
#!/bin/bash

# Run autotests (devel's entry-point)
# CMS generated file

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

if [ ! -e $config_root/TestConfig/AutoTests/AutoTestsConfig.xml ]
then
  echo "$config_root/TestConfig/AutoTests/AutoTestsConfig.xml is not exists." \
      " Need run confgen.sh & prepareDB.sh before!" && exit 1
fi 

if [ ! -e $auto_tests_config_root/LocalParams.xml ]
then
  echo "$auto_tests_config_root/LocalParams.xml is not exists." \
      " Need run prepareDB.sh before!" && exit 1
fi 


run_name="$colocation_name-simple"

if [ "x$1" != "x" ]
then
  run_name=$1
fi



run_tests "$run_name" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml \
  --params=$auto_tests_config_root/LocalParams.xml \
  --mode=slow

#!/bin/bash

# AutoTests runner script
# CMS generated file

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

if [ "x$1" != "x" ]
then
export vg_report_suffix=$1
fi



mkdir -p $auto_tests_config_root || { echo "Can't create test_config_root=$auto_tests_config_root" && exit 1 ; }

# Restart Newsgate-emu
./restartNewsgate.sh
[ $? -ne 0 ] &&  exit $?

rm -rf $workspace_root/log/NewsgateEmu/AutoTests/*.log

# Prepare DB data
./prepareDB.sh
[ $? -ne 0 ] &&  exit $?

# Wait data actualization or server restart
read -p "Restart adserver and press any key to continue..." -t 600

# Run tests
stage=0
./runTests.sh "$colocation_name-stage-$stage-simple"


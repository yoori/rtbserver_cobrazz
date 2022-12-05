
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


# Start zerodowntime 

# stage#1 - stop services
stop_service ui
stop_service be
stop_service lp
stop_service fe1


# run tests stage#1 - server stopped for upgrade
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-1" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --categories="GranularUpdate"

# stage#2 - start some service
start_service be
start_service fe1

sleep 300 # TODO: remove this sleep after server fixed

# run tests stage#2 - server partially started
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-2" \
           --config=$config_root/TestConfig/AutoTests/AutoTestsConfig-zerodowntime.xml  \
           --params=$auto_tests_config_root/LocalParams.xml \
           --categories="GranularUpdate"

# stage#3 - start all services
start_service ui
start_service lp

# run tests stage#3 - server was started after upgrade
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-3" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --categories="GranularUpdate"

stopped_services="be-CampaignServer,adsc-nbadsat1"

# Stop services cycle
for service in $stopped_services
do
 srv=(`echo $service | tr ',' ' '`)
 echo "Stop service ${srv[0]} on ${srv[1]}..."
 ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no ${srv[1]} \
        "(cd $config_root; ./adserver.sh ${srv[1]}:${srv[0]} stop)"
 check_error $? "${srv[0]} stop "
 echo "Stop service ${srv[0]} on ${srv[1]}...ok"
done
# run tests - after services was stopped
let "stage += 1"
run_tests "$colocation_name-stage-$stage-after-service-stopped" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig-granular.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --exclude_categories="TriggerMatching,UserProfiling,UserProfilesExchange"

# Start services cycle
for service in $stopped_services
do
 srv=(`echo $service | tr ',' ' '`)
 echo "Start service ${srv[0]} on ${srv[1]}..."
 ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no ${srv[1]} \
        "(cd $config_root; ./adserver.sh ${srv[1]}:${srv[0]} start)"
 check_error $? "${srv[0]} start "
 echo "Start service ${srv[0]} on ${srv[1]}...ok"
 
done

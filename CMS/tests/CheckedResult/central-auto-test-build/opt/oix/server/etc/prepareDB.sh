#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh


# test config files
local_config_schema=$server_root/xsd/tests/AutoTests/LocalParams.xsd
test_local_config=$test_config_root/AutoTests/LocalParams.xml

mkdir -p $test_config_root/AutoTests

# clean all auto-tests data
echo "Cleaning db..."
db_clean.pl --db $db_schema --user $db_user --password $db_password --namespace 'UT'
check_error $? "db_clean.pl --db $db_schema --user $db_user --password $db_password --namespace 'UT'"
echo "Cleaning db...ok"

# fetch data
echo "Fetching data from db..."
fetch_data_from_db.pl \
  --db $db_schema \
  --user $db_user \
  --pswd $db_password \
  --local_params_path $test_local_config  \
  --local_params_scheme $local_config_schema
check_error $? "fetch_data_from_db.pl"
echo "Fetching data from db...ok"


echo "Test data file: $test_local_config"

exit 0


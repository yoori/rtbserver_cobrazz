#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh


# test config files
local_config_schema=$server_root/xsd/tests/AutoTests/LocalParams.xsd
test_local_config=$test_config_root/AutoTests/LocalParams.xml

mkdir -p $test_config_root/AutoTests

optstring_long="no-clean,nc,connection-string:,local_params_path:,local_params_scheme:"
opts=$(getopt -o "" -l "${optstring_long}" --name $0 -- "$@") ||
  exit $?

do_clean=1

eval set -- "$opts"
while true
do
  case "$1" in
  --no-clean|--nc)
    unset do_clean
    shift;;
  --connection-string)
    pg_connection_string=$2
    shift 2;;
  --local_params_path)
    test_local_config=$2
    shift 2;;
  --local_params_scheme)
    local_config_schema=$2
    shift 2;;
  --) shift; break;;
  esac
done

if [ "$do_clean" ]
then
  # clean all auto-tests data
  echo "Cleaning db..."
  db_clean.pl --db $db_schema --user $db_user --password $db_password --namespace 'UT'
  check_error $? "db_clean.pl --db $db_schema --user $db_user --password $db_password --namespace 'UT'"
  echo "Cleaning db...ok"
fi

# fetch data
echo "Fetching data from db..."
fetch_data_from_db.pl \
  --connection-string="$pg_connection_string" \
  --local_params_path $test_local_config  \
  --local_params_scheme $local_config_schema "$@"
check_error $? "fetch_data_from_db.pl"
echo "Fetching data from db...ok"


echo "Generated $test_local_config"

exit 0


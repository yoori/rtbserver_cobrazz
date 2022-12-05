#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

function usage ()
{
  echo "Usage : getlogs.sh <log path>" && exit 1
}

function check_error ()
{
 [ $1 -ne 0 ] && echo "command '$2' return error status code $1" >&2 && exit 3
}

function exec_througth_ssh
{
  ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no $1 "$2"
}

function copy_files
{
TETPATH=$test_log_path
EXIT_CODE=0
for file in $(exec_througth_ssh $1 \
"cd $config_root; find $4 -regex '$2' -not -empty | grep -v $TETPATH")
do
BYTES=3000000
FILENAME=$(basename $file)
echo "copy '$file' from host $1 to \"$3/$FILENAME\""
if [ ! -e "$3/$FILENAME" ]
then
  exec_througth_ssh $1 "tail -c$BYTES $file" > "$3/$FILENAME"
  let "EXIT_CODE |= $?"
fi
done
return $EXIT_CODE
}

HOSTS=$nb_hosts

[ "x$1" == "x" ] && usage

test_log_path=$1

echo "getlogs.sh \"$test_log_path\""

# Clear old logs & output
rm -rf $test_log_path/server-logs/*
rm -rf $test_log_path/server-output/*
rm -rf $test_log_path/server-configs/*
rm -rf $test_log_path/autotests-configs/*

for host in $HOSTS; do

    # Copy server logs
    mkdir -p $test_log_path/server-logs/$host
    echo "Getting logs from $host..."
    copy_files $host ".*\(\.\(log\|err\)\|/error_log\..*\)$" $test_log_path/server-logs/$host $workspace_root/log
    check_error $? "Copy log files"
    echo "Getting logs from $host...done"
    
    # Copy server output
    mkdir -p $test_log_path/server-output/$host
    echo "Getting output from $host..."
    copy_files $host ".*\.out" $test_log_path/server-output/$host $workspace_root/run
    check_error $? "Copy output files"
    echo "Getting output from $host...done"

    # Copy server configs
    mkdir -p $test_log_path/server-configs/$host
    echo "Getting configs from $host..."
    copy_files $host ".*\.xml" $test_log_path/server-configs/$host $config_root
    check_error $? "Copy configs files"
    echo "Getting configs from $host...done"

done


# Copy autotest configs
echo "Getting test configs..."
mkdir -p $test_log_path/autotests-configs/.
cp -rv $test_config_root/*.xml $test_log_path/autotests-configs/
echo "Getting test configs...done"

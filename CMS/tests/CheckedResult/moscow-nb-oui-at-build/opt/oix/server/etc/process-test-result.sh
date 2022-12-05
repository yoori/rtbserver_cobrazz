#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

function usage ()
{
  echo "Usage : process_test_result.sh <tests time> <log path> <destination path suffix> [<description>]" && exit 1
}


OPTIONS=""

[ "x$1" == "x" ] && usage
[ "x$2" == "x" ] && usage
[ "x$3" == "x" ] && usage


./getlogs.sh $2

# URL
if [ "x$url" != "x" ] 
then
OPTIONS+=" --url $url"
fi

# HTTP root
if [ "x$http_root" != "x" ] 
then
OPTIONS+=" --http_root $http_root"
fi

# HTTP test result path
if [ "x$http_test_path" != "x" ] 
then
OPTIONS+=" --http_test_path $http_test_path/$3"
else
OPTIONS+=" --http_test_path $3"
fi

# mail list
if [ "x$mail_list" != "x" ] 
then
OPTIONS+=" --maillist $mail_list"
fi

# history path
if [ "x$history_path" != "x" ] 
then
OPTIONS+=" --history_path $history_path"
fi

if [ "x$dst_sub_path" != "x" ] 
then
OPTIONS+=" --dst_sub_path $dst_sub_path"
fi

if [ "x$4" == "x" ]
then
DESC="Nightly build"
else
DESC=$4
fi

for path in $additional_logs
do
OPTIONS+=" --additional_log_path $path"
done

# make HTML content from logs
echo "Process tests results..."
makeHTML.pl --desc $DESC --in $2 \
            --timestamp $1 --stylesheet=${test_config_root}/tests.css $OPTIONS

check_error $? "makeHTML.pl"
echo "Process tests results...ok"

#!/bin/bash


. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

export TASKBOT_PARENT_PID=0
export TASKBOT_SHARE=$config_root/TestConfig/AutoTests

exec $taskbot_root/core/report.pl \
    $config_root/TestConfig/AutoTests/user.config \
    $taskbot_root/core/devel/user.report


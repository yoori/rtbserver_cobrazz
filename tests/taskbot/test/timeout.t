# -*- sh -*-

set +o errexit -o nounset -o noclobber


export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot


"$TASKBOT" $TASKBOT_OPTIONS timeout.config \
    -c 'echo 1; sleep $[$TASKBOT_SELECT_TIMEOUT + 1]; echo 2'
echo $?


export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot


"$TASKBOT" $TASKBOT_OPTIONS timeout.config \
    -c 'while true; do
            sleep $[$TASKBOT_SELECT_TIMEOUT - 1]; echo test
        done'
echo $?


export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot

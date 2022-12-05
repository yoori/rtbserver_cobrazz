# -*- sh -*-

set -o errexit -o nounset -o noclobber


"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo $1 $2' -- --arg1 arg2

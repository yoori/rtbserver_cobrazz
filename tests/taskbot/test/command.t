# -*- sh -*-

set -o errexit -o nounset -o noclobber


"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test'

"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test && false'

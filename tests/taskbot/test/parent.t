# -*- sh -*-

set -o errexit -o nounset -o noclobber


# Tets that variable is set.
echo $TASKBOT_PARENT_PID > /dev/null


echo '"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c '\''kill -KILL -- $TASKBOT_PARENT_PID'\' \
    | sh 2>&1 | perl -pe 's/\s+\d+ Killed/  <pid> Killed/;'

"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test' | \
    perl -pe 's/user \S+/user <user>/;s/on \S+:/on <host>:/;
              s/pid = \d+/pid = <pid>/;'

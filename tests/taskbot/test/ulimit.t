# -*- sh -*-
#
# This test is not 100% reliable because of the unavoidable
# parent-child race.  Sometimes you may see the diff:
#
# >  -70|69|-c 'echo test'|got SIGPIPE
# >  +70|69|-c 'echo test'|non-zero exit status, /bin/sh exited
#
# This is safe.  Note BTW how leading '>' is used to suppress '# +'
# recognition :'(.
#

set -o errexit -o nounset -o noclobber

ulimit -n 14
"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" -c 'echo test'

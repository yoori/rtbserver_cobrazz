# -*- sh -*-

set -o errexit -o nounset -o noclobber

exec >&- 2>&-
./zombie.pl $TASKBOT_SELECT_TIMEOUT


# -*- sh -*-

set -o errexit -o nounset -o noclobber


echo 1
kill -KILL $$
echo 2

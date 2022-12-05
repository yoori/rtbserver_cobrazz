task:
|taskbot self test main|
-1|set +o errexit -o nounset -o noclobber|
-2|for test in *.t; do
    echo -n $test...
    "$TASKBOT" --description "$test" "$TASKBOT_CONFIG" "$test"
    echo done
done|
-1|args.t|
-1|set -o errexit -o nounset -o noclobber|
-2|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo $1 $2' -- --arg1 arg2|
-1|echo $1 $2|
-5|command.t|
-1|set -o errexit -o nounset -o noclobber|
-2|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test'|
-1|echo test|
-4|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test && false'|
-1|echo test && false|non-zero exit status
-11|errexit.t|non-zero exit status, /bin/sh exited
-1|set -o errexit -o nounset -o noclobber|
-2|echo 1
false
echo 2|non-zero exit status
-14|here_document.t|
-1|set +o errexit -o nounset -o noclobber|
-2|# This is a command in line 11.
/no/such/command|non-zero exit status
-3|cat <<EOF
#
1
#
2
#
3
#
4
EOF|
-4|# This is a command in line 25.
/no/such/command|non-zero exit status
-5|/no/such/command
echo The above command is in line 28|
-20|kill.t|non-zero exit status, /bin/sh exited
-1|set -o errexit -o nounset -o noclobber|
-2|echo 1
kill -KILL $$
echo 2|non-zero exit status, probably killed by SIGKILL
-23|nesting.t|
-1|set -o errexit +o nounset -o noclobber|
-2|1|
-1|echo 1|
-2|11|
-1|echo 11|
-4|12|
-1|121|
-6|13|
-1|if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi|
-1|set -o errexit +o nounset -o noclobber|
-2|1|
-1|echo 1|
-2|11|
-1|echo 11|
-4|12|
-1|121|
-6|13|
-1|if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi|
-10|2|
-1|21|
-1|# Comment for 'echo 21'.
echo 21|
-13|3|
-23|2|
-1|21|
-1|# Comment for 'echo 21'.
echo 21|
-26|3|
-50|not_found.t|non-zero exit status
-1|set +o errexit -o nounset -o noclobber|
-2|/no/such/program --opt1 --opt2 arg more args|non-zero exit status
-53|parent.t|
-1|set -o errexit -o nounset -o noclobber|
-2|# Tets that variable is set.
echo $TASKBOT_PARENT_PID > /dev/null|
-3|echo '"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c '\''kill -KILL -- $TASKBOT_PARENT_PID'\' \
    | sh 2>&1 | perl -pe 's/\s+\d+ Killed/  <pid> Killed/;'|
-1|kill -KILL -- $TASKBOT_PARENT_PID|
-5|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" \
    -c 'echo test' | \
    perl -pe 's/user \S+/user <user>/;s/on \S+:/on <host>:/;
              s/pid = \d+/pid = <pid>/;'|
-1|echo test|
-60|timeout.t|
-1|set +o errexit -o nounset -o noclobber|
-2|export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot|
-1|set -o errexit -o nounset -o noclobber|
-2|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_ORIG_CONFIG" \
    -c "sleep $[$TASKBOT_SELECT_TIMEOUT + 5]; echo test"
echo $?|
-1|sleep 7; echo test|
-6|"$TASKBOT" $TASKBOT_OPTIONS timeout.config \
    -c 'echo 1; sleep $[$TASKBOT_SELECT_TIMEOUT + 1]; echo 2'
echo $?|
-1|echo 1; sleep $[$TASKBOT_SELECT_TIMEOUT + 1]; echo 2|select_timeout (2 secs) has expired
-8|export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot|
-1|set -o errexit -o nounset -o noclobber|
-2|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_ORIG_CONFIG" \
    -c "sleep $[$TASKBOT_SELECT_TIMEOUT + 5]; echo test"
echo $?|
-1|sleep 7; echo test|
-12|"$TASKBOT" $TASKBOT_OPTIONS timeout.config \
    -c 'while true; do
            sleep $[$TASKBOT_SELECT_TIMEOUT - 1]; echo test
        done'
echo $?|
-1|while true; do
            sleep $[$TASKBOT_SELECT_TIMEOUT - 1]; echo test
        done|run_timeout (10 secs) has expired
-14|export TASKBOT_ORIG_CONFIG="$TASKBOT_CONFIG"
"$TASKBOT" $TASKBOT_OPTIONS timeout.config timeout.taskbot|
-1|set -o errexit -o nounset -o noclobber|
-2|"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_ORIG_CONFIG" \
    -c "sleep $[$TASKBOT_SELECT_TIMEOUT + 5]; echo test"
echo $?|
-1|sleep 7; echo test|
-78|ulimit.t|non-zero exit status, /bin/sh exited
-1|set -o errexit -o nounset -o noclobber|
-2|ulimit -n 14
"$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" -c 'echo test'|non-zero exit status
-1|echo test|non-zero exit status
-82|var.t|
-1|set -o errexit -o nounset -o noclobber|
-2|OK=1|
-3|test $OK -eq 1|
-86|zombie.t|non-zero exit status, /bin/sh exited
-1|set -o errexit -o nounset -o noclobber|
-2|exec >&- 2>&-
./zombie.pl $TASKBOT_SELECT_TIMEOUT|non-zero exit status
|taskbot self test root level nesting|
-1|set -o errexit +o nounset -o noclobber|
-2|1|
-1|echo 1|
-2|11|
-1|echo 11|
-4|12|
-1|121|
-6|13|
-1|if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi|
-1|set -o errexit +o nounset -o noclobber|
-2|1|
-1|echo 1|
-2|11|
-1|echo 11|
-4|12|
-1|121|
-6|13|
-1|if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi|
-10|2|
-1|21|
-1|# Comment for 'echo 21'.
echo 21|
-13|3|
-23|2|
-1|21|
-1|# Comment for 'echo 21'.
echo 21|
-26|3|
command:
||0
args.t...done
command.t...done
errexit.t...done
here_document.t...done
kill.t...done
nesting.t...done
not_found.t...done
parent.t...done
timeout.t...done
ulimit.t...done
var.t...done
zombie.t...done
||0
||0
||0
--arg1 arg2
||0
||0
||0
test
||0
||0
test
||1
||0
1
||1
||0
|/bin/sh: line 11: /no/such/command: No such file or directory
|127
#
1
#
2
#
3
#
4
||0
|/bin/sh: line 25: /no/such/command: No such file or directory
|127
The above command is in line 28
|/bin/sh: line 28: /no/such/command: No such file or directory
|0
||0
1
||137
||0
1
||0
11
||0
||0
||0
1
||0
11
||0
||0
21
||0
21
||0
||0
|/bin/sh: line 6: /no/such/program: No such file or directory
|127
||0
||0
sh: line 2:  <pid> Killed                  "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" -c 'kill -KILL -- $TASKBOT_PARENT_PID'
||0
Deleting stale records from running_task table for user <user> on <host>:
  pid = <pid>
||0
test
||0
||0
||0
||0
0
||0
test
||0
1
||0
1
||
||0
||0
0
||0
test
||0
1
||0
test
test
test
test
test
test
test
test
test
||
||0
||0
0
||0
test
||0
||0
||1
|Can't exec "/bin/sh": Too many open files at <path>/taskbot.pl line <line>.
exec failed: Too many open files at <path>/taskbot.pl line <line>.
|24
||0
||0
||0
||0
||255
||0
1
||0
11
||0
set -o errexit +o nounset -o noclobber
0
echo 1
1
0
echo 11
11
0
if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi
0
# Comment for 'echo 21'.
echo 21
21
0
||0
||0
1
||0
11
||0
||0
21
||0
21
||0
running_task:
0

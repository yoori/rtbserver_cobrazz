#! /bin/sh

set -o errexit -o nounset -o noclobber -o xtrace
trap 'echo FAILED' ERR


cd $(dirname $0)

rm -f test.sqlite result.sql
sqlite3 test.sqlite < ../schema.sqlite

../taskbot.pl --description "taskbot self test main" \
    --trace run.config run.taskbot
../taskbot.pl --description "taskbot self test root level nesting" \
    --trace run.config nesting.t


sqlite3 test.sqlite <<EOF > result.sql
   SELECT 'task:';
   SELECT parent_task_id - id, description, error_message
   FROM task ORDER BY id;

   SELECT 'command:';
   SELECT stdout, stderr, exit_status FROM command ORDER BY task_id;

   SELECT 'running_task:';
   SELECT COUNT(*) FROM running_task;
EOF

ROOT=$(dirname $(pwd))
perl -i -pe "s|\Q$ROOT\E|<path>|g;
             s|(?<= at <path>/taskbot.pl line )\d+|<line>|g;" result.sql

diff -u gold.sql result.sql


rm test.sqlite result.sql


echo All tests passed

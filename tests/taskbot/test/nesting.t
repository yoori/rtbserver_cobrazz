# -*- sh -*-

set -o errexit +o nounset -o noclobber


# + 1
echo 1
# ++ 11
echo 11
# ++ 12
# +++ 121
# ++ 13

if [ "x$TASKBOT_NESTING" == "x" ]; then
    export TASKBOT_NESTING=1
    "$TASKBOT" $TASKBOT_OPTIONS "$TASKBOT_CONFIG" nesting.t
fi

# + 2
# Ordinary comment.
# ++ 21
# Comment for 'echo 21'.
echo 21
# + 3

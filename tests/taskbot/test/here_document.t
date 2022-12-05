# -*- sh -*-

set +o errexit -o nounset -o noclobber

# This
# is
# a
# comment.

# This is a command in line 11.
/no/such/command

cat <<EOF
#
1
#
2
#
3
#
4
EOF

# This is a command in line 25.
/no/such/command


/no/such/command
echo The above command is in line 28

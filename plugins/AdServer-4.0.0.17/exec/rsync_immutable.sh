#!/bin/bash

# util for sync unchangeable directories (on first level of tree) with garantee,
# that new appeared directory will be seen fully
# unchangeable - directory that can't be changed after creation (including its content)
#
# without unchangeable requirement here much inconsistent states for reader that fetch tree
# atomic exchange of directories impossible (application can fetch it now)
# 
if [ $# -lt 2 ] 
then
  echo "Not enough parameters."
  echo "USAGE: rsync_immutable.sh <SRC_FILE_PATH> <DST_FILE_PATH> <INTERMEDIATE_DST_PATH>"
  echo "DST_FILE_PATH and INTERMEDIATE_DST_PATH must be on one file system"
  exit -1;
fi

SRC_PATH=$1
DST_PATH=$2
INTERMEDIATE_DST_PATH=$3

#mkdir -p $DST_PATH || exit -1
if [ ! -e $DST_PATH ] ; then
  echo "destination path '$DST_PATH' not exists" >&2
  exit -1
fi

# convert DST_FILE_PATH to absolute (required for rsync --link-dest option)
DST_PATH=`cd $DST_PATH ; pwd`

if [ -z "$INTERMEDIATE_DST_PATH" ] ; then
  INTERMEDIATE_DST_PATH=`echo $DST_PATH | sed -e 's|/*$||'`~
fi
echo INTERMEDIATE_DST_PATH=$INTERMEDIATE_DST_PATH

# use non rsync error code -1
rm -rf $INTERMEDIATE_DST_PATH || exit -1 # permissions problem

mkdir $INTERMEDIATE_DST_PATH || exit -1

# rsync sources with hard links usage
/usr/bin/rsync -avz -t --link-dest=$DST_PATH --timeout=55 \
  --log-format=%f --delete-after $SRC_PATH $INTERMEDIATE_DST_PATH || exit $?

# remove dissappeared files
ls -1 $DST_PATH | while read FILE; do
  if [ ! -e $INTERMEDIATE_DST_PATH/$FILE ] ; then
    # remove atomically
    mv $DST_PATH/$FILE $INTERMEDIATE_DST_PATH/$FILE || exit -1
    rm -rf $INTERMEDIATE_DST_PATH/$FILE || exit -1
  fi
done

# add appeared files
ls -1 $INTERMEDIATE_DST_PATH | while read FILE; do
  if [ ! -e $DST_PATH/$FILE ] ; then
    echo "move $INTERMEDIATE_DST_PATH/$FILE to $DST_PATH/$FILE"
    mv $INTERMEDIATE_DST_PATH/$FILE $DST_PATH/$FILE || exit -1
  fi
done

rm -rf $INTERMEDIATE_DST_PATH || exit -1

exit 0

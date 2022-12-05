#!/bin/bash

if [ $# -lt 2 ] 
then
  echo "Not enough parameters."
  echo "USAGE: copy_and_backup.sh <COPY COMMAND with SRC_FILE_PATH DST_FILE_PATH> <SRC_FILE_PATH>"
  exit -1;
fi

COMMAND=$1
SRC_PATH=$2
PATTERN="/*"

SRC_DIR=${SRC_PATH%$PATTERN}
BACKUP_DIR=${SRC_DIR//\/Intermediate/}_
BACKUP_DST_PATH="${BACKUP_DIR}${SRC_PATH:${#SRC_DIR}}"
BACKUP_COMMAND="mkdir -p ${BACKUP_DIR} && cp $SRC_PATH $BACKUP_DST_PATH"

eval $BACKUP_COMMAND
RESULT=$?
if [ $RESULT -ne 0 ] 
then
  echo "Can't backup file $SRC_PATH" >&2
  exit $RESULT
fi

eval $COMMAND
RESULT=$?
if [ $RESULT -ne 0 ] 
then
  echo "Can't copy file $SRC_PATH" >&2
fi

exit $RESULT

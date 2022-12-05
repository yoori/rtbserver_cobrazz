#!/bin/bash

APP_XML=''
SERVICES_XPATHS=()
SERVICES_XPATHS_COUNT=0
PLUGIN_ROOT=''
OUT_DIR=''
CMDS=()
CMDS_COUNT=0

while [ "$#" -gt "0" ]
do
  case $1 in
  --app-xml)
    shift
    APP_XML=$1
    ;;
  --services-xpath)
    shift
    SERVICES_XPATHS[$SERVICES_XPATHS_COUNT]=$1
    SERVICES_XPATHS_COUNT=$SERVICES_XPATHS_COUNT+1
    ;;
  --plugin-root)
    shift
    PLUGIN_ROOT=$1
    ;;
  --out-dir)
    shift
    OUT_DIR=$1
    ;;
  --cmd)
    shift
    CMDS[$CMDS_COUNT]=$1
    CMDS_COUNT=$CMDS_COUNT+1
    ;;
  esac
  shift
done

EXEC=$PLUGIN_ROOT/exec

for (( i=0; i<$SERVICES_XPATHS_COUNT; i++ ))
do
  SERVICES_XPATH=${SERVICES_XPATHS[$i]}
  SERVICES_HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$SERVICES_XPATH" --plugin-root $PLUGIN_ROOT`
  [ x"$SERVICES_HOSTS" == 'x' ] && echo "Absent hosts for service \"$SERVICES_XPATH\"" >&2 && exit 1
  for host in $SERVICES_HOSTS
  do
  HOST_DIR=$OUT_DIR/$host
    for (( k=0; k<$CMDS_COUNT; k++ ))
    do
      CMD=${CMDS[$k]}
      eval $CMD
      [ $? -ne 0 ] && echo "Can't execute command $CMD" >&2 && exit 1
    done
  done
done

exit 0

#!/bin/bash

XSL_FILE=''
APP_XML=''
OUT_FILE=''
PLUGIN_ROOT=''
VARS=''
MACROSES=''
OUT_DIR=''
OUT_DIR_SUFFIX=''
CLUSTER_ID='0'

while [ "$#" -gt "0" ]
do
  case $1 in
  --xsl)
    shift
    XSL_FILE=$1
    ;;
  --app-xml)
    shift
    APP_XML=$1
    ;;
  --services-xpath)
    shift
    SERVICES_XPATH="$1"
    ;;
  --out-file)
    shift
    OUT_FILE=$1
    ;;
  --var)
    shift
    VARS+="--var $1 \"$2\" "
    if [ $1 == "XPATH" ] || [ $1 == "OUT_DIR" ]
    then
     echo "XPATH or OUT_DIR var is inadmissible here" >&2
     exit 1
    fi
    shift
    ;;
  --macro)
    shift
    MACROSES+="--macro "\'"$1"\'" "\'"$2"\'" "
    shift
    ;;
  --plugin-root)
    shift
    PLUGIN_ROOT=$1
    ;;
  --out-dir)
    shift
    OUT_DIR=$1
    ;;
  --out-dir-suffix)
    shift
    OUT_DIR_SUFFIX=$1
    ;;
  --service-hosts)
    shift
    SET_SERVICE_HOSTS="$1"
    ;;
  --cluster-id)
    shift
    CLUSTER_ID="$1"
    ;;
  esac
  shift
done

EXEC=$PLUGIN_ROOT/exec
FILE_SUBDIR=`dirname $OUT_FILE`


SERVICES_COUNT_XPATH="count($SERVICES_XPATH)"
SERVICES_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SERVICES_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`
SERVICE_ID=1
EXIT_CODE=0
HOSTS_COUNT=0

for (( i=1; i<=SERVICES_COUNT; i++ ))
do
  SERVICE_XPATH="$SERVICES_XPATH[$i]"

  if [ -z "$SET_SERVICE_HOSTS" ]; then
    SERVICE_HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$SERVICE_XPATH" --plugin-root $PLUGIN_ROOT`
    [ x"$SERVICE_HOSTS" == 'x' ] && echo "Absent hosts for service \"$SERVICE_XPATH\"" >&2 && exit 1
  else
    SERVICE_HOSTS="$SET_SERVICE_HOSTS"
  fi

  for CUR_HOST in $SERVICE_HOSTS
  do
    HOSTS_COUNT=$(($HOSTS_COUNT+1))
  done
done

for (( i=1; i<=SERVICES_COUNT; i++ ))
do
  SERVICE_XPATH="$SERVICES_XPATH[$i]"

  if [ -z "$SET_SERVICE_HOSTS" ]; then
    SERVICE_HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$SERVICE_XPATH" --plugin-root $PLUGIN_ROOT`
    [ x"$SERVICE_HOSTS" == 'x' ] && echo "Absent hosts for service \"$SERVICE_XPATH\"" >&2 && exit 1
  else
    SERVICE_HOSTS="$SET_SERVICE_HOSTS"
  fi

  for host in $SERVICE_HOSTS
  do
    mkdir -p $OUT_DIR/$host/$FILE_SUBDIR

    [ $? -ne 0 ] && echo "Can't create directory $OUT_DIR/$host/$FILE_SUBDIR" >&2 && exit 1

    COMMAND="$EXEC/XsltTransformer.sh  $VARS $MACROSES
        --var OUT_DIR "$OUT_DIR_SUFFIX/$host"
        --var XPATH "\"$SERVICE_XPATH\""
        --var SERVICE_ID "\"$SERVICE_ID\""
        --var SERVICE_COUNT "\"$HOSTS_COUNT\""
        --var HOST "$host"
        --var CLUSTER_ID "$CLUSTER_ID"
        --app-xml $APP_XML
        --xsl $XSL_FILE
        --out-file $OUT_DIR/$host/$OUT_FILE"

    eval $COMMAND

    let "EXIT_CODE|=$?"

#   echo $COMMAND

    SERVICE_ID=$(($SERVICE_ID + 1))
  done
done

exit $EXIT_CODE

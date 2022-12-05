#!/bin/bash

APP_XML=''
CONFIG_XPATH=''
PLUGIN_ROOT=''
SERVICE_NAME=''
CLUSTER_NAME=''
DEF_PORT_PREFIX='def-'
DEF_PORT_POSTFIX='-port'
SEARCH_XPATH=''
SERVICES_XPATH=''
OUT_DIR=''
OUT_FILE=''
PORT_XPATH=''
RELATIVE_PORT_XPATH=''

while [ "$#" -gt "0" ]
do
  case $1 in
  --plugin-root)
    shift
    PLUGIN_ROOT=$1
    ;;
  --app-xml)
    shift
    APP_XML=$1
    ;;
  --config-xpath)
    shift
    CONFIG_XPATH=$1
    ;;
  --service-name)
    shift
    SERVICE_NAME=$1
    ;;
  --cluster)
    shift
    CLUSTER_NAME=$1
    ;;
  --def-port-add-prefix)
    shift
    DEF_PORT_PREFIX=$DEF_PORT_PREFIX$1
    ;;
  --search-xpath)
    shift
    SEARCH_XPATH=$1
    ;;
  --services-xpath)
    shift
    SERVICES_XPATH=$1
    ;;
  --out-dir)
    shift
    OUT_DIR=$1
    ;;
  --out-file)
    shift
    OUT_FILE=$1
    ;;
  --service-hosts)
    shift
    SET_SERVICE_HOSTS="$1"
    ;;
  --port-xpath)
    shift
    PORT_XPATH="$1"
    ;;
  --relative-port-xpath)
    shift
    RELATIVE_PORT_XPATH="$1"
    ;;
  esac
  shift
done

DEF_PORT_VAR_NAME=${DEF_PORT_PREFIX}`(echo -n ${SERVICE_NAME} | sed -e 's|_|-|g')`${DEF_PORT_POSTFIX}

EXEC=$PLUGIN_ROOT/exec
FILE_SUBDIR=`dirname $OUT_FILE`

SERVICES_COUNT_XPATH="count($SERVICES_XPATH)"
SERVICES_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SERVICES_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`

for (( i=1; i<=SERVICES_COUNT; i++ ))
do
  SERVICE_XPATH="$SERVICES_XPATH[$i]"

  if [ -z "$SET_SERVICE_HOSTS" ]; then
    SERVICE_HOSTS=`$EXEC/GetHosts.sh --app-xml $APP_XML --host-service-xpath "$SERVICE_XPATH" --plugin-root $PLUGIN_ROOT`
    [ x"$SERVICE_HOSTS" == 'x' ] && echo "Absent hosts for service \"$SERVICE_XPATH\"" >&2 && exit 1
  else
    SERVICE_HOSTS="$SET_SERVICE_HOSTS"
  fi

  if [ x"$PORT_XPATH" == 'x' ]; then
    SERVICE_CONFIG_XPATH="${SERVICES_XPATH}[$i]/$SEARCH_XPATH"
    if [ x"$CONFIG_XPATH" == 'x' ]
    then
      CONFIG_XPATH_EXP="$SERVICE_CONFIG_XPATH"
    else
      CONFIG_XPATH_EXP="$SERVICE_CONFIG_XPATH[count($SERVICE_CONFIG_XPATH/*) > 0] |
        $CONFIG_XPATH[count($SERVICE_CONFIG_XPATH/*) = 0]"
    fi

    if [ x"$RELATIVE_PORT_XPATH" == 'x' ]; then
      PORT_XPATH="$CONFIG_XPATH_EXP/cfg:networkParams/@port"
    else
      PORT_XPATH="$CONFIG_XPATH_EXP/$RELATIVE_PORT_XPATH"
    fi
  fi

  for host in $SERVICE_HOSTS
  do
    mkdir -p $OUT_DIR/$host/$FILE_SUBDIR
    [ $? -ne 0 ] && echo "Can't create directory $OUT_DIR/$host/$FILE_SUBDIR" >&2 && exit 1

    #echo '# PORT_XPATH='$PORT_XPATH'' >> $OUT_DIR/$host/$OUT_FILE

    $EXEC/XsltTransformer.sh \
        --var XPATH "$PORT_XPATH" \
        --var SERVICE_NAME "$SERVICE_NAME" \
        --var CLUSTER_NAME "$CLUSTER_NAME" \
        --var DEF_PORT_VAR_NAME "$DEF_PORT_VAR_NAME" \
        --app-xml $APP_XML \
        --xsl $PLUGIN_ROOT/xslt/CurrentEnv.xsl \
        --out-file - >> $OUT_DIR/$host/$OUT_FILE
    [ $? -ne 0 ] && echo "Can't set config for ${SERVICE_NAME} by xpath $CONFIG_XPATH_EXP" >&2 && exit 1
  done
done

exit 0

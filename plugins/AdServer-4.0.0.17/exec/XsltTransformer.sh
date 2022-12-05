#!/bin/bash

XSL_FILE=''
APP_XML=''
OUT_FILE=''

PARAMS=''

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
  --service-xpath)
    shift
    SERVICE_XPATH=$1
    ;;
  --out-file)
    shift
    OUT_FILE=$1
    ;;
  --var)
    shift
    PARAMS+="--stringparam $1 \"$2\" "
    shift
    ;;
  esac
  shift
done

EX_CODE=0

#echo "to transform 'xsltproc $PARAMS $XSL_FILE $APP_XML'" >&2
#echo "eval xsltproc $PARAMS -o $OUT_FILE $XSL_FILE $APP_XML" >>/tmp/test.log

if [ $OUT_FILE != '-' ]
then
  eval xsltproc $PARAMS -o $OUT_FILE $XSL_FILE $APP_XML
  EX_CODE=$?
else
  eval xsltproc $PARAMS $XSL_FILE $APP_XML
  EX_CODE=$?
fi

#echo "from transform '$XSL_FILE'" >&2

[ $EX_CODE -ne 0 ] && echo "XsltTransform error ($XSL_FILE) to $OUT_FILE: xsltproc $PARAMS $XSL_FILE $APP_XML" >&2 &&  exit 1

exit 0

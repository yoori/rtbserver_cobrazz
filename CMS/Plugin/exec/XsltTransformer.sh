#!/bin/bash

XSL_FILE=''
APP_XML=''
OUT_FILE=''

PARAMS=''
MACROSES=''

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
  --macro)
    shift
    MACROSES+="s/$1/$2/g ; "
    shift
    ;;
  esac
  shift
done

EX_CODE=0

#echo "to transform 'xsltproc $PARAMS $XSL_FILE $APP_XML'" >&2
#echo "eval xsltproc $PARAMS -o $OUT_FILE $XSL_FILE $APP_XML" >>/tmp/test.log

TMP_FILE=''

if [ "$MACROSES" != "" ]
then
  TMP_FILE="/tmp/$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1).xslt"
  #echo ">>>>>>>>>>>>>>>>>>>>>> sed '$MACROSES'"
  cat "$XSL_FILE" | sed "$MACROSES" >"$TMP_FILE"
  PXSL_FILE="$TMP_FILE"
else
  PXSL_FILE="$XSL_FILE"
fi

INCDIR="$INCDIR --path '$(pwd)'"

pushd $(dirname "$XSL_FILE") >/dev/null 2>&1
INCDIR="$INCDIR --path '$(pwd)'"
popd >/dev/null 2>&1

pushd $(dirname "$XSL_FILE") >/dev/null 2>&1
cd ..
INCDIR="$INCDIR --path '$(pwd)'"
popd >/dev/null 2>&1

#echo "TTTTTTTTTTT: $INCDIR"
#INCDIR="--path "$(pushd $("dirname $XSL_FILE") 2>/dev/null 2>&1 ; pwd ; popd 2>/dev/null 2>&1)" --path '$PWD'"

if [ $OUT_FILE != '-' ]
then
  eval xsltproc $PARAMS -o $OUT_FILE $INCDIR "$PXSL_FILE" $APP_XML 
  EX_CODE=$?
else
  eval xsltproc $PARAMS $INCDIR "$PXSL_FILE" $APP_XML
  EX_CODE=$?
fi

#echo "from transform '$XSL_FILE'" >&2

[ $EX_CODE -ne 0 ] && echo "XsltTransform error ($XSL_FILE) to $OUT_FILE: xsltproc $PARAMS $INCDIR $PXSL_FILE $APP_XML" >&2 &&  exit 1

exit 0

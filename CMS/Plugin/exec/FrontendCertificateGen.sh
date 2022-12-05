#!/bin/bash

EXIT_CODE=0
PLUGIN_ROOT=''
APP_XML=''
CLUSTER_XPATH=''
CERT_ROOT=''

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
  --cluster-xpath)
    shift
    CLUSTER_XPATH=$1
    ;;
  --certificate-root)
    shift
    CERT_ROOT=$1
    ;;
  esac
  shift
done

EXEC=$PLUGIN_ROOT/exec

ALL_SSL_PORTS_XPATH="$CLUSTER_XPATH/configuration/cfg:cluster/
  cfg:coloParams/cfg:secureVirtualServer[
  not(@internal_port = ../preceding-sibling::cfg:secureVirtualServer/@internal_port)
  ]"

SSL_COUNT_XPATH="count($ALL_SSL_PORTS_XPATH)"
SSL_COUNT=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SSL_COUNT_XPATH" --plugin-root $PLUGIN_ROOT`
INTERNAL_HTTPS_DEF_PORT=10143

mkdir -p $CERT_ROOT

for (( i=1; i<=SSL_COUNT; i++ ))
do
  SSL_XPATH="$ALL_SSL_PORTS_XPATH[$i]"
  SSL_PORT=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SSL_XPATH/@internal_port" --plugin-root $PLUGIN_ROOT)
  if [ x"$SSL_PORT" == 'x' ]; then
    SSL_PORT=$INTERNAL_HTTPS_DEF_PORT
  fi

  SSLKEY_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "count($SSL_XPATH/cfg:key)" --plugin-root $PLUGIN_ROOT)
  SSLCERT_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "count($SSL_XPATH/cfg:cert)" --plugin-root $PLUGIN_ROOT)
  SSLCA_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath "count($SSL_XPATH/cfg:ca)" --plugin-root $PLUGIN_ROOT)

  # default certificates apkey.pem apcert.pem apca.pem store in ...
  # it use if colocation.xml doesn't contain SSL httpsParams

  # rewrite certificates if it directly defined
  if [ $SSLKEY_PRESENTED -ne 0 ]
  then
    echo "SSL: Generate SSL Key for port=$SSL_PORT"
    $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SSL_XPATH/cfg:key/cfg:content/text()" \
      --plugin-root $PLUGIN_ROOT >$CERT_ROOT/apkey-$SSL_PORT.pem
    let "EXIT_CODE|=$?"
  fi

  if [ $SSLCERT_PRESENTED -ne 0 ]
  then
    echo "SSL: Generate SSL Cert for port=$SSL_PORT"
    $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SSL_XPATH/cfg:cert/cfg:content/text()" \
      --plugin-root $PLUGIN_ROOT >$CERT_ROOT/apcert-$SSL_PORT.pem
    let "EXIT_CODE|=$?"
  fi

  if [ $SSLCA_PRESENTED -ne 0 ]
  then
    echo "SSL: Generate SSL CA for port=$SSL_PORT"
    $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$SSL_XPATH/cfg:ca/cfg:content/text()" \
      --plugin-root $PLUGIN_ROOT >$CERT_ROOT/apca-$SSL_PORT.pem
    let "EXIT_CODE|=$?"
  fi

  if [ $SSLCERT_PRESENTED -ne 0 -a $SSLCA_PRESENTED -ne 0 ]
  then
    cat $CERT_ROOT/apcert-$SSL_PORT.pem > $CERT_ROOT/apcertca-$SSL_PORT.pem
    echo '' >> $CERT_ROOT/apcertca-$SSL_PORT.pem
    cat $CERT_ROOT/apca-$SSL_PORT.pem >> $CERT_ROOT/apcertca-$SSL_PORT.pem
  fi
done

COLO_PARAMS_XPATH="$CLUSTER_XPATH/configuration/cfg:cluster/cfg:coloParams"
UID_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --plugin-root $PLUGIN_ROOT --xml $APP_XML \
  --xpath "count($COLO_PARAMS_XPATH/*[name() = 'configuration:uid_key' and . != ''])")

if [ $UID_KEY_PRESENTED -ne 0 ]
then
  $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$COLO_PARAMS_XPATH/cfg:uid_key[1]" \
    --plugin-root $PLUGIN_ROOT >$CERT_ROOT/uid-key.pem
  openssl rsa -in $CERT_ROOT/uid-key.pem -out $CERT_ROOT/private.der -outform DER 1>/dev/null 2>&1
  if [ $? -eq 0 ]
  then
    rm $CERT_ROOT/uid-key.pem
  else
    let "EXIT_CODE|=$?"
    echo "incorrect uid key" >&2
  fi
  perl $EXEC/bin/ResignUids.pl $CERT_ROOT $PLUGIN_ROOT/data/SubAgentShell/SignedUids.pm
  openssl rsa -inform DER -in $CERT_ROOT/private.der -outform DER -pubout -out $CERT_ROOT/public.der 1>/dev/null 2>&1
  let "EXIT_CODE|=$?"
else
  cp $PLUGIN_ROOT/data/SubAgentShell/SignedUids.pm $CERT_ROOT 
fi
TMP_UID_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --plugin-root $PLUGIN_ROOT --xml $APP_XML \
  --xpath "count($COLO_PARAMS_XPATH/*[name() = 'configuration:temporary_uid_key' and . != ''])")
if [ $TMP_UID_KEY_PRESENTED -ne 0 ]
then
  $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$COLO_PARAMS_XPATH/cfg:temporary_uid_key[1]" \
    --plugin-root $PLUGIN_ROOT >$CERT_ROOT/temporary-uid-key.pem
  openssl rsa -in $CERT_ROOT/temporary-uid-key.pem -outform DER -pubout -out $CERT_ROOT/public_temp.der 1>/dev/null 2>&1
  if [ $? -eq 0 ]
  then
    rm $CERT_ROOT/temporary-uid-key.pem
  else
    let "EXIT_CODE|=$?"
    echo "failed temporary uid public key generation" >&2
  fi
fi

exit $EXIT_CODE

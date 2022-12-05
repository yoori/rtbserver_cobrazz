#!/bin/bash

if [ $# -lt 8 ]
then
  echo "$0 : number of argument must be equal 8."
  exit 1
fi

APP_XML=$1
CLUSTER_XPATH=$2
PLUGIN_ROOT=$3
SPEC_PATH=$4
PACKAGE_NAME=$5
BUILD_ROOT=$6
VERSION=$7
RELEASE=$8

EXEC=$PLUGIN_ROOT/exec
XSLT_ROOT=$PLUGIN_ROOT/xslt

. $EXEC/ColocationName.sh $APP_XML $PLUGIN_ROOT

CMANAGER_VERSION='3.5.0'

APP_ENV_XPATH=$CLUSTER_XPATH/configuration/cfg:environment

KEY_FILE_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "count($APP_ENV_XPATH[@ssh_key != ''])" --plugin-root $PLUGIN_ROOT)
PRIVATE_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "count($APP_ENV_XPATH/cfg:forosZoneManagement/cfg:private_key[ . != '' ])" --plugin-root $PLUGIN_ROOT)

SEPARATE_ISP_ZONE=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "$APP_ENV_XPATH/cfg:ispZoneManagement/@separate_isp_zone" --plugin-root $PLUGIN_ROOT)
#default value of separate zone config is false, so if absent will set it
if [ -z "$SEPARATE_ISP_ZONE" ]
then
SEPARATE_ISP_ZONE=false
fi

ISP_PRIVATE_KEY_PRESENTED=0
ISP_PUBLIC_KEY_PRESENTED=0
if [ "$SEPARATE_ISP_ZONE" = "true" ]
then
  ISP_PRIVATE_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
    "count($APP_ENV_XPATH/cfg:ispZoneManagement/cfg:private_key[ . != '' ])" --plugin-root $PLUGIN_ROOT)
  ISP_PUBLIC_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
    "count($APP_ENV_XPATH/cfg:ispZoneManagement/@public_key)" --plugin-root $PLUGIN_ROOT)
  PRODUCT_IDENTIFIER=$COLOCATION_NAME-foros-$VERSION-$RELEASE
  PRODUCT_ISP_IDENTIFIER=$COLOCATION_NAME-isp-$VERSION-$RELEASE
else
  PRODUCT_IDENTIFIER=$COLOCATION_NAME-$VERSION-$RELEASE
fi

PUBLIC_KEY_PRESENTED=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "count($APP_ENV_XPATH/cfg:forosZoneManagement/@public_key)" --plugin-root $PLUGIN_ROOT)
AUTORESTART_ENABLE=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "$APP_ENV_XPATH/@autorestart" --plugin-root $PLUGIN_ROOT)

if [ "$AUTORESTART_ENABLE" = "true" ]
then
  AUTORESTART_PREDEFINED_FOLDER=$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
    "$APP_ENV_XPATH/cfg:develParams/@autorestart_state_root" --plugin-root $PLUGIN_ROOT)

  if [ -z "$AUTORESTART_PREDEFINED_FOLDER" ]
  then
    AUTORESTART_PREDEFINED_FOLDER=/opt/foros/manager/var/state/server-$COLOCATION_NAME
  fi

  mkdir -p $BUILD_ROOT$AUTORESTART_PREDEFINED_FOLDER
fi

KEY_FILE_PATH="$BUILD_ROOT/home/$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
  "$APP_ENV_XPATH/cfg:forosZoneManagement/@user_name" --plugin-root $PLUGIN_ROOT)/.ssh"
mkdir -p $KEY_FILE_PATH

if [ $PRIVATE_KEY_PRESENTED -ne 0 ]
then
# write private key to file
  KEY_FILE="$KEY_FILE_PATH/adkey.$PRODUCT_IDENTIFIER"
  $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$APP_ENV_XPATH/cfg:forosZoneManagement/cfg:private_key" \
    --plugin-root $PLUGIN_ROOT >$KEY_FILE
  chmod 600 $KEY_FILE
# generate public keys
  PUBLIC_KEY=`ssh-keygen -y -P '' -f "${KEY_FILE}"`
  if [ $? -ne 0 ]
  then
    echo "can't generate key for foros key $KEY_FILE"
    exit $?
  fi
fi

if [ $PUBLIC_KEY_PRESENTED -ne 0 ]
then
# get public key value from colocation.xml
  PUBLIC_KEY=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$APP_ENV_XPATH/cfg:forosZoneManagement/@public_key" \
    --plugin-root $PLUGIN_ROOT`
fi

if [ $ISP_PRIVATE_KEY_PRESENTED -eq 1 ]
then
  ISP_KEY_FILE_PATH="$BUILD_ROOT/home/$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
    "$APP_ENV_XPATH/cfg:ispZoneManagement/@user_name" --plugin-root $PLUGIN_ROOT)/.ssh"
  mkdir -p $ISP_KEY_FILE_PATH
# write private key to file
  ISP_KEY_FILE="$ISP_KEY_FILE_PATH/adkey.$PRODUCT_ISP_IDENTIFIER"
  $EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$APP_ENV_XPATH/cfg:ispZoneManagement/cfg:private_key" \
    --plugin-root $PLUGIN_ROOT >$ISP_KEY_FILE
# generate public keys
# If need for ISP add injection into .spec
  if [ -z "$ISP_PUBLIC_KEY" ]
  then
    chmod 600 $ISP_KEY_FILE
    ISP_PUBLIC_KEY=`ssh-keygen -y -P '' -f "${ISP_KEY_FILE}"`
    if [ $? -ne 0 ]
    then
      echo "can't generate key for isp key $ISP_KEY_FILE"
      exit $?
    fi
  fi
else
  if [ "$SEPARATE_ISP_ZONE" = "true" -a $PRIVATE_KEY_PRESENTED -ne 0 ]
  then
    ISP_KEY_FILE_PATH="$BUILD_ROOT/home/$($EXEC/XPathGetValue.sh --xml $APP_XML --xpath \
      "$APP_ENV_XPATH/cfg:ispZoneManagement/@user_name" --plugin-root $PLUGIN_ROOT)/.ssh"
    mkdir -p $ISP_KEY_FILE_PATH
    ISP_KEY_FILE="$ISP_KEY_FILE_PATH/adkey.$PRODUCT_ISP_IDENTIFIER"
    cp "$KEY_FILE" "$ISP_KEY_FILE"
    chmod 600 $ISP_KEY_FILE
    ISP_PUBLIC_KEY=$PUBLIC_KEY
  fi
fi

if [ $ISP_PUBLIC_KEY_PRESENTED -ne 0 ]
then
# get public key value from colocation.xml
  ISP_PUBLIC_KEY=`$EXEC/XPathGetValue.sh --xml $APP_XML --xpath "$APP_ENV_XPATH/cfg:ispZoneManagement/@public_key" \
    --plugin-root $PLUGIN_ROOT`
fi

### Spec
$EXEC/XsltTransformer.sh \
  --var XPATH "$CLUSTER_XPATH" \
  --var PACKAGE_NAME "$PACKAGE_NAME" \
  --var BUILD_ROOT "$BUILD_ROOT" \
  --var RELEASE "$RELEASE" \
  --var PRODUCT_IDENTIFIER "$PRODUCT_IDENTIFIER" \
  --var PRODUCT_ISP_IDENTIFIER "$PRODUCT_ISP_IDENTIFIER" \
  --var CMANAGER_VERSION "$CMANAGER_VERSION" \
  --var PLUGIN_ROOT "$PLUGIN_ROOT" \
  --app-xml $APP_XML \
  --xsl $XSLT_ROOT/specs/AdServerSpec.xsl \
  --out-file $SPEC_PATH

#inject public key value into AdServer.spec
if [ -n "${PUBLIC_KEY}" -a -f "${SPEC_PATH}" ]; then
  sed -e "s#___PUBLIC_KEY___#${PUBLIC_KEY}#g" -i "${SPEC_PATH}"
else
  sed -e "/__public_key/d" -i "${SPEC_PATH}"
fi

if [ -n "${ISP_PUBLIC_KEY}" -a -f "${SPEC_PATH}" -a "${PUBLIC_KEY}" != "${ISP_PUBLIC_KEY}" ]; then
  sed -e "s#___ISP_PUBLIC_KEY___#${ISP_PUBLIC_KEY}#g" -i "${SPEC_PATH}"
else
  sed -e "/__isp_public_key/d" -i "${SPEC_PATH}"
fi

exit $?

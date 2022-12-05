#!/bin/bash

COLOCATION=colocation.xml.t

. envdev.sh

COLOCATION_OUT=$config_root/colocation.xml

if [ "x$1" != "x" ]
then
  COLOCATION=$1
fi

if [ "x$2" != "x" ]
then
  COLOCATION_OUT=$2
fi

if [ "x$3" != "x" ]
then
  config_root=$3
fi

mkdir -p $config_root/

ptempl.pl -i $COLOCATION -o $COLOCATION_OUT -p devvars

EXIT_CODE=$?

PREFIX=build/colo_null/app_null
cp -r $config_root/$PREFIX/BackendSubCluster/opt/oix/server/etc/* $config_root/ 2>/dev/null
cp -r $config_root/$PREFIX/FrontendSubCluster/opt/oix/server/etc/* $config_root/ 2>/dev/null
cp -r $config_root/$PREFIX/AdCluster/opt/oix/server/etc/* $config_root/ 2>/dev/null

exit $EXIT_CODE

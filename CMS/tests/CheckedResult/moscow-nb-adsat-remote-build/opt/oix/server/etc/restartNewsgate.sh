#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

# Restart Newsgate-emu
if [ -n "$newsgate_port" ]
then
  echo "Restart newsgate..."
  mkdir -p $workspace_root/log/NewsgateEmu/AutoTests || { echo "Can't create $workspace_root/log/NewsgateEmu" && exit 1 ; }
  rm -rf $workspace_root/log/NewsgateEmu/AutoTests/*
  mkdir -p $workspace_root/run/NewsgateEmu || { echo "Can't create $workspace_root/log/NewsgateEmu" && exit 1 ; }
  /sbin/fuser -n tcp $newsgate_port -k -KILL || true
  rm -rf $workspace_root/run/NewsgateEmu/*
  path=`pwd`
  cd $workspace_root/run/NewsgateEmu
  newsgate.py -c=$config_root/TestConfig/AutoTests/newsgate.cfg start
  cd $path
  echo "Restart newsgate...ok"
fi

exit 0
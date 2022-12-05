#!/bin/bash

if [ ! -f "./envdev.sh" ]; then
echo "./envdev.sh doesn't exists"
exit 1;
fi
source ./envdev.sh

COLOCATION=$config_root/colocation.xml
PREFIX=build/build_null/colo_null/AdServer-$VERSION

if [ "x$1" != "x" ]
then
  COLOCATION=$1
fi

PLUGIN_ROOT=$server_root/CMS/Plugin
echo "$PLUGIN_ROOT"
EXEC=$PLUGIN_ROOT/exec

ISP_ZONE_ENABLED_XPATH="/colo:colocation/application[1]/configuration/cfg:environment/cfg:ispZoneManagement/@separate_isp_zone"
ISP_ZONE_ENABLED=`$EXEC/XPathGetValue.sh --xml $COLOCATION --xpath "$ISP_ZONE_ENABLED_XPATH" --plugin-root $PLUGIN_ROOT`

rm -r $config_root/$PREFIX/* 2>/dev/null

/opt/cms/bin/cfgen -o $config_root/ makeConfig $COLOCATION "$server_root/CMS/Plugin" || exit $?
env_name=adserver
if [ ! -d $config_root/$PREFIX/AdServer/opt/foros/server/etc/adserver/ ]; then
  env_name=adcontent
fi

out_dir=$config_root/$env_name/

rm -r $out_dir 2>/dev/null ;
cp -r $config_root/$PREFIX/AdServer/opt/foros/server/etc/* $config_root/ 2>/dev/null &&
{ test -e $config_root/$PREFIX/AdServer/u01/foros/server/var/ &&
  mkdir -p $workspace_root &&
  cp -r $config_root/$PREFIX/AdServer/u01/foros/server/var/* $workspace_root/ 2>/dev/null || true; } &&
cp $config_root/$PREFIX/AdServer/opt/foros/manager/etc/config.d/* $out_dir &&

rm -f $config_root/adserver.sh

echo "#!/bin/bash" >$config_root/adserver.sh
if [ "$ISP_ZONE_ENABLED" = "true" -o "$ISP_ZONE_ENABLED" = "1" ]
then
echo "echo \"Foros zone\"
/opt/foros/manager/bin/cmgr -C $out_dir -f server-${env_name}-foros \"\$@\"
echo \"ISP zone\"
/opt/foros/manager/bin/cmgr -C $out_dir -f server-${env_name}-isp \"\$@\"" >>$config_root/adserver.sh
else
echo "/opt/foros/manager/bin/cmgr -C $out_dir \"\$@\"" >>$config_root/adserver.sh
fi
chmod +x $config_root/adserver.sh

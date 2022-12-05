#!/bin/bash

function print_usage()
{
  echo "Usage: upload_plugin.sh [OPTIONS]"
  echo "Uploads AdServer plugin from svn to CMS by version."
  echo "  OPTIONS:"
  echo -e "  -h            Print this help and exit.\n"
  echo -e "  -v version    If defined, checks out plugin with \033[4mversion\033[0m version from svn, else uses version of working copy.\n"
  echo -e "  -c cert       Use client certificate \033[4mcert\033[0m for CMS authentication."
  echo -e "                By default script asks your CMS login and password.\n"
  echo -e "  -u            Updates all existing colocations with major version equal to new uploaded.\n"
  echo -e "  -d            If enabled, deploys RPMs for updated colocations to central repository. Makes sense if -u key is enabled.\n"
}

while getopts "v:c:uhd" option
do
    case $option in
      v)  VERSION=$OPTARG;;
      c)  CERT=$OPTARG;;
      u)  UPDATE="true";;
      d)  DEPLOY="true";;
      h)  print_usage && exit 0;;
      ?)  print_usage && exit 1;;
    esac
done

shift $((OPTIND - 1))

if [ "x$CERT" != "x" ]
then
  ls $CERT 2>/dev/null 1>/dev/null
  [ $? -ne 0 ] && echo -e "\033[31mERROR: $CERT: No such file or directory!\033[0m" && exit 2;
  auth="-E $CERT"
  auth_port="8182"
else
  echo -n "CMS login: "
  read login
  echo -n "CMS password for $login: "
  read -s password
  echo ""
  auth="-u $login:$password"
  auth_port="8181"
fi

working_dir=`dirname $0`
cms_root=`pwd`/$working_dir/..

max_minor_version=-1
TIMESTAMP=`date +%Y%m%d%H%M`

if [ "x$VERSION" != "x" ]
then
  ### Check out plugin from svn tag
  echo "Checking out plugin with $VERSION version from svn..."
  mkdir -p $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP
  svn co svn+ssh://svn.ocslab.com/home/svnroot/oix/server/tags/$VERSION/CMS/Plugin \
    $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP 1>/dev/null 2>/dev/null

  [ $? -ne 0 ] && echo -e "\033[31mERROR: svn: tag for AdServer $VERSION version doesn't exist!\033[0m" && \
  rm -rf $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP && exit 1
  plugin_root="$cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP"
  plugin_version=$VERSION
  echo -e "\033[32mDone.\033[0m"
else
  plugin_root="$cms_root/Plugin"
  ### get current plugin version
  plugin_version=`grep -e 'version=\".*\..*\..*\..*\"' $plugin_root/AdServerAppDescriptor.xml \
    | cut -d'"' -f2`
fi

plugin_minor_version=`echo $plugin_version | cut -d'.' -f4`
plugin_major_version=`echo $plugin_version | grep -o -E '[1-9]+.[0-9]+.[0-9]+'`

### archiving plugin
echo "Archiving plugin..."
cd $plugin_root && zip -qr $plugin_root/AdServer-$plugin_version.zip * -x *svn*
if [ $? -ne 0 ]
then
  echo -e "\033[31mERROR: failed to archive plugin!\033[0m" 
  rm -rf $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP
  exit 1
fi
echo -e "\033[32mDone.\033[0m"

### uploading plugin
echo "Uploading plugin to CMS..."
upload=`curl -k $auth -F file=@$plugin_root/AdServer-$plugin_version.zip https://cms-test.ocslab.com:$auth_port/services/uploadPlugin \
| grep "<status>" | cut -d '>' -f 2 | cut -d '<' -f 1`
[ "$upload" != "SUCCEEDED" ] && echo -e "\033[31mERROR: failed to upload plugin!\033[0m" && \
  rm -rf $plugin_root/AdServer-$plugin_version.zip $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP && exit 1
echo -e "\033[32mDone.\033[0m"

if [ "$UPDATE" ]
then
  curl -k $auth https://cms-test.ocslab.com:$auth_port/services/listColocations \
  | sed 's/.*\<colocation name="\([^"]*\)">/COLO="\1"/g' \
  | sed 's/.*<application name="[^"]*" plugin="\([^"]*\)"\/>/PLUGIN="\1"/g' \
  | sed 's/.*<\/colocation>/END_COLO=1/g' \
  | sed '/colocations/d' | sed '/xml/d' | sed 's/\r//g' |
  while read VAR; do
    eval $VAR ;
    if [ "$END_COLO" ]
    then
      if [ $max_minor_version -gt -1 -a $max_minor_version -lt $plugin_minor_version ]
      then
        colo_version=$plugin_major_version.$max_minor_version
        echo "Updating plugin for $COLO colo from $colo_version to $plugin_version... "
        update=`curl -k $auth --get -d "colo=$COLO&plugin=AdServer-$colo_version&newVersion=$plugin_version" \
        https://cms-test.ocslab.com:$auth_port/services/updateVersion | grep "<status>" | cut -d '>' -f 2 | cut -d '<' -f 1`
        if [ "$update" == "SUCCEEDED" ]
        then
          echo -e "\033[32mDone.\033[0m"
          if [ "$DEPLOY" ]
          then
            echo "Building and deploing RPM for $COLO colo with $plugin_version version of application..."
            RPM=`curl -k $auth --get -d "colo=$COLO&plugin=AdServer-$plugin_version" \
              https://cms-test.ocslab.com:$auth_port/services/buildDeployRpms | grep "RPMs:" | cut -d' ' -f2 | cut -d',' -f1`
            if [ "$RPM" ]
            then
              echo -e "\033[33m$RPM successfully deployed on central repository.\033[0m"
            else
              echo -e "\033[31mERROR: failed to build and deploy config for $COLO colocation!\033[0m"
            fi
          fi
        else
          echo -e "\033[31mERROR: failed to update plugin for $COLO colo from $colo_version to $plugin_version!\033[0m"
        fi
      fi
      unset COLO
      unset PLUGIN
      unset END_COLO
      max_minor_version=-1
    else
      minor_version=`echo $PLUGIN | grep "AdServer-$plugin_major_version.[0-9]" | cut -d'.' -f4`
      [ "$minor_version" ] && [ $minor_version -gt $max_minor_version ] && max_minor_version=$minor_version
    fi
  done
fi

rm -rf $plugin_root/AdServer-$plugin_version.zip $cms_root/temp-AdServer-plugin-$VERSION.$TIMESTAMP

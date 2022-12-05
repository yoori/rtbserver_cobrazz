#!/bin/bash

function print_usage()
{
  echo "Util for automatic increasing version of AdServer plugin and configs."
  echo "Uasge:"
  echo -e "increase_plugin_version.sh [OPTIONS]\n"
  echo "OPTIONS:"
  echo -e "    -h                   Print this help and exit.\n"
  echo -e "    -v version           Change AdServer plugin and configs version to \033[4mversion\033[0m."
  echo -e "                         If not defined, increase current minor version by one.\n"
  echo -e "    -b branch_version    Check out configs and plugin from svn branch \033[4mbranch_version\033[0m,"
  echo -e "                         increase plugin and configs version and commit changes to svn.\n"

}

while getopts "hv:b:" option
do
  case $option in
  h) print_usage && exit 0;;
  v) VERSION=$OPTARG;;
  b) BRANCH_VER=$OPTARG;;
  ?) print_usage && exit 1;;
  esac
done

TIMESTAMP=`date +%Y%m%d%H%M`

working_dir=`dirname $0`
cms_root=`pwd`/$working_dir/..

if [ "x$BRANCH_VER" != "x" ]
then
  if [ "x$VERSION" != "x" ]
  then
    new_major_version=`echo $VERSION | grep -o -E '[1-9]+.[0-9]+.[0-9]+'`
    [ "x$BRANCH_VER" != "x" -a $new_major_version != $BRANCH_VER ] && \
      echo -e "\033[31mERROR: new major version is not equal to branch major version!\033[0m" && exit 1;
  fi
  ### Check out plugin from svn branch
  mkdir -p $cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP
  echo -n "Checking out plugin with $BRANCH_VER version from svn..."
  svn co svn+ssh://svn.ocslab.com/home/svnroot/foros/server/branches/$BRANCH_VER/CMS/ \
    $cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP 2>/dev/null 1>/dev/null

  if [ $? -ne 0 ]
  then
    echo -e "\n\033[31mERROR: svn: branch for AdServer $BRANCH_VER version doesn't exist!\033[0m"
    rm -rf $cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP
    exit 1
  else
    working_cms_root="$cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP"
    echo -e "\033[32mDone.\033[0m"
  fi
else
    working_cms_root=$cms_root
fi

plugin_root="$working_cms_root/Plugin"
configs_root="$working_cms_root/tests/Configs/QA"

if [ "x$VERSION" != "x" ]
then
  new_version=$VERSION
else
  cur_version=`grep -E 'version=\"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\"' $plugin_root/AdServerAppDescriptor.xml | cut -d'"' -f2`
  cur_minor_version=`echo $cur_version | cut -d'.' -f4`
  #increase current minor version
  cur_minor_version=$(($cur_minor_version+1));
  new_version=`echo $cur_version | sed -r "s/([1-9]+.[0-9]+.[0-9]+.)[0-9]+/\1$cur_minor_version/"`
fi

CONFIGS=`ls $configs_root`

# increase plugin version
echo -n "Increasing plugin version to $new_version..."
sed "s/version=\".*\..*\..*\..*\"/version=\"$new_version\"/" -i $plugin_root/AdServerAppDescriptor.xml
echo -e "\033[32mDone.\033[0m"

# increase configs version
for config in $CONFIGS; do
  [ "$config" != "transformer" ] && \
  echo -n "Increasing version of $config colo to $new_version..." && \
  sed "s/descriptor=\"AdServer-.*\..*\..*\..*\"/descriptor=\"AdServer-$new_version\"/" -i $configs_root/$config.xml && \
  echo -e "\033[32mDone.\033[0m"
done

cd $cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP 2>/dev/null && svn commit --message "Up plugin version => $new_version"

rm -rf $cms_root/temp-AdServer-$BRANCH_VER.$TIMESTAMP

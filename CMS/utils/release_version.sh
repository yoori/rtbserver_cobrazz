#!/bin/sh

USAGE="Usage: `basename $0` -v version -s source_version -n no_task -j no_jira -c install_colo -u no_unixcommons -p no_plugin";

source_version=trunk
no_task=no
no_jira=no
no_unixcommons=no
no_plugin=no
user=andrey.gusev
colo=test
while getopts junphv:s:c: OPT; do
  case "$OPT" in 
    v) version=$OPTARG
      ;;
    s) source_version=branches/$OPTARG
      ;;
    n) no_task=yes
      ;;
    j) no_jira=yes
      ;;
    p) no_plugin=yes
      ;;
    u) no_unixcommons=yes
      ;;
    c) colo=$OPTARG
      ;;
    h) echo $USAGE
      exit 0
      ;;
  esac
done

if [ -z $version ]; then
  echo $USAGE
  exit 0
fi

#validate version
version=`echo $version | awk '{if ($0 ~ /[1-9]\.[0-9]+\.[0-9]+\.[0-9]+/) print ; }'`
if [ -z $version ]; then
  echo wrong version format
  exit 1
fi

#make branch if it need
repository=svn+ssh://svn/home/svnroot/foros/server/
minor=`echo $version | awk 'BEGIN { FS = "."}; {print $4 }'`
major=${version%.$minor}
branch_minor=`echo $major | awk 'BEGIN { FS = "."}; {print $3 }'`
branch=${repository}branches/$major
if [ "$minor" = "0" ]; then
  svn ls $branch >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "branch $branch already exists"
  else
    source_branch=${repository}${source_version}
    svn ls ${source_branch} >/dev/null 2>&1
    if [ $? -ne 0 ]; then
      echo "branch $source_branch doesn't exist"
      exit 1;
    fi
    echo create branch $branch
    svn copy ${source_branch} $branch -m "create branch $major from $source_version"
    if [ $? -ne 0 ]; then
      echo "Cannot create branch $branch from $source_branch"
      exit 1;
    fi
  fi
fi

local_rep_path=$myproj/
if [ ! -d $local_rep_path ]; then
  local_rep_path=~/projects/
  if [ ! -d $local_rep_path ]; then
    local_rep_path=~/prj/
  fi
  if [ ! -d $local_rep_path ]; then
    echo "can't find project directory"
  fi
fi
local_branch_place=$local_rep_path$major 
if [ ! -d $local_branch_place ]; then
  mkdir $local_branch_place
fi
cd $local_branch_place
if [ ! -d server ]; then 
  svn co $branch server
fi
cd server
svn update
if [ $no_unixcommons = "no" ]; then
  command="svn ls svn+ssh://svn/home/svnroot/unixcommons/tags/ | grep ^$major | sed 's/$major.\([0-9]*\)\//\1/' | sort -n | tail -n 1"
  un_minor=`eval "$command"`
  if [ -z "$un_minor" ]; then
    echo "Cannot detect unixcommons version"
    exit 1;
  fi
  old_un_version=`cat unixcommons.version`
  un_version="$major.$un_minor"
  echo $un_version >unixcommons.version
  echo "up unixcommons version from $old_un_version to $un_version" unixcommons.version
  svn commit -m "up unixcommons version from $old_un_version to $un_version" unixcommons.version
fi

if [ $no_plugin = "no" ]; then
  old_version=`grep -E version=\"[1-9]\.[0-9]+\.[0-9]+\.[0-9]+\" CMS/Plugin/AdServerAppDescriptor.xml | awk 'BEGIN { FS="\"" }; {print $2}'`
  old_envdev_version=`grep -E VERSION=\'[1-9]\.[0-9]+\.[0-9]+\.[0-9]+\' CMS/tests/Configs/devel/envdev.sh | awk 'BEGIN { FS="'\''" }; {print $2}'`
  echo "find old version. it is $old_version"
  echo "old envdev version. it is $old_envdev_version"
  sed -i s/version=\"$old_version\"/version=\"$version\"/ CMS/Plugin/AdServerAppDescriptor.xml
  sed -i s/VERSION=\'$old_envdev_version\'/VERSION=\'$version\'/ CMS/tests/Configs/devel/envdev.sh
  echo "change plugin version. from $old_version to $version"
  sed -i s/$old_version/$version/ `find CMS/tests/Configs -name "*xml"` 
  echo "up versions in configuration files from $old_version to $version"
  svn update
  svn commit -m "up plugin version to $version"
  if [ $? -ne 0 ]; then
    echo "can not commit plugin"
    exit 1
  fi
  cd CMS/utils
  ./build_tests_rpms.sh -v $major
fi

if [ $? -eq 0 ]; then
  tag=${repository}tags/$version
  svn ls $tag >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "tag $tag already exists, will remove it"
    svn remove $tag -m "remove tag $tag" || exit 1;
  fi
  svn copy $branch $tag -m "make tag $version"
  if [ $? -ne 0 ]; then
    echo "can create tag $tag"
    exit 1
  fi
  if [ $no_jira = "no" ]; then
    jira --login-name=$user addVersion ADSC $major.$((minor+1)) $version 
    jira --login-name=$user releaseVersion ADSC $version
    if [ $minor -ne 0 ] && [ $no_task = "no" ]; then
      assign='assignee=andrey.azeev'
      if [ "$colo" = "test" ];
      then
        if [ $branch_minor -eq 0 ]; then 
          colo="test colocations"
        else
          colo="test2 colocations"
        fi
      fi
      task_text="build foros-server $version and upgrade $colo" 
      jira --login-name=$user createIssue ENVDEV Task summary="${task_text}" components=Deployment priority=Major description="${task_text}" ${assign}
    fi
  fi
fi
exit 0
#echo $old_vrsion


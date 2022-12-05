#!/bin/bash

function print_usage()
{
  echo "Usage: build_tests_rpms.sh [OPTIONS]"
  echo "Builds RPMs for test colocations configs and deploys it in central repository."
  echo "OPTIONS:"
  echo -e "  -h, --help               Print this help and exit.\n"
  echo -e "  -v, --version \033[4mversion\033[0m    Checks out plugin and configs with \033[4mversion\033[0m version from svn brunch."
  echo -e "                           If \033[4mversion\033[0m equal to 'wcopy' uses version of working copy.\n"
  echo -e "  -c, --cert \033[4mcert\033[0m          Use client certificate \033[4mcert\033[0m for CMS authentication."
  echo -e "                           By default script asks your CMS login and password.\n"
  echo -e "  -l, --login \033[4mlogin\033[0m        Use this \033[4mlogin\033[0m for CMS authentication." 
  echo -e "  -r, --remove \033[4mrem\033[0m         If \033[4mrem\033[0m equal to 1 script removes all colo configs having magor version"
  echo -e "                           same as new colo config version and minor vaersion less than new colo config minor vesion "
  echo -e "                           after deploing new colo config. By default \033[4mrem\033[0m equal to 1."
}

REMOVE_OLD=1;
OPT="";
while [ "$#" -gt "0" ]
do
  case $1 in
  -v | --version)
    OPT=$1;
    shift
    if [ "x$1" == "x" -o "${1:0:1}" == "-" ]
    then
      echo -e "\033[31mERROR: build_tests_rpms.sh: option requires an argument - '$OPT'.\033[0m"
      print_usage
      exit 1
    fi
    branch_version=$1
    ;;
  -l | --login)
    OPT=$1;
    shift
    if [ "x$1" == "x" -o "${1:0:1}" == "-" ]
    then
      echo -e "\033[31mERROR: build_tests_rpms.sh: option requires an argument - '$OPT'.\033[0m"
      print_usage
      exit 1
    fi
    login=$1;;
  -c | --cert)
    OPT=$1;
    shift
    if [ "x$1" == "x" -o "${1:0:1}" == "-" ]
    then
      echo -e "\033[31mERROR: build_tests_rpms.sh: option requires an argument - '$OPT'.\033[0m"
      print_usage
      exit 1
    fi
    CERT=$1;;
  -r | --remove)
    OPT=$1;
    shift
    if [ "x$1" == "x" -o "${1:0:1}" == "-" ]
    then
      echo -e "\033[31mERROR: build_tests_rpms.sh: option requires an argument - '$OPT'.\033[0m"
      print_usage
      exit 1
    fi
    if [ $1 != 1 -a $1 != 0 ]
    then
      echo -e "\033[31mERROR: build_tests_rpms.sh: '$OPT' option's argument can be only '1' or '0'.\033[0m"
      print_usage
      exit 1
    fi
    REMOVE_OLD=$1;;
  -h | --help)
    print_usage
    exit 0;;
  *)
    echo -e "\033[31mERROR: build_tests_rpms.sh: illegal option -- $1\033[0m"
    print_usage
    exit 1;;
  esac
  shift
done


if [ "x$branch_version" == "x" ]
then
  echo -e "\033[31mERROR: build_tests_rpms.sh: option is required - version!\033[0m"
  print_usage && exit 1;
else
  minor=${branch_version##*.}
fi

if [ "x$CERT" != "x" ]
then
  ls $CERT 2>/dev/null 1>/dev/null
  [ $? -ne 0 ] && echo -e "\033[31mERROR: build_tests_rpms.sh: $CERT: No such file or directory!\033[0m" && exit 2;
  auth="-E $CERT"
  auth_port="8182"
else
  if [ "x$login" = "x" ]
  then
    echo -n "CMS login: "
    read login
  fi
  echo -n "CMS password for $login: "
  read -s password
  echo ""
  auth="-u $login:$password"
  auth_port="8181"
fi

working_dir=`dirname $0`
cms_root=`pwd`/$working_dir/..

if ((minor == 0)); then
  TEST_CONFIGS="";
else
  TEST_CONFIGS="";
fi

TIMESTAMP=`date +%Y%m%d%H%M`

if [ "$branch_version" == "wcopy" ]
then
  plugin_root="$cms_root/Plugin"
  configs_root="$cms_root/tests/Configs/QA"
else
  ### Check out plugin from svn branch
  mkdir -p $cms_root/temp-AdServer-$branch_version.$TIMESTAMP
  echo "Checking out plugin with $branch_version version from svn..."
  svn co svn+ssh://svn.ocslab.com/home/svnroot/oix/server/branches/$branch_version/CMS/ \
    $cms_root/temp-AdServer-$branch_version.$TIMESTAMP 2>/dev/null 1>/dev/null

  if [ $? -ne 0 ]
  then
    echo -e "\033[31mERROR: build_tests_rpms.sh: svn: branch for AdServer $branch_version version doesn't exist!\033[0m"
    rm -rf $cms_root/temp-AdServer-$branch_version.$TIMESTAMP
    exit 1
  else
    plugin_root="$cms_root/temp-AdServer-$branch_version.$TIMESTAMP/Plugin"
    configs_root="$cms_root/temp-AdServer-$branch_version.$TIMESTAMP/tests/Configs/QA"
    echo -e "\033[32mDone.\033[0m"
  fi
fi
### get current plugin version
plugin_version=`grep -e 'version=\".*\..*\..*\..*\"' $plugin_root/AdServerAppDescriptor.xml \
  | cut -d'"' -f2`

### archiving plugin
echo "Archiving plugin..."
cd $plugin_root && zip -qr $plugin_root/AdServer-$plugin_version.zip * -x *svn*
if [ $? -ne 0 ]
then
  echo -e "\033[31mERROR: build_tests_rpms.sh: failed to archive plugin!\033[0m" 
  rm -rf $cms_root/temp-AdServer-$branch_version.$TIMESTAMP
  exit 1
fi
echo -e "\033[32mDone.\033[0m"

### uploading plugin
echo "Uploading plugin to CMS..."
upload=`curl -k $auth -F file=@$plugin_root/AdServer-$plugin_version.zip https://cms-test.ocslab.com:$auth_port/services/uploadPlugin \
| grep "<status>" | cut -d '>' -f 2 | cut -d '<' -f 1`
[ "$upload" != "SUCCEEDED" ] && echo -e "\033[31mERROR: build_tests_rpms.sh: failed to upload plugin! : $upload\033[0m" && \
rm -rf $plugin_root/AdServer-$plugin_version.zip $cms_root/temp-AdServer-$branch_version.$TIMESTAMP && exit 1
echo -e "\033[32mDone.\033[0m"


### importing tests colocations
for config in $TEST_CONFIGS; do
  colo_name=`cat $configs_root/$config.xml | grep -m1 'name=' | sed -r 's/^.*\sname\s*=\s*"([^"]*)".*$/\1/'`
  config_version=`grep -e 'application descriptor=\"AdServer-.*\..*\..*\..*\"' $configs_root/$config.xml | cut -d'"' -f2 | cut -d'-' -f2`
  conf_minor_ver=`echo $config_version | cut -d'.' -f'4'`
  if [ "$config_version" == "$plugin_version" ]; then
  echo "Importing $colo_name $config.xml..."
  import=`curl -k $auth -F coloXml=@$configs_root/$config.xml \
    https://cms-test.ocslab.com:$auth_port/services/importColo | grep "<status>" | cut -d '>' -f 2 | cut -d '<' -f 1`
    if [ "$import" == "SUCCEEDED" ]
    then
        echo -e "\033[32mDone.\033[0m"
        ### building and deploing config
        echo "Building and deploing RPM for $colo_name colocation..."

        RPM=`curl -k $auth --get -d "colo=$colo_name&plugin=AdServer-$plugin_version" \
          https://cms-test.ocslab.com:$auth_port/services/buildDeployRpms | grep "RPMs:" | cut -d' ' -f2 | cut -d',' -f1`
        if [ "$RPM" ]
        then
          echo -e "\033[33m$RPM successfully deployed on central repository.\033[0m"

          if [ $REMOVE_OLD == 1 ]
          then
            maj_version=`echo $config_version | grep -o -E '[1-9]+.[0-9]+.[0-9]+'`
            to_remove=`curl -k $auth --request GET https://cms-test.ocslab.com:$auth_port/services/colocations/$colo_name/applications/AdServer \
            | grep AdServer-$maj_version | cut -d'"' -f'4' | cut -d'-' -f'2' `
            for conf_to_remove in $to_remove
            do
              conf_to_rem_minor_ver=`echo $conf_to_remove | cut -d'.' -f'4'`
              if [ $conf_to_rem_minor_ver -lt $conf_minor_ver ]
              then
                del_index=0;
                curl -k $auth --request DELETE \
                https://cms-test.ocslab.com:$auth_port/services/colocations/$colo_name/applications/AdServer/$conf_to_remove/ \
                | grep -E '(status|error|info)' | cut -d'>' -f'2' | cut -d'<' -f'1' | \
                {
                while read var;
                do
                  del_status[$del_index]=$var;
                  del_index=1;
                done
                if [ "${del_status[0]}" == "SUCCEEDED" ]
                then
                  echo -e "\033[32mApplication AdServer-$conf_to_remove was successfully deleted from $colo_name colocation.\033[0m"
                else
                  echo -e "\033[31mDeleting AdServer-$conf_to_remove application from $colo_name colocation failed : ${del_status[1]}\033[0m"
                fi
                }
              fi
            done
          fi
        else
          echo -e "\033[31mERROR: build_tests_rpms.sh: failed to build and deploy config for $colo_name colocation!\033[0m"
        fi
    else
        echo -e "\033[31mERROR: build_tests_rpms.sh: failed to import $config.xml for $colo_name colocation!\033[0m"
    fi
  else
    echo -e "\033[31mERROR: build_tests_rpms.sh: Application descriptor's version for $colo_name colocation is not equal to plugin version!\033[0m"
  fi
done
rm -rf $plugin_root/AdServer-$plugin_version.zip $cms_root/temp-AdServer-$branch_version.$TIMESTAMP

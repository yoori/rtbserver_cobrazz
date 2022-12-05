#!/bin/bash

trap ctrl_c INT

function ctrl_c()
{
  exit 1;
}

rm -r build/* 2>/dev/null

RESULT_DIR=Result
CHECKED_RESULT_DIR=./CheckedResult

PLUGIN_ROOT=../Plugin
WORK_PLUGIN_ROOT=./Work-Plugin
LOG_ROOT=log
TEST_CONFIGS_ROOT=./Configs
EXPAND_OUTPUT=0
COMMAND="build"
CMS_BUILD_COMMAND="makeConfig"

function configure_test ()
{
  CONFIGPATH=$1
  CONFIG=$(basename $CONFIGPATH | sed -e 's/\.xml$//')
  if [ "$COMMAND" = "build" ]
  then
    echo "$CONFIG: to configure (plugin=$WORK_PLUGIN_ROOT)"
    /opt/cms/bin/cfgen makeConfig $CONFIGPATH $WORK_PLUGIN_ROOT 1>$LOG_ROOT/$CONFIG.log 2>$LOG_ROOT/$CONFIG.err
  else
    echo "$CONFIG: to build RPM (plugin=$WORK_PLUGIN_ROOT)"
    rm -r $RESULT_DIR/$CONFIG-build/ 2>/dev/null
    /opt/cms/bin/cfgen -o $RESULT_DIR/$CONFIG-build/ buildRpm $CONFIGPATH $WORK_PLUGIN_ROOT 1>$LOG_ROOT/$CONFIG.log 2>$LOG_ROOT/$CONFIG.err
  fi

  if [ $? -eq 0 ]
  then
    echo "$CONFIG: done ($RESULT_DIR/$CONFIG-build)"
  else
    echo -e "$CONFIG: \033[31mcontains errors\033[0m"
    if [ $EXPAND_OUTPUT -ne 0 ]
    then
      cat $LOG_ROOT/$CONFIG.err | sed 's/^\(.*\)$/  \1/'
    fi
    return 1
  fi

  if [ "$COMMAND" = "build" ]
  then
    ADSERVER_DIR=`ls build/build_null/colo_null/ | grep AdServer-`
    if [ "$ADSERVER_DIR" = "" ]
    then
      echo -e "$CONFIG: \033[31mcontains errors\033[0m: can't find build/build_null/colo_null/AdServer-* directory"
      return 1
    fi

    rm -r $RESULT_DIR/$CONFIG-build 2>/dev/null
    mv build/build_null/colo_null/$ADSERVER_DIR/AdServer/ $RESULT_DIR/$CONFIG-build

    if [ $EXPAND_OUTPUT -gt 1 ] ; then
      find $RESULT_DIR/$CONFIG-build | sed "s/^$RESULT_DIR\/$CONFIG-build\(.*\)\$/  \1/" | grep -v -E '^  $'
    fi

    diff --exclude-from=diff.exclude -r $CHECKED_RESULT_DIR/$CONFIG-build $RESULT_DIR/$CONFIG-build >$LOG_ROOT/$CONFIG.diff 2>&1

    if [ -s $LOG_ROOT/$CONFIG.diff ]
    then
      echo -e "$CONFIG: \033[33mcontains difference from reviewed\033[0m"
      if [ $EXPAND_OUTPUT -ne 0 ]
      then
        cat $LOG_ROOT/$CONFIG.diff  | sed 's/^\(.*\)$/  \1/'
      fi
    fi
  fi

  return 0
}

function reviewed ()
{
  CONFIG=$(basename $1 | sed -e 's/\.xml$//')
  # remove files from svn
  if [ -e $CHECKED_RESULT_DIR/$CONFIG-build ]
  then
    pushd ./ 1>/dev/null
    cd $CHECKED_RESULT_DIR/$CONFIG-build
    SVN_FILE_LIST=`find . ! \( -type d -and -name .svn -prune \) | sed "s/^[.]\/\(.*\)\$/\1/" | grep -v -E '^[.]?$'`
    popd 1>/dev/null

    for FILE in $SVN_FILE_LIST ; do
      if [ ! -e $RESULT_DIR/$CONFIG-build/$FILE ]
      then
        svn rm --force $CHECKED_RESULT_DIR/$CONFIG-build/$FILE
        if [ $? -ne 0 ] ; then
          echo "can't 'svn rm $CHECKED_RESULT_DIR/$CONFIG-build/$FILE'"
          exit 1
        fi
        echo "svn rm $CHECKED_RESULT_DIR/$CONFIG-build/$FILE"
      fi
    done
  fi

  cp -r $RESULT_DIR/$CONFIG-build $CHECKED_RESULT_DIR/

  ADDLIST=`svn status $CHECKED_RESULT_DIR/$CONFIG-build | grep -E '^[?]   ' | sed 's/^?[ ]*\([^ ].*\)\$/\1/'`

  for FILE in $ADDLIST ; do
    svn add $FILE
    if [ $? -ne 0 ] ; then
      echo "can't do: svn add $FILE"
      exit 1
    fi
    echo "svn add $FILE"
  done

  echo "Recheck and commit '$CHECKED_RESULT_DIR/$CONFIG-build'"
}

function config_diff ()
{
  CONFIG=$(basename $1 | sed -e 's/\.xml$//')
  if [ -s $LOG_ROOT/$CONFIG.diff ] ; then
    echo "$CONFIG: diff"
    cat $LOG_ROOT/$CONFIG.diff
  else
    echo "$CONFIG: no diff"
  fi
}

mkdir -p $RESULT_DIR
mkdir -p $LOG_ROOT

ALL_CONFIG_PATHS="NB Unsorted UnitTests QA"

COLO_NAME=""

while [ $# != 0 ]; do
  ARG="$1"
  case "$ARG" in
    -v) EXPAND_OUTPUT=1
     ;;
    -vv) EXPAND_OUTPUT=2
     ;;
    --build) COMMAND=build
     ;;
    --build-rpm) COMMAND="build-rpm"
     ;;
    --reviewed) COMMAND=reviewed
     ;;
    --diff) COMMAND=diff
     ;;
    *) COLO_NAME="$COLO_NAME $ARG"
     ;;
  esac
  shift
done

SEARCH_COLO_NAME="*.xml"

if [ ! -z "$COLO_NAME" ]
then
  SEARCH_COLO_NAME="$COLO_NAME.xml"
fi

EXIT_CODE=0

# prepare Plugin without .svn folders
if [ "$COMMAND" = "build" ] || [ "$COMMAND" = "build-rpm" ]
then
  rm -rf $WORK_PLUGIN_ROOT
  rsync -r --exclude=*.svn $PLUGIN_ROOT/ $WORK_PLUGIN_ROOT
fi


for COLOPATH in $ALL_CONFIG_PATHS
do
  for COLO in $(find $TEST_CONFIGS_ROOT/$COLOPATH -name $SEARCH_COLO_NAME -type f)
  do
    case "$COMMAND" in
      build|build-rpm) configure_test $COLO 
      let "EXIT_CODE|=$?"
      ;;
      reviewed) reviewed $COLO 
      ;;
      diff) config_diff $COLO
      ;;
    esac
  done
done

exit $EXIT_CODE

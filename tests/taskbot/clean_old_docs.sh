#!/bin/bash

function print_usage()
{
  echo "clean_old_docs.sh OPTIONS"
  echo "Removes autotest commons docs, which are older than indicated number of days."
  echo "OPTIONS:"
  echo -e "    -h, --help       Print this message and exit"
  echo -e "    -d, --days \033[4mn\033[0m     Required. Number of days. If some doc's mtime more than \033[4mn\033[0m, it will be deleted."
  echo -e "    --docs-dir \033[4mdir\033[0m   Folder containing docs. Script uses '/home/taskbot/public_html/docs/AutoTest/Commons' by default.\n"  
}

function check_option()
{
  if [ "x$2" == "x" -o "${2:0:1}" == "-" ]
  then
    echo -e "\033[31mERROR: clean_old_docs.sh: option requires an argument - '$1'.\033[0m"
    print_usage
    exit 1
  fi
}

DOCS_DIR='/home/taskbot/public_html/docs/AutoTest/Commons';

while [ "$#" -gt "0" ]
do
  case $1 in
  -d | --days)
  OPT=$1;
  shift
  check_option $OPT $1;
  days=`echo "$1" | egrep "^[0-9]+$"`;
  if [ ! "$days" ]
  then
    echo -e "\033[31mERROR: clean_old_docs.sh: option requires numeric argument - '$OPT'.\033[0m"
    print_usage
    exit 1
  fi
  ;;
  --docs-dir)
  OPT=$1;
  shift
  check_option $OPT $1;
  DOCS_DIR=$1;
  ;;
  -h | --help)
    print_usage
    exit 0;;
  *)
    echo -e "\033[31mERROR: clean_old_docs.sh: illegal option -- $1\033[0m"
    print_usage
    exit 1;;
  esac
  shift
done

if [ ! "$days" ]
then
  echo -e "\033[31mERROR: clean_old_docs.sh: option is required - 'd'.\033[0m"
  print_usage
  exit 1
fi

to_remove=`find $DOCS_DIR -regextype posix-extended -regex "$DOCS_DIR/[A-Za-z0-9]{40}" -type d -mtime "+$days"`
if [ "$to_remove" ]
then
  echo -e "Removing obsolete docs from taskbot:\n"
  for doc in $to_remove; do
    commit_name=`basename $doc`
    echo -ne "  removing doc of $commit_name commit... "
    rm_result=`rm -r $doc 2>&1`
    if [ $? -ne 0 ]
    then
      echo "failed"
      echo -e "  Reason: $rm_result.\n"
    else
      echo -e "done.\n"
    fi
  done
fi

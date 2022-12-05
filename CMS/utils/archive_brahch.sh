#!/bin/bash

user=$1
start_version=$2
minor=${start_version##*.}
major=${start_version%*.*}
echo $major
echo $minor
while [ true ]; do
have=`jira --login-name=$user getVersion ADSC $major.$minor`
if [ -z "$have" ]; then
  echo "There isn't version $major.$minor"
  exit 0
fi
echo "Archive $major.$minor"
jira --login-name=$user archiveVersion ADSC $major.$minor
#sleep 1
let minor=$minor+1
done

#!/bin/bash

VERSION=$1
UNIXCOMMONSDIR=$2

# script require 'sudo rpm' for install RPM packages
# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build epel-rpm-macros || \
  { echo "can't install RPM build packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`
DSP_SPEC_FILE=`rpm -E %_specdir`/server.spec

rm -f "$DSP_SPEC_FILE"

# download sources from git tag, pack and pass to sources dir
echo "to download sources from dev branch"
RPM_SOURCES_DIR=`rpm -E %_sourcedir`


# create tag and push it to repo
#git tag v"$VERSION"
#git push origin --tags
# get spec from tag
cp RPM/SPECS/server.spec "$DSP_SPEC_FILE"

#git archive --format tar.gz --output "$RPM_SOURCES_DIR/stream-dsp-$VERSION.tar.gz" dev
rm -rf "$RES_TMP/foros-server-$VERSION"
mkdir "$RES_TMP/foros-server-$VERSION"
rsync -av --exclude '/build' --exclude ".git" --exclude ".svn" ./ "$RES_TMP/foros-server-$VERSION/server"
rsync -av --exclude '/build' --exclude ".git" --exclude ".svn" "$UNIXCOMMONSDIR" "$RES_TMP/foros-server-$VERSION/unixcommons"
rm -f "$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz"
pushd "$RES_TMP"
#echo 'tar -czvf '"$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz"' '"$RES_TMP/foros-server-$VERSION"
tar -czvf "$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz" "foros-server-$VERSION"
echo "Created archive: $RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz"
popd

#tar -czvf "$RPM_SOURCES_DIR/foros-server-$VERSION.tar.gz" `ls -1 -f | grep -v build | grep -v -E '^[.]'` --exclude ".git" --exclude ".svn"
#tar -czvf  "$RPM_SOURCES_DIR/stream-dsp-$VERSION.tar.gz" *  --exclude ".git" --exclude ".svn" --exclude "./build"

#popd
#popd

cat "$DSP_SPEC_FILE" | sed 's/%{version}/1.0.0.0/' | sed 's/%{__product}/server/' | sed 's/%{__server_type}/central/' >s.spec
yum-builddep -y ./s.spec || { echo "can't install build requirements" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "__product server" --define "__server_type central" --define "version $VERSION" "$DSP_SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp $BIN_RPM_FOLDER/foros-server*.rpm $RES_RPMS/


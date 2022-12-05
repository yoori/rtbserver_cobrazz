#!/bin/bash

# this is template for build rpm by content:
#   replace NAME
#   replace TTT to folder
NAME=TTT
VERSION=1.0.0
RELEASE=ssv4

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
mkdir -vp $(rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/noarch')
SPEC_FILE=$(rpm -E %_specdir)/"$NAME.spec"

rm -f "$SPEC_FILE"

# download sources from git tag, pack and pass to sources dir
#echo "to download sources from dev branch"
RPM_SOURCES_DIR=`rpm -E %_sourcedir`

#cat << "EOF" | sed "s/#NAME#/$NAME/g" > $SPEC_FILE

cat << "EOF" > $SPEC_FILE
Name:    %{name}
Version: %{version}
Release: %{_release}%{?dist}
Summary: Unknown
Group:   Development/Libraries
License: GPL
Source0: %{name}-%{version}-%{_release}.tar.gz
BuildArch: noarch

%define __install_dir /

BuildRoot: %{_tmppath}/%{name}-%{version}-%{_release}-buildroot

%description

%prep
%setup -q -n %{name}-%{version}-%{_release}


%build


%install
%__rm -rf %buildroot
%__mkdir_p %{buildroot}%{__install_dir}
cp -r * %{buildroot}%{__install_dir}/


%clean
rm -rf %{buildroot}


%files -n %{name}
%defattr(-,root,root)
/*

EOF

rm -rf "$RES_TMP/$NAME-$VERSION-$RELEASE"
mkdir "$RES_TMP/$NAME-$VERSION-$RELEASE"

# push sources for pack
rsync -av --exclude '/build' --exclude ".git" --exclude ".svn" ./TTT/ "$RES_TMP/$NAME-$VERSION-$RELEASE/"

rm -f "$RPM_SOURCES_DIR/$NAME-$VERSION-$RELEASE.tar.gz"

pushd "$RES_TMP"
tar -czvf "$RPM_SOURCES_DIR/$NAME-$VERSION-$RELEASE.tar.gz" "$NAME-$VERSION-$RELEASE"
popd

echo ">>>>> TO BUILD"

rpmbuild --force -ba \
  --define "name $NAME" \
  --define "version $VERSION" \
  --define "_release $RELEASE" \
  --define "_os_release noarch" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp $BIN_RPM_FOLDER/$NAME*.rpm $RES_RPMS/

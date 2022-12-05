#!/bin/bash

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build libtool \
  epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
RPM_SOURCES_DIR=$(rpm -E '%_sourcedir')

# create archive
VERSION=3.7.0.23

tar -czvf "$RPM_SOURCES_DIR/foros-pagesense-programmatic-$VERSION.tar.gz" foros-pagesense-programmatic/

echo "Created $RPM_SOURCES_DIR/foros-pagesense-programmatic-$VERSION.tar.gz"

# download librdkafka source RPM
PAGESENSE_SPEC_FILE=`rpm -E %_specdir`/pagesense.spec

cat << 'EOF' > $PAGESENSE_SPEC_FILE
Name:    foros-pagesense-programmatic
Version: %{version}
Release: ssv4%{?dist}
Group:   Applications/Publishing
License: Commercial
Summary: x
Source0: foros-pagesense-programmatic-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description

%prep
%setup -q -c foros-pagesense-programmatic-%{version}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/opt/foros/server/etc/www/PageSense
pwd
ls -l
cp -r foros-pagesense-programmatic/PageSense %{buildroot}/opt/foros/server/etc/www/

%clean
rm -rf %{buildroot}

%files -n %{name}
%defattr(-,root,root)
/opt/foros/server/etc/www/PageSense

EOF

rpmbuild --force -ba --define "version $VERSION" "$PAGESENSE_SPEC_FILE" || \
  { echo "can't build rocksdbf RPM" >&2 ; exit 1 ; }

# install librdkafka
cp $BIN_RPM_FOLDER/foros-pagesense-programmatic-*.rpm $RES_RPMS/


#!/bin/bash

VERSION="1.1.0"
VERSIONSSV=ssv7
TAG=v1.1.0rc2

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

pushd `dirname $0` >/dev/null
SCRIPT_DIR=`pwd`
popd >/dev/null

PATH=$PATH:$SCRIPT_DIR

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build libtool \
  glib2-devel gcc-c++ epel-rpm-macros cmake3 \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`
RPM_SOURCES_DIR=`rpm -E %_sourcedir`

# prepare archive
pushd "$RES_TMP"

rm -rf xgboost
git clone --recursive https://github.com/dmlc/xgboost.git \
  || \
  { echo "can't clone git repo" >&2 ; exit 1 ; }

cd xgboost
git checkout tags/$TAG
git submodule update --init

git-archive-all.sh --format tar.gz "$RPM_SOURCES_DIR/xgboost-$VERSION.tar.gz" \
  || \
  { echo "can't archive source" >&2 ; exit 1 ; }

popd

echo "archive created in $RPM_SOURCES_DIR/xgboost-$VERSION.tar.gz"

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

SPEC_FILE=`rpm -E %_specdir`/xgboost.spec

cat << 'EOF' > $SPEC_FILE
Name:    xgboost
Version: %{version}
Release: %{release}%{?dist}
Summary: Aerospike
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/dmlc/xgboost
BuildRequires: git libtool curl make
BuildRequires: gcc-c++
# disable strip for static library
%global __os_install_post %{nil}
%define __build_folder xgboost
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Source0: xgboost-%{version}.tar.gz

%description
XGBoost

%package -n %{name}-devel
Summary: XGBoost
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel
XGBoost devel

%prep
%setup -q -c xgboost-%{version}

%build
mkdir build
pushd build

#scl enable devtoolset-8 -- cmake3 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
#scl enable devtoolset-8 -- %{__make} -j4
cmake3 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
%{__make} -j4

popd

%install
pushd build
DESTDIR=%{buildroot} %{__make} install 
mkdir -p %{buildroot}/usr/include/xgboost/data/
mkdir -p %{buildroot}/usr/include/xgboost/c_api/
mkdir -p %{buildroot}/usr/include/xgboost/gbm/

popd
cp ./src/data/*.h %{buildroot}/usr/include/xgboost/data/
cp ./src/c_api/*.h %{buildroot}/usr/include/xgboost/c_api/
cp ./src/gbm/*.h %{buildroot}/usr/include/xgboost/gbm/

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
/usr/lib64/libxgboost.so
/usr/lib/librabit*.so
/usr/bin/xgboost

%files -n %{name}-devel
%defattr(-,root,root)
%{_includedir}/xgboost
%{_includedir}/dmlc
%{_includedir}/rabit

/usr/lib/librabit*.a
/usr/lib64/libdmlc.a

/usr/lib/cmake
/usr/lib64/cmake

EOF

$SUDO_PREFIX yum-builddep -y --define "version $VERSION" --define "release $VERSIONSSV" "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

echo "to download source"

spectool --force -g -R --define "version $VERSION" --define "release $VERSIONSSV" "$SPEC_FILE" || \
  { echo "can't download source RPM" >&2 ; exit 1 ; }

echo "to build rpm"

rpmbuild --force -ba --define "version $VERSION" --define "release $VERSIONSSV" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install librdkafka
cp $BIN_RPM_FOLDER/xgboost*.rpm $RES_RPMS/

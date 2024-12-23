#!/bin/bash

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/

RES_RPMS=build/RPMS/
VERSION=8.8.1
RELEASE=4
DOWNLOAD_VERSION=8.8.1-ssv4

_version=%{VERSION}
_release=%{RELEASE}
_download_version=%{DOWNLOAD_VERSION}

rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake libtool \
  glib2-devel cmake gcc-c++ epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install libzstd-devel zlib-devel || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

ROCKSDB_SPEC_FILE=`rpm -E %_specdir`/rocksdb.spec

cat << 'EOF' > $ROCKSDB_SPEC_FILE
Name:    rocksdb
Version: %{_version}
Release: %{_release}%{?dist}
Summary: A Persistent Key-Value Store for Flash and RAM Storage
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/facebook/rocksdb
#Source0: https://github.com/facebook/rocksdb/archive/v%{_version}.tar.gz
Source0: https://github.com/yoori/rocksdb/archive/v%{_download_version}.tar.gz

BuildRequires: autoconf automake libtool curl make
BuildRequires: gcc-c++
BuildRequires: libzstd-devel bzip2-devel snappy
Requires: zlib libstdc++ libzstd

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)
%define __product protobuf
%define __cmake_build_dir build
%description
RocksDB - A Persistent Key-Value Store for Flash and RAM Storage

%package -n %{name}-devel
Summary: RocksDB - A Persistent Key-Value Store for Flash and RAM Storage (Devel)
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel
RocksDB - A Persistent Key-Value Store for Flash and RAM Storage
This package contains headers and libraries required to build applications
using RocksDB.

%prep
%setup -q -n rocksdb-%{_download_version}

%build
export PATH=/opt/rh/gcc-toolset-10/root/usr/bin/:$PATH
#PORTABLE=1 make -j6 DISABLE_WARNING_AS_ERROR=1 DEBUG_LEVEL=0 WITH_TESTS=0
mkdir build
pushd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_TESTS:BOOL=OFF ..
cmake --build . -j
popd

%install
rm -rf %{buildroot}
echo 'rm -rf ' %{buildroot}
mkdir -p %{buildroot}/usr
echo 'mkdir -p %{buildroot}/usr'
echo "Install to %{buildroot}/usr"

#DESTDIR=%{buildroot} make install INSTALL_PATH=%{buildroot}
pushd build
cmake --install . --prefix %{buildroot}/usr/
popd

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
/usr/lib64/librocksdb.so*
/usr/lib64/cmake/*
#/usr/lib64/pkgconfig/*

%files -n %{name}-devel
%defattr(-,root,root)
/usr/include/rocksdb

%defattr(444,root,root)
/usr/lib64/*.a

EOF

$SUDO_PREFIX yum-builddep -y "$ROCKSDB_SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_download_version $DOWNLOAD_VERSION" \
  "$ROCKSDB_SPEC_FILE" || \
  { echo "can't download rocksdb source RPM" >&2 ; exit 1 ; }

rpmbuild --force -ba \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_download_version $DOWNLOAD_VERSION" \
  "$ROCKSDB_SPEC_FILE" || \
  { echo "can't build rocksdbf RPM" >&2 ; exit 1 ; }

cp $BIN_RPM_FOLDER/rocksdb*.rpm $RES_RPMS/


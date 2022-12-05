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
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake libtool \
  glib2-devel cmake gcc-c++ epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install libzstd-devel zlib-devel || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

#cp /home/ykuznetsov/rocksdb-6.5.2.tar.gz "`rpm -E %_sourcedir`"/

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

# download librdkafka source RPM
ROCKSDB_SPEC_FILE=`rpm -E %_specdir`/rocksdb.spec

cat << 'EOF' > $ROCKSDB_SPEC_FILE
Name:    rocksdb
Version: %{_version}
Release: ssv2%{?dist}
Summary: A Persistent Key-Value Store for Flash and RAM Storage
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/facebook/rocksdb
#Source0: rocksdb-%{_version}.tar.gz
Source0: https://github.com/facebook/rocksdb/archive/v%{_version}.tar.gz
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
%setup -q -n rocksdb-%{_version}

%build
PORTABLE=1 make -j6 static_lib shared_lib DISABLE_WARNING_AS_ERROR=1 DEBUG_LEVEL=0

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr
echo "Install to %{buildroot}/usr"
DESTDIR=%{buildroot}/usr make install INSTALL_PATH=%{buildroot}/usr
mv %{buildroot}/usr/lib %{buildroot}/usr/lib64

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
%{_libdir}/librocksdb.so*

%files -n %{name}-devel
%defattr(-,root,root)
%{_includedir}/rocksdb
%defattr(444,root,root)
%{_libdir}/*.a

EOF

#VERSION=6.5.2
VERSION=7.6.0

$SUDO_PREFIX yum-builddep -y "$ROCKSDB_SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$ROCKSDB_SPEC_FILE" || \
  { echo "can't download rocksdb source RPM" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$ROCKSDB_SPEC_FILE" || \
  { echo "can't build rocksdbf RPM" >&2 ; exit 1 ; }

# install librdkafka
cp $BIN_RPM_FOLDER/rocksdb*.rpm $RES_RPMS/


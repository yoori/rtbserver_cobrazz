#!/bin/bash

VERSION=8.10.2
RELEASE=ssv1

# script require 'sudo rpm' for install RPM packages
# and access to repository mirrors

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake \
  libtool glib2-devel cmake gcc-c++ epel-rpm-macros \
  zlib-devel bzip2-devel snappy-devel lz4-devel libzstd-devel \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp $(rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
TOPLINGDB_SPEC_FILE=$(rpm -E %_specdir)/toplingdb.spec

cat << 'EOF_SPEC' > "$TOPLINGDB_SPEC_FILE"
Name:    toplingdb
Version: %{_version}
Release: %{_release}%{?dist}
Summary: ToplingDB - high performance kv storage based on RocksDB
Group:   Development/Libraries/C and C++
License: Apache-2.0
URL:     https://github.com/topling/toplingdb
Source0: https://github.com/topling/toplingdb/archive/refs/tags/topling-8.10.2-frocks-1.0.tar.gz
BuildRequires: autoconf automake libtool curl make
BuildRequires: gcc-c++ cmake
BuildRequires: libaio-devel gcc-c++ gflags-devel zlib-devel bzip2-devel libcurl-devel liburing-devel
BuildRequires: snappy-devel jemalloc-devel
BuildRequires: gflags-devel
Requires: zlib libstdc++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description
ToplingDB - high performance kv storage based on RocksDB.

%package -n %{name}-devel
Summary: ToplingDB development files
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}

%description -n %{name}-devel
ToplingDB development headers and static libraries.

%prep
rm -rf %{_sourcedir}/toplingdb
git clone --branch topling-8.10.2-frocks-1.0 https://github.com/topling/toplingdb %{_sourcedir}/toplingdb
cd %{_sourcedir}/toplingdb

%build
make -j${nproc} db_bench DEBUG_LEVEL=0

%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} make install PREFIX=/usr

%clean
rm -rf %{buildroot}

%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
%{_libdir}/*.so*

%files -n %{name}-devel
%defattr(-,root,root)
%{_includedir}/*
%defattr(444,root,root)
%{_libdir}/*.a
%{_libdir}/cmake
%{_libdir}/pkgconfig
EOF_SPEC

$SUDO_PREFIX yum-builddep -y --define "_version $VERSION" --define "_release $RELEASE" "$TOPLINGDB_SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" --define "_release $RELEASE" "$TOPLINGDB_SPEC_FILE" || \
  { echo "can't download toplingdb source RPM" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" --define "_release $RELEASE" "$TOPLINGDB_SPEC_FILE" || \
  { echo "can't build toplingdb RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/toplingdb*.rpm "$RES_RPMS"/

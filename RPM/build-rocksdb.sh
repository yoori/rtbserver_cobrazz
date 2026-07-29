#!/bin/bash

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
VERSION=${VERSION:-11.1.1}
RELEASE=${RELEASE:-ssv4}
BOOST_PACKAGE_NAME=${BOOST_PACKAGE_NAME:-boost185}
BOOST_DEVEL_PACKAGE_NAME=${BOOST_DEVEL_PACKAGE_NAME:-${BOOST_PACKAGE_NAME}-devel}
BOOST_PREFIX=${BOOST_PREFIX:-/opt/foros/${BOOST_PACKAGE_NAME}}
FOLLY_PACKAGE_NAME=${FOLLY_PACKAGE_NAME:-folly}
FOLLY_DEVEL_PACKAGE_NAME=${FOLLY_DEVEL_PACKAGE_NAME:-${FOLLY_PACKAGE_NAME}-devel}
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install \
  spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  autoconf automake libtool \
  glib2-devel cmake gcc-c++ epel-rpm-macros gcc-toolset-10-gcc-c++ \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install \
  libzstd-devel zlib-devel "$FOLLY_DEVEL_PACKAGE_NAME" \
  "$BOOST_DEVEL_PACKAGE_NAME" liburing-devel || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
RPM_DIRS=`rpm -E \
  '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`
mkdir -vp $RPM_DIRS

#cp /home/ykuznetsov/rocksdb-6.5.2.tar.gz "`rpm -E %_sourcedir`"/

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

# download librdkafka source RPM
ROCKSDB_SPEC_FILE=`rpm -E %_specdir`/rocksdb.spec

cat << 'EOF' > $ROCKSDB_SPEC_FILE
Name:    rocksdb
Version: %{_version}
Release: %{_release}%{?dist}
%global debug_package %{nil}
Summary: A Persistent Key-Value Store for Flash and RAM Storage
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/facebook/rocksdb
#Source0: rocksdb-%{_version}.tar.gz
Source0: https://github.com/facebook/rocksdb/archive/v%{_version}.tar.gz
BuildRequires: autoconf automake libtool curl make
BuildRequires: gcc-c++
BuildRequires: gcc-toolset-10-gcc-c++
BuildRequires: libzstd-devel bzip2-devel snappy liburing-devel
BuildRequires: %{_folly_devel_package_name}
BuildRequires: %{_boost_devel_package_name}
Requires: zlib libstdc++ libzstd
Requires: %{_folly_package_name} %{_boost_package_name} liburing

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
find . -type f \
  \( -name '*.cc' -o -name '*.h' \) \
  -exec perl -pi -e 's#folly/coro/#folly/experimental/coro/#g' {} +
perl -0pi -e '
  s#co_withExecutor\(
    \s*&range->context\(\)->executor\(\),
    \s*
  #folly::coro::co_viaIfAsync(
    folly::getKeepAliveToken(&range->context()->executor()), #gx
' \
  db/version_set.cc
sed -i '/int ret = io_uring_queue_init(kIoUringDepth, new_io_uring, flags);/a\
  if (ret == -EINVAL) {\
    ret = io_uring_queue_init(kIoUringDepth, new_io_uring, 0);\
  }' env/io_posix.h
sed -i '2260,2262c\
  options.include_memtables =\
      ((include_flags & SizeApproximationFlags::INCLUDE_MEMTABLES) !=\
       SizeApproximationFlags::NONE);\
  options.include_files =\
      ((include_flags & SizeApproximationFlags::INCLUDE_FILES) !=\
       SizeApproximationFlags::NONE);' include/rocksdb/db.h

%build
env \
  CXX=/opt/rh/gcc-toolset-10/root/usr/bin/g++ \
  CC=/opt/rh/gcc-toolset-10/root/usr/bin/gcc \
  ROCKSDB_CXX_STANDARD=gnu++20 \
  make -j6 static_lib \
    USE_COROUTINES=1 \
    WITH_LIBURING=1 \
    DISABLE_JEMALLOC=1 \
    DISABLE_WARNING_AS_ERROR=1 \
    DEBUG_LEVEL=0 \
    PORTABLE=1 \
    EXTRA_CXXFLAGS='-std=gnu++20 -fPIC -I%{_boost_prefix}/include' \
    EXTRA_LDFLAGS='-L%{_boost_prefix}/lib64 -lfolly'

%install
rm -rf %{buildroot}
mkdir -p \
  %{buildroot}%{_includedir} \
  %{buildroot}%{_libdir}/pkgconfig \
  %{buildroot}%{_libdir}/cmake/rocksdb
cp -a include/rocksdb %{buildroot}%{_includedir}/
install -m 0644 librocksdb.a %{buildroot}%{_libdir}/librocksdb.a
cat > %{buildroot}%{_libdir}/pkgconfig/rocksdb.pc <<PC_EOF
prefix=/usr
exec_prefix=\${prefix}
includedir=\${prefix}/include
libdir=/usr/lib64

Name: rocksdb
Description: An embeddable persistent key-value store for fast storage
Version: %{version}
Libs: -L\${libdir} -lrocksdb
Libs.private: -L%{_boost_prefix}/lib64 -lfolly -luring -lzstd -lz -lbz2 -lsnappy -llz4 -pthread -lrt -ldl
Cflags: -I\${includedir} -I%{_boost_prefix}/include -std=c++20
PC_EOF
mkdir -p %{buildroot}%{_libdir}/cmake/rocksdb
cat > %{buildroot}%{_libdir}/cmake/rocksdb/RocksDBConfig.cmake <<'CMAKE_EOF'
include(CMakeFindDependencyMacro)

add_library(RocksDB::rocksdb STATIC IMPORTED)
set_target_properties(RocksDB::rocksdb PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../librocksdb.a"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include;%{_boost_prefix}/include"
  INTERFACE_COMPILE_FEATURES cxx_std_20
  INTERFACE_LINK_LIBRARIES
    "folly;uring;zstd;z;bz2;snappy;lz4;pthread;rt;dl")
CMAKE_EOF

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%license COPYING LICENSE.Apache LICENSE.leveldb

%files -n %{name}-devel
%defattr(-,root,root)
%{_includedir}/rocksdb
%defattr(444,root,root)
%{_libdir}/*.a
%{_libdir}/cmake/rocksdb
%{_libdir}/pkgconfig/rocksdb.pc

EOF

${SUDO_PREFIX:-sudo} yum-builddep -y \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_folly_package_name $FOLLY_PACKAGE_NAME" \
  --define "_folly_devel_package_name $FOLLY_DEVEL_PACKAGE_NAME" \
  --define "_boost_package_name $BOOST_PACKAGE_NAME" \
  --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
  --define "_boost_prefix $BOOST_PREFIX" \
  "$ROCKSDB_SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_folly_package_name $FOLLY_PACKAGE_NAME" \
  --define "_folly_devel_package_name $FOLLY_DEVEL_PACKAGE_NAME" \
  --define "_boost_package_name $BOOST_PACKAGE_NAME" \
  --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
  --define "_boost_prefix $BOOST_PREFIX" \
  "$ROCKSDB_SPEC_FILE" || \
  { echo "can't download rocksdb source RPM" >&2 ; exit 1 ; }

rpmbuild --force -ba \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_folly_package_name $FOLLY_PACKAGE_NAME" \
  --define "_folly_devel_package_name $FOLLY_DEVEL_PACKAGE_NAME" \
  --define "_boost_package_name $BOOST_PACKAGE_NAME" \
  --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
  --define "_boost_prefix $BOOST_PREFIX" \
  "$ROCKSDB_SPEC_FILE" || \
  { echo "can't build rocksdbf RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/rocksdb-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/
cp "$BIN_RPM_FOLDER"/rocksdb-devel-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/

echo "Built packages:"
ls -1 "$RES_RPMS"/rocksdb-"$VERSION"-"$RELEASE".*.rpm
ls -1 "$RES_RPMS"/rocksdb-devel-"$VERSION"-"$RELEASE".*.rpm

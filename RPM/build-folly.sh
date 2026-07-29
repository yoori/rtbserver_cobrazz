#!/bin/bash

set -euo pipefail

VERSION=${1:-2024.01.08.00}
RELEASE=${RELEASE:-ssv2}
BOOST_PACKAGE_NAME=${BOOST_PACKAGE_NAME:-boost185}
BOOST_DEVEL_PACKAGE_NAME=${BOOST_DEVEL_PACKAGE_NAME:-${BOOST_PACKAGE_NAME}-devel}
BOOST_PREFIX=${BOOST_PREFIX:-/opt/foros/${BOOST_PACKAGE_NAME}}
BOOST_CMAKE_DIR=${BOOST_CMAKE_DIR:-${BOOST_PREFIX}/lib64/cmake/Boost-1.85.0}

# Optional offline inputs:
#   FOLLY_SOURCE_DIR=/path/to/folly ./RPM/build-folly.sh 2024.01.08.00
#   FOLLY_SOURCE_ARCHIVE=/path/to/folly.tar.gz ./RPM/build-folly.sh 2024.01.08.00

RES_TMP=build/TMP/folly
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"

YUM_REPO_ARGS=()
if [ -n "${DISABLE_REPOS:-}" ]; then
  IFS=',' read -r -a DISABLE_REPO_LIST <<< "$DISABLE_REPOS"
  for REPO in "${DISABLE_REPO_LIST[@]}"; do
    [ -n "$REPO" ] && YUM_REPO_ARGS+=(--disablerepo="$REPO")
  done
fi

sudo yum -y "${YUM_REPO_ARGS[@]}" install \
  spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  cmake ninja-build gcc-c++ gcc-toolset-10-gcc-c++ \
  "$BOOST_DEVEL_PACKAGE_NAME" bzip2-devel double-conversion-devel fmt-devel \
  gflags-devel glog-devel libevent-devel libsodium-devel \
  libzstd-devel lz4-devel openssl-devel zlib-devel \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

mkdir -vp $(
  rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'
)

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
SOURCE_DIR=$(rpm -E '%_sourcedir')
SPEC_FILE=$(rpm -E %_specdir)/folly-"$VERSION"-"$RELEASE".spec
SOURCE_ARCHIVE="$SOURCE_DIR"/v"$VERSION".tar.gz

if [ -n "${FOLLY_SOURCE_ARCHIVE:-}" ]; then
  cp "$FOLLY_SOURCE_ARCHIVE" "$SOURCE_ARCHIVE"
elif [ -n "${FOLLY_SOURCE_DIR:-}" ]; then
  STAGED_SOURCE="$RES_TMP"/folly-"$VERSION"
  mkdir -p "$STAGED_SOURCE"
  cp -a "$FOLLY_SOURCE_DIR"/. "$STAGED_SOURCE"/
  rm -rf "$STAGED_SOURCE"/.git
  tar -C "$RES_TMP" -czf "$SOURCE_ARCHIVE" folly-"$VERSION"
fi

cat << 'EOF_SPEC' > "$SPEC_FILE"
Name:    folly
Version: %{_version}
Release: %{_release}%{?dist}
Summary: Facebook Open-source Library
Group:   Development/Libraries/C and C++
License: Apache-2.0
URL:     https://github.com/facebook/folly
Source0: https://github.com/facebook/folly/archive/refs/tags/v%{version}.tar.gz

BuildRequires: %{_boost_devel_package_name}
BuildRequires: bzip2-devel
BuildRequires: cmake
BuildRequires: double-conversion-devel
BuildRequires: fmt-devel
BuildRequires: gcc-c++
BuildRequires: gcc-toolset-10-gcc-c++
BuildRequires: gflags-devel
BuildRequires: glog-devel
BuildRequires: libevent-devel
BuildRequires: libsodium-devel
BuildRequires: libzstd-devel
BuildRequires: lz4-devel
BuildRequires: ninja-build
BuildRequires: openssl-devel
BuildRequires: zlib-devel

Requires: %{_boost_package_name}
Requires: double-conversion
Requires: fmt
Requires: gflags
Requires: glog
Requires: libevent
Requires: libsodium
Requires: libzstd
Requires: openssl-libs

%description
Folly is a library of C++ components used at Facebook.

%package -n %{name}-devel
Summary: Folly development files
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}-%{release}
Requires: %{_boost_devel_package_name}
Requires: double-conversion-devel
Requires: fmt-devel
Requires: gflags-devel
Requires: glog-devel
Requires: libevent-devel
Requires: libsodium-devel
Requires: openssl-devel

%description -n %{name}-devel
Headers, CMake files, and shared library symlinks for Folly.

%prep
%setup -q -n folly-%{version}

%build
export PATH="/opt/rh/gcc-toolset-10/root/usr/bin:${PATH}"
export CC=/opt/rh/gcc-toolset-10/root/usr/bin/gcc
export CXX=/opt/rh/gcc-toolset-10/root/usr/bin/g++

FOLLY_CMAKE_ARGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
  -DCMAKE_CXX_STANDARD=20
  -DCMAKE_INSTALL_PREFIX=/usr
  -DCMAKE_INSTALL_LIBDIR=lib64
  -DLIB_INSTALL_DIR=lib64
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  -DBOOST_ROOT=%{_boost_prefix}
  -DBoost_ROOT=%{_boost_prefix}
  -DBOOST_INCLUDEDIR=%{_boost_prefix}/include
  -DBOOST_LIBRARYDIR=%{_boost_prefix}/lib64
  -DBoost_DIR=%{_boost_cmake_dir}
  -DBoost_NO_SYSTEM_PATHS=ON
  -DCMAKE_PREFIX_PATH=%{_boost_prefix}
  -DBUILD_TESTS=OFF
  -DFOLLY_BUILD_TESTS=OFF
  -DFOLLY_HAVE_INT128_T=ON
)

cmake -S . -B build-shared "${FOLLY_CMAKE_ARGS[@]}" \
  -DBUILD_SHARED_LIBS=ON
cmake --build build-shared --parallel %{?_smp_build_ncpus}

cmake -S . -B build-static "${FOLLY_CMAKE_ARGS[@]}" \
  -DBUILD_SHARED_LIBS=OFF
cmake --build build-static --parallel %{?_smp_build_ncpus}

%install
DESTDIR=%{buildroot} cmake --install build-shared
DESTDIR=%{buildroot} cmake --install build-static

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%doc README.md
%{_libdir}/libfolly*.so.*

%files devel
%{_includedir}/folly
%{_libdir}/libfolly*.so
%{_libdir}/libfolly*.a
%{_prefix}/lib/cmake/folly
%{_libdir}/pkgconfig/libfolly.pc
EOF_SPEC

${SUDO_PREFIX:-sudo} yum-builddep -y "${YUM_REPO_ARGS[@]}" \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_boost_package_name $BOOST_PACKAGE_NAME" \
  --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
  --define "_boost_prefix $BOOST_PREFIX" \
  --define "_boost_cmake_dir $BOOST_CMAKE_DIR" \
  "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

if [ ! -f "$SOURCE_ARCHIVE" ]; then
  spectool --force -g -R \
    --define "_version $VERSION" \
    --define "_release $RELEASE" \
    --define "_boost_package_name $BOOST_PACKAGE_NAME" \
    --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
    --define "_boost_prefix $BOOST_PREFIX" \
    --define "_boost_cmake_dir $BOOST_CMAKE_DIR" \
    "$SPEC_FILE" || \
    { echo "can't download sources" >&2 ; exit 1 ; }
fi

rpmbuild --force -ba \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  --define "_boost_package_name $BOOST_PACKAGE_NAME" \
  --define "_boost_devel_package_name $BOOST_DEVEL_PACKAGE_NAME" \
  --define "_boost_prefix $BOOST_PREFIX" \
  --define "_boost_cmake_dir $BOOST_CMAKE_DIR" \
  --define "debug_package %{nil}" \
  "$SPEC_FILE" || \
  { echo "can't build folly RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/folly-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/
cp "$BIN_RPM_FOLDER"/folly-devel-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/

echo "Built packages:"
ls -1 "$RES_RPMS"/folly-"$VERSION"-"$RELEASE".*.rpm
ls -1 "$RES_RPMS"/folly-devel-"$VERSION"-"$RELEASE".*.rpm

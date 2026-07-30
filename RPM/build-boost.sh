#!/bin/bash

set -euo pipefail

VERSION=${1:-1.85.0}
RELEASE=${RELEASE:-ssv1}
BOOST_SUFFIX=${BOOST_SUFFIX:-$(echo "$VERSION" | awk -F. '{print $1 $2}')}
PACKAGE_NAME=${PACKAGE_NAME:-boost${BOOST_SUFFIX}}
BOOST_TAR_VERSION=${VERSION//./_}
BOOST_PREFIX=${BOOST_PREFIX:-/opt/foros/${PACKAGE_NAME}}
BOOST_GCC_VERSION=${BOOST_GCC_VERSION:-10}
BOOST_GCC=${BOOST_GCC:-/opt/rh/gcc-toolset-10/root/usr/bin/g++}
DEFAULT_BOOST_LIBRARIES=system,thread,chrono,date_time,atomic
DEFAULT_BOOST_LIBRARIES+=,filesystem,regex,program_options,serialization
DEFAULT_BOOST_LIBRARIES+=,iostreams,random,timer,json,container
DEFAULT_BOOST_LIBRARIES+=,context,coroutine
BOOST_LIBRARIES=${BOOST_LIBRARIES:-$DEFAULT_BOOST_LIBRARIES}

RES_TMP=${RES_TMP:-/tmp/${PACKAGE_NAME}-rpmbuild-${VERSION}}
RES_RPMS=${RES_RPMS:-build/RPMS}
rm -rf "$RES_TMP" || sudo rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"
if [ ! -w "$RES_RPMS" ]; then
  RES_RPMS=/tmp/${PACKAGE_NAME}-RPMS
  mkdir -p "$RES_RPMS"
fi

TOPDIR=$(readlink -f "$RES_TMP/rpmbuild")
RPM_DEFINES=(
  --define "_topdir $TOPDIR"
)

sudo yum -y install \
  spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  gcc gcc-c++ gcc-toolset-10-gcc gcc-toolset-10-gcc-c++ \
  make python3 bzip2-devel zlib-devel xz-devel \
  || { echo "can't install build requirements" >&2 ; exit 1 ; }

RPM_DIRS=$(rpm "${RPM_DEFINES[@]}" -E \
  '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch')
mkdir -vp $RPM_DIRS

BIN_RPM_FOLDER=$(rpm "${RPM_DEFINES[@]}" -E '%_rpmdir/%_arch')
SOURCE_DIR=$(rpm "${RPM_DEFINES[@]}" -E %_sourcedir)
SPEC_FILE=$(rpm "${RPM_DEFINES[@]}" -E %_specdir)/${PACKAGE_NAME}-${VERSION}-${RELEASE}.spec

cat << EOF_SPEC > "$SPEC_FILE"
Name:    ${PACKAGE_NAME}
Version: ${VERSION}
Release: ${RELEASE}%{?dist}
%global debug_package %{nil}
Summary: Parallel Boost C++ libraries build
Group:   Development/Libraries/C and C++
License: Boost
URL:     https://www.boost.org
Source0: https://archives.boost.io/release/%{version}/source/boost_${BOOST_TAR_VERSION}.tar.gz

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: gcc-toolset-10-gcc
BuildRequires: gcc-toolset-10-gcc-c++
BuildRequires: make
BuildRequires: python3
BuildRequires: bzip2-devel
BuildRequires: zlib-devel
BuildRequires: xz-devel

Requires: libstdc++
Requires: glibc

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
Parallel Boost build installed under ${BOOST_PREFIX}. This package is intended
for controlled builds that need a newer Boost without replacing the system
boost176 packages.

%package devel
Summary: Development files for %{name}
Group:   Development/Libraries/C and C++
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Headers, shared library linker symlinks, and optional CMake files for %{name}.

%prep
%autosetup -n boost_${BOOST_TAR_VERSION}

%build
env \
  CC=/opt/rh/gcc-toolset-10/root/usr/bin/gcc \
  CXX=${BOOST_GCC} \
  ./bootstrap.sh \
    --with-toolset=gcc \
    --with-libraries=${BOOST_LIBRARIES}

cat > user-config.jam << 'JAM_EOF'
using gcc : ${BOOST_GCC_VERSION} : ${BOOST_GCC} ;
JAM_EOF

env \
  CC=/opt/rh/gcc-toolset-10/root/usr/bin/gcc \
  CXX=${BOOST_GCC} \
  ./b2 \
    --user-config=user-config.jam \
    -j%{?_smp_build_ncpus}%{!?_smp_build_ncpus:1} \
    toolset=gcc-${BOOST_GCC_VERSION} \
    variant=release \
    threading=multi \
    link=shared \
    runtime-link=shared \
    cxxstd=20 \
    --layout=system \
    stage

%install
rm -rf %{buildroot}
env \
  CC=/opt/rh/gcc-toolset-10/root/usr/bin/gcc \
  CXX=${BOOST_GCC} \
  ./b2 \
    --user-config=user-config.jam \
    -j%{?_smp_build_ncpus}%{!?_smp_build_ncpus:1} \
    toolset=gcc-${BOOST_GCC_VERSION} \
    variant=release \
    threading=multi \
    link=shared \
    runtime-link=shared \
    cxxstd=20 \
    --layout=system \
    --prefix=%{buildroot}${BOOST_PREFIX} \
    --libdir=%{buildroot}${BOOST_PREFIX}/lib64 \
    --includedir=%{buildroot}${BOOST_PREFIX}/include \
    install

find %{buildroot} -name '*.a' -delete
find %{buildroot} -name '*.la' -delete
find %{buildroot}${BOOST_PREFIX} -type f \
  \( -name '*.cmake' -o -name '*.pc' \) \
  -exec sed -i "s#%{buildroot}##g" {} +

mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d
echo "${BOOST_PREFIX}/lib64" > %{buildroot}%{_sysconfdir}/ld.so.conf.d/%{name}.conf

mkdir -p %{buildroot}${BOOST_PREFIX}/lib64/cmake

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE_1_0.txt
%doc README.md
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/%{name}.conf
%dir ${BOOST_PREFIX}
%dir ${BOOST_PREFIX}/lib64
${BOOST_PREFIX}/lib64/libboost_*.so.*

%files devel
${BOOST_PREFIX}/include
${BOOST_PREFIX}/lib64/libboost_*.so
${BOOST_PREFIX}/lib64/cmake
EOF_SPEC

sudo yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -C "$SOURCE_DIR" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba "${RPM_DEFINES[@]}" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/${PACKAGE_NAME}-*"$VERSION"-"$RELEASE"*.rpm "$RES_RPMS"/

ls -1 "$RES_RPMS"/${PACKAGE_NAME}-*"$VERSION"-"$RELEASE"*.rpm

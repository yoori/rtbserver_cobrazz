#!/bin/bash

set -e

VERSION=${1:-5.3.0}
RELEASE=${RELEASE:-ssv1}

RES_TMP=${RES_TMP:-/tmp/jemalloc-rpmbuild-$VERSION}
RES_RPMS=${RES_RPMS:-build/RPMS}
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"
if [ ! -w "$RES_RPMS" ]; then
  RES_RPMS=/tmp/jemalloc-RPMS
  mkdir -p "$RES_RPMS"
fi

TOPDIR=$(readlink -f "$RES_TMP/rpmbuild")
RPM_DEFINES=(--define "_topdir $TOPDIR")

sudo yum -y install \
  spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  autoconf make gcc gcc-c++ perl \
  || { echo "can't install build requirements" >&2 ; exit 1 ; }

mkdir -vp $(rpm "${RPM_DEFINES[@]}" -E '%_tmppath %_rpmdir %_builddir %_sourcedir')
mkdir -vp $(rpm "${RPM_DEFINES[@]}" -E '%_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm "${RPM_DEFINES[@]}" -E '%_rpmdir/%_arch')
SPEC_FILE=$(rpm "${RPM_DEFINES[@]}" -E %_specdir)/jemalloc-$VERSION-ssv.spec
SOURCE_DIR=$(rpm "${RPM_DEFINES[@]}" -E %_sourcedir)

cat << 'EOF' > "$SPEC_FILE"
Name:    jemalloc
Version: %{_version}
Release: %{_release}%{?dist}
Summary: General-purpose scalable concurrent malloc implementation
Group:   System Environment/Libraries
License: BSD
URL:     https://jemalloc.net/
Source0: https://github.com/jemalloc/jemalloc/releases/download/%{version}/jemalloc-%{version}.tar.bz2

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: autoconf
BuildRequires: perl

%description
jemalloc is a general-purpose malloc implementation that emphasizes
fragmentation avoidance and scalable concurrency support.

%package devel
Summary: Development files for jemalloc
Group:   Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: pkgconfig

%description devel
Headers, linker library, pkg-config metadata, and helper tools for building
applications that use jemalloc.

%prep
%autosetup -n jemalloc-%{version} -p1

%build
%configure \
  --disable-static \
  --enable-prof

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install
find %{buildroot} -name '*.la' -delete

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license COPYING
%doc README VERSION doc/jemalloc.html
%{_bindir}/jemalloc.sh
%{_libdir}/libjemalloc.so.*
%{_mandir}/man3/jemalloc.3*

%files devel
%{_bindir}/jemalloc-config
%{_bindir}/jeprof
%{_includedir}/jemalloc
%{_libdir}/libjemalloc.so
%{_libdir}/pkgconfig/jemalloc.pc
EOF

sudo yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -C "$SOURCE_DIR" \
  "${RPM_DEFINES[@]}" \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba \
  "${RPM_DEFINES[@]}" \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/jemalloc-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/
cp "$BIN_RPM_FOLDER"/jemalloc-devel-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/

ls -1 "$RES_RPMS"/jemalloc-"$VERSION"-"$RELEASE".*.rpm
ls -1 "$RES_RPMS"/jemalloc-devel-"$VERSION"-"$RELEASE".*.rpm

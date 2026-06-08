#!/bin/bash

set -e

VERSION=2.18.1
RELEASE=ssv1

RES_TMP=${RES_TMP:-/tmp/gperftools-rpmbuild-$VERSION}
RES_RPMS=${RES_RPMS:-build/RPMS}
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"
if [ ! -w "$RES_RPMS" ]; then
  RES_RPMS=/tmp/gperftools-RPMS
  mkdir -p "$RES_RPMS"
fi
TOPDIR=$(readlink -f "$RES_TMP/rpmbuild")
RPM_DEFINES=(--define "_topdir $TOPDIR")

sudo yum -y install \
  spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  autoconf automake libtool make gcc gcc-c++ libunwind-devel \
  || { echo "can't install build requirements" >&2 ; exit 1 ; }

mkdir -vp $(rpm "${RPM_DEFINES[@]}" -E '%_tmppath %_rpmdir %_builddir %_sourcedir')
mkdir -vp $(rpm "${RPM_DEFINES[@]}" -E '%_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm "${RPM_DEFINES[@]}" -E '%_rpmdir/%_arch')
SPEC_FILE=$(rpm "${RPM_DEFINES[@]}" -E %_specdir)/gperftools-$VERSION-ssv.spec
SOURCE_DIR=$(rpm "${RPM_DEFINES[@]}" -E %_sourcedir)

cat << 'EOF' > "$SPEC_FILE"
Name:    gperftools
Version: %{_version}
Release: %{_release}%{?dist}
Summary: CPU profiler and malloc implementation
Group:   Development/Tools
License: BSD
URL:     https://github.com/gperftools/gperftools
Source0: https://github.com/gperftools/gperftools/releases/download/gperftools-%{version}/gperftools-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: libunwind-devel

Requires: %{name}-libs%{?_isa} = %{version}-%{release}

%description
gperftools contains a high-performance multi-threaded malloc implementation,
including tcmalloc, and CPU profiling libraries.

%package libs
Summary: Libraries provided by gperftools
Group:   System Environment/Libraries
Requires: libunwind%{?_isa}

%description libs
Libraries provided by gperftools, including libtcmalloc and libprofiler.

%package devel
Summary: Development libraries and headers for gperftools
Group:   Development/Libraries
Requires: %{name}-libs%{?_isa} = %{version}-%{release}
Requires: libunwind-devel%{?_isa}
Requires: pkgconfig

%description devel
Headers, linker libraries, and pkg-config files for applications that use
gperftools.

%prep
%autosetup -n gperftools-%{version} -p1

%build
%configure \
  --disable-static \
  --enable-libunwind

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install
find %{buildroot} -name '*.la' -delete

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%files
%license COPYING
%{_docdir}/gperftools

%files libs
%{_libdir}/libprofiler.so.*
%{_libdir}/libtcmalloc*.so.*

%files devel
%{_includedir}/gperftools
%{_libdir}/libprofiler.so
%{_libdir}/libtcmalloc*.so
%{_libdir}/pkgconfig/libprofiler.pc
%{_libdir}/pkgconfig/libtcmalloc*.pc
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

cp "$BIN_RPM_FOLDER"/gperftools*-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/

ls -1 "$RES_RPMS"/gperftools*-"$VERSION"-"$RELEASE".*.rpm

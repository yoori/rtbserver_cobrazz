#!/bin/bash

set -e

VERSION=0.3.5
RELEASE=7.ssv1

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"

sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config \
  rpm-build cmake gcc-c++ epel-rpm-macros gflags-devel || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

mkdir -vp  $(rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir')
mkdir -vp  $(rpm -E '%_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
SPEC_FILE=$(rpm -E %_specdir)/glog-$VERSION-ssv.spec

cat << 'EOF' > "$SPEC_FILE"
Name:    glog
Version: %{_version}
Release: %{_release}%{?dist}
Summary: A C++ application logging library
Group:   System Environment/Libraries
License: BSD
URL:     https://github.com/google/glog
Source0: https://github.com/google/glog/archive/v%{version}.tar.gz

BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: gflags-devel

Requires: gflags

%description
Google glog is a library that implements application-level logging.

%package devel
Summary: Development files for glog
Group:   Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: gflags-devel
Requires: pkgconfig

%description devel
This package contains headers and metadata required to build against glog.

%prep
%autosetup -n glog-%{version} -p1
sed -i \
  -e 's/DESTINATION lib/DESTINATION %{_lib}/g' \
  -e 's#INSTALL_DESTINATION lib/cmake/glog#INSTALL_DESTINATION %{_lib}/cmake/glog#g' \
  -e 's#DESTINATION lib/cmake/glog#DESTINATION %{_lib}/cmake/glog#g' \
  -e 's/SOVERSION ${GLOG_VERSION}/SOVERSION ${GLOG_MAJOR_VERSION}/g' \
  CMakeLists.txt

%build
%cmake \
  -DBUILD_SHARED_LIBS=ON \
  -DWITH_GFLAGS=ON \
  -DWITH_GTEST=OFF \
  -DBUILD_TESTING=OFF

%cmake_build

%install
%cmake_install
mkdir -p %{buildroot}%{_libdir}/pkgconfig
cat > %{buildroot}%{_libdir}/pkgconfig/libglog.pc << PC_EOF
prefix=%{_prefix}
exec_prefix=\${prefix}
libdir=%{_libdir}
includedir=%{_includedir}

Name: libglog
Description: Google logging library
Version: %{version}
Libs: -L\${libdir} -lglog -lgflags
Cflags: -I\${includedir}
PC_EOF

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license COPYING
%doc AUTHORS README
%{_libdir}/libglog.so.*

%files devel
%{_includedir}/glog
%{_libdir}/libglog.so
%{_libdir}/cmake/glog
%{_libdir}/pkgconfig/libglog.pc
EOF

sudo yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba \
  --define "_version $VERSION" \
  --define "_release $RELEASE" \
  "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/glog-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/
cp "$BIN_RPM_FOLDER"/glog-devel-"$VERSION"-"$RELEASE".*.rpm "$RES_RPMS"/

ls -1 "$RES_RPMS"/glog-"$VERSION"-"$RELEASE".*.rpm
ls -1 "$RES_RPMS"/glog-devel-"$VERSION"-"$RELEASE".*.rpm

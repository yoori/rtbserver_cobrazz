#!/bin/bash

VERSION=$1
VERSIONSSV=ssv4

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
mkdir -vp `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

SPEC_FILE=$(rpm -E %_specdir)/abseil-cpp-$VERSION-$VERSIONSSV.spec

cat << 'EOF_SPEC' > $SPEC_FILE
Name:    abseil-cpp
Version: %{_version}
Release: ssv4%{?dist}
Summary: C++ common libraries by Google
Group:   Development/Libraries/C and C++
License: ASL 2.0
URL:     https://abseil.io
Source0: https://github.com/abseil/abseil-cpp/archive/refs/tags/%{version}.tar.gz
Requires: libstdc++
Requires: libgcc
Requires: glibc
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: ninja-build

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description
Abseil is an open-source collection of C++ library code designed to augment the C++ standard library.

%package -n %{name}-devel
Summary: Development files for %{name}
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}-%{release}
%description -n %{name}-devel
Development headers and CMake files for %{name}.

%prep
%autosetup -p1 -n abseil-cpp-%{_version}

%build
%cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON \
  -DCMAKE_INSTALL_LIBDIR=%{_libdir} \
  -DABSL_PROPAGATE_CXX_STD=ON \
  -DABSL_ENABLE_INSTALL=ON \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_CXX_STANDARD=17

%cmake_build

%install
%cmake_install
rm -f %{buildroot}%{_libdir}/pkgconfig/absl_heterogeneous_lookup_testing.pc

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%license LICENSE
%doc README.md
%{_libdir}/libabsl*.so.*

%files -n %{name}-devel
%{_includedir}/absl
%{_libdir}/libabsl*.so
%{_libdir}/cmake/absl
%{_libdir}/pkgconfig/absl*.pc
EOF_SPEC

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/abseil-cpp-*$VERSION-*.rpm $RES_RPMS/
cp $BIN_RPM_FOLDER/abseil-cpp-devel-*$VERSION-*.rpm $RES_RPMS/


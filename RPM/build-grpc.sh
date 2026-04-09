#!/bin/bash

set -euo pipefail

VERSION="${1:-}"
VERSIONSSV=ssv4
SUDO_PREFIX=sudo

if [[ -z "$VERSION" ]]; then
  echo "Usage: $0 <grpc-version>" >&2
  echo "Example: $0 1.62.2" >&2
  exit 1
fi

# script requires sudo permissions for yum/rpm operations

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p "$RES_TMP"
mkdir -p "$RES_RPMS"

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  gcc-c++ cmake3 make pkgconfig autoconf automake libtool \
  glib2-devel epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install openssl-devel zlib-devel c-ares-devel re2-devel protobuf-devel protobuf-compiler \
  || \
  { echo "can't install grpc build dependencies" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
SPEC_FILE=$(rpm -E %_specdir)/grpc-$VERSION-$VERSIONSSV.spec

cat << 'EOF_SPEC' > "$SPEC_FILE"
Name:           grpc
Version:        %{_version}
Release:        ssv4%{?dist}
Summary:        High performance, open source universal RPC framework
License:        Apache-2.0
URL:            https://grpc.io/
Source0:        https://github.com/grpc/grpc/archive/refs/tags/v%{version}.tar.gz

Requires: zlib
Requires: glibc
Requires: libgcc
Requires: libstdc++
Requires: re2
Requires: c-ares
Requires: openssl-libs

BuildRequires:  cmake3
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  openssl-devel
BuildRequires:  zlib-devel
BuildRequires:  c-ares-devel
BuildRequires:  re2-devel
BuildRequires:  protobuf-devel
BuildRequires:  protobuf-compiler
BuildRequires:  pkgconfig

%description
gRPC is a modern open source high performance RPC framework that can run in
any environment.

%package devel
Summary: Development files for %{name}
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires:  openssl-devel
Requires:  zlib-devel
Requires:  c-ares-devel
Requires:  re2-devel
Requires:  protobuf-devel
Requires:  protobuf-compiler

%description devel
Headers and CMake configuration files needed to develop applications with gRPC.

%package plugins
Summary: gRPC protocol buffer plugins
Requires: %{name}%{?_isa} = %{version}-%{release}

%description plugins
Code generation plugins for use with the Protocol Buffers compiler.

%prep
%autosetup -n grpc-%{version}

%build
%cmake3 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DgRPC_BUILD_TESTS=OFF \
  -DgRPC_BUILD_CSHARP_EXT=OFF \
  -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
  -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
  -DgRPC_ABSL_PROVIDER=module \
  -DgRPC_CARES_PROVIDER=package \
  -DgRPC_RE2_PROVIDER=package \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_PROTOBUF_PROVIDER=package \
  -DgRPC_ZLIB_PROVIDER=package \
  -DgRPC_INSTALL=ON
%cmake3_build

%install
%cmake3_install

find %{buildroot} -type f -name '*.la' -delete

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%doc README.md
/usr/lib/lib*.so.*
/usr/share/grpc

%files devel
%{_includedir}/grpc*
/usr/lib/lib*.so
/usr/lib/cmake/grpc
/usr/lib/pkgconfig/*.pc

%files plugins
%{_bindir}/grpc_*_plugin
EOF_SPEC

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# copy built rpms
cp "$BIN_RPM_FOLDER"/grpc*"$VERSION"*.rpm "$RES_RPMS"/

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
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake libtool  python39\
  glib2-devel cmake gcc-c++ epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install libzstd-devel zlib-devel cyrus-sasl lz4-devel || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`

# download librdkafka source RPM
SPEC_FILE=`rpm -E %_specdir`/librdkafka.spec

cat << 'EOF' > $SPEC_FILE
Name:    librdkafka
Version: %{version}
Release: ssv1%{?dist}
Summary: The Apache Kafka C library
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/edenhill/librdkafka
Source0: https://github.com/edenhill/librdkafka/archive/v%{version}.tar.gz

%define soname 1

BuildRequires: zlib-devel libstdc++-devel gcc >= 4.1 gcc-c++ openssl-devel cyrus-sasl-devel lz4-devel python39
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%define _source_payload w9.gzdio
%define _binary_payload w9.gzdio

%description
librdkafka is the C/C++ client library implementation of the Apache Kafka protocol, containing both Producer and Consumer support.

%package -n %{name}%{soname}
Summary: The Apache Kafka C library
Group:   Development/Libraries/C and C++
Requires: zlib libstdc++ cyrus-sasl
# openssl libraries were extract to openssl-libs in RHEL7
%if 0%{?rhel} >= 7
Requires: openssl-libs
%else
Requires: openssl
%endif

%description -n %{name}%{soname}
librdkafka is the C/C++ client library implementation of the Apache Kafka protocol, containing both Producer and Consumer support.


%package -n %{name}-devel
Summary: The Apache Kafka C library (Development Environment)
Group:   Development/Libraries/C and C++
Requires: %{name}%{soname} = %{version}

%description -n %{name}-devel
librdkafka is the C/C++ client library implementation of the Apache Kafka protocol, containing both Producer and Consumer support.

This package contains headers and libraries required to build applications
using librdkafka.


%prep
%setup -q -n %{name}-%{version}

%configure

%build
make

%install
rm -rf %{buildroot}
DESTDIR=%{buildroot} make install
#mv %{buildroot}/usr/share/doc/librdkafka/ %{buildroot}/usr/share/doc/librdkafka1-%{version}/
mv %{buildroot}/usr/share/doc/librdkafka/ %{buildroot}/usr/share/doc/librdkafka1/

%clean
rm -rf %{buildroot}

%post   -n %{name}%{soname} -p /sbin/ldconfig
%postun -n %{name}%{soname} -p /sbin/ldconfig

%files -n %{name}%{soname}
%defattr(444,root,root)
%{_libdir}/librdkafka-static.a
%{_libdir}/librdkafka.so.%{soname}
%{_libdir}/librdkafka++.so.%{soname}
%defattr(-,root,root)
%doc README.md CONFIGURATION.md INTRODUCTION.md CHANGELOG.md STATISTICS.md
%doc LICENSE LICENSE.pycrc LICENSE.queue LICENSE.snappy LICENSE.tinycthread LICENSE.wingetopt LICENSES.txt
%defattr(-,root,root)
#%{_bindir}/rdkafka_example
#%{_bindir}/rdkafka_performance

%files -n %{name}-devel
%defattr(-,root,root)
%{_includedir}/librdkafka
%defattr(444,root,root)
%{_libdir}/librdkafka.a
%{_libdir}/librdkafka.so
%{_libdir}/librdkafka++.a
%{_libdir}/librdkafka++.so
%{_libdir}/pkgconfig/*.pc
EOF

VERSION=1.9.2

echo "install dependencies by $SPEC_FILE"

SUDO_PREFIX=sudo

echo "============="

cat "$SPEC_FILE" | sed 's/%{version}/1.4.4/g' >"$SPEC_FILE.builddep"

#$SUDO_PREFIX yum-builddep -y "$SPEC_FILE.builddep" || \
#  { echo "can't install build requirements" >&2 ; exit 1 ; }

echo "============"

spectool --force -g -R --define "version $VERSION" "$SPEC_FILE" || \
  { echo "can't download librdkafka source RPM" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "version $VERSION" "$SPEC_FILE" || \
  { echo "can't build librdkafka RPM" >&2 ; exit 1 ; }

# install librdkafka
cp $BIN_RPM_FOLDER/librdkafka*.rpm $RES_RPMS/


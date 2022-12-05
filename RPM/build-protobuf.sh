#!/bin/bash

VERSION=$1
#VERSION="3.21.8"
VERSIONSSV=ssv1

pushd `dirname $0` >/dev/null
SCRIPT_DIR=`pwd`
popd >/dev/null

PATH=$PATH:$SCRIPT_DIR

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
#sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm rpm-build libtool \
#  glib2-devel gcc-c++ epel-rpm-macros cmake3 \
#  || \
#  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`
RPM_SOURCES_DIR=`rpm -E %_sourcedir`

# prepare archive
BIN_RPM_FOLDER=`rpm -E '%_rpmdir/%_arch'`
SPEC_FILE=`rpm -E %_specdir`/protobuf-$VERSION-$VERSIONSSV.spec

#curl 'https://github.com/protocolbuffers/protobuf/archive/refs/tags/v$VERSION.tar.gz' >"$RPM_SOURCES_DIR/v$VERSION.tar.gz"

cat << 'EOF' > $SPEC_FILE
Summary:        Protocol Buffers - Google's data interchange format
Name:           protobuf
Version:        %{_version}
Release:        ssv1%{?dist}
License:        BSD
Group:          Development/Libraries
Source:         https://github.com/google/protobuf/archive/v%{version}.tar.gz
URL:            https://github.com/google/protobuf
BuildRequires:  automake autoconf libtool pkgconfig zlib-devel

%description
Protocol Buffers are a way of encoding structured data in an efficient
yet extensible format. Google uses Protocol Buffers for almost all of
its internal RPC protocols and file formats.

Protocol buffers are a flexible, efficient, automated mechanism for
serializing structured data – think XML, but smaller, faster, and
simpler. You define how you want your data to be structured once, then
you can use special generated source code to easily write and read
your structured data to and from a variety of data streams and using a
variety of languages. You can even update your data structure without
breaking deployed programs that are compiled against the "old" format.

%package compiler
Summary: Protocol Buffers compiler
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description compiler
This package contains Protocol Buffers compiler for all programming
languages

%package devel
Summary: Protocol Buffers C++ headers and libraries
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: %{name}-compiler = %{version}-%{release}
Requires: zlib-devel
Requires: pkgconfig

%description devel
This package contains Protocol Buffers compiler for all languages and
C++ headers and libraries

%package static
Summary: Static development files for %{name}
Group: Development/Libraries
Requires: %{name}-devel = %{version}-%{release}

%description static
Static libraries for Protocol Buffers

%package lite
Summary: Protocol Buffers LITE_RUNTIME libraries
Group: Development/Libraries

%description lite
Protocol Buffers built with optimize_for = LITE_RUNTIME.

The "optimize_for = LITE_RUNTIME" option causes the compiler to generate code
which only depends libprotobuf-lite, which is much smaller than libprotobuf but
lacks descriptors, reflection, and some other features.

%package lite-devel
Summary: Protocol Buffers LITE_RUNTIME development libraries
Requires: %{name}-devel = %{version}-%{release}
Requires: %{name}-lite = %{version}-%{release}

%description lite-devel
This package contains development libraries built with
optimize_for = LITE_RUNTIME.

The "optimize_for = LITE_RUNTIME" option causes the compiler to generate code
which only depends libprotobuf-lite, which is much smaller than libprotobuf but
lacks descriptors, reflection, and some other features.

%package lite-static
Summary: Static development files for %{name}-lite
Group: Development/Libraries
Requires: %{name}-devel = %{version}-%{release}

%description lite-static
This package contains static development libraries built with
optimize_for = LITE_RUNTIME.

The "optimize_for = LITE_RUNTIME" option causes the compiler to generate code
which only depends libprotobuf-lite, which is much smaller than libprotobuf but
lacks descriptors, reflection, and some other features.

%package python36
Summary: Python bindings for Google Protocol Buffers
Group: Development/Languages
BuildRequires: python3-devel python3-setuptools
Conflicts: %{name}-compiler > %{version}
Conflicts: %{name}-compiler < %{version}

%description python36
This package contains Python libraries for Google Protocol Buffers

%package python38
Summary: Python bindings for Google Protocol Buffers
Group: Development/Languages
BuildRequires: python38-devel python38-setuptools
Conflicts: %{name}-compiler > %{version}
Conflicts: %{name}-compiler < %{version}

%description python38
This package contains Python libraries for Google Protocol Buffers

%prep
%setup -q
chmod 644 examples/*

%build
iconv -f iso8859-1 -t utf-8 CONTRIBUTORS.txt > CONTRIBUTORS.txt.utf8
mv CONTRIBUTORS.txt.utf8 CONTRIBUTORS.txt
export PTHREAD_LIBS="-lpthread"
./autogen.sh
%configure

make %{?_smp_mflags}

#mkdir -p build/python36
#mkdir -p build/python38

pushd python
echo "================="
pwd
echo %{__python36}
echo "================="
%{__python36} ./setup.py build
%{__python38} ./setup.py build
echo ">================"
find build/ -type f
echo ">================"
sed -i -e 1d build/lib/google/protobuf/descriptor_pb2.py

popd

#%check
#make %{?_smp_mflags} check

%install
rm -rf %{buildroot}
make %{?_smp_mflags} install DESTDIR=%{buildroot} STRIPBINARIES=no INSTALL="%{__install} -p" CPPROG="cp -p"
find %{buildroot} -type f -name "*.la" -exec rm -f {} \;

pushd python
%{__python36} ./setup.py install --root=%{buildroot} --single-version-externally-managed --record=INSTALLED_FILES --optimize=1
%{__python38} ./setup.py install --root=%{buildroot} --single-version-externally-managed --record=INSTALLED_FILES --optimize=1
popd

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post lite -p /sbin/ldconfig
%postun lite -p /sbin/ldconfig

%post compiler -p /sbin/ldconfig
%postun compiler -p /sbin/ldconfig

%files
%{_libdir}/libprotobuf.so.*
%doc CHANGES.txt CONTRIBUTORS.txt README.md
%license LICENSE

%files compiler
%{_bindir}/protoc
%{_libdir}/libprotoc.so.*
%doc README.md
%license LICENSE

%files devel
%dir %{_includedir}/google
%{_includedir}/google/protobuf/
%{_libdir}/libprotobuf.so
%{_libdir}/libprotoc.so
%{_libdir}/pkgconfig/protobuf.pc

%files static
%{_libdir}/libprotobuf.a
%{_libdir}/libprotoc.a

%files lite
%{_libdir}/libprotobuf-lite.so.*

%files lite-devel
%{_libdir}/libprotobuf-lite.so
%{_libdir}/pkgconfig/protobuf-lite.pc

%files lite-static
%{_libdir}/libprotobuf-lite.a

%define python36_sitelib /usr/lib/python3.6/site-packages
%files python36
%dir %{python36_sitelib}/google
%{python36_sitelib}/google/protobuf/
%{python36_sitelib}/protobuf-*-py?.?.egg-info/
%{python36_sitelib}/protobuf-*-py?.?-nspkg.pth
%doc python/README.md
%doc examples/add_person.py examples/list_people.py examples/addressbook.proto

%define python38_sitelib /usr/lib/python3.8/site-packages
%files python38
%dir %{python38_sitelib}/google
%{python38_sitelib}/google/protobuf/
%{python38_sitelib}/protobuf-*-py?.?.egg-info/
%{python38_sitelib}/protobuf-*-py?.?-nspkg.pth
%doc python/README.md
%doc examples/add_person.py examples/list_people.py examples/addressbook.proto

EOF

PASS_ARGS=("--define" "_version $VERSION" "--define" "_release $VERSIONSSV" \
  "--define" "__python36 /usr/bin/python3.6" \
  "--define" "__python38 /usr/bin/python3.8" \
  )
#echo "ARGS: " $PASS_ARGS

$SUDO_PREFIX yum-builddep -y "${PASS_ARGS[@]}" "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

echo "to download source"

spectool --force -g -R "${PASS_ARGS[@]}" "$SPEC_FILE" || \
  { echo "can't download source RPM" >&2 ; exit 1 ; }

echo "to build rpm"

rpmbuild --force -ba "${PASS_ARGS[@]}" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }


cp $BIN_RPM_FOLDER/protobuf*.rpm $RES_RPMS/

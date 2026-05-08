#!/bin/bash

VERSION=8.10.2
RELEASE=ssv1

# script require 'sudo rpm' for install RPM packages
# and access to repository mirrors

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
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

# download and install packages required for build
sudo yum -y "${YUM_REPO_ARGS[@]}" install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake \
  libtool glib2-devel cmake gcc-c++ epel-rpm-macros \
  zlib-devel bzip2-devel snappy-devel lz4-devel libzstd-devel \
  liburing-devel boost176-devel \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp $(rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch')

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')
TOPLINGDB_SPEC_FILE=$(rpm -E %_specdir)/toplingdb.spec

cat << 'EOF_SPEC' > "$TOPLINGDB_SPEC_FILE"
Name:    toplingdb
Version: %{_version}
Release: %{_release}%{?dist}
Summary: ToplingDB - high performance kv storage based on RocksDB
Group:   Development/Libraries/C and C++
License: Apache-2.0
URL:     https://github.com/topling/toplingdb
BuildRequires: autoconf automake libtool curl make
BuildRequires: gcc-c++ cmake
BuildRequires: libaio-devel gcc-c++ gflags-devel zlib-devel bzip2-devel libcurl-devel liburing-devel
BuildRequires: snappy-devel jemalloc-devel boost176-devel
BuildRequires: gflags-devel
Requires: zlib libstdc++
Requires: jemalloc

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description
ToplingDB - high performance kv storage based on RocksDB.

%package -n %{name}-devel
Summary: ToplingDB development files
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
Requires: boost176-devel

%description -n %{name}-devel
ToplingDB development headers and CMake/pkg-config integration files.

%global toplingdb_rocksdb_namespace toplingdb
%global toplingdb_rocksdb_libname libtoplingdb_rocksdb

%prep
%setup -q -c -T -n %{name}-%{_version}
export GIT_TERMINAL_PROMPT=0
git -c http.lowSpeedLimit=1 -c http.lowSpeedTime=60 clone --depth 1 --branch topling-8.10.2-frocks-1.0 https://github.com/topling/toplingdb .
sed -i '/boost-include\/boost/d' Makefile
sed -i 's/\bgit pull\b/git status --short/g' Makefile
sed -i 's/^shared_lib: $(SHARED) dcompact_worker/shared_lib: $(SHARED)/' Makefile
git submodule update --init --recursive
mkdir -p sideplugin
for plugin in \
  cspp-memtable \
  cspp-wbwi \
  topling-sst \
  topling-zip \
  topling-zip_table_reader \
  toplingdb-fs \
  topling-dcompact; do
  if [ ! -d "sideplugin/${plugin}/.git" ]; then
    git -c http.lowSpeedLimit=1 -c http.lowSpeedTime=60 clone --depth 1 "https://github.com/topling/${plugin}" "sideplugin/${plugin}"
  else
    git -C "sideplugin/${plugin}" pull --ff-only
  fi
  git -C "sideplugin/${plugin}" submodule update --init --recursive || :
done
find . \
  \( -path './.git' -o -path './third-party' \) -prune -o \
  -type f \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.hh' \) \
  -exec sed -i \
    -e 's/\bnamespace rocksdb\b/namespace ROCKSDB_NAMESPACE/g' \
    -e 's/\brocksdb::/ROCKSDB_NAMESPACE::/g' \
    {} +

%build
echo "To make from $(pwd)"
export LIBRARY_PATH="%{_libdir}:/usr/lib64:/lib64${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="%{_libdir}:/usr/lib64:/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LDFLAGS="${LDFLAGS:-} -L%{_libdir} -L/usr/lib64 -L/lib64"
export EXTRA_LDFLAGS="${EXTRA_LDFLAGS:-} -L%{_libdir} -L/usr/lib64 -L/lib64"
#PORTABLE=1 make %{?_smp_mflags} shared_lib db_bench \
#  DISABLE_WARNING_AS_ERROR=1 DEBUG_LEVEL=0
PORTABLE=1 make shared_lib db_bench \
  DISABLE_WARNING_AS_ERROR=1 DEBUG_LEVEL=0 LINK_SHARED_LIBURING=1 WITH_TOPLING_ROCKS=0 MAKE_UNIT_TEST=0 V=1 \
  LIBNAME=%{toplingdb_rocksdb_libname} EXTRA_CXXFLAGS="-DROCKSDB_NAMESPACE=%{toplingdb_rocksdb_namespace}"

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr
export LIBRARY_PATH="%{_libdir}:/usr/lib64:/lib64${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="%{_libdir}:/usr/lib64:/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LDFLAGS="${LDFLAGS:-} -L%{_libdir} -L/usr/lib64 -L/lib64"
export EXTRA_LDFLAGS="${EXTRA_LDFLAGS:-} -L%{_libdir} -L/usr/lib64 -L/lib64"
DESTDIR=%{buildroot} PORTABLE=1 make install-dev-shared PREFIX=/usr \
  DISABLE_WARNING_AS_ERROR=1 DEBUG_LEVEL=0 LINK_SHARED_LIBURING=1 WITH_TOPLING_ROCKS=0 MAKE_UNIT_TEST=0 V=1 \
  LIBNAME=%{toplingdb_rocksdb_libname} EXTRA_CXXFLAGS="-DROCKSDB_NAMESPACE=%{toplingdb_rocksdb_namespace}"
rm -rf %{buildroot}%{_includedir}/boost
if [ -d %{buildroot}/usr/lib ] && [ "%{_libdir}" != "/usr/lib" ]; then
  mkdir -p %{buildroot}%{_libdir}
  mv %{buildroot}/usr/lib/* %{buildroot}%{_libdir}/
  rmdir %{buildroot}/usr/lib
fi
mkdir -p %{buildroot}%{_includedir}/toplingdb
if [ -d %{buildroot}%{_includedir}/rocksdb ]; then
  mv %{buildroot}%{_includedir}/rocksdb %{buildroot}%{_includedir}/toplingdb/
fi
if [ -d %{buildroot}%{_includedir}/terark ]; then
  mv %{buildroot}%{_includedir}/terark %{buildroot}%{_includedir}/toplingdb/
fi
if [ -d %{buildroot}%{_includedir}/topling ]; then
  mv %{buildroot}%{_includedir}/topling %{buildroot}%{_includedir}/toplingdb/
fi
mkdir -p %{buildroot}%{_libdir}/toplingdb
find %{buildroot}%{_libdir} -maxdepth 1 -name '%{toplingdb_rocksdb_libname}.so*' \
  -exec mv -t %{buildroot}%{_libdir}/toplingdb {} +
rm -f %{buildroot}%{_libdir}/pkgconfig/rocksdb.pc
mkdir -p %{buildroot}%{_libdir}/pkgconfig
cat > %{buildroot}%{_libdir}/pkgconfig/toplingdb.pc << EOF
prefix=/usr
exec_prefix=\${prefix}
libdir=%{_libdir}/toplingdb
includedir=%{_includedir}/toplingdb

Name: ToplingDB
Description: ToplingDB RocksDB-compatible storage engine
Version: %{version}
Libs: -L\${libdir} -Wl,-rpath,\${libdir} -ltoplingdb_rocksdb -ljemalloc
Cflags: -I\${includedir} -DROCKSDB_NAMESPACE=%{toplingdb_rocksdb_namespace}
EOF
mkdir -p %{buildroot}%{_libdir}/cmake/toplingdb
cat > %{buildroot}%{_libdir}/cmake/toplingdb/ToplingDBConfig.cmake << EOF
include(CMakeFindDependencyMacro)

find_library(ToplingDB_JEMALLOC_LIBRARY NAMES jemalloc REQUIRED)

add_library(ToplingDB::rocksdb SHARED IMPORTED)
set_target_properties(ToplingDB::rocksdb PROPERTIES
  IMPORTED_LOCATION "%{_libdir}/toplingdb/%{toplingdb_rocksdb_libname}.so"
  INTERFACE_INCLUDE_DIRECTORIES "%{_includedir}/toplingdb"
  INTERFACE_COMPILE_DEFINITIONS "ROCKSDB_NAMESPACE=%{toplingdb_rocksdb_namespace}"
  INTERFACE_LINK_LIBRARIES "\${ToplingDB_JEMALLOC_LIBRARY}"
  INTERFACE_LINK_OPTIONS "-Wl,-rpath,%{_libdir}/toplingdb"
)
EOF
cat > %{buildroot}%{_libdir}/cmake/toplingdb/ToplingDBConfigVersion.cmake << EOF
set(PACKAGE_VERSION "%{version}")
if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
endif()
if(PACKAGE_FIND_VERSION VERSION_LESS_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
EOF

%clean
rm -rf %{buildroot}

%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
%dir %{_libdir}/toplingdb
%{_libdir}/toplingdb/*.so*
%dir /usr/site
/usr/site/index.html
/usr/site/style.css
%dir /usr/toplingdb-conf
/usr/toplingdb-conf/db_bench_enterprise.yaml

%files -n %{name}-devel
%defattr(-,root,root)
%dir %{_includedir}/toplingdb
%{_includedir}/toplingdb/*
%defattr(444,root,root)
%{_libdir}/pkgconfig/toplingdb.pc
%{_libdir}/cmake/toplingdb
EOF_SPEC

$SUDO_PREFIX yum-builddep -y "${YUM_REPO_ARGS[@]}" --define "_version $VERSION" --define "_release $RELEASE" "$TOPLINGDB_SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" --define "_release $RELEASE" "$TOPLINGDB_SPEC_FILE" || \
  { echo "can't build toplingdb RPM" >&2 ; exit 1 ; }

cp "$BIN_RPM_FOLDER"/toplingdb*.rpm "$RES_RPMS"/

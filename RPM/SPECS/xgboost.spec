%include %{_sourcedir}/foros-cxx-abi.macros
%{!?xgboost_version:%global xgboost_version 2.0.2}
%{!?xgboost_release:%global xgboost_release ssv5}

Name: xgboost
Version: %{xgboost_version}
Release: %{xgboost_release}%{?dist}
Summary: Gradient boosting library
License: Apache-2.0
URL: https://github.com/dmlc/xgboost
Source0: xgboost-%{version}.tar.gz
Source1: foros-cxx-abi.macros
Source2: xgboost-filesystem-provider.cpp
Source3: xgboost-abi-smoke.cpp
Source4: xgboost-devel-smoke.cpp
BuildRequires: gcc-toolset-%{foros_gcc_toolset}-gcc-c++
BuildRequires: cmake >= 3.18
BuildRequires: make
Provides: xgboost(cxx-abi)%{?_isa} = %{foros_cxx_abi}

%description
XGBoost built with the server C++ ABI profile.

%package devel
Summary: Development files for XGBoost
Requires: %{name}%{?_isa} = %{version}-%{release}
Provides: xgboost-devel(cxx-abi)%{?_isa} = %{foros_cxx_abi}

%description devel
Headers and build configuration for XGBoost, including the internal headers used by the server.

%prep
%setup -q
# dmlc-core otherwise overrides the parent C++ standard with C++11/14.
sed -i -E 's/set\(CMAKE_CXX_STANDARD (11|14)\)/set(CMAKE_CXX_STANDARD %{foros_cxx_standard})/' \
  dmlc-core/CMakeLists.txt
# XGBoost also pins the standard per target, overriding CMAKE_CXX_STANDARD.
sed -i 's/CXX_STANDARD 17/CXX_STANDARD %{foros_cxx_standard}/' cmake/Utils.cmake

# These serialization literals contain only ASCII; C++20 u8 literals have type char8_t.
sed -i 's/u8"/"/g' src/common/json.cc src/common/version.cc src/common/charconv.cc \
  src/data/data.cc src/learner.cc

%build
export CC=%{foros_toolchain_root}/bin/gcc
export CXX=%{foros_toolchain_root}/bin/g++
# GCC 10 crashes while diagnosing deprecated template declarations in C++20.
abi_flags="-D_GLIBCXX_USE_CXX11_ABI=%{foros_cxx11_abi}"
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DCMAKE_INSTALL_LIBDIR=%{_lib} \
  -DCMAKE_CXX_STANDARD=%{foros_cxx_standard} \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_CXX_EXTENSIONS=OFF \
  -DCMAKE_CXX_FLAGS="$abi_flags -Wno-deprecated-declarations" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DUSE_CUDA=OFF
cmake --build build -- %{_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build
for dir in common data c_api gbm; do
  mkdir -p %{buildroot}%{_includedir}/xgboost/$dir
  cp src/$dir/*.h %{buildroot}%{_includedir}/xgboost/$dir/
done
cp -a rabit/include/rabit %{buildroot}%{_includedir}/

%check
# Load a filesystem-using DSO before XGBoost, reproducing the server's symbol interposition.
CXX=%{foros_toolchain_root}/bin/g++
flags="-std=c++%{foros_cxx_standard} -D_GLIBCXX_USE_CXX11_ABI=%{foros_cxx11_abi}"
$CXX $flags -fPIC -shared %{SOURCE2} -o build/libxgboost-filesystem-provider.so
$CXX $flags -Wno-deprecated-declarations -fsyntax-only \
  -I%{buildroot}%{_includedir} %{SOURCE4}
$CXX $flags %{SOURCE3} -I%{buildroot}%{_includedir} -Lbuild -lxgboost-filesystem-provider \
  -L%{buildroot}%{_libdir} -lxgboost -Wl,-rpath,'$ORIGIN' -o build/xgboost-abi-smoke
LD_LIBRARY_PATH=%{buildroot}%{_libdir}:"$PWD/build" build/xgboost-abi-smoke

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%{_libdir}/libxgboost.so
%{_bindir}/xgboost

%files devel
%{_includedir}/xgboost
%{_includedir}/dmlc
%{_includedir}/rabit
%{_libdir}/libdmlc.a
%{_libdir}/cmake/dmlc
%{_libdir}/cmake/xgboost
%{_libdir}/pkgconfig/xgboost.pc

%global debug_package %{nil}

%global __strip /bin/true
%global debug_package %{nil}

Name: foros-python3.12-torch
Version: %{torch_version}
Release: %{torch_variant}.ssv3%{?dist}
Summary: Isolated PyTorch runtime for Foros AI services
License: BSD-3-Clause
Group: Development/Libraries
Source0: python3.12-torch-site-packages.tar.zst
BuildArch: x86_64
AutoReqProv: no

BuildRequires: tar
BuildRequires: zstd
Requires: glibc
Requires: libgcc
Requires: libstdc++
Requires: python3.12
Provides: python3.12dist(torch) = %{torch_version}

%description
PyTorch and its Python and CUDA wheel dependencies installed into an isolated
site-packages directory for Foros AI services.

%prep

%build

%install
rm -rf %{buildroot}
install -d \
  %{buildroot}/u01/foros/server-ai/python3.12-torch/site-packages
tar --use-compress-program=unzstd \
  -xf %{SOURCE0} \
  -C %{buildroot}/u01/foros/server-ai/python3.12-torch/site-packages

%files
%defattr(-, root, root)
/u01/foros/server-ai/python3.12-torch

%clean
rm -rf %{buildroot}

%changelog

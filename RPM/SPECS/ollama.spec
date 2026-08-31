%global debug_package %{nil}

Name: foros-ollama
Version: %{ollama_version}
Release: ssv1%{?dist}
Summary: Ollama runtime for Foros AI services
License: MIT
Group: Applications/System
Source0: ollama-linux-amd64.tar.zst
BuildArch: x86_64
AutoReqProv: no

BuildRequires: tar
BuildRequires: zstd
Requires: glibc
Requires: libgcc
Requires: libstdc++

%description
Pinned Ollama runtime and bundled accelerator libraries for Foros AI services.

%prep

%build

%install
rm -rf %{buildroot}
install -d %{buildroot}/opt/foros/ollama
tar --use-compress-program=unzstd \
  -xf %{SOURCE0} \
  -C %{buildroot}/opt/foros/ollama
test -x %{buildroot}/opt/foros/ollama/bin/ollama

%files
%defattr(-, root, root)
/opt/foros/ollama/bin
/opt/foros/ollama/lib

%clean
rm -rf %{buildroot}

%changelog

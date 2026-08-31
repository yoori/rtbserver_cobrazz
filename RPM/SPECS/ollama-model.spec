%global debug_package %{nil}

Name: foros-ollama-model-%{model_package_suffix}
Version: %{version}
Release: ssv2%{?dist}
Summary: Ollama %{model} model for Foros AIAgent
License: Commercial
Group: Applications/System
Source0: ollama-model.tar.zst
Source1: ollama-model.env
Source2: ollama-model.sha256
BuildArch: noarch
AutoReqProv: no

BuildRequires: tar
BuildRequires: zstd
Requires: foros-ollama >= %{ollama_version}

%description
The %{model} model downloaded and verified during the package build. Runtime
hosts do not download or modify the model.

%prep

%build

%install
rm -rf %{buildroot}
install -d \
  %{buildroot}/u01/foros/server-ai/models/ollama/%{model_package_suffix}
tar --use-compress-program=unzstd \
  -xf %{SOURCE0} \
  -C %{buildroot}/u01/foros/server-ai/models/ollama/%{model_package_suffix}
install -m 0644 %{SOURCE1} \
  %{buildroot}/u01/foros/server-ai/models/ollama/%{model_package_suffix}/model.env
install -m 0644 %{SOURCE2} \
  %{buildroot}/u01/foros/server-ai/models/ollama/%{model_package_suffix}/model.sha256

%files
%defattr(-, root, root)
/u01/foros/server-ai/models/ollama/%{model_package_suffix}

%clean
rm -rf %{buildroot}

%changelog

Name: foros-server-ai
Version: %{version}
Release: ssv4%{?dist}
Summary: Foros AI services
License: Commercial
Group: System Environment/Daemons
Source0: foros-server-ai-%{version}.tar.gz
AutoReqProv: no

BuildRequires: gcc-c++
Requires: bash
Requires: coreutils
Requires: foros-ollama >= 0.33.2
Requires: perl-interpreter
Requires: python3.12
Requires: python3.12dist(numpy)
Requires: python3.12dist(torch)
Requires: util-linux

%description
Foros AI service scripts and DACS control modules. GPU and Python dependencies
are intentionally isolated from the main Foros server package.

%prep
%setup -q

%build
g++ \
  %{optflags} \
  -std=c++17 \
  -DAI_PACKAGE_VERSION='"%{version}-%{release}"' \
  AI/src/AIPackageInfo.cpp \
  -o AIPackageInfo

%install
rm -rf %{buildroot}
install -d %{buildroot}/opt/foros/server-ai/bin
install -d %{buildroot}/opt/foros/server-ai/conf
install -d %{buildroot}/opt/foros/server-ai/DACS/AICluster
install -d %{buildroot}/opt/foros/server-ai/lib/segment_model
install -m 0755 AIPackageInfo %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/aiserver.pl %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/AIAgent.sh %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentGenerator.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelEvaluate.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelExtract.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelScenarioSeed.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelScenarioServer.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelScenarioTrain.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0755 AI/bin/SegmentModelTrain.py %{buildroot}/opt/foros/server-ai/bin/
install -m 0644 AI/conf/SegmentModelConfig.example.json %{buildroot}/opt/foros/server-ai/conf/
install -m 0644 AI/lib/segment_model/*.py %{buildroot}/opt/foros/server-ai/lib/segment_model/
install -m 0644 AI/lib/segment_model/README.md %{buildroot}/opt/foros/server-ai/lib/segment_model/
install -m 0644 DACS/AICluster/*.pm %{buildroot}/opt/foros/server-ai/DACS/AICluster/

%files
%defattr(-, root, root)
/opt/foros/server-ai

%changelog

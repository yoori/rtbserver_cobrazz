%{!?telegraf_upstream_release:%global telegraf_upstream_release 1}
%global _build_id_links none

Name:           foros-telegraf
Version:        %{version}
Release:        ssv1%{?dist}
Summary:        Telegraf binary for Foros-managed cluster instances
License:        MIT
URL:            https://github.com/influxdata/telegraf
Source0:        telegraf-%{version}-%{telegraf_upstream_release}.x86_64.rpm
BuildRequires:  cpio
ExclusiveArch:  x86_64
AutoReq:        no

%description
Static Telegraf binary used by Foros-managed cluster instances. This package
does not install a systemd unit or configuration under /etc.

%prep
rm -rf telegraf
mkdir telegraf
rpm2cpio %{SOURCE0} | (cd telegraf && cpio -id --quiet ./usr/bin/telegraf)

%install
rm -rf %{buildroot}
install -Dpm 0755 telegraf/usr/bin/telegraf \
  %{buildroot}/opt/foros/server/bin/telegraf

%files
%defattr(-, root, root)
/opt/foros/server/bin/telegraf

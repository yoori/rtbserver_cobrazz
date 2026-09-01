%global __strip /bin/true
%global debug_package %{nil}

Name: foros-playwright-mcp
Version: %{playwright_mcp_version}
Release: ssv1%{?dist}
Summary: Playwright MCP browser runtime for Foros AI services
License: Apache-2.0 and MIT and BSD
Group: Applications/System
Source0: playwright-mcp-runtime.tar.zst
Source1: playwright-mcp
BuildArch: x86_64
AutoReqProv: no

BuildRequires: tar
BuildRequires: zstd
Requires: alsa-lib
Requires: at-spi2-atk
Requires: at-spi2-core
Requires: atk
Requires: bash
Requires: dbus-libs
Requires: expat
Requires: glib2
Requires: glibc
Requires: libX11
Requires: libXcomposite
Requires: libXdamage
Requires: libXext
Requires: libXfixes
Requires: libXrandr
Requires: libgcc
Requires: libstdc++
Requires: libxcb
Requires: libxkbcommon
Requires: mesa-libgbm
Requires: nspr
Requires: nss
Requires: systemd-libs

%description
Pinned Playwright MCP server with a private Node.js runtime and the matching
headless Chromium build. No packages are downloaded on the target host.

%prep

%build

%install
rm -rf %{buildroot}
install -d %{buildroot}/opt/foros/playwright-mcp/bin
install -m 0755 %{SOURCE1} \
  %{buildroot}/opt/foros/playwright-mcp/bin/playwright-mcp
tar --use-compress-program=unzstd \
  -xf %{SOURCE0} \
  -C %{buildroot}/opt/foros/playwright-mcp
test -x %{buildroot}/opt/foros/playwright-mcp/node/bin/node
test -r \
  %{buildroot}/opt/foros/playwright-mcp/lib/node_modules/@playwright/mcp/cli.js

%files
%defattr(-, root, root)
/opt/foros/playwright-mcp

%clean
rm -rf %{buildroot}

%changelog

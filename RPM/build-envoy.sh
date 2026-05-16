#!/usr/bin/env bash
set -euo pipefail

ENVOY_VERSION="${ENVOY_VERSION:-1.23.12}"
ENVOY_SHA256="${ENVOY_SHA256:-9395a202982185fe907cafb336568e3d418bb4057158aa32c4dc328743205731}"
ENVOY_URL="${ENVOY_URL:-https://github.com/envoyproxy/envoy/releases/download/v${ENVOY_VERSION}/envoy-${ENVOY_VERSION}-linux-x86_64}"

workdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
topdir="${workdir}/.rpmbuild/envoy-${ENVOY_VERSION}"
binary_name="envoy-${ENVOY_VERSION}-linux-x86_64"

rm -rf "${topdir}"
mkdir -p "${topdir}"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

curl --fail --location --show-error --output "${topdir}/SOURCES/${binary_name}" "${ENVOY_URL}"
printf '%s  %s\n' "${ENVOY_SHA256}" "${topdir}/SOURCES/${binary_name}" | sha256sum --check -
chmod 0755 "${topdir}/SOURCES/${binary_name}"

cat > "${topdir}/SPECS/envoy.spec" <<SPEC
Name: envoy
Version: ${ENVOY_VERSION}
Release: 1%{?dist}
Summary: Envoy cloud-native high-performance edge and service proxy
License: Apache-2.0
URL: https://www.envoyproxy.io/
Source0: ${binary_name}
BuildArch: x86_64
AutoReqProv: no

%global debug_package %{nil}

%description
Envoy is a cloud-native high-performance edge, middle, and service proxy.
This RPM packages the official upstream Linux x86_64 release binary.

%prep

%build

%install
mkdir -p %{buildroot}%{_bindir}
install -m 0755 %{SOURCE0} %{buildroot}%{_bindir}/envoy

%files
%{_bindir}/envoy

%changelog
* Thu May 14 2026 Local Build <root@localhost> - ${ENVOY_VERSION}-1
- Package official upstream Envoy ${ENVOY_VERSION} Linux x86_64 release binary.
SPEC

rpmbuild --define "_topdir ${topdir}" -bb "${topdir}/SPECS/envoy.spec"

find "${topdir}/RPMS" -type f -name '*.rpm' -print

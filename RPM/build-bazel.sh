#!/bin/bash

VERSION=${1:-7.2.1}
VERSIONSSV=ssv1

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake libtool \
  glib2-devel cmake gcc-c++ epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install java-21-openjdk-devel zip unzip which python3 || \
  { echo "can't install bazel build packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

SPEC_FILE=$(rpm -E %_specdir)/bazel-$VERSION-$VERSIONSSV.spec

cat << 'EOF_SPEC' > $SPEC_FILE
%global debug_package %{nil}
%undefine _debugsource_packages
Name:    bazel
Version: %{_version}
Release: ssv1%{?dist}
Summary: Fast, scalable, multi-language and extensible build system
Group:   Development/Tools
License: Apache-2.0
URL:     https://bazel.build
Source0: https://github.com/bazelbuild/bazel/releases/download/%{_version}/bazel-%{_version}-dist.zip
BuildRequires: gcc-c++
BuildRequires: java-21-openjdk-devel
BuildRequires: zip
BuildRequires: unzip
BuildRequires: which
BuildRequires: python3

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description
Bazel is Google's open source build system used to build software quickly and reliably.

%prep
%setup -q -c -T
unzip -q %{SOURCE0}

%build
export JAVA_HOME=/usr/lib/jvm/java-21-openjdk
export EXTRA_BAZEL_ARGS="--host_javabase=@local_jdk//:jdk"
./compile.sh

%install
rm -rf %{buildroot}
install -D -m 0755 output/bazel %{buildroot}%{_bindir}/bazel
if [ -f scripts/bazel-complete-template.bash ]; then
  install -D -m 0644 scripts/bazel-complete-template.bash \
    %{buildroot}%{_sysconfdir}/bash_completion.d/bazel
else
  install -d %{buildroot}%{_sysconfdir}/bash_completion.d
  : > %{buildroot}%{_sysconfdir}/bash_completion.d/bazel
fi

%clean
rm -rf %{buildroot}

%files
%{_bindir}/bazel
%config(noreplace) %{_sysconfdir}/bash_completion.d/bazel
EOF_SPEC

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

python3.11 -m venv bazel_build_env
source bazel_build_env/bin/activate

pip3 install pkg

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/bazel-*$VERSION-*.rpm $RES_RPMS/

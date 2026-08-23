#!/bin/bash

set -euo pipefail

FASTAPI_VERSION="${FASTAPI_VERSION:-0.141.1}"
UVICORN_VERSION="${UVICORN_VERSION:-0.52.1}"
RELEASE="${RELEASE:-2}"
TOPDIR="${TMPDIR:-/tmp}/fastapi-rpmbuild"
OUTPUT_DIR="${OUTPUT_DIR:-$(pwd)/build/RPMS}"
PACKAGE_NAME="python3.12-fastapi-uvicorn"

rm -rf "$TOPDIR"
mkdir -p "$TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

python3.12 -m venv "$TOPDIR/venv"
"$TOPDIR/venv/bin/python" -m pip install \
  --disable-pip-version-check \
  --no-cache-dir \
  --no-compile \
  --only-binary=:all: \
  --target "$TOPDIR/site-packages" \
  "fastapi==$FASTAPI_VERSION" \
  "uvicorn==$UVICORN_VERSION" \
  "annotated-doc==0.0.5" \
  "annotated-types==0.8.0" \
  "anyio==4.14.2" \
  "click==8.4.2" \
  "h11==0.16.0" \
  "idna==3.19" \
  "pydantic==2.13.4" \
  "pydantic-core==2.46.4" \
  "starlette==1.6.0" \
  "typing-extensions==4.16.0" \
  "typing-inspection==0.4.4"

rm -rf \
  "$TOPDIR/site-packages/idna" \
  "$TOPDIR/site-packages/idna-"*.dist-info \
  "$TOPDIR/site-packages/typing_extensions.py" \
  "$TOPDIR/site-packages/typing_extensions-"*.dist-info

find "$TOPDIR/site-packages" -type d -name __pycache__ -prune -exec rm -rf {} +
tar -C "$TOPDIR" -czf "$TOPDIR/SOURCES/site-packages.tar.gz" site-packages

cat >"$TOPDIR/SPECS/$PACKAGE_NAME.spec" <<EOF
Name: $PACKAGE_NAME
Version: $FASTAPI_VERSION
Release: $RELEASE
Summary: FastAPI and Uvicorn runtime for Python 3.12
License: MIT AND BSD-3-Clause
Source0: site-packages.tar.gz
BuildArch: x86_64
Requires: python(abi) = 3.12
Requires: python3.12-idna
Requires: python3.12-typing-extensions
AutoReqProv: no
%global debug_package %{nil}

%description
FastAPI, Uvicorn and their Python runtime dependencies bundled for Python 3.12.

%prep
%setup -q -c -T

%build

%install
mkdir -p %{buildroot}/usr/lib/python3.12/site-packages
tar -C %{buildroot}/usr/lib/python3.12/site-packages \
  --strip-components=1 -xzf %{SOURCE0}

%files
/usr/lib/python3.12/site-packages/*
EOF

rpmbuild \
  --define "_topdir $TOPDIR" \
  -bb "$TOPDIR/SPECS/$PACKAGE_NAME.spec"

mkdir -p "$OUTPUT_DIR"
cp "$TOPDIR"/RPMS/x86_64/"$PACKAGE_NAME"-*.rpm "$OUTPUT_DIR/"

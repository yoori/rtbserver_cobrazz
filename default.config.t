#if DEBUG
#  define OPT_DEBUG_INFO -ggdb3 -DDEV_DEBUG -rdynamic
#  define OPT_OPTIMIZATION -O0 -fno-inline
#else
#  define OPT_DEBUG_INFO -g
#  define OPT_OPTIMIZATION -O3
#endif

#if WERROR
#  define WARN_FLAGS -W -Wall -fno-strict-aliasing
#else
#  define WARN_FLAGS -W -Wall -fno-strict-aliasing
#endif

#define COMMON_FLAGS -pthread WARN_FLAGS

#ifdef DEV

#if !defined(UNIXCOMMONSROOT)
#error UNIXCOMMONSROOT is undefined!
#else
#define UNIXCOMMONS_ROOT UNIXCOMMONSROOT
#define UNIXCOMMONS_DEF UNIXCOMMONS_ROOT/build/libdefs
#define UNIXCOMMONS_INCLUDE src build/src
#define UNIXCOMMONS_CORBA_INCLUDE src/CORBA build/src/CORBA
#define UNIXCOMMONS_LIB build/lib
#endif

#else

#ifndef UNIXCOMMONS_ROOT
#error UNIXCOMMONS_ROOT is undefined!
#endif

#ifndef UNIXCOMMONS_INCLUDE
#define UNIXCOMMONS_INCLUDE include
#endif

#ifndef UNIXCOMMONS_CORBA_INCLUDE
#define UNIXCOMMONS_CORBA_INCLUDE src/CORBA build/src/CORBA
#endif

#ifndef UNIXCOMMONS_LIB
#define UNIXCOMMONS_LIB build/lib
#endif

#ifndef UNIXCOMMONS_DEF
#define UNIXCOMMONS_DEF UNIXCOMMONS_ROOT/build/libdefs
#endif

#endif

build_tests_status=build_tests

unixcommons_idls_path = `uc=UNIXCOMMONS_ROOT; for dir in UNIXCOMMONS_CORBA_INCLUDE; do echo -n " $uc/$dir"; done`

cxx.id = GCC

#ifdef CACHE
cxx.cxx = ccache g++
#else
cxx.cxx = g++
#endif

cxx.def.path = /usr/share/OpenSBE/defs UNIXCOMMONS_DEF
cxx.cpp_flags = -std=c++2a -D_REENTRANT -fPIC -DPIC -m64 -march=x86-64
cxx.flags = COMMON_FLAGS OPT_DEBUG_INFO OPT_OPTIMIZATION
cxx.obj.ext = .o

cxx.ld.flags = COMMON_FLAGS OPT_DEBUG_INFO OPT_OPTIMIZATION -m64 -march=x86-64
cxx.ld.libs =
cxx.so.ld = g++ -shared
cxx.so.primary.target.templ = lib%.so
cxx.so.secondary.target.templ =
cxx.ex.ld = g++
cxx.ex.target.templ = %

cxx.corba.orb.id = TAO
cxx.corba.orb.dir = /none
cxx.corba.orb.idl = /usr/bin/tao_idl
cxx.corba.cppflags = -I.
cxx.corba.cxxflags =
cxx.corba.ldflags =
cxx.corba.skelsuffix = _s
cxx.corba.stubsuffix =
cxx.corba.idlppflags = `uc=UNIXCOMMONS_ROOT; for dir in UNIXCOMMONS_CORBA_INCLUDE; do echo -n " -I$uc/$dir"; done`
cxx.corba.extraidlflags =

cxx.xml.xsd.id = XSD
cxx.xml.xsd.dir = /none
cxx.xml.xsd.translator = /usr/bin/xsdcxx
cxx.xml.xsd.cppflags = -DXSD_CXX11
cxx.xml.xsd.cxxflags =
cxx.xml.xsd.ldflags =
cxx.xml.xsd.extra_translator_flags = --std c++11

cxx.dr.protobuf.id = ProtoBuf
cxx.dr.protobuf.dir = /none
cxx.dr.protobuf.translator = /usr/bin/protoc
cxx.dr.protobuf.cppflags =
cxx.dr.protobuf.cxxflags = 
cxx.dr.protobuf.ldflags = 
cxx.dr.protobuf.flags = 

documentation.doxygen.path = doxygen

# // 3rd party libraries: alphabetic order
//external.lib.ace.root = /usr
external.lib.apache.root = /usr
external.lib.apr.root = /usr
//external.lib.bzip2.root = /usr
//external.lib.crypto.root = /usr
//external.lib.dynloader.root = /usr
//external.lib.event.root = /usr
//external.lib.geoip.root = /usr
//external.lib.gzip.root = /usr
external.lib.java.root = /usr/lib/jvm/java
external.lib.klt.root = /opt/KLT
//external.lib.mecab.root = /usr
external.lib.moran.root = /opt/Moran
//external.lib.netsnmp.root = /usr
//external.lib.openssl.root = /usr
//external.lib.pcre.root = /usr
//external.lib.rpm.root = /usr
//external.lib.sensors.root = /usr
//external.lib.tcpwrappers.root = /usr
//external.lib.tokyocabinet.root = /usr
//external.lib.xerces.root = /usr
external.lib.xml.root = /usr
external.lib.xml.cxx.lib =
//external.lib.xslt.root = /usr
external.lib.postgresql.root = /usr/pgsql-9.4
external.lib.postgresql.cxx.lib = /usr/pgsql-9.4/lib

# // UnixCommons party libraries: order they appear in config.log
unixcommons_root = UNIXCOMMONS_ROOT
unixcommons_include = UNIXCOMMONS_INCLUDE
unixcommons_corba_include = UNIXCOMMONS_CORBA_INCLUDE
unixcommons_lib = UNIXCOMMONS_LIB

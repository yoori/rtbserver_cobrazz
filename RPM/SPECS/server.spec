%if %{?__target:1}%{!?__target:0}
%define _target_cpu             %{__target}
%endif
%if %{?__server_type:0}%{!?__server_type:1}
%define _target_cpu             noarch
%endif

%ifnarch noarch
%define __type                  %{?__server_type:%__server_type}%{!?__server_type:remote}
%endif

%define __product               server
%define __ob_ver_req            4.3.2
%define __inst_root             /opt/foros/%{__product}
%define __osbe_build_dir        .

%define __cms_plugin_name       %{__product}-plugin-%{version}
%define __cms_plugin_src_dir    %{__product}/CMS/Plugin
%define __cms_plugin_dst_dir    /opt/cms/var/spool

%define __apache_ver_req        1:2.2.21.4
%define __ace_ver_req           6.2.1.17
%define __glibc_ver_req         2.28
%define __geoip_ver_req         1.6.12-7
%define __net_snmp_ver_req      5.8-25
%define __libevent_ver_req      2.1.8-5.el8
%define __postgresql_ver_req    9.4.26
#define __protobuf_ver_req      3.6.1-4
%define __protobuf_ver_req      3.21.9
%define __zeromq_ver_req        4.3.4
%define __librdkafka_ver_req    1.9.2
#define __open_ssl_ver_req      1.0.1e-42.el7
%define __open_ssl_ver_req      1.1.1k-7
%define __vanga_ver_req         1.0.0.21
%define __rocksdb_ver_req       8.8.1
%define __boost_suffix          176

Name:    foros-server%{?__type:-%__type}
Version: %{version}
Release: ssv427%{?dist}
Summary: Advertizing Server
License: Commercial
Group:   System Environment/Daemons

BuildRoot: %{_tmppath}/%{name}-buildroot
Source0: foros-server-%{version}.tar.gz
AutoReq: no

%ifnarch noarch

Requires: bash

BuildRequires: catboost-devel
Requires: catboost
BuildRequires: cmake3
BuildRequires: libstdc++
Requires: libstdc++
#BuildRequires: glibc-devel
BuildRequires: glibc-devel = %__glibc_ver_req
#Requires: glibc
Requires: glibc = %__glibc_ver_req

BuildRequires: ace-tao-devel >= %__ace_ver_req
Requires: ace-tao = %__ace_ver_req
BuildRequires: apr-devel >= 1.6.3-12
BuildRequires: autoconf >= 2.63
BuildRequires: gcc-c++
BuildRequires: GeoIP-devel >= %__geoip_ver_req
Requires: GeoIP >= %__geoip_ver_req
Requires: foros-geoip >= 1.0.0.0
#BuildRequires: httpd-devel >= {__apache_ver_req}
#BuildRequires: httpd-devel
#Requires: httpd = {__apache_ver_req}
Requires: httpd
Requires: nginx = 1:1.22.1
#Requires: mod_ssl >= 2.2.21.4-1.ssv1
BuildRequires: libevent-devel = 2.1.8
Requires: libevent = %__libevent_ver_req
%if "%{?__type:%{__type}}%{!?__type:0}" == "central"
Conflicts: foros-server-remote-debuginfo
Conflicts: foros-server-central-debuginfo < %{version}
Conflicts: foros-server-central-debuginfo > %{version}
%else
Conflicts: foros-server-central-debuginfo
Conflicts: foros-server-remote-debuginfo < %{version}
Conflicts: foros-server-remote-debuginfo > %{version}
%endif
%if "%{?buildType}" == "nb" 
BuildRequires: valgrind-devel
%endif
BuildRequires: pcre-devel >= 8.42
#BuildRequires: prelink
BuildRequires: selinux-policy
BuildRequires: xerces-c-devel
BuildRequires: xsd >= 4.1.0
BuildRequires: net-snmp-devel >= %{__net_snmp_ver_req} lm_sensors-libs
Requires: net-snmp >= %{__net_snmp_ver_req} lm_sensors-libs
#Requires: net-snmp-subagent >= 2.1.3.1
#BuildRequires: java-1.8.0-oracle-devel >= 1.7.0.45
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: libxml2-devel = 2.9.7
BuildRequires: libxslt-devel = 1.1.32
BuildRequires: make >= 4.2.1
BuildRequires: openssl-devel >= %{__open_ssl_ver_req}
BuildRequires: zlib-devel >= 1.2.11-18
Requires: zlib >= 1.2.11-18
BuildRequires: bzip2-devel
BuildRequires: flex >= 2.6.1-9
BuildRequires: bison >= 3.0.4-10
BuildRequires: zeromq-devel = %__zeromq_ver_req
BuildRequires: librdkafka-devel = %__librdkafka_ver_req
BuildRequires: vanga-devel = %__vanga_ver_req
BuildRequires: rocksdb-devel = %__rocksdb_ver_req
Requires: postgresql94-libs >= %{__postgresql_ver_req}
Requires: openssl >= %{__open_ssl_ver_req}
BuildRequires: postgresql94-devel >= %{__postgresql_ver_req}
BuildRequires: protobuf-devel = %{__protobuf_ver_req}
BuildRequires: protobuf-compiler = %{__protobuf_ver_req}
BuildRequires: userver-devel = 1.0.29
# userver-devel dependencies workaround:
BuildRequires: libev-devel yaml-cpp-devel cryptopp-devel libpq-devel http-parser-devel
BuildRequires: c-ares-devel >= 1.18.1
BuildRequires: grpc-plugins = 1.48.1-ssv2
BuildRequires: jemalloc-devel >= 5.2.1

Requires: protobuf = %{__protobuf_ver_req}
Requires: foros-polyglot-dict >= 1.0.0.15-ssv1.el5
Requires: rsync >= 3.0.7-3.el6
Requires: stunnel >= 4.29
Requires: perl-Template-Toolkit
#Requires: perl-Path-Iterator-Rule
Requires: perl-List-BinarySearch
Requires: perl-List-BinarySearch-XS
Requires: perl-Proc-Daemon
Requires: perl-Log-Dispatch
Requires: perl-Log-Dispatch-FileRotate
Requires: perl-Specio
Requires: perl-Hash-MultiKey perl-Digest-CRC perl-open

Requires: foros-dictionaries
Requires: zeromq = %__zeromq_ver_req
Requires: librdkafka1 = %__librdkafka_ver_req
Requires: vanga = %__vanga_ver_req
Requires: rocksdb = %__rocksdb_ver_req
Requires: gflags = 2.1.2
Requires: userver = 1.0.29
Requires: cryptopp libatomic libev

Requires: perl-B-Hooks-EndOfScope perl-Class-Data-Inheritable perl-Class-Method-Modifiers perl-Devel-Caller
Requires: perl-Devel-GlobalDestruction perl-Devel-LexAlias perl-Devel-StackTrace perl-Dist-CheckConflicts perl-Email-Date-Format
Requires: perl-Eval-Closure perl-Exception-Class perl-File-Find-Rule perl-HTML-Tree perl-libnetcfg perl-Log-Dispatch perl-Mail-Sendmail
Requires: perl-MIME-Lite perl-MIME-Types perl-Module-Implementation perl-namespace-autoclean perl-namespace-clean perl-Package-Stash
Requires: perl-Package-Stash-XS perl-PadWalker perl-Params-ValidationCompiler perl-Proc-Daemon perl-Proc-ProcessTable perl-Ref-Util
Requires: perl-Ref-Util-XS perl-Role-Tiny perl-Specio perl-Sub-Exporter-Progressive perl-Sub-Identify perl-Variable-Magic perl-XML-Twig
Requires: perl-Text-CSV perl-Text-CSV_XS
Requires: perl-Text-Template perl-Time-HiRes perl-DateTime perl-Path-Iterator-Rule

Requires: python3.12
Requires: python3.12-aiohttp python3.12-psycopg2
Requires: python3.12-minio python3.12-catboost python3.12-clickhouse-connect

#Requires: libicu = 50.2-4
#BuildRequires: libicu-devel = 50.2-4
BuildRequires: boost%{__boost_suffix}-devel = 1.76.0
Requires: boost%{__boost_suffix} = 1.76.0
BuildRequires: xgboost-devel
Requires: xgboost
BuildRequires: gtest-devel = 1.12.1
Requires: gtest = 1.12.1

Requires: glibc-all-langpacks
Requires: foros-pagesense

Provides: perl(Base) perl(Common) perl(TestHTML) perl(TestResultProc) perl(EmailNotifySend) perl(CampaignConfig) perl(CampaignTemplates) perl(DBEntityChecker) perl(PerformanceDB) perl(XMLEncoder)

Provides: perl(Predictor::BidCostModel) perl(Predictor::BidCostModelEvaluator)

Provides: perl(DB::Entities::Options) perl(DB::Entities::Users) perl(DB::Entities::WDTags) perl(DB::Entities::AudienceChannel)
Provides: perl(DB::Entities::BehavioralChannel) perl(DB::EntitiesImpl) perl(DB::Entities::ExpressionChannel)
Provides: perl(DB::Entities::GEOChannel) perl(DB::Entities::DeviceChannel) perl(DB::Entities::Site)
Provides: perl(DB::Entities::Account) perl(DB::Entities::Creative) perl(DB::Entities::TargetingChannel) perl(DB::Entities::Global)
Provides: perl(DB::Entities::SpecialChannel) perl(DB::Entities::Colocation) perl(DB::Entities::Tags)
Provides: perl(DB::Entities::Campaign) perl(DB::Entities::FreqCap) perl(DB::Entities::Currency)
Provides: perl(DB::Entities::ChannelInventory) perl(DB::Entities::CCG) perl(DB::Entities::Country)
Provides: perl(DB::Entities::CategoryChannel)

%endif

%description
advertizing server main package
this package is for %{__type} colocations

%ifnarch noarch
%package  tests
Summary:  Test programs
Group:    System Environment/Daemons
Requires: postgresql94-libs >= %{__postgresql_ver_req}
Requires(pre): %{name} 
Provides: perl(ChannelConfig)

%if "%{?__type:%{__type}}%{!?__type:0}" == "central"
Requires: foros-server-central = %{version}
%else
Requires: foros-server-remote = %{version}
%endif

%description tests
advertizing server related test programs
%endif

%ifarch noarch
%package cms
Summary: AdServer CMS plugin
Group: System Environment/Daemons
Requires: cms
BuildRequires: zip

%description cms
advertizing server Configuration Management System plugin
%endif

%prep
%setup -q -n foros-server-%{version}

cpp_flags=''
if [ '%{?buildType}' == 'nb' ]; then cpp_flags='-DDEBUG '; fi

mkdir -p unixcommons/%{__osbe_build_dir}
echo "DIRECTORY"
pwd
#cp unixcommons/default.config.t /tmp/unixcommons.default.config.t
#cpp -DOS_%{_os_release} -DARCH_%{_target_cpu} -DARCH_FLAGS='%{__arch_flags}' ${cpp_flags} \
#    unixcommons/default.config.t > unixcommons/%{__osbe_build_dir}/default.config

if [ '%__type' == 'central' ]; then cpp_flags+='-DUSE_OCCI'; fi

mkdir -p %{__product}/%{__osbe_build_dir}

#cpp -DOS_%{_os_release} -DARCH_%{_target_cpu} -DARCH_FLAGS='%{__arch_flags}' ${cpp_flags} \
#    -DUNIXCOMMONS_ROOT=%{_builddir}/foros-server-%{version}/unixcommons/%{__osbe_build_dir} \
#    -DUNIXCOMMONS_INCLUDE=src \
#    -DUNIXCOMMONS_CORBA_INCLUDE=src/CORBA \
#    -DUNIXCOMMONS_DEF=%{_builddir}/foros-server-%{version}/unixcommons/libdefs \
#    %{__product}/default.config.t > %{__product}/%{__osbe_build_dir}/default.config

%build
%ifnarch noarch
pushd unixcommons
#osbe
product_root=`pwd`
cd %{__osbe_build_dir}
#echo "CALL UNIXCOMMONS CMAKE in $(pwd)"
#cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
#%{__make} %{_smp_mflags}

#${product_root}/configure --enable-no-questions --enable-guess-location=no --prefix=%{__inst_root}
popd

pushd %{__product}
#osbe
product_root=`pwd`
cd %{__osbe_build_dir}
#export PYTHONPATH=$PYTHONPATH:/usr/lib/python3.8/site-packages/
cmake -DUNIXCOMMONS=$(pwd)/../unixcommons -DCMAKE_BUILD_TYPE=RelWithDebInfo .
#${product_root}/configure --enable-no-questions --enable-guess-location=no --prefix=%{__inst_root}
#scl enable devtoolset-8 -- %{__make} %{_smp_mflags}
%{__make} %{_smp_mflags} VERBOSE=1
popd
%endif

%ifarch noarch
pushd %{__cms_plugin_src_dir}
sed "/<application/,/>/s/\(\bversion=\)\"[^\"]\+\"/\1\"%{version}\"/" -i AdServerAppDescriptor.xml
zip -r %{__cms_plugin_name}.zip * -x .svn
popd
%endif

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{__inst_root}

%ifnarch noarch
#make -C unixcommons/%{__osbe_build_dir} install destdir=%{buildroot}
#cmake --install unixcommons/%{__osbe_build_dir} --prefix "%{buildroot}%{__inst_root}/"

rm -rf %{buildroot}%{__inst_root}/include
rm -rf `find %{buildroot}%{__inst_root}/lib -type f -name '*.a'`

#scl enable devtoolset-8 --
#make -C %{__product}/%{__osbe_build_dir} install destdir=%{buildroot}
cmake --install %{__product}/%{__osbe_build_dir} --prefix "%{buildroot}%{__inst_root}/"

# create so aliases for old UI
ln -s "%{__inst_root}/lib/libHTTP.so" "%{buildroot}%{__inst_root}/lib/libhttp.so"
ln -s "%{__inst_root}/lib/libTestCommons.so" "%{buildroot}%{__inst_root}/lib/libCheckCommons.so"
ln -s "%{__inst_root}/lib/libSNMPAgent.so" "%{buildroot}%{__inst_root}/lib/libSNMPAgentX.so"
ln -s "%{__inst_root}/lib/libCORBAInfResolveServerIDL.so" "%{buildroot}%{__inst_root}/lib/libInfResolveStubs.so"
ln -s "%{__inst_root}/lib/libCORBAInfResolveClientIDL.so" "%{buildroot}%{__inst_root}/lib/libInfResolveClientStubs.so"
ln -s "%{__inst_root}/lib/libGeoip.so" "%{buildroot}%{__inst_root}/lib/libIPMap.so"

# TODO: tests have files in xsd too
rm -f %{__product}.list
rm -f tests.list
test_dir="unixcommons/tests %{__product}/tests"
if [ "%{__osbe_build_dir}" != "." ]; then
  test_dir="$test_dir unixcommons/%{__osbe_build_dir}/tests %{__product}/%{__osbe_build_dir}/tests %{__product}/%{__osbe_build_dir}/utils"
fi

for f in $(find -L %{buildroot} %{buildroot}%{__inst_root} -type f) ; do
  binary=`basename $f`
  inst_name=`echo "$f" | sed -e 's#%{buildroot}##g' -e 's#\.py$#\.py*#'`
  res=`find $test_dir -type f -name $binary`
    if [ -n "$res" ] || \
      [ "$binary" = "ChannelTest" ] || \
      [ "$binary" = "UserInfoAdmin" ] || \
      [ "$binary" = "ProfileDump" ] || \
      [ "$binary" = "UserInfoExchangerProxy" ] || \
      [ "$binary" = "ExtractMatchedChannels.pl" ] || \
      [ "$binary" = "ExtractCampaignClick.pl" ] || \
      [ "$binary" = "XslTransformAdmin" ] ; then
      echo "$inst_name" >> tests.list
    else
      echo "$inst_name" >> %{__product}.list
    fi
done

for f in $(find %{buildroot}%{__inst_root} -type l) ; do
  inst_name=`echo "$f" | sed -e 's#%{buildroot}##g' -e 's#\.py$#\.py*#'`
  echo "$inst_name" >> %{__product}.list
done

%endif

%ifarch noarch
mkdir -p %{buildroot}%{__cms_plugin_dst_dir}
install -m 0644 -T %{__cms_plugin_src_dir}/%{__cms_plugin_name}.zip %{buildroot}%{__cms_plugin_dst_dir}/%{__cms_plugin_name}.zip.rpm
%endif

%ifarch noarch
%post cms
ln -s %{__cms_plugin_name}.zip.rpm %{__cms_plugin_dst_dir}/%{__cms_plugin_name}.zip

%preun cms
rm -f %{__cms_plugin_dst_dir}/%{__cms_plugin_name}.zip ||:
%endif

%ifnarch noarch
%pre
# for clean upgrade from previous version
[ -h %__inst_root ] && rm  %__inst_root ||:

%files -f %{__product}.list
%defattr(-, root, root)
%dir %{__inst_root}/

%files tests -f tests.list
%defattr(-, root, root)
%endif

%ifarch noarch
%files cms
%defattr(-, root, root)
%{__cms_plugin_dst_dir}/%{__cms_plugin_name}.zip.rpm
%endif

%clean
rm -rf %{buildroot}

%changelog

%define __apache_ver_req        1:2.2.21.2-1.ssv1.el5
%define __ace_ver_req           5.6.5.25-ssv1
%define __oracle_ver_req        10.2.0.4-ssv3.el5
%define __glibc_ver_req         2.5-65
%define __geoip_ver_req         1.4.8-1
%define __net_snmp_ver_req      5.3.2.2-14.ssv1.el5
%define __libevent_ver_req      1:1.4.13-11.ssv1.el5

Summary: Taskbot
Name: foros-taskbot
Version: 2.3.0.6
Release: ssv1.%{_os_release}
License: Commercial
Group: System Environment/Daemons
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-buildroot
Source: %{name}-%{version}.tar.gz

Requires: glibc-devel = %__glibc_ver_req ace-tao-devel = %__ace_ver_req apr-devel = 1:1.4.5-1.ssv1 autoconf = 2.59-12 gcc-c++ GeoIP-devel = %__geoip_ver_req httpd-devel = %{__apache_ver_req} libevent-devel = %__libevent_ver_req OpenSBE = 1.0.44-ssv1 OpenSBE-defs = 1.0.11.0-ssv1.el5 oracle-instantclient-devel = %__oracle_ver_req pcre-devel = 6.6-6.el5_6.1 prelink rpm-utils selinux-policy specspo xerces-c-devel >= 3.1.1 xsd >= 3.3.0 net-snmp-devel = 2:%{__net_snmp_ver_req} java-1.6.0-sun-devel >= 1:1.6.0.15 libxml2-devel = 2.7.8-7 libxslt-devel = 1.1.17-2.el5_2.2 make = 1:3.81-3.el5 openssl-devel = 0.9.8e-20.el5 zlib-devel = 1.2.3-4.el5 flex = 2.5.4a-41.fc6 bison = 2.3-2.1  tokyocabinet-devel = 1.4.43-2 omniORB-devel >= 4.1.4 omniORBpy-devel >= 3.4 postgresql-devel = 8.1.23-1.el5_6.1 glibc = %__glibc_ver_req httpd = %{__apache_ver_req} GeoIP = %__geoip_ver_req GeoIP-City ace-tao = %__ace_ver_req foros-polyglot-dict >= 1.0.0.11-ssv1.el5 mod_ssl >= 2.2.14.1-1.ssv2.el5 net-snmp >= %{__net_snmp_ver_req} rsync >= 3.0.7-3 stunnel libevent = %__libevent_ver_req git = 1.5.5.6-ssv1.el5 git-svn = 1.5.5.6-ssv1.el5 git-gui = 1.5.5.6-ssv1.el5 mysql >= 5.0.7

%description
taskbot package

%pre
/usr/sbin/useradd -d /home/taskbot taskbot

%prep
%setup -q -n %{name}-%{version} -c %{buildroot}/%{name}-%{version}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/etc/httpd/conf.d
cp -R conf/my.cnf %{buildroot}/etc/my.cnf.rpm
HOSTNAME=`hostname -f`
sed -e s@___HOSTNAME___@$HOSTNAME@g conf/taskbot.conf > %{buildroot}/etc/httpd/conf.d/taskbot.conf.rpm
mkdir -p %{buildroot}/home/taskbot/sql
cp -R initial.mysql %{buildroot}/home/taskbot/sql
cp -R schema.mysql %{buildroot}/home/taskbot/sql

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
/etc/*
/home/taskbot/*

%post

[ -f /etc/httpd/conf.d/taskbot.conf ] && %__mv /etc/httpd/conf.d/taskbot.conf /etc/httpd/conf.d/taskbot.conf.rpmsave
%__install -m 644 /etc/httpd/conf.d/taskbot.conf.rpm /etc/httpd/conf.d/taskbot.conf
[ -f /etc/my.cnf ] && %__mv /etc/my.cnf /etc/my.cnf.rpmsave
%__install -m 644 /etc/my.cnf.rpm /etc/my.cnf


service httpd restart 2>&1 > /dev/null
service mysqld stop 2>&1 > /dev/null
mysql_install_db --user=root --ldata=/home/taskbot/mysql
service mysqld start 2>&1 > /dev/null

mysql --user=root mysql < /home/taskbot/sql/initial.sql
mysql --user=taskbot --password=taskbot taskbot < /home/taskbot/sql/schema.mysql

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:key name="host" match="/host" use="."/>
<xsl:key name="adcluster-host" match="service[contains(@descriptor,'AdCluster')]" use="@host"/>
<xsl:key name="adproxy-host" match="service[contains(@descriptor,'AdProxyCluster')]" use="@host"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="autorestart">
  <xsl:call-template name="GetAutoRestart">
    <xsl:with-param name="app-env" select="$xpath/configuration/cfg:environment"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="zenoss-enabled">
  <xsl:call-template name="GetZenOSSEnabled">
    <xsl:with-param name="app-xpath" select="$xpath"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="ad-colo-config" select="$xpath/serviceGroup[@descriptor =
  $ad-cluster-descriptor]/configuration/cfg:cluster"/>
<xsl:variable name="ad-snmp-stats-enabled">
  <xsl:if test="count($ad-colo-config/cfg:snmpStats) > 0">
    <xsl:value-of select="string($ad-colo-config/cfg:snmpStats/@enable)"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="proxy-colo-config" select="$xpath/serviceGroup[@descriptor =
  $ad-proxycluster-descriptor]/configuration/cfg:cluster"/>
<xsl:variable name="proxy-snmp-stats-enabled">
  <xsl:if test="count($proxy-colo-config/cfg:snmpStats) > 0">
    <xsl:value-of select="string($proxy-colo-config/cfg:snmpStats/@enable)"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="ad-profiling-colo-config" select="$xpath/serviceGroup[@descriptor =
  'AdProfilingCluster']/configuration/cfg:cluster"/>
<xsl:variable name="ad-profiling-snmp-stats-enabled">
  <xsl:if test="count($ad-profiling-colo-config/cfg:snmpStats) > 0">
    <xsl:value-of select="string($ad-profiling-colo-config/cfg:snmpStats/@enable)"/>
  </xsl:if>
</xsl:variable>

<xsl:template name="GetAllHosts">
  <xsl:param name="path"/>
  <xsl:for-each select="$path">
    <xsl:call-template name="GetHosts">
      <xsl:with-param name="hosts" select="@host"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>

<xsl:template name="MgrSpecGenerator">
  <xsl:param name="app-zone-config"/>
  <xsl:param name="prefix"/>

  <xsl:variable name="user">
     <xsl:call-template name="GetUserName">
       <xsl:with-param name="app-env" select="$app-zone-config"/>
     </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="group">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$app-zone-config"/>
      <xsl:with-param name="name" select="'user_group'"/>
      <xsl:with-param name="type" select="$xsd-zone-management-type"/>
    </xsl:call-template>
  </xsl:variable>

%package <xsl:value-of select="$prefix"/>mgr

Release: <xsl:value-of select="$RELEASE"/>
Summary: ad products operation package
License: Commercial
Group: System Environment/Daemons
Version: <xsl:value-of select="$app-version"/>
BuildArch: noarch
BuildRoot: <xsl:value-of select="$BUILD_ROOT"/>

Obsoletes: <xsl:value-of select="$PACKAGE_NAME"/>-sms
Conflicts: <xsl:value-of select="$PACKAGE_NAME"/><xsl:if
  test="$prefix = ''">-foros-mgr <xsl:value-of select="$PACKAGE_NAME"/>-isp-mgr</xsl:if><xsl:if
  test="$prefix != ''">-mgr</xsl:if>
Obsoletes: <xsl:value-of select="$PACKAGE_NAME"/><xsl:if
  test="$prefix = ''">-foros-mgr &lt;= <xsl:value-of
  select="$app-version"/> <xsl:value-of
  select="concat(' ', $PACKAGE_NAME)"/>-isp-mgr &lt;= <xsl:value-of
  select="$app-version"/></xsl:if><xsl:if test="$prefix != ''">-mgr &lt;= <xsl:value-of
  select="$app-version"/></xsl:if>
# requires for ConfigureUserChunks.pl
Requires: tar
Requires: findutils
Requires: coreutils
Requires: openssh-clients
Requires: sed
Requires: perl
<xsl:if test="$autorestart = 'false' or $autorestart = '0'">
Requires: foros-manager >= <xsl:value-of select="$CMANAGER_VERSION"/>
</xsl:if>
<xsl:if test="$autorestart = 'true' or $autorestart = '1'">
Requires: <xsl:value-of select="concat($PACKAGE_NAME, '-', $prefix)"/>local-mgr = <xsl:value-of
  select="$app-version"/>-<xsl:value-of select="$RELEASE"/>
</xsl:if>

%define __setup_dir /opt/foros/manager/etc/config.d
%define __tr_dir /opt/foros/manager/var/server-<xsl:value-of select="$colo-name"/>
<xsl:if test="$prefix='foros-' or $prefix=''">
%define __mgr_ext_root /opt/foros/server/manager/<xsl:value-of select="$PRODUCT_IDENTIFIER"/>
</xsl:if>
<xsl:if test="$prefix='isp-'">
%define __mgr_ext_root /opt/foros/server/manager/<xsl:value-of select="$PRODUCT_ISP_IDENTIFIER"/>
</xsl:if>
%define __mgr_ext_bin_dir %{__mgr_ext_root}/bin
%define __mgr_ext_lib_dir %{__mgr_ext_root}/lib
%define __mgr_ext_workspace_dir %{__mgr_ext_root}/var

%description <xsl:value-of select="$prefix"/>mgr
foros server management configuration

%files <xsl:value-of select="$prefix"/>mgr
%defattr(-, root, root)
<xsl:if test="$autorestart = 'false' or $autorestart = '0'">
%{__setup_dir}/*
</xsl:if>
%{__mgr_ext_root}
%{__mgr_ext_bin_dir}
%{__mgr_ext_lib_dir}

%defattr(-, <xsl:value-of select="$user"/>, <xsl:value-of select="$group"/>)
%{__tr_dir}
%{__mgr_ext_workspace_dir}

<xsl:variable name="private-key-defined"
  select="string-length($app-zone-config/../cfg:forosZoneManagement/cfg:private_key/text()) != 0 or
          string-length($app-zone-config/cfg:private_key/text()) != 0"/>
<xsl:if test="$private-key-defined">
%defattr(0600, <xsl:value-of select="$user"/>, <xsl:value-of select="$group"/>)
<xsl:if test="$prefix='foros-' or $prefix=''">
<xsl:call-template name="PrivateKeyFile">
  <xsl:with-param name="product-identifier" select="$PRODUCT_IDENTIFIER"/>
  <xsl:with-param name="app-env" select="$app-zone-config"/>
</xsl:call-template>
</xsl:if>
<xsl:if test="$prefix='isp-'">
<xsl:call-template name="PrivateKeyFile">
  <xsl:with-param name="product-identifier" select="$PRODUCT_ISP_IDENTIFIER"/>
  <xsl:with-param name="app-env" select="$app-zone-config"/>
</xsl:call-template>
</xsl:if>
</xsl:if>

%pre <xsl:value-of select="$prefix"/>mgr
<xsl:if test="$autorestart = 'false' or $autorestart = '0'">
rm %{__setup_dir}/backup/server-<xsl:value-of select="$colo-name"/>-* >/dev/null 2>/dev/null ||:
cp %{__setup_dir}/server-<xsl:value-of select="$colo-name"/>-* %{__setup_dir}/backup/ >/dev/null 2>/dev/null ||:
</xsl:if>

%_sbindir/groupadd -r -f -g 506 <xsl:value-of select="$user"/> >/dev/null ||:
%_sbindir/useradd -r -m -c 'ad products operation account' -u 506 -g <xsl:value-of
  select="concat($group, ' ', $user)"/> >/dev/null 2>/dev/null ||:

%changelog <xsl:value-of select="$prefix"/>mgr
</xsl:template>

<xsl:template name="LocalMgrSpecGenerator">
  <xsl:param name="app-path"/>
  <xsl:param name="app-zone-config"/>
  <xsl:param name="prefix"/>

  <xsl:variable name="user">
     <xsl:call-template name="GetUserName">
       <xsl:with-param name="app-env" select="$app-zone-config"/>
     </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="group">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$app-zone-config"/>
      <xsl:with-param name="name" select="'user_group'"/>
      <xsl:with-param name="type" select="$xsd-zone-management-type"/>
    </xsl:call-template>
  </xsl:variable>

%package <xsl:value-of select="$prefix"/>local-mgr

Release: <xsl:value-of select="$RELEASE"/>
Summary: ad products operation package
License: Commercial
Group: System Environment/Daemons
Version: <xsl:value-of select="$app-version"/>
BuildArch: noarch
BuildRoot: <xsl:value-of select="$BUILD_ROOT"/>

Requires: foros-manager >= <xsl:value-of select="$CMANAGER_VERSION"/>
Requires: yum-plugin-downloadonly

Conflicts: <xsl:value-of select="$PACKAGE_NAME"/><xsl:if
  test="$prefix != ''">-local-mgr</xsl:if><xsl:if
  test="$prefix = ''">-foros-local-mgr <xsl:value-of select="$PACKAGE_NAME"/>-isp-local-mgr</xsl:if>

%define __setup_dir /opt/foros/manager/etc/config.d

%description <xsl:value-of select="$prefix"/>local-mgr
foros server autostart configuration

%files <xsl:value-of select="$prefix"/>local-mgr
%defattr(-, root, root)
<xsl:if test="$prefix=''">
%{__setup_dir}/*.xml
</xsl:if>
<xsl:if test="$prefix='foros-'">
%{__setup_dir}/*foros*.xml
</xsl:if>
<xsl:if test="$prefix='isp-'">
%{__setup_dir}/*isp*.xml
</xsl:if>

<xsl:variable name="autorestart_state_dir_for_create"><xsl:call-template name="GetAutoRestartFolder">
    <xsl:with-param name="app-env" select="$app-path/configuration/cfg:environment"/>
  </xsl:call-template></xsl:variable>

<xsl:if test="string-length($autorestart_state_dir_for_create) > 0">
%defattr(-, <xsl:value-of select="$user"/>, <xsl:value-of select="$group"/>)
<xsl:value-of select="$autorestart_state_dir_for_create"/>/
</xsl:if>

%pre <xsl:value-of select="$prefix"/>local-mgr
rm %{__setup_dir}/backup/server-<xsl:value-of select="$colo-name"/>-* >/dev/null 2>/dev/null ||:
cp %{__setup_dir}/server-<xsl:value-of select="$colo-name"/>-* %{__setup_dir}/backup/ >/dev/null 2>/dev/null ||:
%_sbindir/groupadd -r -f -g 506 <xsl:value-of select="$user"/> >/dev/null ||:
%_sbindir/useradd -r -m -c 'ad products operation account' -u 506 -g <xsl:value-of
  select="concat($group, ' ', $user)"/> >/dev/null 2>/dev/null ||:

<xsl:if test="string-length($autorestart_state_dir_for_create) > 0">
%preun <xsl:value-of select="$prefix"/>local-mgr
rm -rf <xsl:value-of select="$autorestart_state_dir_for_create"/>/* >/dev/null 2>/dev/null ||:
</xsl:if>

%changelog <xsl:value-of select="$prefix"/>local-mgr
</xsl:template>

<xsl:template name="AdClusterSpecGenerator">
  <xsl:param name="app-path"/>
  <xsl:param name="foros-zone-config"/>
  <xsl:param name="isp-zone-config"/>
  <xsl:param name="user-group"/>
  <xsl:param name="user-name"/>

Release: <xsl:value-of select="$RELEASE"/>
Summary: server proxy configuration
License: Commercial
Group: System Environment/Daemons
Name: <xsl:value-of select="$PACKAGE_NAME"/>
Version: <xsl:value-of select="$app-version"/>
BuildArch: noarch
BuildRoot: <xsl:value-of select="$BUILD_ROOT"/>

<xsl:variable name="foros-private-key-defined"
  select="string-length($foros-zone-config/cfg:private_key/text()) != 0"/>
<xsl:variable name="isp-private-key-defined"
  select="string-length($isp-zone-config/cfg:private_key/text()) != 0"/>
<xsl:variable name="private-key-defined"
  select="string-length($xpath/configuration/cfg:environment/cfg:private_key/text()) != 0"/>
<xsl:variable name="foros-public-key-defined"
  select="$foros-private-key-defined or
  string-length($foros-zone-config/@public_key) != 0"/>
<xsl:variable name="isp-public-key-defined"
  select="$isp-private-key-defined or
  string-length($isp-zone-config/@public_key) != 0"/>
<xsl:variable name="public-key-defined" select="$foros-public-key-defined or $isp-public-key-defined"/>

<xsl:variable name="requires">
  <xsl:choose>
    <xsl:when test="count($app-path/serviceGroup[
      @descriptor = $ad-cluster-descriptor]/configuration/cfg:cluster/cfg:central) > 0"><xsl:value-of
      select="concat('foros-server-central = ', $app-version)"/><xsl:if
      test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor
        ]/serviceGroup[@descriptor = $tests-descriptor]) > 0"><xsl:value-of
        select="concat(' foros-server-central-tests = ', $app-version, ' foros-server-central-debuginfo = ', $app-version)"/></xsl:if>
    </xsl:when>
    <xsl:when test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor or @descriptor = $ad-profilingcluster-descriptor
      ]/configuration/cfg:cluster/cfg:remote) > 0 or
      count($app-path/serviceGroup[@descriptor = $ad-proxycluster-descriptor]) > 0"><xsl:value-of
      select="concat('foros-server-remote = ', $app-version)"/><xsl:if
      test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor
        ]/serviceGroup[@descriptor = $tests-descriptor]) > 0"><xsl:value-of
        select="concat(' foros-server-remote-tests = ', $app-version, ' foros-server-remote-debuginfo = ', $app-version)"/></xsl:if>
    </xsl:when>
  </xsl:choose>
</xsl:variable>

<xsl:if test="string-length($requires) > 0">
Requires: <xsl:value-of select="$requires"/>
</xsl:if>

<xsl:if test="$autorestart = 'true' or $autorestart = '1'">
Requires: <xsl:if test="$foros-zone-config != $isp-zone-config"><xsl:value-of
    select="concat($PACKAGE_NAME, '-foros-local-mgr = ', $app-version, '-', $RELEASE ,' ')"/>
  <xsl:value-of select="concat($PACKAGE_NAME, '-isp-local-mgr = ', $app-version, '-', $RELEASE)"/>
  </xsl:if>
  <xsl:if test="$foros-zone-config = $isp-zone-config or count($isp-zone-config) = 0"><xsl:value-of
    select="concat($PACKAGE_NAME, '-local-mgr = ', $app-version, '-', $RELEASE)"/>
  </xsl:if>
</xsl:if>

%define __setup_dir /opt/foros/server/etc
%define __workspace_dir /u01/foros/server/var
%define __workspace_link /opt/foros/server/var
%define __begin_tag   # required by %{name}-%{version}-%{release}
%define __end_tag     # end of %{name}-%{version}-%{release} requirements
<!-- The value of __public_key will inject through sed.-->
%define __public_key   ___PUBLIC_KEY___
%define __isp_public_key   ___ISP_PUBLIC_KEY___

%define __plugin_root <xsl:value-of select="$PLUGIN_ROOT"/>

%description
foros server configuration

%install
pushd <xsl:value-of select="$BUILD_ROOT"/>
if [[ `pwd` != %{buildroot} ]]; then
mv -T <xsl:value-of select="$BUILD_ROOT"/>/ %{buildroot}/
fi
popd
%{__ln_s} -f -T %{__workspace_dir} %{buildroot}%{__workspace_link}

mkdir -p %{buildroot}/etc/sysctl.d/
mkdir -p %{buildroot}/etc/security/limits.d/
install --mode 644 %{__plugin_root}/data/Config/adserver_sysctl.conf %{buildroot}/etc/sysctl.d/adserver.conf
install --mode 644 %{__plugin_root}/data/Config/91-aduser.conf %{buildroot}/etc/security/limits.d/91-aduser.conf

%files
%defattr(-, %{__user}, %{__group})
%{__workspace_dir}
%{__workspace_link}

%defattr(-, root, root)
<xsl:if test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor or @descriptor = $ad-proxycluster-descriptor]) > 0">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/*.sh
</xsl:if>
<xsl:if test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]/serviceGroup[@descriptor = $tests-descriptor]) > 0">
%{__setup_dir}/*.sh
</xsl:if>

<xsl:if test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]) > 0">
   <xsl:variable name="cluster-name">
     <xsl:call-template name="NormalizeName">
       <xsl:with-param name="name" select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]/@descriptor"/>
     </xsl:call-template>
   </xsl:variable>

%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/RIChunksConfig
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/*.sh

<xsl:variable name="frontends">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
       select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]/serviceGroup[@descriptor = $fe-cluster-descriptor]/service[@descriptor = $http-frontend-descriptor]"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($frontends)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/PS/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/http/bin/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/http/htdocs/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/*.txt
  </xsl:if>
</xsl:for-each>

<xsl:variable name="adcluster-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
       select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]//service"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($adcluster-hosts)//host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/CurrentEnv/*
  </xsl:if>
</xsl:for-each>

<xsl:variable name="adfrontend-https-enabled">
  <xsl:value-of select="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor
     ]/serviceGroup[@descriptor = $fe-cluster-descriptor]//service[@descriptor = $http-frontend-descriptor]/
     configuration/cfg:frontend/cfg:networkParams[@https_enabled = 1 or @https_enabled = 'true'])"/>
</xsl:variable>

<xsl:variable name="adfrontend-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]/serviceGroup[@descriptor = $fe-cluster-descriptor]//service[@descriptor = $http-frontend-descriptor]"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="cert-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]//service[@descriptor = $campaign-server-descriptor]"/>
  </xsl:call-template>
  <xsl:if test="$adfrontend-https-enabled > 0">
    <xsl:value-of select="$adfrontend-hosts"/>
  </xsl:if>
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]//service[@descriptor = $user-info-manager-descriptor]"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="sync-server-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor
        ]//service[@descriptor = $campaign-manager-descriptor or @descriptor = $http-frontend-descriptor or @descriptor = $log-generalizer-descriptor or @descriptor = $expression-matcher-descriptor or @descriptor = $request-info-manager-descriptor]"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($sync-server-hosts)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/conf/*
  </xsl:if>
</xsl:for-each>

%defattr(400, %{__user}, %{__group})

<xsl:for-each select="exsl:node-set($cert-hosts)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/cert/*
  </xsl:if>
</xsl:for-each>

</xsl:if>

<xsl:if test="count($app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]) > 0">
   <xsl:variable name="cluster-name">
     <xsl:call-template name="NormalizeName">
       <xsl:with-param name="name" select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]/@descriptor"/>
     </xsl:call-template>
   </xsl:variable>

%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/*.sh

<xsl:variable name="frontends">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
       select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($frontends)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/http/bin/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/http/htdocs/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/*.txt
  </xsl:if>
</xsl:for-each>

<xsl:variable name="adcluster-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
       select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]//service"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($adcluster-hosts)//host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/*
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/CurrentEnv/*
  </xsl:if>
</xsl:for-each>

<xsl:variable name="adfrontend-https-enabled">
  <xsl:value-of select="count($app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor
     ]/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']//service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']/
     configuration/cfg:frontend/cfg:networkParams[@https_enabled = 1 or @https_enabled = 'true'])"/>
</xsl:variable>

<xsl:variable name="adfrontend-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']//service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="cert-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]//service[@descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer']"/>
  </xsl:call-template>
  <xsl:if test="$adfrontend-https-enabled > 0">
    <xsl:value-of select="$adfrontend-hosts"/>
  </xsl:if>
</xsl:variable>

<xsl:variable name="sync-server-hosts">
  <xsl:call-template name="GetAllHosts">
    <xsl:with-param name="path"
      select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor
        ]//service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']"/>
  </xsl:call-template>
</xsl:variable>

<xsl:for-each select="exsl:node-set($sync-server-hosts)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/conf/*
  </xsl:if>
</xsl:for-each>

%defattr(400, %{__user}, %{__group})

<xsl:for-each select="exsl:node-set($cert-hosts)/host">
  <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$colo-name"/>/<xsl:value-of select="$cluster-name"/>/<xsl:value-of select="."/>/cert/*
  </xsl:if>
</xsl:for-each>

</xsl:if>

<xsl:for-each select="$app-path/serviceGroup[@descriptor = $ad-proxycluster-descriptor]">
  <xsl:variable name="cluster-name">
    <xsl:call-template name="NormalizeName">
       <xsl:with-param name="name" select="@descriptor"/>
     </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="cluster-index"><xsl:value-of select="position()"/></xsl:variable>
    <xsl:variable name="dir-suffix">
      <xsl:value-of select="concat('/', $colo-name, '/', $cluster-name, '-', $cluster-index)"/>
    </xsl:variable>
%defattr(-, root, root)
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/*.sh

  <xsl:variable name="adproxy-hosts">
    <xsl:call-template name="GetAllHosts">
      <xsl:with-param name="path" select=".//service"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="cert-hosts">
    <xsl:call-template name="GetAllHosts">
      <xsl:with-param name="path" select=".//service[
        @descriptor = $pbe-stunnel-server-descriptor or
        @descriptor = $pbe-campaign-server-descriptor or
        @descriptor = $pbe-channel-proxy-descriptor or
        @descriptor = $pbe-user-info-exchanger-descriptor]"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="adproxy-subagent-hosts">
    <xsl:variable name="adproxy-snmp-stats-enabled">
      <xsl:if test="count(./configuration/cfg:cluster/cfg:snmpStats) > 0">
        <xsl:value-of select="string(./configuration/cfg:cluster/cfg:snmpStats/@enable)"/>
      </xsl:if>
    </xsl:variable>
    <xsl:if test="$adproxy-snmp-stats-enabled = 'true'">
    <xsl:call-template name="GetAllHosts">
      <xsl:with-param name="path" select=".//service[
        @descriptor = $pbe-campaign-server-descriptor or
        @descriptor = $pbe-channel-proxy-descriptor or
        @descriptor = $pbe-user-info-exchanger-descriptor or
        @descriptor = $pbe-stunnel-server-descriptor]"/>
    </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="conf-hosts">
    <xsl:call-template name="GetAllHosts">
      <xsl:with-param name="path" select=".//service"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($adproxy-hosts)/host">
    <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/<xsl:value-of select="."/>/*.xml
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/<xsl:value-of select="."/>/CurrentEnv/*
    </xsl:if>
  </xsl:for-each>

  <xsl:if test="$proxy-snmp-stats-enabled = 'true'">
  <xsl:for-each select="exsl:node-set($adproxy-subagent-hosts)/host">
    <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/<xsl:value-of select="."/>/subagent/*
    </xsl:if>
  </xsl:for-each>
  </xsl:if>

  <xsl:for-each select="exsl:node-set($conf-hosts)/host">
    <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/<xsl:value-of select="."/>/conf/*
    </xsl:if>
  </xsl:for-each>

%defattr(400, %{__user}, %{__group})
  <xsl:for-each select="exsl:node-set($cert-hosts)/host">
    <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
%{__setup_dir}/<xsl:value-of select="$dir-suffix"/>/<xsl:value-of select="."/>/cert/*
  </xsl:if>
  </xsl:for-each>
</xsl:for-each>

<xsl:if test="count($app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]/serviceGroup[@descriptor = $tests-descriptor]) > 0">
%defattr(-, %{__user}, %{__group})
%{__setup_dir}/TestConfig/*
</xsl:if>

%defattr(-, root, root)
/etc/sysctl.d/adserver.conf
/etc/security/limits.d/91-aduser.conf

%postun
<xsl:if test="$public-key-defined">
# change authorized_keys back
sed -i '/%{__begin_tag}/,/%{__end_tag}/d' /home/%{__user}/.ssh/authorized_keys 2&gt;/dev/null ||:
</xsl:if>

%clean
rm -rf %{buildroot}

%pre
%_sbindir/groupadd -r -f -g 506 <xsl:value-of select="$user-group"/> >/dev/null ||:
%_sbindir/useradd -r -m -c 'ad products operation account' -u 506 -g <xsl:value-of
  select="concat($user-group, ' ', $user-name)"/> >/dev/null 2>/dev/null ||:

%post

USER=<xsl:value-of select="$user-name"/>

<xsl:if test="$public-key-defined">
sed -i '/%{__begin_tag}/,/%{__end_tag}/d' /home/%{__user}/.ssh/authorized_keys 2&gt;/dev/null ||:
if cd /home/%{__user}; then
  mkdir -p .ssh; chmod 700 .ssh; chown -R %{__user}:%{__group} .ssh
  if [ ! -f .ssh/authorized_keys ]; then
    install -m 644 -o %{__user} -g %{__group} -T /dev/null .ssh/authorized_keys;
  else
    chmod 644 .ssh/authorized_keys;
  fi
  <![CDATA[cat <<EOF >> .ssh/authorized_keys
%{__begin_tag}
%{__public_key}
%{__isp_public_key}
%{__end_tag}
EOF]]>
else
  echo 'failed to "cd /home/%{__user}"' >&amp;2
fi
</xsl:if>

sysctl -p /etc/sysctl.d/adserver.conf

%changelog
</xsl:template>

<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath"/>
  <xsl:variable name="app-config" select="$xpath/configuration/cfg:environment"/>

  <xsl:variable name="separate_isp_zone" select="
    $app-config/cfg:ispZoneManagement/@separate_isp_zone |
    $xsd-isp-zone-management-type/xsd:attribute[@name='separate_isp_zone']/@default"/>
  <xsl:variable name="zone-condition"
    select="$separate_isp_zone = '1' or $separate_isp_zone = 'true'"/>

  <xsl:variable name="isp-zone-config"
    select="$app-config/cfg:ispZoneManagement[$zone-condition] |
      $app-config/cfg:forosZoneManagement[not($zone-condition)]"/>

  <xsl:variable name="foros-zone-config"
    select="$app-config/cfg:forosZoneManagement"/>

  <xsl:variable name="user-group">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$foros-zone-config"/>
      <xsl:with-param name="name" select="'user_group'"/>
      <xsl:with-param name="type" select="$xsd-zone-management-type"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="user-name">
     <xsl:call-template name="GetUserName">
       <xsl:with-param name="app-env" select="$foros-zone-config"/>
     </xsl:call-template>
  </xsl:variable>

%define __user <xsl:value-of select="$user-name"/>
%define __group <xsl:value-of select="$user-group"/>
  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
%define __zenoss_dir        <xsl:call-template name="ZenossFolder">
    <xsl:with-param name="app-xpath" select="$app-path"/>
  </xsl:call-template>
  </xsl:if>

  <xsl:call-template name="AdClusterSpecGenerator">
    <xsl:with-param name="app-path" select="$app-path"/>
    <xsl:with-param name="foros-zone-config" select="$foros-zone-config"/>
    <xsl:with-param name="isp-zone-config" select="$isp-zone-config"/>
    <xsl:with-param name="user-group" select="$user-group"/>
    <xsl:with-param name="user-name" select="$user-name"/>
  </xsl:call-template>

  <xsl:choose>
  <xsl:when test="string($zone-condition) = 'false'">
    <xsl:call-template name="MgrSpecGenerator">
      <xsl:with-param name="app-zone-config" select="$foros-zone-config"/>
      <xsl:with-param name="prefix" select="''"/>
    </xsl:call-template>
    <xsl:if test="$autorestart = 'true' or $autorestart = '1'">
      <xsl:call-template name="LocalMgrSpecGenerator">
        <xsl:with-param name="app-path" select="$app-path"/>
        <xsl:with-param name="app-zone-config" select="$foros-zone-config"/>
        <xsl:with-param name="prefix" select="''"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:when>
  <xsl:otherwise>
  <xsl:call-template name="MgrSpecGenerator">
    <xsl:with-param name="app-zone-config" select="$foros-zone-config"/>
    <xsl:with-param name="prefix" select="'foros-'"/>
  </xsl:call-template>
  <xsl:call-template name="MgrSpecGenerator">
    <xsl:with-param name="app-zone-config" select="$isp-zone-config"/>
    <xsl:with-param name="prefix" select="'isp-'"/>
  </xsl:call-template>

  <xsl:if test="$autorestart = 'true' or $autorestart = '1'">
    <xsl:call-template name="LocalMgrSpecGenerator">
      <xsl:with-param name="app-path" select="$app-path"/>
      <xsl:with-param name="app-zone-config" select="$foros-zone-config"/>
      <xsl:with-param name="prefix" select="'foros-'"/>
    </xsl:call-template>
    <xsl:call-template name="LocalMgrSpecGenerator">
      <xsl:with-param name="app-path" select="$app-path"/>
      <xsl:with-param name="app-zone-config" select="$isp-zone-config"/>
      <xsl:with-param name="prefix" select="'isp-'"/>
    </xsl:call-template>
  </xsl:if>
  </xsl:otherwise>
  </xsl:choose>

  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
    <xsl:if test="$ad-snmp-stats-enabled = 'true' or
      $proxy-snmp-stats-enabled = 'true' or
      $ad-profiling-snmp-stats-enabled = 'true'">
%package zenoss
Group: Monitoring
Summary: foros server ZenOSS integration
Requires: foros-zenoss

%description zenoss
Configuration files for foros server ZenOSS integration

%files zenoss
%defattr(-, root, root)
%config %{__zenoss_dir}/*.xml
%config %{__zenoss_dir}/mibs/*.mib
  <xsl:if test="(count($ad-colo-config) > 0 and $ad-snmp-stats-enabled = 'true') or
                (count($ad-profiling-colo-config) > 0 and $ad-profiling-snmp-stats-enabled = 'true')">
%config %{__zenoss_dir}/mibs/LogProcessing/LogGeneralizer/*.mib
%config %{__zenoss_dir}/mibs/CampaignSvcs/CampaignServer/*.mib
%config %{__zenoss_dir}/mibs/Controlling/*.mib
%config %{__zenoss_dir}/mibs/RequestInfoSvcs/ExpressionMatcher/*.mib
%config %{__zenoss_dir}/mibs/RequestInfoSvcs/RequestInfoManager/*.mib
  </xsl:if>
  </xsl:if>
  </xsl:if>

</xsl:template>


</xsl:stylesheet>


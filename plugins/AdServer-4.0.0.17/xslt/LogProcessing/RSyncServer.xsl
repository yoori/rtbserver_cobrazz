<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- RSyncServer config generate function -->
<xsl:template name="RSyncServerConfigGenerator">
  <xsl:param name="app-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="stunnel-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="user-name"><xsl:value-of select="$app-config/cfg:forosZoneManagement/@user_name"/>
    <xsl:if test="count($app-config/cfg:forosZoneManagement/@user_name) = 0"><xsl:value-of select="$def-user-name"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="user-group"><xsl:value-of select="$app-config/@user_group"/>
    <xsl:if test="count($app-config/@user_group) = 0"><xsl:value-of select="$def-user-group"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="rsync-local-port">
    <xsl:value-of select="$stunnel-config/cfg:networkParams/@internal_port"/>
    <xsl:if test="count($stunnel-config/cfg:networkParams/@internal_port) = 0">
      <xsl:value-of select="$def-stunnel-server-internal-port"/>
    </xsl:if>
  </xsl:variable>

uid=<xsl:value-of select="$user-name"/>
gid=<xsl:value-of select="$user-group"/>
use chroot=false
pid file=<xsl:value-of select="$workspace-root"/>/run/rsyncserver.pid
address=127.0.0.1
port=<xsl:value-of select="$rsync-local-port"/>
max verbosity=0
log file = /dev/fd/666

[ad-logs]
read only=false
write only=true
path = <xsl:value-of select="concat($workspace-root, '/log')"/>
comment = Ad Server logs transfer

[ad-content]
read only=true
path = <xsl:value-of select="$data-root"/>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/../.."/>
  <xsl:variable name="stunnel-path" select="$xpath"/>
  <xsl:variable name="pbe-cluster-path" select="$xpath/.."/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> STunnelServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($pbe-cluster-path) = 0">
       <xsl:message terminate="yes"> STunnelServer: Can't find pbe cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="pbe-cluster-config"
    select="$pbe-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="stunnel-config"
    select="$stunnel-path/configuration/cfg:sTunnelServer | $pbe-cluster-config/cfg:sTunnelServer"/>

  <xsl:variable
    name="env-config"
    select="$pbe-cluster-config/cfg:environment"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($pbe-cluster-config) = 0">
       <xsl:message terminate="yes"> RSyncServer: Can't find pbe config </xsl:message>
    </xsl:when>
    <xsl:when test="count($stunnel-config) = 0">
       <xsl:message terminate="yes"> RSyncServer: Can't find stunnel config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="RSyncServerConfigGenerator">
    <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="stunnel-config" select="$stunnel-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

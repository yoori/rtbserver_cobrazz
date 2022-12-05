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

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- STunnelServer config generate function -->
<xsl:template name="STunnelServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="stunnel-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="stunnel-external-port">
    <xsl:value-of select="$stunnel-config/cfg:networkParams/@port"/>
    <xsl:if test="count($stunnel-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-stunnel-server-port"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="rsync-local-port">
    <xsl:value-of  select="$stunnel-config/cfg:networkParams/@internal_port"/>
    <xsl:if test="count($stunnel-config/cfg:networkParams/@internal_port) = 0">
      <xsl:value-of select="$def-stunnel-server-internal-port"/>
    </xsl:if>
  </xsl:variable>

CAfile = <xsl:value-of select="concat($config-root, '/' , $out-dir, '/cert/npca.pem')"/>
cert = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npcert.pem')"/>
key = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npkey.pem')"/>

verify = 2

cert = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npcert.pem')"/>
key = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npkey.pem')"/>

socket = l:SO_KEEPALIVE=1
socket = r:SO_KEEPALIVE=1

#cert = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/scert.pem')"/>
client = no
pid = <xsl:value-of select="$workspace-root"/>/run/stunnelserver.pid
debug = warning

[ssync]
accept = 0.0.0.0:<xsl:value-of select="$stunnel-external-port"/>
connect = localhost:<xsl:value-of select="$rsync-local-port"/>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
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
       <xsl:message terminate="yes"> STunnelServer: Can't find pbe config </xsl:message>
    </xsl:when>
    <xsl:when test="count($stunnel-config) = 0">
       <xsl:message terminate="yes"> STunnelServer: Can't find stunnel config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="STunnelServerConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="stunnel-config" select="$stunnel-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

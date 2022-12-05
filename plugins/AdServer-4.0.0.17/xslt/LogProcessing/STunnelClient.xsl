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

<!-- STunnelClient config generate function -->
<xsl:template name="STunnelClientConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="remote-colo-config"/>
  <xsl:param name="stunnel-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="stunnel-local-port">
    <xsl:value-of select="$stunnel-config/cfg:networkParams/@port"/>
    <xsl:if test="count($stunnel-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-stunnel-client-port"/>
    </xsl:if>
  </xsl:variable>

CAfile = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npca.pem')"/>
cert = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npcert.pem')"/>
key = <xsl:value-of select="concat($config-root, '/', $out-dir, '/cert/npkey.pem')"/>

verify = 2

client = yes
pid = <xsl:value-of select="$workspace-root"/>/run/stunnel.pid
debug = warning
output = <xsl:value-of select="$workspace-root"/>/log/STunnelClient/STunnelClient.log

[ssync]
accept = 0.0.0.0:<xsl:value-of select="$stunnel-local-port"/>
failover = prio
<xsl:for-each select="$remote-colo-config/cfg:sTunnelRef">
connect = <xsl:value-of select="@host"/>:<xsl:value-of select="@port"/>
</xsl:for-each>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="stunnel-client-path" select="$xpath"/>
  <xsl:variable name="full-cluster-path" select="$xpath/../../.."/>
  <xsl:variable name="fe-cluster-path" select="$xpath/.."/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> STunnelClient: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> STunnelClient: Can't find full cluster group </xsl:message>
    </xsl:when>
    
    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> STunnelClient: Can't find fe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="remote-colo-config"
    select="$colo-config/cfg:remote"/>

  <xsl:variable
    name="fe-cluster-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>
    
  <xsl:variable
    name="env-config"
    select="$fe-cluster-config/cfg:environment | $colo-config/cfg:environment"/>
    
  <xsl:variable
    name="stunnel-config"
    select="$stunnel-client-path/configuration/cfg:sTunnelClient"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($remote-colo-config) = 0">
       <xsl:message terminate="yes"> STunnelClient: Can't find remote config </xsl:message>
    </xsl:when>
    <xsl:when test="count($stunnel-config) = 0">
       <xsl:message terminate="yes"> STunnelClient: Can't find sTunnelClient config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="STunnelClientConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="remote-colo-config" select="$remote-colo-config"/>
    <xsl:with-param name="stunnel-config" select="$stunnel-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

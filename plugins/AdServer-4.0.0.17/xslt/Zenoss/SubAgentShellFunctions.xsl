<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:str="http://exslt.org/strings"
  xmlns:exsl="http://exslt.org/common"
  exclude-result-prefixes="xsl dyn cfg colo">

<xsl:output method="xml" indent="yes" encoding="utf-8" omit-xml-declaration="yes"/>
<xsl:variable name="connections-only" select="dyn:evaluate($CONNECTIONS_ONLY)"/>
<xsl:variable name="full-cluster-path" select="dyn:evaluate($CLUSTER_XPATH)"/>

<xsl:include href="../Functions.xsl"/>

<xsl:template name="GenerateSubAgentShellConfig">
  <xsl:param name="colo-config"/>
  <xsl:variable name="frontend-port"><xsl:if
    test="count($colo-config/cfg:coloParams/cfg:virtualServer[
     count(@proxy_protocol) = 0 or @proxy_protocol = 'false' or @proxy_protocol = '0'][1]/@internal_port) = 0"><xsl:value-of
    select="$def-frontend-port"/></xsl:if><xsl:value-of
    select="$colo-config/cfg:coloParams/cfg:virtualServer[
      count(@proxy_protocol) = 0 or @proxy_protocol = 'false' or @proxy_protocol = '0'][1]/@internal_port"/></xsl:variable>
  <xsl:variable name="oid-suffix">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$colo-config/cfg:snmpStats"/>
      <xsl:with-param name="name" select="'oid_suffix'"/>
      <xsl:with-param name="type" select="$xsd-snmp-config-type"/>
    </xsl:call-template>
  </xsl:variable>

<config>
  <xsl:if test="not($connections-only)">
  <xsl:if test="$full-cluster-path/@descriptor = 'AdCluster'">

    <xsl:variable name="host-name" select="concat($HOST, ':', $frontend-port)"/>
    <function id="adServerMonitoringProfilingRequestMIB">
      <args index="{$oid-suffix}"
        host_name="{$host-name}"/>
    </function>
    <xsl:if test="count($colo-config/cfg:snmpStats/@monitoring_tag_id) > 0">
      <function id="adServerMonitoringAdRequestMIB">
        <args index="{$oid-suffix}"
          tag_id="{$colo-config/cfg:snmpStats/@monitoring_tag_id}"
          host_name="{$host-name}">
          <xsl:attribute name="country"><xsl:choose>
            <xsl:when
              test="string-length($colo-config/cfg:snmpStats/@monitoring_country) > 0"><xsl:value-of
                select="$colo-config/cfg:snmpStats/@monitoring_country"/></xsl:when>
            <xsl:otherwise>
              <xsl:variable name="filter-country-tokens"
                select="str:tokenize($colo-config/cfg:coloParams/@enabled_countries,
                  ',&#x9;&#xA;&#xD;&#x20;')"/>
              <xsl:if test="count($filter-country-tokens) > 0"><xsl:value-of
                select="$filter-country-tokens[1]"/></xsl:if></xsl:otherwise>
          </xsl:choose></xsl:attribute>
        </args>
      </function>
    </xsl:if>

    <xsl:if test="count($colo-config/cfg:snmpStats/@monitoring_site_id) > 0">

      <xsl:variable name="virtual-servers-bid-raw">
        <xsl:call-template name="GetVirtualServers">
          <xsl:with-param name="xpath" select="$colo-config/cfg:coloParams/cfg:virtualServer[
            (count(@proxy_protocol) = 0 or @proxy_protocol = 'false' or
              @proxy_protocol = '0') and count(cfg:biddingDomain) > 0]"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="virtual-servers-bid" select="exsl:node-set($virtual-servers-bid-raw)/cfg:virtualServer"/>

      <xsl:if test="count($virtual-servers-bid) = 0">
        <xsl:message terminate="yes">Error: Active virtual servers with biddingDomain  not found</xsl:message>
      </xsl:if>

      <xsl:variable name="bid-frontend-port">
        <xsl:value-of select="$virtual-servers-bid[1]/@internal_port" />
      </xsl:variable>

      <xsl:variable name="bid-host-name" select="concat($HOST, ':', $bid-frontend-port)"/>
      <function id="adServerMonitoringBidRequestMIB">
        <args index="{$oid-suffix}"
          sid="{$colo-config/cfg:snmpStats/@monitoring_site_id}"
          ip="{$colo-config/cfg:snmpStats/@monitoring_ip}"
          host_name="{$bid-host-name}">
        </args>
      </function>
    </xsl:if>

    <function id="adServerApacheStatusMIB">
      <args index="{$oid-suffix}"
            url="{concat('localhost:', $frontend-port, '/server-status?auto')}"/>
    </function>
  </xsl:if>
  <xsl:if test="$full-cluster-path/@descriptor = 'AdCluster'">

    <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <function id="adServerFilesInFoldersMIB">
      <args index="{$oid-suffix}" log_root="{concat($workspace-root, '/log')}"/>
    </function>
  </xsl:if>
  </xsl:if>
  <!-- Presents in any clusters -->
  <!--
  <function id="adServerConnectionsStatesMIB">
    <args index="{$oid-suffix}"/>
  </function>
  -->
</config>
</xsl:template>

<xsl:template match="/">
  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> SubAgentShellFunctions: Can't find colo config.
       </xsl:message>
    </xsl:when>
  </xsl:choose>
  <xsl:choose>
    <xsl:when test="count($full-cluster-path) = 0">
      <xsl:message terminate="yes"> SubAgentShellFunctions: full cluster path error.
      </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="GenerateSubAgentShellConfig">
    <xsl:with-param name="colo-config" select="$colo-config"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>

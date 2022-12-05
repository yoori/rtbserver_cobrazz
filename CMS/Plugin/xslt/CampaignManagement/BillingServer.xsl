<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="service-id" select="$SERVICE_ID"/>

<xsl:template name="BillingServerConfigGenerator">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="server-root"/>
  <xsl:param name="billing-server-config"/>
  <xsl:param name="campaign-servers"/>

  <xsl:variable name="billing-server-hosts">
    <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor =
      $fe-cluster-descriptor]/service[@descriptor = $billing-server-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'CampaignManager'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="all-billing-server-count"
    select="count(exsl:node-set($billing-server-hosts)/host)"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$colo-config/cfg:environment/@workspace_root[1]"/>
    <xsl:if test="count($colo-config/cfg:environment/@workspace_root[1]) = 0"><xsl:value-of
      select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$colo-config/cfg:environment/@config_root[1]"/>
    <xsl:if test="count($colo-config/cfg:environment/@config_root[1]) = 0"><xsl:value-of
      select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>              
    <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>        
  </xsl:variable>

  <xsl:variable name="billing-server-port"><xsl:value-of select="$billing-server-config/cfg:networkParams/@port"/>
    <xsl:if test="count($billing-server-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-billing-server-port"/>
   </xsl:if>
  </xsl:variable>

  <exsl:document href="billingServer.port"
    method="text" omit-xml-declaration="yes"
    >  ['billingServer', <xsl:copy-of select="$billing-server-port"/>],</exsl:document>

  <xsl:variable name="update-config" select="$billing-server-config/cfg:updateParams"/>

  <cfg:BillingServer
    log_root="{concat($workspace-root, '/log/BillingServer/Out')}"
    service_index="{$service-id}"
    service_count="{$all-billing-server-count}"
    reserve_timeout="60">

    <xsl:attribute name="stat_update_period"><xsl:value-of select="$update-config/@stat_update_period"/>
      <xsl:if test="count($update-config/@stat_update_period) = 0">600</xsl:if>
    </xsl:attribute>

    <xsl:attribute name="config_update_period"><xsl:value-of select="$update-config/@config_update_period"/>
      <xsl:if test="count($update-config/@config_update_period) = 0">60</xsl:if>
    </xsl:attribute>

    <xsl:attribute name="max_stat_delay"><xsl:value-of select="$update-config/@max_stat_delay"/>
      <xsl:if test="count($update-config/@max_stat_delay) = 0">604800</xsl:if>
    </xsl:attribute>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$billing-server-config/cfg:threadParams/@min"/>
        <xsl:if test="count($billing-server-config/cfg:threadParams/@min) = 0">200</xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$billing-server-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="BillingServer" name="{$current-billing-server-obj}"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$billing-server-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $billing-server-log-path)"/>
      <xsl:with-param name="default-log-level" select="$billing-server-log-level"/>
    </xsl:call-template>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'CampaignManager'"/>
    </xsl:call-template>

    <cfg:Storage dir="{concat($cache-root, '/BillingServer/Amount')}"
      dump_period="900"/>
  </cfg:BillingServer>

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable name="full-cluster-path"
    select="$xpath/../.. | $xpath/.."/>

  <xsl:variable name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable name="fe-cluster-path"
    select="$xpath/.."/>

  <xsl:variable name="billing-server-path"
    select="$xpath"/>

  <xsl:variable name="billing-server-config"
     select="$billing-server-path/configuration/cfg:billingServer"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find full-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find fe cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($billing-server-path) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find billing server node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster/cfg:environment |
     $colo-config/cfg:environment"/>

  <xsl:variable name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>

  <xsl:variable name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find colo config </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-servers) = 0">
       <xsl:message terminate="yes"> BillingServer: Can't find campaign server nodes</xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/CampaignSvcs/BillingServerConfig.xsd')}">
    <xsl:call-template name="BillingServerConfigGenerator">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="server-root" select="$server-root"/>
      <xsl:with-param name="billing-server-config" select="$billing-server-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

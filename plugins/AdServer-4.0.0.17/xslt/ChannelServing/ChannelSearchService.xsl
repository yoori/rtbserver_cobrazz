<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- ChannelSearchService config generate function -->
<xsl:template name="ChannelSearchServiceConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="channel-controller-path"/>
  <xsl:param name="channel-search-service-config"/>

  <xsl:variable name="channel-search-service-port"><xsl:value-of select="$channel-search-service-config/cfg:networkParams/@port"/>
    <xsl:if test="count($channel-search-service-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-channel-search-service-port"/>
    </xsl:if>
  </xsl:variable>

  <exsl:document href="channelSearchService.port"
    method="text" omit-xml-declaration="yes"
    >  ['channelSearchService', <xsl:copy-of select="$channel-search-service-port"/>],</exsl:document>

  <xsl:variable name="channel-search-service-logging" select="$channel-search-service-config/cfg:logging"/>
  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>


  <cfg:ChannelSearchServiceConfig service_index="{$SERVICE_ID}">
    <!-- start config generation -->

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$channel-search-service-config/cfg:threadParams/@min"/>
        <xsl:if test="count($channel-search-service-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-channel-search-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$channel-search-service-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ChannelSearch" name="ChannelSearch"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-search-service-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-search-service-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-search-service-log-level"/>
    </xsl:call-template>

    <cfg:ChannelManagerControllerRefs name="ChannelManagerControllers">
      <xsl:for-each select="$channel-controller-path">

        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'ChannelSearchService:ChannelManagerController'"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:variable name="channel-controller-port">
          <xsl:value-of select="./configuration/cfg:channelController/cfg:networkParams/@port"/>
          <xsl:if test="count(./configuration/cfg:channelController/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-channel-controller-port"/>
          </xsl:if>
        </xsl:variable>

        <xsl:for-each select="exsl:node-set($hosts)//host">
        <cfg:Ref>
          <xsl:attribute name="ref">
            <xsl:value-of
              select="concat('corbaloc:iiop:', ., ':', $channel-controller-port, '/ChannelManagerController')"/>
          </xsl:attribute>
        </cfg:Ref>
        </xsl:for-each>
      </xsl:for-each>
    </cfg:ChannelManagerControllerRefs>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'ChannelSearchService'"/>
    </xsl:call-template>

  </cfg:ChannelSearchServiceConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="fe-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor] |
            $full-cluster-path/serviceGroup[@descriptor ='AdProfilingCluster/BackendSubCluster']"/>

  <xsl:variable
    name="channel-controller-path"
    select="$fe-cluster-path/service[@descriptor = $channel-controller-descriptor]"/>

  <xsl:variable
    name="channel-search-path"
    select="$fe-cluster-path/service[@descriptor = $channel-search-service-descriptor]"/>

  <xsl:variable
    name="campaign-server-path"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor][1] |
            $be-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer'][1]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelSearchService: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelSearchService: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-server-path) = 0">
       <xsl:message terminate="yes"> ChannelSearchService: Can't find no one CampaignServer </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelSearchService: Can't find fe-cluster group </xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="campaign-server-config"
    select="$campaign-server-path/configuration/cfg:campaignServer"/>

  <xsl:variable
    name="channel-search-service-config"
    select="$channel-search-path/configuration/cfg:channelSearchService"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> ChannelSearchService: Can't find colo config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSearchSvcs/ChannelSearchServiceConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelSearchServiceConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-server-path"/>
      <xsl:with-param name="channel-controller-path" select="$channel-controller-path"/>
      <xsl:with-param name="channel-search-service-config" select="$channel-search-service-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="exsl dyn"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration">

<xsl:output method="text" indent="no" encoding="utf-8"/>
<xsl:include href="Functions.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="conf-host" select="$HOST"/>

<xsl:template name="DBAccessConfigGenerator">
  <xsl:param name="log-generalizer-path"/>
  <xsl:param name="channel-servers-path"/>
  <xsl:param name="campaign-servers"/>

  <xsl:variable name="this-host" select="$conf-host"/>
  <xsl:for-each select="$log-generalizer-path">
    <xsl:variable name="log-generalizer-port"><xsl:value-of
       select="./configuration/cfg:logGeneralizer/cfg:networkParams/@port"/>
      <xsl:if test="count(./configuration/cfg:logGeneralizer/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-log-generalizer-port"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="./@host"/>
        <xsl:with-param name="error-prefix"  select="'DBaccess: log generalizer hosts resolving: '"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <xsl:if test=". = $this-host">
  $log_generalizer = "<xsl:value-of select="concat('corbaloc:iiop:', ., ':', $log-generalizer-port, '/ProcessControl')"/>";
      </xsl:if>
    </xsl:for-each>
  </xsl:for-each>

  <xsl:for-each select="$channel-servers-path">
    <xsl:variable name="channel-server-port"><xsl:value-of select="configuration/cfg:channelServer/cfg:networkParams/@port"/>
      <xsl:if test="count(configuration/cfg:channelServer/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-channel-server-port"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"  select="'DBaccess: channel server host resolving'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <xsl:if test=". = $this-host">
  $channel_server = "<xsl:value-of select="concat('corbaloc:iiop:', ., ':', $channel-server-port, '/ProcessControl')"/>";
      </xsl:if>
    </xsl:for-each>
  </xsl:for-each>

  <xsl:for-each select="$campaign-servers">
    <xsl:variable name="campaign-server-port"><xsl:value-of select="configuration/cfg:campaignServer/cfg:networkParams/@port"/>
      <xsl:if test="count(configuration/cfg:campaignServer/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-campaign-server-port"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"  select="'DBaccess: campaign server host resolving: '"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <xsl:if test=". = $this-host">
  $campaign_server = "<xsl:value-of select="concat('corbaloc:iiop:', ., ':', $campaign-server-port, '/ProcessControl')"/>";
      </xsl:if>
    </xsl:for-each>
  </xsl:for-each>
  
  1;

  <!--

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

%CONFIG = ( %CONFIG,
  log_file => "<xsl:value-of select="concat($workspace-root, '/log/PreStart/PreStart.log')"/>",
);

  <xsl:variable name="channel-controller-host-name">
    <xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="$channel-controller-host"/>
      <xsl:with-param name="error-prefix" select="'prestart: channel controller host resolving: '"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="user-info-manager-controller-host-name">
    <xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="$user-info-manager-controller-host"/>
      <xsl:with-param name="error-prefix" select="'prestart: user info manager controller host resolving: '"/>
    </xsl:call-template>
  </xsl:variable>

%CHECK_CORBA_REFS = ( %CHECK_CORBA_REFS,
  CampaignManager => {
    ref => "<xsl:value-of select="concat('corbaloc:iiop:localhost:', $campaign-manager-port, '/ProcessControl')"/>",
    status => "ready",
    required => 1},
  ChannelCluster => {
    ref => "<xsl:value-of select="concat('corbaloc:iiop:', $channel-controller-host-name, ':', $channel-controller-port, '/ChannelClusterControl')"/>",
    status => "ready",
    required => 1},
  UserInfoCluster => {
    ref => "<xsl:value-of select="concat('corbaloc:iiop:', $user-info-manager-controller-host-name, ':', $user-info-manager-controller-port, '/UserInfoClusterControl')"/>",
    status => "ready",
    required => 0},
);
-->
  
</xsl:template>

<xsl:template match="/">
  <xsl:variable
    name="full-cluster-path"
    select="$xpath"/>

  <xsl:variable
    name="backend-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>
  
  <xsl:variable
    name="frontend-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="channel-servers-path"
    select="$frontend-cluster-path/service[@descriptor = $channel-server-descriptor]"/>

  <xsl:variable
    name="campaign-servers"
    select="$backend-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>

  <xsl:variable
    name="log-generalizer-path"
    select="$backend-cluster-path/service[@descriptor = $log-generalizer-descriptor]"/>

  <!-- configs -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> DBAccess: Can't find XPATH element: <xsl:value-of select="dyn:evaluate($XPATH)"/> </xsl:message>
    </xsl:when>
    <xsl:when test="count($backend-cluster-path) = 0">
       <xsl:message terminate="yes"> DBAccess: Can't find backend cluster path</xsl:message>
    </xsl:when>
    <xsl:when test="count($frontend-cluster-path) = 0">
       <xsl:message terminate="yes"> DBAccess: Can't find frontend cluster path</xsl:message>
    </xsl:when>
    <xsl:when test="count($log-generalizer-path) = 0">
       <xsl:message terminate="yes"> DBAccess: Can't find log generalizer path </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="DBAccessConfigGenerator">
    <xsl:with-param name="log-generalizer-path" select="$log-generalizer-path"/>
    <xsl:with-param name="channel-servers-path" select="$channel-servers-path"/>
    <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

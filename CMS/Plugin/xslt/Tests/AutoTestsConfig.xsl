<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:str="http://exslt.org/strings"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- AutoTest config generate function -->
<xsl:template name="AutoTestConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="cluster-type"/>
  <xsl:param name="auto-test-config"/>
  <xsl:param name="campaign-manager-path"/>
  <xsl:param name="campaign-manager-config"/>
  <xsl:param name="campaign-server-path"/>
  <xsl:param name="campaign-server-config"/> 
  <xsl:param name="channel-search-path"/>
  <xsl:param name="channel-search-config"/>
  <xsl:param name="channel-server-path"/>
  <xsl:param name="channel-server-config"/>
  <xsl:param name="channel-controller-path"/>
  <xsl:param name="channel-controller-config"/>
  <xsl:param name="expression-matcher-path"/>
  <xsl:param name="expression-matcher-config"/>
  <xsl:param name="request-info-manager-path"/>
  <xsl:param name="request-info-manager-config"/>
  <xsl:param name="user-info-manager-path"/>
  <xsl:param name="user-info-controller-config"/>
  <xsl:param name="user-info-controller-path"/>
  <xsl:param name="user-info-manager-config"/>
  <xsl:param name="pg-connection"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="request-domain" select="$colo-config/cfg:coloParams/cfg:virtualServer/cfg:adservingDomain/@name"/>
  <xsl:variable name="external-http-domain-suffix">
    <xsl:if test="count($colo-config/cfg:coloParams/cfg:virtualServer/@port) > 0 and
      $colo-config/cfg:coloParams/cfg:virtualServer/@port != 80">:<xsl:value-of
      select="$colo-config/cfg:coloParams/cfg:virtualServer/@port"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="channel-search-host">
    <xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="$channel-search-path/@host"/>
    </xsl:call-template>
  </xsl:variable>
 
  <xsl:variable name="channel-search-port" select="$channel-search-config/cfg:networkParams/@port"/>

  <xsl:variable name="channel-server-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$channel-server-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'ChannelServer'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>

  <xsl:variable name="channel-server-port" select="$channel-server-config/cfg:networkParams/@port"/>


  <xsl:variable name="channel-controller-host">
    <xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="$channel-controller-path/@host"/>
    </xsl:call-template>
  </xsl:variable>
 
  <xsl:variable name="channel-controller-port" select="$channel-controller-config/cfg:networkParams/@port"/>

  <xsl:variable name="campaign-manager-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$campaign-manager-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'CampaignManager'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="campaign-manager-port" select="$campaign-manager-config/cfg:networkParams/@port"/>


  <xsl:variable name="campaign-server-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$campaign-server-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'CampaignServer'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="campaign-server-port" select="$campaign-server-config/cfg:networkParams/@port"/>


  <xsl:variable name="expression-matcher-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$expression-matcher-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'ExpressionMatcher'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="expression-matcher-port" select="$expression-matcher-config/cfg:networkParams/@port"/>


  <xsl:variable name="request-info-manager-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$request-info-manager-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'RequestInfoManager'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="request-info-manager-port" select="$request-info-manager-config/cfg:networkParams/@port"/>


  <xsl:variable name="user-info-manager-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$user-info-manager-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'UserInfoManager'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="user-info-manager-port" select="$user-info-manager-config/cfg:networkParams/@port"/>

  <xsl:variable name="user-info-controller-host">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$user-info-controller-path[1]/@host"/>
        <xsl:with-param name="error-prefix"  select="'UserInfoController'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="exsl:node-set($hosts)//host[1]"/>
  </xsl:variable>
 
  <xsl:variable name="user-info-controller-port" select="$user-info-controller-config/cfg:networkParams/@port"/>

  <TimeOuts frontend_timeout="60" wait_timeout="300"/>

  <Cluster>
    <xsl:attribute name="name">central</xsl:attribute>
    <Service>
      <xsl:attribute name="name">AdFrontend</xsl:attribute>
      <xsl:attribute name="address">http://<xsl:value-of select="$request-domain"/><xsl:value-of select="$external-http-domain-suffix"/></xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">ChannelServer</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$channel-server-host"/>:<xsl:value-of select="$channel-server-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">ChannelController</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$channel-controller-host"/>:<xsl:value-of select="$channel-controller-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">ChannelSearchServer</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$channel-search-host"/>:<xsl:value-of select="$channel-search-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">CampaignManager</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$campaign-manager-host"/>:<xsl:value-of select="$campaign-manager-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">CampaignServer</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$campaign-server-host"/>:<xsl:value-of select="$campaign-server-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">ExpressionMatcher</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$expression-matcher-host"/>:<xsl:value-of select="$expression-matcher-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">RequestInfoManager</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$request-info-manager-host"/>:<xsl:value-of select="$request-info-manager-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">UserInfoManager</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$user-info-manager-host"/>:<xsl:value-of select="$user-info-manager-port"/>
      </xsl:attribute>
    </Service>
    <Service>
      <xsl:attribute name="name">UserInfoManagerController</xsl:attribute>
      <xsl:attribute name="address">
        <xsl:value-of select="$user-info-controller-host"/>:<xsl:value-of select="$user-info-controller-port"/>
      </xsl:attribute>
    </Service>
  </Cluster>

  <LoggerGroup>
    <xsl:attribute name="path"><xsl:value-of select="$workspace-root"/>/log/Tests/AutoTests</xsl:attribute>
    <LogFile level_to="4" extension="err" />
    <LogFile level_to="17" extension="out" />
  </LoggerGroup>

  <ThreadsNum value="10"/>

  <PGDBConnection>
    <xsl:for-each select="str:tokenize($pg-connection/@connection_string,' ')">
      <xsl:variable name="connpart" select="str:tokenize(.,'=')"/>
      <xsl:if test="$connpart[1] != 'port'">
        <xsl:attribute name="{str:replace($connpart[1], 'dbname', 'db')}">
          <xsl:value-of select="$connpart[2]/text()"/>
        </xsl:attribute>
      </xsl:if>
    </xsl:for-each>
  </PGDBConnection>

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable name="auto-test-path" select="$xpath"/>

  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor][1]"/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor][1]"/>

  <xsl:variable
    name="channel-server-path"
    select="$fe-cluster-path/service[@descriptor = $channel-server-descriptor]"/>

  <xsl:variable
    name="channel-controller-path"
    select="$fe-cluster-path/service[@descriptor = $channel-controller-descriptor]"/>

  <xsl:variable
    name="channel-search-path"
    select="$fe-cluster-path/service[@descriptor = $channel-search-service-descriptor]"/>

  <xsl:variable
    name="campaign-manager-path"
    select="$fe-cluster-path/service[@descriptor = $campaign-manager-descriptor][1]"/>

  <xsl:variable
    name="campaign-server-path"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor][1]"/>

  <xsl:variable
    name="expression-matcher-path"
    select="$be-cluster-path//service[@descriptor = $expression-matcher-descriptor][1]"/>

  <xsl:variable
    name="request-info-manager-path"
    select="$be-cluster-path//service[@descriptor = $request-info-manager-descriptor][1]"/>

  <xsl:variable
    name="user-info-manager-path"
    select="$fe-cluster-path//service[@descriptor = $user-info-manager-descriptor][1]"/>

  <xsl:variable
    name="user-info-controller-path"
    select="$fe-cluster-path//service[@descriptor = $user-info-manager-controller-descriptor][1]"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find full-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find fe-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-search-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find channel search service node </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-server-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find channel server node </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-controller-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find channel controller node </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-server-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find campaign server node </xsl:message>
    </xsl:when>

    <xsl:when test="count($expression-matcher-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find expression matcher server node</xsl:message>
    </xsl:when>

    <xsl:when test="count($request-info-manager-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find request info manager server node</xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-manager-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find user info manager server node</xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-controller-path) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find user info controller server node</xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config" 
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="central-colo-config"
    select="$colo-config/cfg:central"/>

  <xsl:variable 
    name="remote-colo-config"
    select="$colo-config/cfg:remote"/>

  <xsl:variable name="cluster-type">
    <xsl:choose>    
      <xsl:when test="count($remote-colo-config) > 0">remote</xsl:when>
      <xsl:when test="count($central-colo-config) > 0">central</xsl:when>
      <xsl:otherwise>unknown</xsl:otherwise>
    </xsl:choose>
 </xsl:variable>

  <xsl:variable
    name="channel-search-config"
    select="$channel-search-path/configuration/cfg:channelSearchService"/>

  <xsl:variable
    name="channel-server-config"
    select="$channel-server-path/configuration/cfg:channelServer | $fe-cluster-path/configuration/cfg:frontendCluster/cfg:channelServer"/>

  <xsl:variable
    name="channel-controller-config"
    select="$channel-controller-path/configuration/cfg:channelController"/>

  <xsl:variable
    name="campaign-manager-config"
    select="$campaign-manager-path/configuration/cfg:campaignManager"/>

  <xsl:variable
    name="campaign-server-config"
    select="$campaign-server-path/configuration/cfg:campaignServer"/>

  <xsl:variable
    name="expression-matcher-config"
    select="$expression-matcher-path/configuration/cfg:expressionMatcher"/>

  <xsl:variable
    name="request-info-manager-config"
    select="$request-info-manager-path/configuration/cfg:requestInfoManager | $be-cluster-path/configuration/cfg:requestInfoManager"/>

  <xsl:variable
    name="user-info-manager-config"
    select="$user-info-manager-path/configuration/cfg:userInfoManager"/>

  <xsl:variable
    name="user-info-controller-config"
    select="$user-info-controller-path/configuration/cfg:userInfoManagerController"/>

  <xsl:variable
    name="auto-test-config"
    select="$auto-test-path/configuration/cfg:autoTest"/>

  <xsl:variable
    name="test-global-config"
    select="$auto-test-path/../configuration/cfg:testsCommon"/>

  <xsl:variable name="pg-connection" 
     select="$auto-test-config/cfg:pgConnection |
       $test-global-config/cfg:pgConnection"/>

  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find colo config </xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-manager-config) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find user info manager config</xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-controller-config) = 0">
       <xsl:message terminate="yes"> AutoTest: Can't find user info controller config</xsl:message>
    </xsl:when>

  </xsl:choose>

  <GeneralParams>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/tests/AutoTests/AutoTests.xsd')"/></xsl:attribute>
    <xsl:attribute name="userpref_body_xsd"><xsl:value-of select="concat($server-root, '/xsd/Frontends/UserPrefBody.xsd')"/></xsl:attribute>
    <xsl:attribute name="userdata_body_xsd"><xsl:value-of select="concat($server-root, '/xsd/Frontends/UserData.xsd')"/></xsl:attribute>
    <xsl:call-template name="AutoTestConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="cluster-type" select="$cluster-type"/>
      <xsl:with-param name="auto-test-config" select="$auto-test-config"/>
      <xsl:with-param name="campaign-manager-path" select="$campaign-manager-path"/>
      <xsl:with-param name="campaign-manager-config" select="$campaign-manager-config"/>
      <xsl:with-param name="campaign-server-path" select="$campaign-server-path"/>
      <xsl:with-param name="campaign-server-config" select="$campaign-server-config"/>
      <xsl:with-param name="channel-search-path" select="$channel-search-path"/>
      <xsl:with-param name="channel-search-config" select="$channel-search-config"/>
      <xsl:with-param name="channel-server-path" select="$channel-server-path"/>
      <xsl:with-param name="channel-server-config" select="$channel-server-config"/>
      <xsl:with-param name="channel-controller-path" select="$channel-controller-path"/>
      <xsl:with-param name="channel-controller-config" select="$channel-controller-config"/>
      <xsl:with-param name="expression-matcher-path" select="$expression-matcher-path"/>
      <xsl:with-param name="expression-matcher-config" select="$expression-matcher-config"/>
      <xsl:with-param name="request-info-manager-path" select="$request-info-manager-path"/>
      <xsl:with-param name="request-info-manager-config" select="$request-info-manager-config"/>
      <xsl:with-param name="user-info-manager-path" select="$user-info-manager-path"/>
      <xsl:with-param name="user-info-manager-config" select="$user-info-manager-config"/>
      <xsl:with-param name="user-info-controller-path" select="$user-info-controller-path"/>
      <xsl:with-param name="user-info-controller-config" select="$user-info-controller-config"/>
      <xsl:with-param name="pg-connection" select="$pg-connection"/>
    </xsl:call-template>
  </GeneralParams>

</xsl:template>

</xsl:stylesheet>

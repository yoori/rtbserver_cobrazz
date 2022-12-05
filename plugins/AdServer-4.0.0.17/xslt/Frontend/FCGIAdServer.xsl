<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- FCGIServer config generate function -->
<xsl:template name="FCGIServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="fcgi-adserver-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>

  <cfg:FCGIServerConfig>
    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="fe_config">
      <xsl:value-of select="concat($config-root,'/',$out-dir,'/FeConfig.xml')"/>
    </xsl:attribute>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="socket_arr">
      <i>1</i><i>2</i><i>3</i><i>4</i>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($socket_arr)/i">
      <cfg:BindSocket>
        <xsl:attribute name="backlog">
          <xsl:value-of select="$fcgi-adserver-config/cfg:adFCGINetworkParams/@backlog"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:adFCGINetworkParams/@backlog) = 0">1000</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="accept_threads">
          <xsl:value-of select="$fcgi-adserver-config/cfg:adFCGINetworkParams/@accept_threads"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:adFCGINetworkParams/@accept_threads) = 0">5</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="bind">
          <xsl:value-of select="concat($workspace-root,'/run/fcgi_adserver',., '.sock')"/>
        </xsl:attribute>
      </cfg:BindSocket>
    </xsl:for-each>

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="fcgi-adserver-port">
      <xsl:value-of select="$fcgi-adserver-config/cfg:adFCGINetworkParams/@port"/>
      <xsl:if test="count($fcgi-adserver-config/cfg:adFCGINetworkParams/@port) = 0">
        <xsl:value-of select="$def-fcgi-adserver-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="FCGIServer.port"
      method="text" omit-xml-declaration="yes"
      >  ['FCGIServer', <xsl:copy-of select="$fcgi-adserver-port"/>],</exsl:document>

    <xsl:variable name="fcgi-adserver-logging" select="$fcgi-adserver-config/cfg:logging"/>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$fcgi-adserver-config/cfg:threadParams/@min"/>
        <xsl:if test="count($fcgi-adserver-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$fcgi-adserver-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="FCGIServerStats" name="FCGIServerStats"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$fcgi-adserver-logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $fcgi-adserver-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-fcgiserver-log-level"/>
    </xsl:call-template>

    <cfg:Module name="pubpixel"/>
    <cfg:Module name="content"/>
    <cfg:Module name="directory"/>
    <cfg:Module name="webstat"/>
    <cfg:Module name="action"/>
    <cfg:Module name="passback"/>
    <cfg:Module name="passbackpixel"/>
    <cfg:Module name="optout"/>
    <cfg:Module name="nullad"/>
    <cfg:Module name="adinst"/>
    <cfg:Module name="click"/>
    <cfg:Module name="imprtrack"/>
    <cfg:Module name="ad"/>
  </cfg:FCGIServerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="fcgi-adserver-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> FCGIServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> FCGIServer: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> FCGIServer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fcgi-adserver-path) = 0">
       <xsl:message terminate="yes"> FCGIServer: Can't find log fcgi server node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="fcgi-adserver-config"
    select="$fcgi-adserver-path/configuration/cfg:frontend"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> FCGIServer: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- AdFrontend config required for FCGIServer -->
  <xsl:variable name="frontend-hosts">
    <xsl:for-each select="$xpath/../service[@descriptor = $http-frontend-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'AdFrontend hosts resolving'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

   <xsl:variable name="fcgi-hosts">
    <xsl:for-each select="$xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'AdFrontend hosts resolving'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($fcgi-hosts)/host">
    <xsl:variable name="fhost" select="."/>
    <xsl:choose>
      <xsl:when test="count(exsl:node-set($frontend-hosts)[host = $fhost]) = 0">
        <xsl:message terminate="yes"> FCGIServer: Can't find NginxFrontend on host '<xsl:value-of select="."/>' (NginxFrontend hosts: '<xsl:value-of select="$frontend-hosts"/>')</xsl:message>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/FCGIServerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="FCGIServerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="fcgi-adserver-config" select="$fcgi-adserver-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

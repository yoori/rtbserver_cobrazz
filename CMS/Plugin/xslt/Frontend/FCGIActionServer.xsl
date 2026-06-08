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
    <xsl:attribute name="grpc_coalesce_threads">
      <xsl:value-of select="$fcgi-adserver-config/cfg:trackConfig/@threads"/>
      <xsl:if test="count($fcgi-adserver-config/cfg:trackConfig/@threads) = 0">64</xsl:if>
    </xsl:attribute>
    <xsl:attribute name="service_index">0</xsl:attribute>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="pid_file">
      <xsl:value-of select="concat($workspace-root, '/run/FCGIActionServer.pid')"/>
    </xsl:attribute>

    <xsl:variable name="socket_arr">
      <i>1</i><i>2</i><i>3</i><i>4</i>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($socket_arr)/i">
      <cfg:BindSocket>
        <xsl:attribute name="backlog">
          <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@backlog"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@backlog) = 0">1000</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="accept_threads">
          <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@accept_threads"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@accept_threads) = 0">5</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="bind">
          <xsl:value-of select="concat($workspace-root,'/run/fcgi_actionserver_', . , '.sock')"/>
        </xsl:attribute>
      </cfg:BindSocket>
    </xsl:for-each>

    <xsl:variable name="fcgi-adserver-logging" select="$fcgi-adserver-config/cfg:logging"/>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$fcgi-adserver-logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $fcgi-actionserver-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-fcgiserver-log-level"/>
    </xsl:call-template>

    <xsl:variable name="fcgi-actionserver-mon-port">
      <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@monitoring_port"/>
      <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@monitoring_port) = 0">
        <xsl:value-of select="$def-fcgi-actionserver-mon-port"/>
      </xsl:if>
    </xsl:variable>

    <cfg:Monitoring port="{$fcgi-actionserver-mon-port}"/>

    <cfg:Module name="pubpixel"/>
    <cfg:Module name="webstat"/>
    <cfg:Module name="action"/>
    <cfg:Module name="passback"/>
    <cfg:Module name="passbackpixel"/>
    <cfg:Module name="optout"/>
  </cfg:FCGIServerConfig>

</xsl:template>

<xsl:template match="/">
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
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> FCGIActionServer: Can't find XPATH element </xsl:message>
    </xsl:when>
    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> FCGIActionServer: Can't find full cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> FCGIActionServer: Can't find be-cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($fcgi-adserver-path) = 0">
       <xsl:message terminate="yes"> FCGIActionServer: Can't find campaign server node </xsl:message>
    </xsl:when>
  </xsl:choose>

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

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/FCGIServerConfig.xsd')}">
    <xsl:call-template name="FCGIServerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="fcgi-adserver-config" select="$fcgi-adserver-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>
</xsl:stylesheet>

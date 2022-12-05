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

<xsl:include href="../Functions.xsl"/>

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="channel-proxy-mode" select="$MODE"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- ChannelProxy config generate function -->
<xsl:template name="ChannelProxyConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="proxy-mode"/>
  <xsl:param name="remote-colo-config"/>
  <xsl:param name="local-proxy-config"/>
  <xsl:param name="channel-proxy-config"/>
  <xsl:param name="secure-files-root"/>

  <cfg:ChannelProxyConfig>
    <xsl:variable name="channel-proxy-port">
    <xsl:value-of select="$channel-proxy-config/cfg:networkParams/@port"/>
      <xsl:if test="count($channel-proxy-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-channel-proxy-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="channelProxy.port"
      method="text" omit-xml-declaration="yes"
      >  ['channelProxy', <xsl:copy-of select="$channel-proxy-port"/>],</exsl:document>

    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="channel-proxy-ref" select="$channel-proxy-config/cfg:channelProxyRef | $remote-colo-config/cfg:channelProxyRef"/>
    <xsl:variable name="channel-controller-ref" select="$channel-proxy-config/cfg:channelControllerRef | $remote-colo-config/cfg:channelControllerRef"/>

    <xsl:variable name="external-network-params" select="$channel-proxy-config/cfg:externalNetworkParams"/>
    <xsl:variable name="global-secure-params" select="$remote-colo-config/cfg:secureParams"/>

    <xsl:variable name="channel-controllers">
      <xsl:call-template name="GetServiceRefs">
        <xsl:with-param name="services" 
          select="//service[@descriptor = $channel-controller-descriptor] |
                  //service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController']"/>
        <xsl:with-param name="def-port" select="$def-channel-controller-port"/>
        <xsl:with-param name="error-prefix" select="'ChannelProxy: ChannelController'" />
      </xsl:call-template>
    </xsl:variable>

    <!-- start config generation -->
    <!-- check that defined all needed parameters -->
    <xsl:choose>
      <xsl:when test="count($channel-controller-ref) = 0 and count($channel-proxy-ref) = 0
                      and count(exsl:node-set($channel-controllers)) = 0">
        <xsl:message terminate="yes"> ChannelProxy: channelControllerRef undefined. </xsl:message>
      </xsl:when>
    </xsl:choose>

    <xsl:attribute name="log_root"><xsl:value-of select="$workspace-root"/>/log/ChannelProxy/Out</xsl:attribute>
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$channel-proxy-config/cfg:threadParams/@min"/>
        <xsl:if test="count($channel-proxy-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-channel-proxy-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$channel-proxy-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ChannelProxy_v33" name="ChannelProxy_v33"/>
        <cfg:Object servant="ChannelProxy_v28" name="ChannelProxy_v28"/>
        <cfg:Object servant="ChannelProxy" name="ChannelProxy"/>
      </cfg:Endpoint>
      <xsl:if test="count($external-network-params) > 0">
        <xsl:variable name="external-host" select="$external-network-params/@host"/>

        <cfg:Endpoint>
          <xsl:attribute name="host"><xsl:value-of select="$external-host"/></xsl:attribute>
          <xsl:attribute name="port"><xsl:value-of select="$external-network-params/@port"/></xsl:attribute>
          <xsl:call-template name="ConvertSecureParams">
            <xsl:with-param name="secure-node" select="$external-network-params/@secure"/>
            <xsl:with-param name="global-secure-node" select="$global-secure-params"/>
            <xsl:with-param name="config-root" select="$config-root"/>
            <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
          </xsl:call-template>

          <cfg:Object servant="ChannelProxy_v33" name="ChannelProxy_v33"/>
          <cfg:Object servant="ChannelProxy_v28" name="ChannelProxy_v28"/>
          <cfg:Object servant="ChannelProxy" name="ChannelProxy"/>
        </cfg:Endpoint>
      </xsl:if>
    </cfg:CorbaConfig>

    <xsl:variable name="channel-controller-proxy-refs" select="$channel-proxy-config/cfg:channelControllerRef | $remote-colo-config/cfg:channelControllerRef"/>

    <xsl:variable
      name="channel-controller-refs"
      select="$channel-controller-proxy-refs[count($channel-controller-proxy-refs) > 0] |
              exsl:node-set($channel-controllers)[count($channel-controller-proxy-refs) = 0]//serviceRef"/>

    <xsl:choose>
      <xsl:when test="$proxy-mode = 'PROXY-MODE'">
        <cfg:ControllerCorbaRefs name="ChannelController">
          <xsl:for-each select="$channel-controller-refs">
            <cfg:Ref>
              <xsl:attribute name="ref">
                <xsl:value-of
                  select="concat('corbaloc:iiop:', @host, ':', @port, '/ChannelManagerController')"/>
              </xsl:attribute>
            </cfg:Ref>
          </xsl:for-each>
        </cfg:ControllerCorbaRefs>
      </xsl:when>
      <xsl:when test="$proxy-mode = 'REMOTE-MODE'">
        <cfg:ProxyCorbaRefs name="ChannelProxy">
          <xsl:for-each select="$channel-proxy-ref">
            <xsl:variable name="secure-params" select="./cfg:secureParams"/>
            <xsl:variable name="channel-proxy-host" select="@host"/>
            <xsl:variable name="proxy-port" select="@port"/>

            <cfg:Ref>
              <xsl:choose>
                <xsl:when test="count($secure-params) > 0">
                  <xsl:attribute name="ref">
                    <xsl:value-of
                      select="concat('corbaloc:ssliop:', $channel-proxy-host, ':', $proxy-port, $current-channel-proxy-obj)"/>
                  </xsl:attribute>
                  <xsl:call-template name="ConvertSecureParams">
                    <xsl:with-param name="secure-node" select="'true'"/>
                    <xsl:with-param name="global-secure-node" select="$global-secure-params"/>
                    <xsl:with-param name="config-root" select="$config-root"/>
                    <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
                  </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:attribute name="ref">
                    <xsl:value-of
                      select="concat('corbaloc:iiop:', $channel-proxy-host, ':', $proxy-port, $current-channel-proxy-obj)"/>
                  </xsl:attribute>
                </xsl:otherwise>
              </xsl:choose>
            </cfg:Ref>
          </xsl:for-each>
        </cfg:ProxyCorbaRefs>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"> ChannelProxy: unknown mode <xsl:value-of select="$MODE"/>. </xsl:message>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-proxy-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-proxy-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-proxy-log-level"/>
    </xsl:call-template>

    <xsl:if test="$proxy-mode = 'PROXY-MODE'">
      <cfg:UpdateStatLogger>
        <xsl:attribute name="size">3</xsl:attribute>
        <xsl:attribute name="period">30</xsl:attribute>
        <xsl:attribute name="path">ColoUpdateStat</xsl:attribute>
      </cfg:UpdateStatLogger>
    </xsl:if>

  </cfg:ChannelProxyConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../../.. | $xpath/.."/>

  <xsl:variable
    name="channel-proxy-path"
    select="$xpath"/>

  <xsl:variable
    name="local-proxy-path"
    select="$xpath/.."/>

  <xsl:variable name="proxy-mode" select="$channel-proxy-mode"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelProxy: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelProxy: Can't find full cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="remote-colo-config"
    select="$colo-config | $colo-config/cfg:remote"/>

  <xsl:variable
    name="local-proxy-config"
    select="$local-proxy-path/configuration/cfg:localProxy"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable
    name="channel-proxy-config"
    select="$xpath/configuration/cfg:channelProxy"/>

  <xsl:variable name="secure-files-root" select="concat('/', $out-dir, '/cert/')"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> ChannelProxy: Can't find colocation config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/ChannelProxyConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelProxyConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="proxy-mode" select="$proxy-mode"/>
      <xsl:with-param name="remote-colo-config" select="$remote-colo-config"/>
      <xsl:with-param name="local-proxy-config" select="$local-proxy-config"/>
      <xsl:with-param name="channel-proxy-config" select="$channel-proxy-config"/>
      <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

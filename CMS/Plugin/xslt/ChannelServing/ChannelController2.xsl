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
  exclude-result-prefixes="colo exsl dyn">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="AddChannelServerGroup">
  <xsl:param name="all-hosts"/>
  <xsl:param name="start"/>
  <xsl:param name="end"/>
  <xsl:param name="channel-servers"/>

  <cfg:ChannelServerGroup>
    <xsl:for-each select="$all-hosts">
      <xsl:if test="$start &lt; position() and position() &lt; $end">
        <xsl:variable name="this" select="."/>
        <xsl:variable name="channel-server-grpc-port">
          <xsl:for-each select="$channel-servers">
            <xsl:variable name="channel-server-config" select="configuration/cfg:channelServer"/>
            <xsl:variable name="hosts-for-config">
              <xsl:call-template name="GetHosts">
                <xsl:with-param name="hosts" select="@host"/>
                <xsl:with-param name="error-prefix" select="'ChannelController2'"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:for-each select="exsl:node-set($hosts-for-config)//host">
              <xsl:if test=". = $this">
                <xsl:value-of select="$channel-server-config/cfg:networkParams/@grpc_port"/>
                <xsl:if test="count($channel-server-config/cfg:networkParams/@grpc_port) = 0">
                  <xsl:variable name="channel-server-port">
                    <xsl:value-of select="$channel-server-config/cfg:networkParams/@port"/>
                    <xsl:if test="count($channel-server-config/cfg:networkParams/@port) = 0">
                      <xsl:value-of select="$def-channel-server-port"/>
                    </xsl:if>
                  </xsl:variable>
                  <xsl:value-of select="$channel-server-port + 500"/>
                </xsl:if>
              </xsl:if>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:variable>

        <cfg:ChannelServerHost>
          <cfg:ChannelServerGrpcRef host="{.}" port="{$channel-server-grpc-port}"/>
        </cfg:ChannelServerHost>
      </xsl:if>
    </xsl:for-each>
  </cfg:ChannelServerGroup>
</xsl:template>

<xsl:template name="ChannelController2ConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="channel-controller-config"/>
  <xsl:param name="channel-serving-config"/>
  <xsl:param name="channel-servers"/>

  <cfg:ChannelController2Config>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="chunks-count"><xsl:value-of select="$channel-serving-config/cfg:scaleParams/@chunks_count"/>
      <xsl:if test="count($channel-serving-config/cfg:scaleParams/@chunks_count) = 0">
        <xsl:value-of select="$channel-serving-scale-chunks"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="channel-controller-grpc-port">
      <xsl:choose>
        <xsl:when test="count($channel-controller-config/cfg:networkParams/@port) > 0">
          <xsl:value-of select="$channel-controller-config/cfg:networkParams/@port"/>
        </xsl:when>
        <xsl:when test="count($channel-controller-config/cfg:networkParams/@grpc_port) > 0">
          <xsl:value-of select="$channel-controller-config/cfg:networkParams/@grpc_port"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$def-channel-controller2-grpc-port"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:attribute name="count_chunks"><xsl:value-of select="$chunks-count"/></xsl:attribute>

    <cfg:GrpcConfig>
      <cfg:Endpoint host="*" port="{$channel-controller-grpc-port}"/>
    </cfg:GrpcConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-controller-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-controller2-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-controller-log-level"/>
    </xsl:call-template>

    <xsl:variable name="all-hosts">
      <xsl:for-each select="$channel-servers">
        <xsl:value-of select="@host"/>
        <xsl:if test="position() != last()">
          <xsl:value-of select="' '"/>
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="all-val-hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$all-hosts"/>
        <xsl:with-param name="error-prefix" select="'ChannelController2'"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="all-ch-hosts" select="exsl:node-set($all-val-hosts)//host"/>
    <xsl:variable name="count-hosts" select="count($all-ch-hosts)"/>

    <xsl:variable name="host-in-group">
      <xsl:choose>
        <xsl:when test="count($channel-controller-config/@hosts_in_group) = 0 or
          $channel-controller-config/@hosts_in_group = 0">
          <xsl:value-of select="$count-hosts"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$channel-controller-config/@hosts_in_group"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:for-each select="$all-ch-hosts">
      <xsl:variable name="num" select="position()"/>
      <xsl:if test="($count-hosts - $num + 1 &gt; $host-in-group and $num mod $host-in-group = 0)
        or ($count-hosts = $num)">
        <xsl:call-template name="AddChannelServerGroup">
          <xsl:with-param name="all-hosts" select="$all-ch-hosts"/>
          <xsl:with-param name="start" select="(floor($num div $host-in-group) - 1) * $host-in-group"/>
          <xsl:with-param name="end" select="$num + 1"/>
          <xsl:with-param name="channel-servers" select="$channel-servers"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>
  </cfg:ChannelController2Config>
</xsl:template>

<xsl:template match="/">
  <xsl:variable name="full-cluster-path" select="$xpath/../.."/>
  <xsl:variable name="fe-cluster-path" select="$xpath/.."/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelController2: Can't find XPATH element </xsl:message>
    </xsl:when>
    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController2: Can't find full cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController2: Can't find fe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="colo-config" select="$full-cluster-path/configuration/cfg:cluster"/>
  <xsl:variable name="fe-config" select="$fe-cluster-path/configuration/cfg:frontendCluster"/>
  <xsl:variable name="env-config" select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>
  <xsl:variable name="channel-controller-config" select="$xpath/configuration/cfg:channelController2"/>
  <xsl:variable name="channel-serving-config" select="$fe-config/cfg:channelServing"/>
  <xsl:variable name="channel-servers" select="$fe-cluster-path/service[@descriptor = $channel-server-descriptor]"/>
  <xsl:variable name="server-install-root" select="$env-config/@server_root"/>
  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/ChannelController2Config.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelController2ConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="channel-controller-config" select="$channel-controller-config"/>
      <xsl:with-param name="channel-serving-config" select="$channel-serving-config"/>
      <xsl:with-param name="channel-servers" select="$channel-servers"/>
    </xsl:call-template>
  </cfg:AdConfiguration>
</xsl:template>

</xsl:stylesheet>

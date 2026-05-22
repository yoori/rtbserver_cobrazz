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
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="cluster_id" select="$CLUSTER_ID * 1024"/>

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
                <xsl:with-param name="error-prefix" select="'ChannelController'"/>
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
          <xsl:variable name="channel-server-corba-port">
            <xsl:for-each select="$channel-servers">
              <xsl:variable name="channel-server-config" select="configuration/cfg:channelServer"/>
              <xsl:variable name="hosts-for-config">
                <xsl:call-template name="GetHosts">
                  <xsl:with-param name="hosts" select="@host"/>
                  <xsl:with-param name="error-prefix" select="'ChannelController'"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:for-each select="exsl:node-set($hosts-for-config)//host">
                <xsl:if test=". = $this">
                  <xsl:value-of select="$channel-server-config/cfg:networkParams/@port"/>
                  <xsl:if test="count($channel-server-config/cfg:networkParams/@port) = 0">
                    <xsl:value-of select="$def-channel-server-port"/>
                  </xsl:if>
                </xsl:if>
              </xsl:for-each>
            </xsl:for-each>
          </xsl:variable>
          <cfg:ChannelUpdateRef name="ChannelUpdate" ref="{concat('corbaloc:iiop:', ., ':', $channel-server-corba-port, '/ChannelUpdate')}"/>
        </cfg:ChannelServerHost>
      </xsl:if>
    </xsl:for-each>
  </cfg:ChannelServerGroup>
</xsl:template>

<xsl:template name="ChannelControllerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="central-config"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="channel-controller-config"/>
  <xsl:param name="channel-serving-config"/>
  <xsl:param name="channel-servers"/>
  <xsl:param name="campaign-servers"/>

  <cfg:ChannelControllerConfig>
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
          <xsl:value-of select="$def-channel-controller-grpc-port"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:attribute name="count_chunks"><xsl:value-of select="$chunks-count"/></xsl:attribute>

    <cfg:GrpcConfig>
      <cfg:Endpoint host="*" port="{$channel-controller-grpc-port}"/>
    </cfg:GrpcConfig>

    <cfg:ColoSettings>
      <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>
      <xsl:variable name="version" select="$colo-config/cfg:coloParams/@version"/>
      <xsl:if test="count($colo-id) = 0">
        <xsl:message terminate="yes"> ChannelController: undefined colo_id. </xsl:message>
      </xsl:if>
      <xsl:attribute name="colo"><xsl:value-of select="$colo-id"/></xsl:attribute>
      <xsl:attribute name="version">
        <xsl:choose>
          <xsl:when test="count($version)!=0">
            <xsl:value-of select="$version"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$app-version"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </cfg:ColoSettings>

    <cfg:ChannelSource>
      <xsl:if test="$channel-controller-config/@local_load = 'true'">
        <xsl:attribute name="local_groups"><xsl:value-of select="'0'"/></xsl:attribute>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="count($central-config) > 0">
          <xsl:if test="string-length($central-config/cfg:pgConnection/@connection_string) = 0">
            <xsl:message terminate="yes"> ChannelController(central mode): db connection undefined. </xsl:message>
          </xsl:if>
          <cfg:RegularSource>
            <xsl:call-template name="CampaignServerCorbaRefs">
              <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
              <xsl:with-param name="service-name" select="'ChannelController(central mode)'"/>
            </xsl:call-template>
            <cfg:PGConnection>
              <xsl:attribute name="connection_string">
                <xsl:value-of select="$central-config/cfg:pgConnection/@connection_string"/>
              </xsl:attribute>
            </cfg:PGConnection>
          </cfg:RegularSource>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes"> ChannelController: can't find channel source. </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </cfg:ChannelSource>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-controller-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-controller-log-path)"/>
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
        <xsl:with-param name="error-prefix" select="'ChannelController'"/>
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
  </cfg:ChannelControllerConfig>
</xsl:template>

<xsl:template match="/">
  <xsl:variable name="full-cluster-path" select="$xpath/../.."/>
  <xsl:variable name="fe-cluster-path" select="$xpath/.."/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find XPATH element </xsl:message>
    </xsl:when>
    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find full cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find fe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="colo-config" select="$full-cluster-path/configuration/cfg:cluster"/>
  <xsl:variable name="fe-config" select="$fe-cluster-path/configuration/cfg:frontendCluster"/>
  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor] |
            $full-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster']"/>
  <xsl:variable name="central-config" select="$colo-config/cfg:central"/>
  <xsl:variable name="env-config" select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>
  <xsl:variable name="channel-controller-config" select="$xpath/configuration/cfg:channelController"/>
  <xsl:variable name="channel-serving-config" select="$fe-config/cfg:channelServing"/>
  <xsl:variable name="channel-servers" select="$fe-cluster-path/service[@descriptor = $channel-server-descriptor]"/>
  <xsl:variable name="campaign-servers" select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>
  <xsl:variable name="server-install-root" select="$env-config/@server_root"/>
  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/ChannelControllerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelControllerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="central-config" select="$central-config"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="channel-controller-config" select="$channel-controller-config"/>
      <xsl:with-param name="channel-serving-config" select="$channel-serving-config"/>
      <xsl:with-param name="channel-servers" select="$channel-servers"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
    </xsl:call-template>
  </cfg:AdConfiguration>
</xsl:template>

</xsl:stylesheet>

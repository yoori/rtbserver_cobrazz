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
<xsl:variable name="cluster_id" select="$CLUSTER_ID * 1024"/>
<xsl:variable name="service_id" select="$SERVICE_ID"/>

<xsl:template name="AddControlHosts">
  <xsl:param name="all-hosts"/>
  <xsl:param name="start"/>
  <xsl:param name="end"/>
  <xsl:param name="port"/>

  <cfg:ControlGroup>
    <xsl:for-each select="$all-hosts">
      <xsl:if test="$start &lt; position() and position() &lt; $end">
        <xsl:variable name="corba-ref-prefix" select="concat('corbaloc:iiop:', ., ':', $port)"/>
        <cfg:ChannelHost>
          <cfg:ChannelServerControlRef name="ChannelServerControl">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba-ref-prefix, '/ChannelServerControl')"/>
            </xsl:attribute>
          </cfg:ChannelServerControlRef>
          <cfg:ChannelServerProcRef name="ChannelServerProc">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba-ref-prefix, '/ProcessControl')"/>
            </xsl:attribute>
          </cfg:ChannelServerProcRef>
          <cfg:ChannelServerRef name="ChannelServer">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba-ref-prefix, '/ChannelServer')"/>
            </xsl:attribute>
          </cfg:ChannelServerRef>
          <cfg:ChannelUpdateRef name="ChannelUpdate">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba-ref-prefix, '/ChannelUpdate')"/>
            </xsl:attribute>
          </cfg:ChannelUpdateRef>
          <cfg:ChannelStatRef name="ProcessStatsControl">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba-ref-prefix, '/ProcessStatsControl')"/>
            </xsl:attribute>
          </cfg:ChannelStatRef>
        </cfg:ChannelHost>
      </xsl:if>
    </xsl:for-each>
  </cfg:ControlGroup>
</xsl:template>

<!-- ChannelController config generate function -->
<xsl:template name="ChannelControllerConfigGenerator">
  <xsl:param name="colo-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="central-config"/>
  <xsl:param name="remote-config"/>
  <xsl:param name="channel-servers"/>
  <xsl:param name="channel-controller-config"/>
  <xsl:param name="channel-serving-config"/>
  <xsl:param name="channel-proxy-path"/>
  <xsl:param name="channel-proxy-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="cluster-id"/>

  <cfg:ChannelControllerConfig>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="chunks_count"><xsl:value-of select="$channel-serving-config/cfg:scaleParams/@chunks_count"/>
      <xsl:if test="count($channel-serving-config/cfg:scaleParams/@chunks_count) = 0"><xsl:value-of select="$channel-serving-scale-chunks"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="channel-controller-port">
      <xsl:value-of select="$channel-controller-config/cfg:networkParams/@port"/>
      <xsl:if test="count($channel-controller-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-channel-controller-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="channelController.port"
      method="text" omit-xml-declaration="yes"
      >  ['channelController', <xsl:copy-of select="$channel-controller-port"/>],</exsl:document>

    <!-- start config generation -->
    <xsl:attribute name="count_chunks"><xsl:value-of select="$chunks_count"/></xsl:attribute>
    <xsl:attribute name="source_id">
      <xsl:choose>
        <xsl:when test="count($central-config) > 0">
          <xsl:value-of select="$cluster-id"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="-1"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <cfg:Primary>
      <xsl:attribute name="control">
        <xsl:choose>
          <xsl:when test="$service_id = 1">
            <xsl:value-of select="'true'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'false'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </cfg:Primary>
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$channel-controller-config/cfg:threadParams/@min"/>
        <xsl:if test="count($channel-controller-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-channel-controller-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$channel-controller-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ChannelManagerController" name="ChannelManagerController"/>
        <cfg:Object servant="ChannelClusterControl" name="ChannelClusterControl"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

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
        <xsl:attribute name="local_groups"><xsl:value-of select='0'/></xsl:attribute>
      </xsl:if>

      <xsl:choose>
        <xsl:when test="count($central-config) > 0">
          <xsl:choose>
            <xsl:when test="string-length($central-config/cfg:pgConnection/@connection_string) = 0">
              <xsl:message terminate="yes"> ChannelController(central mode): db connection undefined. </xsl:message>
            </xsl:when>
          </xsl:choose>
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
        <xsl:when test="count($remote-config) > 0">
          <cfg:ProxySource>
            <xsl:call-template name="CampaignServerCorbaRefs">
              <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
              <xsl:with-param name="service-name" select="'ChannelController(remote)'"/>
            </xsl:call-template>
            <xsl:variable name="channel-proxy-port">
              <xsl:value-of select="$channel-proxy-config/cfg:networkParams/@port"/>
              <xsl:if test="count($channel-proxy-config/cfg:networkParams/@port) = 0">
                <xsl:value-of select="$def-channel-proxy-port"/>
              </xsl:if>
            </xsl:variable>
            <xsl:variable name="channel-proxy-host">
              <xsl:call-template name="ResolveHostName">
                <xsl:with-param name="base-host" select="$channel-proxy-path/@host"/>
                <xsl:with-param name="error-prefix" select="'ChannelController'"/>
              </xsl:call-template>
            </xsl:variable>

            <cfg:ProxyRefs name="ChannelProxy">
              <cfg:Ref>
                <xsl:attribute name="ref">
                  <xsl:value-of
                    select="concat('corbaloc:iiop:', $channel-proxy-host, ':', $channel-proxy-port, $current-channel-proxy-obj)"/>
                </xsl:attribute>
              </cfg:Ref>
            </cfg:ProxyRefs>
          </cfg:ProxySource>
        </xsl:when>
      </xsl:choose>
    </cfg:ChannelSource>

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
        <xsl:with-param name="error-prefix"
          select="'ChannelController'"/>
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

          <xsl:variable name="this" select="."/>
          <xsl:variable name="p-channel-server-port">
            <xsl:for-each select="$channel-servers">
              <xsl:variable name="p-channel-server-config" select="configuration/cfg:channelServer"/>
              <xsl:variable name="hosts-for-config">
                <xsl:call-template name="GetHosts">
                  <xsl:with-param name="hosts" select="@host"/>
                  <xsl:with-param name="error-prefix"
                    select="'ChannelController'"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:for-each select="exsl:node-set($hosts-for-config)//host">
                <xsl:if test=". = $this">
                  <xsl:value-of select="$p-channel-server-config/cfg:networkParams/@port"/>
                  <xsl:if test="count($p-channel-server-config/cfg:networkParams/@port) = 0">
                    <xsl:value-of select="$def-channel-server-port"/>
                  </xsl:if>
                </xsl:if>
              </xsl:for-each>
            </xsl:for-each>
          </xsl:variable>

          <xsl:call-template name="AddControlHosts">
            <xsl:with-param name="all-hosts" select="$all-ch-hosts"/>
            <xsl:with-param name="start" select="(floor($num div $host-in-group) - 1) * $host-in-group"/>
            <xsl:with-param name="end" select="$num + 1"/>
            <xsl:with-param name="port" select="$p-channel-server-port"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:for-each>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-controller-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-controller-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-controller-log-level"/>
    </xsl:call-template>

  </cfg:ChannelControllerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="cluster-id"
    select="$cluster_id"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="backend-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor] |
            $full-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster']"/>

  <xsl:variable
    name="local-proxy-cluster-path"
    select="$backend-cluster-path/serviceGroup[@descriptor = $local-proxy-descriptor] |
            $backend-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster/LocalProxy']"/>

  <xsl:variable
    name="channel-servers"
    select="$fe-cluster-path/service[@descriptor = $channel-server-descriptor] |
            $fe-cluster-path/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelServer']"/>

  <xsl:variable
    name="campaign-servers"
    select="$backend-cluster-path/service[@descriptor = $campaign-server-descriptor] |
            $backend-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer']"/>

  <xsl:variable
    name="channel-controller-path"
    select="$fe-cluster-path/service[@descriptor = $channel-controller-descriptor][1] |
            $fe-cluster-path/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController'][1]"/>

  <xsl:variable
    name="channel-proxy-path"
    select="$local-proxy-cluster-path/service[@descriptor = $remote-channel-proxy-descriptor] |
            $local-proxy-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/LocalProxy/ChannelProxy']"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($backend-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find fe-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-controller-path/@host) = 0">
       <xsl:message terminate="yes"> ChannelController: Channel controller host undefined </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-servers) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find channel server service </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-servers) = 0">
       <xsl:message terminate="yes"> ChannelController: Can't find campaign servers node</xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="central-config"
    select="$colo-config/cfg:central"/>

  <xsl:variable
    name="remote-config"
    select="$colo-config/cfg:remote"/>

  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="channel-server-config"
    select="$xpath/configuration/cfg:channelServer  | $fe-config/cfg:channelServer"/>

  <xsl:variable
    name="channel-controller-config"
    select="$xpath/configuration/cfg:channelController"/>

  <xsl:variable
    name="channel-serving-config"
    select="$fe-config/cfg:channelServing"/>

  <xsl:variable
    name="server-install-root"
    select="$colo-config/cfg:environment/@server_root"/>

  <xsl:variable
    name="channel-proxy-config"
    select="$channel-proxy-path/configuration/cfg:channelProxy"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/ChannelManagerControllerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelControllerConfigGenerator">
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="central-config" select="$central-config"/>
      <xsl:with-param name="remote-config" select="$remote-config"/>
      <xsl:with-param name="channel-servers" select="$channel-servers"/>
      <xsl:with-param name="channel-controller-config" select="$channel-controller-config"/>
      <xsl:with-param name="channel-serving-config" select="$channel-serving-config"/>
      <xsl:with-param name="channel-proxy-path" select="$channel-proxy-path"/>
      <xsl:with-param name="channel-proxy-config" select="$channel-proxy-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="cluster-id" select="$cluster-id"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

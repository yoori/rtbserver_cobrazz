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

<xsl:include href="../GrpcChannelArgs.xsl"/>
<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="cluster-id" select="$CLUSTER_ID"/>

<!-- FCGIServer config generate function -->
<xsl:template name="FCGIServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="fcgi-adserver-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>
  <xsl:param name="channel-servers-path"/>
  <xsl:param name="campaign-manager-host-grpc-port-set"/>

  <cfg:FCGIServerConfig>
    <xsl:attribute name="time_duration_grpc_client_mark_bad">
      <xsl:value-of select="$def-time-duration-grpc-client-mark-bad"/>
    </xsl:attribute>
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
          <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@backlog"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@backlog) = 0">1000</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="accept_threads">
          <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@accept_threads"/>
          <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@accept_threads) = 0">5</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="bind">
          <xsl:value-of select="concat($workspace-root,'/run/fcgi_trackserver',., '.sock')"/>
        </xsl:attribute>
      </cfg:BindSocket>
    </xsl:for-each>

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="fcgi-trackserver-port">
      <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@port"/>
      <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@port) = 0">
        <xsl:value-of select="$def-fcgi-trackserver-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="FCGIServer.port"
      method="text" omit-xml-declaration="yes"
      >  ['FCGIServer', <xsl:copy-of select="$fcgi-trackserver-port"/>],</exsl:document>

    <xsl:variable name="fcgi-adserver-logging" select="$fcgi-adserver-config/cfg:logging"/>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$fcgi-adserver-config/cfg:threadParams/@min"/>
        <xsl:if test="count($fcgi-adserver-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$fcgi-trackserver-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="FCGIServerStats" name="FCGIServerStats"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$fcgi-adserver-logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $fcgi-trackserver-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-fcgiserver-log-level"/>
    </xsl:call-template>

    <xsl:variable name="fcgi-trackserver-mon-port">
      <xsl:value-of select="$fcgi-adserver-config/cfg:trackFCGINetworkParams/@monitoring_port"/>
      <xsl:if test="count($fcgi-adserver-config/cfg:trackFCGINetworkParams/@monitoring_port) = 0">
        <xsl:value-of select="$def-fcgi-trackserver-mon-port"/>
      </xsl:if>
    </xsl:variable>

    <cfg:Monitoring port="{$fcgi-trackserver-mon-port}"/>

    <cfg:Coroutine>
      <cfg:CoroPool
        initial_size="{$coro-pool-initial-size}"
        max_size="{$coro-pool-max-size}"
        stack_size="{$coro-pool-stack-size}"/>
      <cfg:EventThreadPool
        number_threads="{$event-thread-pool-number-threads}"
        name="{$event-thread-pool-name}"
        ev_default_loop_disabled="{$event-thread-pool-ev-default-loop-disabled}"/>
      <cfg:MainTaskProcessor
        name="{$main-task-processor-name}"
        number_threads="{$main-task-processor-number-threads}"
        should_guess_cpu_limit="{$main-task-processor-should-guess-cpu-limit}"
        overload_action="{$main-task-processor-overload-action}"
        wait_queue_length_limit="{$main-task-processor-wait-queue-length-limit}"
        wait_queue_time_limit="{$main-task-processor-wait-queue-time-limit}"
        sensor_wait_queue_time_limit="{$main-task-processor-sensor-wait-queue-time-limit}"/>
    </cfg:Coroutine>

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

    <cfg:ChannelGrpcClientPool
      num_channels="{$grpc-pool-client-num-channels}"
      num_clients="{$grpc-pool-client-num-clients}"
      timeout="{$grpc-pool-client-timeout}"
      enable="true">
      <xsl:call-template name="GrpcClientChannelArgList"/>
    </cfg:ChannelGrpcClientPool>

    <cfg:ChannelServerEndpointList>
      <xsl:for-each select="$channel-servers-path">
        <cfg:Endpoint>
          <xsl:attribute name="host"><xsl:value-of select="@host"/></xsl:attribute>
          <xsl:attribute name="port"><xsl:value-of select="configuration/cfg:channelServer/cfg:networkParams/@grpc_port"/>
            <xsl:if test="count(configuration/cfg:channelServer/cfg:networkParams/@grpc_port) = 0">
              <xsl:value-of select="$def-channel-server-grpc-port"/>
            </xsl:if>
          </xsl:attribute>
        </cfg:Endpoint>
      </xsl:for-each>
    </cfg:ChannelServerEndpointList>

    <cfg:CampaignGrpcClientPool
      num_channels="{$grpc-pool-client-num-channels}"
      num_clients="{$grpc-pool-client-num-clients}"
      timeout="{$grpc-pool-client-timeout}"
      enable="true">
      <xsl:call-template name="GrpcClientChannelArgList"/>
    </cfg:CampaignGrpcClientPool>

    <cfg:CampaignManagerEndpointList>
      <xsl:for-each select="exsl:node-set($campaign-manager-host-grpc-port-set)/host">
        <cfg:Endpoint>
          <xsl:attribute name="host"><xsl:value-of select="."/></xsl:attribute>
          <xsl:attribute name="port"><xsl:value-of select="@grpc_port"/></xsl:attribute>
          <xsl:attribute name="service_index"><xsl:value-of select="concat($cluster-id, '_', position())"/></xsl:attribute>
        </cfg:Endpoint>
      </xsl:for-each>
    </cfg:CampaignManagerEndpointList>

    <cfg:UserBindGrpcClientPool
      num_channels="{$grpc-pool-client-num-channels}"
      num_clients="{$grpc-pool-client-num-clients}"
      timeout="{$grpc-pool-client-timeout}"
      enable="true">
      <xsl:call-template name="GrpcClientChannelArgList"/>
    </cfg:UserBindGrpcClientPool>

    <cfg:UserInfoGrpcClientPool
      num_channels="{$grpc-pool-client-num-channels}"
      num_clients="{$grpc-pool-client-num-clients}"
      timeout="{$grpc-pool-client-timeout}"
      enable="true">
      <xsl:call-template name="GrpcClientChannelArgList"/>
    </cfg:UserInfoGrpcClientPool>

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

  <xsl:variable name="channel-servers-path" select="$xpath/../service[@descriptor = $channel-server-descriptor]"/>

  <xsl:variable name="campaign-manager-host-grpc-port-set">
    <xsl:for-each select="$xpath/../service[@descriptor = $campaign-manager-descriptor]">
      <xsl:variable name="campaign-manager-host-subset">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix" select="'CampaignManager'"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable
        name="campaign-manager-config"
        select="./configuration/cfg:campaignManager"/>
      <xsl:if test="count($campaign-manager-config) = 0">
        <xsl:message terminate="yes"> FCGIAdServer: Can't find campaign manager config </xsl:message>
      </xsl:if>

      <xsl:variable name="campaign-manager-grpc-port">
        <xsl:value-of select="$campaign-manager-config/cfg:networkParams/@grpc_port"/>
        <xsl:if test="count($campaign-manager-config/cfg:networkParams/@grpc_port) = 0">
          <xsl:value-of select="$def-campaign-manager-grpc-port"/>
        </xsl:if>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($campaign-manager-host-subset)/host">
        <host grpc_port="{$campaign-manager-grpc-port}"><xsl:value-of select="."/></host>
      </xsl:for-each>
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
      <xsl:with-param name="channel-servers-path" select="$channel-servers-path"/>
      <xsl:with-param name="campaign-manager-host-grpc-port-set" select="$campaign-manager-host-grpc-port-set"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

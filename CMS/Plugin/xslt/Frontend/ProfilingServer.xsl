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
<xsl:include href="../GrpcChannelArgs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="cluster-id" select="$CLUSTER_ID"/>

<!-- ProfilingServer config generate function -->
<xsl:template name="ProfilingServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="profiling-server-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>
  <xsl:param name="channel-servers-path"/>
  <xsl:param name="campaign-manager-host-grpc-port-set"/>

  <cfg:ProfilingServerConfig
    zmq_io_threads="{$profiling-server-config/@zmq_io_threads}"
    work_threads="{$profiling-server-config/@work_threads}"
    debug_on="{$profiling-server-config/@debug_on}"
    bind_url_suffix="{$profiling-server-config/@bind_url_suffix}">
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="time_duration_grpc_client_mark_bad">
      <xsl:value-of select="$def-time-duration-grpc-client-mark-bad"/>
    </xsl:attribute>
    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="profiling-server-port">
      <xsl:value-of select="$profiling-server-config/cfg:networkParams/@port"/>
      <xsl:if test="count($profiling-server-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-profiling-server-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="profilingServer.port"
      method="text" omit-xml-declaration="yes"
      >  ['profilingServer', <xsl:copy-of select="$profiling-server-port"/>],</exsl:document>

    <xsl:variable name="profiling-server-logging" select="$profiling-server-config/cfg:logging"/>
    <xsl:variable name="log-level"><xsl:value-of select="$profiling-server-config/cfg:logging/@log_level"/>
      <xsl:if test="count($profiling-server-logging/@log_level) = 0">
        <xsl:value-of select="$default-log-level"/>
      </xsl:if>
    </xsl:variable>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$profiling-server-config/cfg:threadParams/@min"/>
        <xsl:if test="count($profiling-server-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="10"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$profiling-server-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$profiling-server-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $profiling-server-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-log-level"/>
    </xsl:call-template>

    <xsl:variable name="dmp-profiling-info-port">
      <xsl:value-of select="$profiling-server-config/cfg:networkParams/@dmp_profiling_info_port"/>
      <xsl:if test="count($profiling-server-config/cfg:networkParams/@dmp_profiling_info_port) = 0">
        <xsl:value-of select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="default-dmp-profiling-info-socket">
      <socket hwm="1" non_block="false">
        <address domain="*" port="{$dmp-profiling-info-port}"/>
      </socket>
    </xsl:variable>

    <cfg:DMPProfilingInfoSocket type="PULL">
      <xsl:call-template name="ZmqSocketGeneratorWithDefault">
        <xsl:with-param name="socket-config" select="$profiling-server-config/cfg:dmpProfilingInfoBindSocket[1]"/>
        <xsl:with-param name="default-socket-config" select="exsl:node-set($default-dmp-profiling-info-socket)/socket"/>
      </xsl:call-template>
    </cfg:DMPProfilingInfoSocket>

   <xsl:if test="count($profiling-server-config/cfg:dmpProfileFilter[
     @enable = 'true' or @enable = '1']) != 0">
     <cfg:DMPProfileFilter
       keep_time="{$profiling-server-config/cfg:dmpProfileFilter/@keep-time}"
       keep_time_period="{$profiling-server-config/cfg:dmpProfileFilter/@keep-time-period}"
       chunk_count="{$profiling-server-config/cfg:dmpProfileFilter/@chunk-count}"
       chunk_size="{$profiling-server-config/cfg:dmpProfileFilter/@chunk-size}"
       time_precision="{$profiling-server-config/cfg:dmpProfileFilter/@time-precision}"/>
   </xsl:if>

   <xsl:variable name="kafka-config" select="$colo-config/cfg:coloParams/cfg:kafkaStorage"/>
   <xsl:if test="count($kafka-config/cfg:uidgenerator) != 0 or count($kafka-config/cfg:clickstream) != 0">
      <cfg:KafkaDMPProfilingStorage>
         <xsl:if test="count($kafka-config/cfg:uidgenerator) != 0">
           <cfg:UidGeneratorTopic>
             <xsl:call-template name="SaveKafkaTopic">
               <xsl:with-param name="topic-config" select="$kafka-config/cfg:uidgenerator"/>
               <xsl:with-param name="default-topic-name" select="$default-uid-generator-topic"/>
               <xsl:with-param name="kafka-config" select="$kafka-config"/>
             </xsl:call-template>
           </cfg:UidGeneratorTopic>
         </xsl:if>
         <xsl:if test="count($kafka-config/cfg:clickstream) != 0">
           <cfg:ClickStreamTopic>
             <xsl:call-template name="SaveKafkaTopic">
               <xsl:with-param name="topic-config" select="$kafka-config/cfg:clickstream"/>
               <xsl:with-param name="default-topic-name" select="$default-click-stream-topic"/>
               <xsl:with-param name="kafka-config" select="$kafka-config"/>
             </xsl:call-template>
           </cfg:ClickStreamTopic>
         </xsl:if>
         <xsl:if test="count($kafka-config/cfg:geo) != 0">
           <cfg:GeoTopic>
             <xsl:call-template name="SaveKafkaTopic">
               <xsl:with-param name="topic-config" select="$kafka-config/cfg:geo"/>
               <xsl:with-param name="default-topic-name" select="$default-geo-topic"/>
               <xsl:with-param name="kafka-config" select="$kafka-config"/>
             </xsl:call-template>
           </cfg:GeoTopic>
         </xsl:if>
      </cfg:KafkaDMPProfilingStorage>
    </xsl:if>

    <cfg:ChannelManagerControllerRefs name="ChannelManagerControllers">
      <xsl:variable
        name="channel-controller-path"
        select="$fe-cluster-path/service[@descriptor = $channel-controller-descriptor] |
            $fe-cluster-path/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController']"/>

      <xsl:for-each select="$channel-controller-path">

        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'AdFrontend:ChannelManagerController'"/>
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
              select="concat('corbaloc:iiop:', ., ':', $channel-controller-port,
                '/ChannelManagerController')"/>
          </xsl:attribute>
        </cfg:Ref>
        </xsl:for-each>
      </xsl:for-each>
    </cfg:ChannelManagerControllerRefs>

    <xsl:call-template name="AddUserBindControllerGroups">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="error-prefix" select="AdFrontend"/>
    </xsl:call-template>

    <xsl:call-template name="AddUserInfoManagerControllerGroups">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="error-prefix" select="ProfilingServer"/>
    </xsl:call-template>

    <cfg:CampaignManagerRef name="CampaignManager">
      <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
        <xsl:if test="count(./service[@descriptor = $campaign-manager-descriptor]) = 0">
          <xsl:message terminate="yes"> ProfilingServer: Can't find campaign manager config </xsl:message>
        </xsl:if>

        <xsl:for-each select="./service[@descriptor = $campaign-manager-descriptor]">
          <xsl:variable name="hosts">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
              <xsl:with-param name="error-prefix" select="'ProfilingServer:CampaignManager'"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:variable
            name="campaign-manager-config"
            select="./configuration/cfg:campaignManager"/>

          <xsl:variable name="campaign-manager-port">
            <xsl:value-of select="$campaign-manager-config/cfg:networkParams/@port"/>
            <xsl:if test="count($campaign-manager-config/cfg:networkParams/@port) = 0">
              <xsl:value-of select="$def-campaign-manager-port"/>
            </xsl:if>
          </xsl:variable>

          <xsl:for-each select="exsl:node-set($hosts)/host">
            <cfg:Ref>
              <xsl:attribute name="ref">
                <xsl:value-of
                  select="concat('corbaloc:iiop:', ., ':',
                    $campaign-manager-port, '/', $current-campaign-manager-obj)"/>
              </xsl:attribute>
            </cfg:Ref>
          </xsl:for-each>
        </xsl:for-each>
      </xsl:for-each>
    </cfg:CampaignManagerRef>

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

  </cfg:ProfilingServerConfig>

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
    name="profiling-server-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ProfilingServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ProfilingServer: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> ProfilingServer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($profiling-server-path) = 0">
       <xsl:message terminate="yes"> ProfilingServer: Can't find log profiling server node </xsl:message>
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
    name="profiling-server-config"
    select="$profiling-server-path/configuration/cfg:profilingServer"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="channel-servers-path" select="$xpath/../service[@descriptor = $channel-server-descriptor]"/>

  <xsl:variable name="campaign-managers-path" select="$xpath/../service[@descriptor = $campaign-manager-descriptor]"/>

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

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> ProfilingServer: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/ProfilingServerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ProfilingServerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="profiling-server-config" select="$profiling-server-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
      <xsl:with-param name="channel-servers-path" select="$channel-servers-path"/>
      <xsl:with-param name="campaign-manager-host-grpc-port-set" select="$campaign-manager-host-grpc-port-set"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

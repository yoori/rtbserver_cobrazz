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
<xsl:template name="HttpServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="http-adserver-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>

  <cfg:ServerConfig>
    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="fe_config">
      <xsl:value-of select="concat($config-root,'/',$out-dir,'/FeConfig.xml')"/>
    </xsl:attribute>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <cfg:Coroutine>
      <cfg:CoroPool
        initial_size="{$coro-pool-initial-size}"
        max_size="{$coro-pool-max-size}"
        stack_size="{$coro-pool-stack-size}"/>
      <cfg:EventThreadPool
        number_threads="{$event-thread-pool-number-threads}"
        name="{$event-thread-pool-name}"
        ev_default_loop_disabled="{$event-thread-pool-ev-default-loop-disabled}"
        defer_events="{$event-thread-pool-defer-events}"/>
      <cfg:MainTaskProcessor
        name="{$main-task-processor-name}"
        number_threads="{$main-task-processor-number-threads}"
        should_guess_cpu_limit="{$main-task-processor-should-guess-cpu-limit}"
        overload_action="{$main-task-processor-overload-action}"
        wait_queue_length_limit="{$main-task-processor-wait-queue-length-limit}"
        wait_queue_time_limit="{$main-task-processor-wait-queue-time-limit}"
        sensor_wait_queue_time_limit="{$main-task-processor-sensor-wait-queue-time-limit}"/>
    </cfg:Coroutine>

    <xsl:variable name="unix_socket_path_id_arr">
      <i>1</i><i>2</i><i>3</i><i>4</i>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($unix_socket_path_id_arr)/i">
      <cfg:HttpServer>
        <xsl:attribute name="backlog">
          <xsl:value-of select="$http-adserver-config/cfg:adHttpNetworkParams/@backlog"/>
          <xsl:if test="count($http-adserver-config/cfg:adHttpNetworkParams/@backlog) = 0">
            <xsl:value-of select="$http-backlog"/>
          </xsl:if>
        </xsl:attribute>
        <xsl:attribute name="max_connections">
          <xsl:value-of select="$http-adserver-config/cfg:adHttpNetworkParams/@max_connections"/>
          <xsl:if test="count($http-adserver-config/cfg:adHttpNetworkParams/@max_connections) = 0">
            <xsl:value-of select="$http-max-connections"/>
          </xsl:if>
        </xsl:attribute>
        <xsl:attribute name="keepalive_timeout_seconds">
          <xsl:value-of select="$http-adserver-config/cfg:adHttpNetworkParams/@keepalive_timeout_seconds"/>
          <xsl:if test="count($http-adserver-config/cfg:adHttpNetworkParams/@keepalive_timeout_seconds) = 0">
            <xsl:value-of select="$http-keepalive-timeout-seconds"/>
          </xsl:if>
        </xsl:attribute>
        <xsl:attribute name="in_buffer_size">
          <xsl:value-of select="$http-adserver-config/cfg:adHttpNetworkParams/@in_buffer_size"/>
          <xsl:if test="count($http-adserver-config/cfg:adHttpNetworkParams/@in_buffer_size) = 0">
            <xsl:value-of select="$http-in-buffer-size"/>
          </xsl:if>
        </xsl:attribute>
        <xsl:attribute name="unix_socket_path">
          <xsl:value-of select="concat($workspace-root,'/run/http_adserver',., '.sock')"/>
        </xsl:attribute>
      </cfg:HttpServer>
    </xsl:for-each>

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="http-adserver-port">
      <xsl:value-of select="$http-adserver-config/cfg:adHttpNetworkParams/@port"/>
      <xsl:if test="count($http-adserver-config/cfg:adHttpNetworkParams/@port) = 0">
        <xsl:value-of select="$def-http-adserver-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="http-adserver-logging" select="$http-adserver-config/cfg:logging"/>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$http-adserver-config/cfg:threadParams/@min"/>
        <xsl:if test="count($http-adserver-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$http-adserver-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="HttpServerStats" name="HttpServerStats"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$http-adserver-logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $http-adserver-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-fcgiserver-log-level"/>
    </xsl:call-template>

    <xsl:variable name="http-adserver-mon-port">
      <xsl:value-of select="$http-adserver-config/cfg:adFCGINetworkParams/@mon_port"/>
      <xsl:if test="count($http-adserver-config/cfg:adFCGINetworkParams/@mon_port) = 0">
        <xsl:value-of select="$def-http-adserver-mon-port"/>
      </xsl:if>
    </xsl:variable>

    <cfg:Monitoring port="{$http-adserver-mon-port}"/>

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
    <cfg:Module name="echo"/>
  </cfg:ServerConfig>

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
    name="http-adserver-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> HttpServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> HttpServer: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> HttpServer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($http-adserver-path) = 0">
       <xsl:message terminate="yes"> HttpServer: Can't find log fcgi server node </xsl:message>
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
    name="http-adserver-config"
    select="$http-adserver-path/configuration/cfg:frontend"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> HttpServer: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- AdFrontend config required for HttpServer -->
  <xsl:variable name="frontend-hosts">
    <xsl:for-each select="$xpath/../service[@descriptor = $http-frontend-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'AdFrontend hosts resolving'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

   <xsl:variable name="http-hosts">
    <xsl:for-each select="$xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'AdFrontend hosts resolving'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($http-hosts)/host">
    <xsl:variable name="fhost" select="."/>
    <xsl:choose>
      <xsl:when test="count(exsl:node-set($frontend-hosts)[host = $fhost]) = 0">
        <xsl:message terminate="yes"> HttpServer: Can't find NginxFrontend on host '<xsl:value-of select="."/>' (NginxFrontend hosts: '<xsl:value-of select="$frontend-hosts"/>')</xsl:message>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/HttpServerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="HttpServerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="http-adserver-config" select="$http-adserver-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>
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

<!-- ZmqBalancer config generate function -->
<xsl:template name="ZmqBalancerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="zmq-balancer-config"/>
  <xsl:param name="fe-cluster-path"/>

  <cfg:ZmqBalancerConfig
    zmq_io_threads="{$zmq-balancer-config/@zmq_io_threads}">
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="zmq-balancer-port">
      <xsl:value-of select="$zmq-balancer-config/cfg:networkParams/@port"/>
      <xsl:if test="count($zmq-balancer-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-zmq-profiling-balancer-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="zmq-balancer-logging" select="$zmq-balancer-config/cfg:logging"/>
    <xsl:variable name="zmq-balancer-log-level"><xsl:value-of select="$zmq-balancer-config/cfg:logging/@log_level"/>
      <xsl:if test="count($zmq-balancer-logging/@log_level) = 0">
        <xsl:value-of select="$default-log-level"/>
      </xsl:if>
    </xsl:variable>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$zmq-balancer-config/cfg:threadParams/@min"/>
        <xsl:if test="count($zmq-balancer-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$zmq-balancer-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:variable
      name="profiling-server-config"
      select="$xpath/../service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']/configuration/cfg:profilingServer"/>

    <xsl:for-each select="$zmq-balancer-config/cfg:dmpProfilingInfo">
      <cfg:BalancingRoute 
         name="DMPProfilingInfo"
         work_threads="{./@work_threads}">
         <xsl:variable name="default-dmp-profiling-info-bind-socket">
           <socket hwm="1" non_block="false">
            <address domain="*" port="{$def-zmq-profiling-balancer-dmp-profiling-info-port}"/>
          </socket>
        </xsl:variable>

        <cfg:BindSocket type="PULL">
          <xsl:call-template name="ZmqSocketGeneratorWithDefault">
            <xsl:with-param name="socket-config" select="./cfg:bindSocket[1]"/>
            <xsl:with-param name="default-socket-config" select="exsl:node-set($default-dmp-profiling-info-bind-socket)/socket"/>
          </xsl:call-template>
        </cfg:BindSocket>

        <xsl:variable name="dmp-profiling-info-port">
          <xsl:value-of select="$profiling-server-config/cfg:networkParams/@dmp_profiling_info_port"/>
          <xsl:if test="count($profiling-server-config/cfg:networkParams/@dmp_profiling_info_port) = 0">
            <xsl:value-of select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
          </xsl:if>
        </xsl:variable>

        <xsl:variable name="default-dmp-profiling-info-connect-socket">
          <xsl:choose>
            <xsl:when test="count(./@distribute_strategy) != 0 and 
              count($fe-cluster-path[1]/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']) > 0 and
              ./@distribute_strategy = 'first'">
              <xsl:call-template name="GetConnectSocketToProfilingServersConfig">
                <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path[1]"/>
                <xsl:with-param name="default-port" select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="count(./@distribute_strategy) != 0 and
              count($fe-cluster-path[2]/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']) > 0 and
              ./@distribute_strategy = 'second'">
              <xsl:call-template name="GetConnectSocketToProfilingServersConfig">
                <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path[2]"/>
                <xsl:with-param name="default-port" select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="GetConnectSocketToProfilingServersConfig">
                <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
                <xsl:with-param name="default-port" select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <cfg:ConnectSocket type="PUSH">
          <xsl:call-template name="ZmqSocketGeneratorWithDefault">
            <xsl:with-param name="socket-config" select="./cfg:connectSocket[1]"/>
            <xsl:with-param name="default-socket-config" select="exsl:node-set($default-dmp-profiling-info-connect-socket)/socket"/>
          </xsl:call-template>
        </cfg:ConnectSocket>
      </cfg:BalancingRoute>
    </xsl:for-each>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$zmq-balancer-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $zmq-profiling-balancer-log-path)"/>
      <xsl:with-param name="default-log-level" select="$zmq-balancer-log-level"/>
    </xsl:call-template>

  </cfg:ZmqBalancerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="zmq-balancer-path"
    select="$xpath"/>

  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ZmqBalancer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ZmqBalancer: Can't find full cluster group </xsl:message>
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
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="zmq-balancer-config"
    select="$zmq-balancer-path/configuration/cfg:zmqProfilingBalancer"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/ZmqBalancerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ZmqBalancerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="zmq-balancer-config" select="$zmq-balancer-config"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

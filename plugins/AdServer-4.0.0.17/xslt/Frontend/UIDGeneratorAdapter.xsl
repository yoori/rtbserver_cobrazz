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

<!-- UIDGeneratorAdapter config generate function -->
<xsl:template name="UIDGeneratorAdapterConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="uid-generator-adapter-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>

  <cfg:UIDGeneratorAdapterConfig>
    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="out-logs-dir" select="concat($workspace-root, '/log/UIDGeneratorAdapter/Out/')"/>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$uid-generator-adapter-config/cfg:threadParams/@min"/>
        <xsl:if test="count($uid-generator-adapter-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port">
          <xsl:value-of select="$uid-generator-adapter-config/cfg:networkParams/@port"/>
          <xsl:if test="count($uid-generator-adapter-config/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-uid-generator-adapter-port"/>
          </xsl:if>
        </xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:variable name="logging-config" select="$uid-generator-adapter-config/cfg:logging"/>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$uid-generator-adapter-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $uid-generator-adapter-log-path)"/>
      <xsl:with-param name="default-log-level" select="$uid-generator-adapter-log-level"/>
    </xsl:call-template>

    <cfg:Processing threads="40" send_threads="20" source="bln">
      <xsl:attribute name="port">
        <xsl:value-of select="$uid-generator-adapter-config/cfg:networkParams/@dmp_profiling_info_port"/>
        <xsl:if test="count($uid-generator-adapter-config/cfg:networkParams/@dmp_profiling_info_port) = 0">
          <xsl:value-of select="$def-uid-generator-adapter-input-port"/>
        </xsl:if>
      </xsl:attribute>

      <xsl:variable name="default-dmp-profiling-info-connect-socket">
        <xsl:choose>
          <xsl:when test="count($uid-generator-adapter-config/@distribute_strategy) != 0 and 
            count($fe-cluster-path[1]/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']) > 0 and
            $uid-generator-adapter-config/@distribute_strategy = 'first'">
            <xsl:call-template name="GetConnectSocketToProfilingServersConfig">
              <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path[1]"/>
              <xsl:with-param name="default-port" select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:when test="count($uid-generator-adapter-config/@distribute_strategy) != 0 and
            count($fe-cluster-path[2]/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']) > 0 and
            $uid-generator-adapter-config/@distribute_strategy = 'second'">
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

      <cfg:ZmqSend type="PUSH">
        <xsl:call-template name="ZmqSocketGeneratorWithDefault">
          <xsl:with-param name="socket-config"
            select="$uid-generator-adapter-config/cfg:dmpProfilingInfoConnectSocket[1]"/>
          <xsl:with-param name="default-socket-config"
            select="exsl:node-set($default-dmp-profiling-info-connect-socket)/socket"/>
        </xsl:call-template>
      </cfg:ZmqSend>
    </cfg:Processing>
    <cfg:LogProcessing>
      <cfg:OutLogs log_root="{$out-logs-dir}">
        <cfg:KeywordHitStat period="1800"/>
      </cfg:OutLogs>
    </cfg:LogProcessing>
  </cfg:UIDGeneratorAdapterConfig>

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
    name="uid-generator-adapter-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UIDGeneratorAdapter: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> UIDGeneratorAdapter: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> UIDGeneratorAdapter: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($uid-generator-adapter-path) = 0">
       <xsl:message terminate="yes"> UIDGeneratorAdapter: Can't find log UIDGeneratorAdapter node </xsl:message>
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
    name="uid-generator-adapter-config"
    select="$uid-generator-adapter-path/configuration/cfg:UIDGeneratorAdapter"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> UIDGeneratorAdapter: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- AdFrontend config required for FCGIServer -->
   <xsl:variable name="uid-generator-adapter-hosts">
    <xsl:for-each select="$xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'AdFrontend hosts resolving'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/UIDGeneratorAdapterConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="UIDGeneratorAdapterConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="uid-generator-adapter-config" select="$uid-generator-adapter-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

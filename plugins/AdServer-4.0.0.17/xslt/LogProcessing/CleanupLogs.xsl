<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"

  xmlns:xsd="http://www.w3.org/2001/XMLSchema">

<xsl:output method="text" indent="no" encoding="UTF-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="real_host" select="/colo:colocation/host[@hostName = $HOST]/@name"/>


<!-- Recursively pass serviceGroups and process service records -->
<xsl:template name="ServiceGroupParser">
  <xsl:param name="workspace-root"/>
  <xsl:param name="path"/>

  <xsl:variable name="local-workspace-root" select="
    $path/configuration/cfg:cluster/cfg:environment/@workspace_root |
    $path/configuration/cfg:backendCluster/cfg:environment/@workspace_root |
    $path/configuration/cfg:frontendCluster/cfg:environment/@workspace_root"/>
  <xsl:variable name="workspace-path">
    <xsl:choose>
      <xsl:when test="$local-workspace-root">
        <xsl:value-of select="$local-workspace-root"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$workspace-root"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="srv-grp-xpath" select="$path"/>

  <!-- Service groups parse-->
  <xsl:for-each select="$srv-grp-xpath/serviceGroup">
      <xsl:call-template name="ServiceGroupParser">
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
        <xsl:with-param name="path" select="."/>
      </xsl:call-template>
  </xsl:for-each>
  <!-- services: (has own descriptor) -->
  <xsl:for-each select="$srv-grp-xpath/service[contains(concat(@host, ' '),
    concat($real_host, ' '))]">

    <xsl:variable name="service-name">
      <xsl:call-template name="substring-after-last">
        <xsl:with-param name="arg" select="@descriptor"/>
        <xsl:with-param name="delim" select="'/'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="log-path"
      select="concat('/log/', $service-name, '/', $service-name)"/>

    <!-- Get serviceName and serviceGroupName parts of service descriptor.
      Will used to locate <logging> parameters. -->
    <xsl:variable name="service-path">
      <xsl:call-template name="substring-before-last">
        <xsl:with-param name="arg" select="@descriptor"/>
        <xsl:with-param name="delim" select="'/'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="service-group-name">
      <xsl:call-template name="substring-after-last">
        <xsl:with-param name="arg" select="$service-path"/>
        <xsl:with-param name="delim" select="'/'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="lower">abcdefghijklmnopqrstuvwxyz</xsl:variable>
    <xsl:variable name="upper">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>
    <xsl:variable name="serviceName"><xsl:value-of select="
      translate(substring($service-name, 1, 1), $upper, $lower)"/><xsl:value-of select="
        substring-after($service-name, substring($service-name, 1, 1))"/>
    </xsl:variable>
    <xsl:variable name="serviceGroupName"><xsl:value-of select="
      translate(substring($service-group-name, 1, 1), $upper, $lower)"/><xsl:value-of select="
        substring-after($service-group-name, substring($service-group-name, 1, 1))"/>
    </xsl:variable>

    <!-- The most suitable <logging> node from colocation.xml -->
    <xsl:variable name="logging" select="
      ./configuration/*[local-name() = $serviceName]/cfg:logging |
      $srv-grp-xpath/configuration/*[local-name() = $serviceGroupName]/*[local-name() = $serviceName]/cfg:logging"/>

    <!-- Take default values from XSD, if <logging> omitted -->
    <xsl:variable name="schema" select="
      document('../../xsd/Commons/AdServerCommonsApp.xsd')"/>
    <xsl:variable name="logging-params" select="$schema//xsd:complexType[@name='LoggingParamsType']"/>

    <xsl:variable name="error-time-cleanup" select="
      $logging/@error_time_cleanup | $logging-params/xsd:attribute[@name='error_time_cleanup']/@default"/>

    <xsl:variable name="error-size-cleanup" select="
      $logging/@error_size_cleanup | $logging-params/xsd:attribute[@name='error_size_cleanup']/@default"/>

    <xsl:variable name="trace-time-cleanup" select="
      $logging/@trace_time_cleanup | $logging-params/xsd:attribute[@name='trace_time_cleanup']/@default"/>

    <xsl:variable name="trace-size-cleanup" select="
      $logging/@trace_size_cleanup | $logging-params/xsd:attribute[@name='trace_size_cleanup']/@default"/>

    <xsl:choose>
      <xsl:when test="contains(@descriptor, 'Tests')"/>
      <xsl:when test="@descriptor = $pbe-stunnel-server-descriptor">
  '<xsl:value-of select="concat($workspace-path, $log-path)"/>.log' =>
    {
      'time' => <xsl:value-of select="$error-time-cleanup"/>,
      'size' => <xsl:value-of select="$error-size-cleanup"/>
    },
      </xsl:when>
      <xsl:when test="@descriptor != $http-frontend-descriptor">
  '<xsl:value-of select="concat($workspace-path, $log-path)"/>.error' =>
    {
      'time' => <xsl:value-of select="$error-time-cleanup"/>,
      'size' => <xsl:value-of select="$error-size-cleanup"/>
    },
  '<xsl:value-of select="concat($workspace-path, $log-path)"/>.trace' =>
    {
      'time' => <xsl:value-of select="$trace-time-cleanup"/>,
      'size' => <xsl:value-of select="$trace-size-cleanup"/>
    },
      </xsl:when>
      <xsl:when test="@descriptor = $http-frontend-descriptor">
    <!-- Take default values from XSD, if <logging> omitted -->
    <xsl:variable name="fe-error-time-cleanup" select="
      $logging/@error_time_cleanup |
      $xsd-frontend-logging-params-type/xsd:attribute[@name='error_time_cleanup']/@default"/>
    <xsl:variable name="fe-error-size-cleanup" select="
      $logging/@error_size_cleanup |
      $xsd-frontend-logging-params-type/xsd:attribute[@name='error_size_cleanup']/@default"/>
  '<xsl:value-of select="concat($workspace-path, '/log/Frontend.log')"/>' =>
    {
      'time' => <xsl:value-of select="$fe-error-time-cleanup"/>,
      'size' => <xsl:value-of select="$fe-error-size-cleanup * 1024 * 1024"/>
    },
      </xsl:when>
    </xsl:choose>

  </xsl:for-each>

</xsl:template>


<!-- CleanupLogs config generate function -->
<xsl:template name="CleanupLogsConfigGenerator">
  <xsl:param name="workspace-root"/>
  <xsl:param name="adcluster-path"/>

package cleanup;
use strict;

our %SERVICE_CONFIG = (
  pid_file => 'cleanup_logs.pid',
  check_period => 600, # in seconds
);

# 'time', in minutes.
# 'size', in bytes.


our %CLEANUP_CONFIG = (
    <xsl:call-template name="ServiceGroupParser">
      <xsl:with-param name="workspace-root" select="$workspace-root"/>
      <xsl:with-param name="path" select="$xpath"/>
    </xsl:call-template>

    <xsl:variable name="workspace-root" select="
      $adcluster-path/configuration/cfg:cluster/cfg:environment/@workspace_root |
      $xpath/configuration/cfg:cluster/cfg:environment/@workspace_root"/>

    <!-- Take default log rotation values from XSD -->
    <xsl:variable name="schema" select="
      document('../../xsd/Commons/AdServerCommonsApp.xsd')"/>
    <xsl:variable name="logging-params" select="$schema//xsd:complexType[@name='LoggingParamsType']"/>

    <xsl:variable name="error-time-cleanup" select="
      $logging-params/xsd:attribute[@name='error_time_cleanup']/@default"/>

    <xsl:variable name="error-size-cleanup" select="
      $logging-params/xsd:attribute[@name='error_size_cleanup']/@default"/>

    <xsl:variable name="trace-time-cleanup" select="
      $logging-params/xsd:attribute[@name='trace_time_cleanup']/@default"/>
    <xsl:variable name="trace-size-cleanup" select="
      $logging-params/xsd:attribute[@name='trace_size_cleanup']/@default"/>

  <xsl:if test="count($adcluster-path/configuration/cfg:cluster/cfg:remote/cfg:sTunnelRef) > 0">
  '<xsl:value-of select="concat($workspace-root, '/log/STunnelClient/STunnelClient.log')"/>' =>
    {
      'time' => <xsl:value-of select="$error-time-cleanup"/>,
      'size' => <xsl:value-of select="$error-size-cleanup"/>
    },
  </xsl:if>

  <xsl:variable name="be-synclogs-serv-path"
    select="$xpath//service[
      @descriptor = $campaign-manager-descriptor or
      @descriptor = $log-generalizer-descriptor or
      @descriptor = $expression-matcher-descriptor or
      @descriptor = $request-info-manager-descriptor or
      @descriptor = $stat-receiver-descriptor or
      @descriptor = $user-info-manager-descriptor]"/>
  <xsl:variable name="pbe-synclogs-serv-path"
    select="$xpath//service[
      @descriptor = $pbe-channel-proxy-descriptor or
      @descriptor = $pbe-campaign-server-descriptor or
      @descriptor = $pbe-stunnel-server-descriptor]"/>
  <xsl:variable name="synclogs-serv-path" select="$be-synclogs-serv-path |
    $pbe-synclogs-serv-path"/>

  <xsl:variable name="real-hosts">
    <xsl:for-each select="$synclogs-serv-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'SMS-SyncLogs-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="real-be-hosts">
    <xsl:for-each select="$be-synclogs-serv-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'SMS-SyncLogs-be-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="real-pbe-hosts">
    <xsl:for-each select="$pbe-synclogs-serv-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'SMS-SyncLogs-pbe-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:if test="count(exsl:node-set($real-hosts)[host = $HOST]) > 0">
    <xsl:variable name="be-log-processing-path"
      select="$adcluster-path/serviceGroup[@descriptor =
        $be-cluster-descriptor]/serviceGroup[@descriptor = $log-processing-descriptor]"/>
    <xsl:variable name="pbe-log-processing-path"
      select="$xpath/../serviceGroup[@descriptor =
        $ad-proxycluster-descriptor]"/>
    <xsl:variable name="be-sync-logs-config"
      select="$be-log-processing-path/configuration/cfg:logProcessing/cfg:syncLogs/cfg:logging"/>
    <xsl:variable name="pbe-sync-logs-config"
      select="$pbe-log-processing-path/configuration/cfg:cluster/cfg:syncLogs/cfg:logging"/>
    <xsl:variable name="be-synclogs-condition"
      select="count(exsl:node-set($real-be-hosts)[host = $HOST]) > 0"/>
    <xsl:variable name="pbe-synclogs-condition"
      select="count(exsl:node-set($real-pbe-hosts)[host = $HOST]) > 0"/>

    <xsl:variable name="log-params" select="$be-sync-logs-config[$be-synclogs-condition] |
      $pbe-sync-logs-config[$pbe-synclogs-condition]"/>
  '<xsl:value-of select="concat($workspace-root, '/log/SyncLogs/SyncLogs.error')"/>' =>
    {
      'time' => <xsl:value-of select="$log-params/@error_time_cleanup |
        $error-time-cleanup"/>,
      'size' => <xsl:value-of select="$log-params/@error_size_cleanup |
        $error-size-cleanup"/>
    },
  '<xsl:value-of select="concat($workspace-root, '/log/SyncLogs/SyncLogs.trace')"/>' =>
    {
      'time' => <xsl:value-of select="$log-params/@trace_time_cleanup |
        $trace-time-cleanup"/>,
      'size' => <xsl:value-of select="$log-params/@trace_size_cleanup |
        $trace-size-cleanup"/>
    },
  </xsl:if>
);

1;

</xsl:template>


<!-- ENTRY POINT -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/.."/>

  <xsl:variable
    name="adcluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]"/>

  <xsl:variable
    name="proxycluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-proxycluster-descriptor]"/>

  <xsl:variable
    name="profilingcluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]"/>

  <!-- check config sections -->
  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> SMS: Can't find XPATH element </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="colo-config"
    select="$adcluster-path/configuration/cfg:cluster |
      $proxycluster-path/configuration/cfg:cluster |
      $profilingcluster-path/configuration/cfg:cluster"/>

  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> SMS: Can't find colo config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="workspace-root">
    <xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0">
      <xsl:value-of select="$def-workspace-root"/>
    </xsl:if>
  </xsl:variable>

  <xsl:call-template name="CleanupLogsConfigGenerator">
    <xsl:with-param name="workspace-root" select="$workspace-root"/>
    <xsl:with-param name="adcluster-path" select="$adcluster-path"/>
  </xsl:call-template>

</xsl:template>
</xsl:stylesheet>

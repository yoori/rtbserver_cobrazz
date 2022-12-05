<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="stat-receiver-path" select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($stat-receiver-path) = 0">
       <xsl:message terminate="yes"> StatReceiver: Can't find service element(XPATH parameter) </xsl:message>
    </xsl:when>
    <xsl:when test="count($cluster-path) = 0">
       <xsl:message terminate="yes"> StatReceiver: Can't find cluster path</xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>

  <xsl:variable name="service-config"
    select="$stat-receiver-path/configuration/cfg:statReceiver"/>

  <!-- get used configuration parameters -->
  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="service-port">
    <xsl:value-of select="$service-config/cfg:networkParams/@port"/>
    <xsl:if test="count($service-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-stat-receiver-port"/>
    </xsl:if>
  </xsl:variable>

pid file = <xsl:value-of select="$workspace-root"/>/run/StatReceiver.pid
port = <xsl:value-of select="$service-port"/>
timeout = 600
max verbosity = 0
log file = /dev/null

[stat]
comment = server for uploading stats to AdServer
path = <xsl:value-of select="$workspace-root"/>/log/StatReceiver/Out/ExtStat
use chroot = false
read only = false

</xsl:template>

</xsl:stylesheet>

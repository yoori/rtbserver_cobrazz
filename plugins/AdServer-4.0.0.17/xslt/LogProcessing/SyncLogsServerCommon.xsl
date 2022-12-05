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

<xsl:include href="../Functions.xsl"/>

<!-- STunnelServer config generate template -->
<xsl:template name="STunnelServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="service-config"/>

  <xsl:variable name="service-port">
    <xsl:value-of select="$service-config/cfg:networkParams/@port"/>
    <xsl:if test="count($service-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-sync-logs-server-port"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="log-level"><xsl:value-of select="$service-config/cfg:logging/@log_level"/>
    <xsl:if test="count($service-config/cfg:logging) = 0">
      <xsl:value-of select="$def-sync-logs-server-log-level"/>
    </xsl:if>
  </xsl:variable>

pid file = <xsl:value-of select="$workspace-root"/>/run/synclogsserver.pid
port = <xsl:value-of select="$service-port"/>
timeout = 600
max verbosity = 0
log file = /dev/null

[ad-logs]
comment = server for moving logs of AdServer
path = <xsl:value-of select="$workspace-root"/>/log
use chroot = false
read only = false

[ad-content]
comment = server for provide creatives content
path = <xsl:value-of select="$data-root"/>
use chroot = false
read only = true

</xsl:template>

</xsl:stylesheet>

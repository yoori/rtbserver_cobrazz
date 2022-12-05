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
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- -->
<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>

  <xsl:variable name="predictor-path" select="$xpath"/>

  <xsl:variable
    name="colo-config"
    select="$cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="service-config"
    select="$predictor-path/configuration/cfg:predictor"/>


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

  <xsl:variable name="service-port">
    <xsl:choose>
      <xsl:when test="count($predictor-path) != 0">
        <xsl:value-of select="$service-config/cfg:syncServer/cfg:networkParams/@port"/>
        <xsl:if test="count($service-config/cfg:syncServer/cfg:networkParams/@port) = 0">
          <xsl:value-of select="$def-predictor-sync-logs-server-port"/>
        </xsl:if>
       </xsl:when>
       <xsl:otherwise>
         <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:ref/@port"/>
         <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:ref/@port) = 0">
           <xsl:value-of select="$def-predictor-sync-logs-server-port"/>
         </xsl:if>
       </xsl:otherwise>           
     </xsl:choose>
   </xsl:variable>

pid file = <xsl:value-of select="$workspace-root"/>/run/predictor_synclogsserver.pid
port = <xsl:value-of select="$service-port"/>
timeout = 600
max verbosity = 0
log file = /dev/null

[ctr]
comment = AdServer CTR config
path = <xsl:value-of select="$workspace-root"/>/log/Predictor/CTRConfig
use chroot = false
read only = true

[conv]
comment = AdServer Conv config
path = <xsl:value-of select="$workspace-root"/>/log/Predictor/ConvConfig
use chroot = false
read only = true

[logs]
comment = server for moving research logs of AdServer
path = <xsl:value-of select="$workspace-root"/>/log/Predictor/ResearchLogs
use chroot = false
read only = false

<xsl:if test="$service-config/cfg:syncServer/@enable_backup = 'true' or 
  $service-config/cfg:syncServer/@enable_backup = '1'">
[backup]
comment = server for backup research logs of AdServer
path = <xsl:value-of select="$workspace-root"/>/log/Predictor/Backup
use chroot = false
read only = false
</xsl:if>
</xsl:template>

</xsl:stylesheet>

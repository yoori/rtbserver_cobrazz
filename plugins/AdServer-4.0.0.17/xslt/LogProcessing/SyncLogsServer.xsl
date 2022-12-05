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

<xsl:include href="./SyncLogsServerCommon.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="cluster-path" select="$xpath/.."/>
  <xsl:variable name="log-processing-path" select="$xpath/serviceGroup[@descriptor = $log-processing-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($cluster-path) = 0">
       <xsl:message terminate="yes"> SyncLogsServer: Can't find cluster path</xsl:message>
    </xsl:when>
    <xsl:when test="count($log-processing-path) = 0">
       <xsl:message terminate="yes"> SyncLogsServer: Can't find log processing(XPATH) element </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="sync-server-config"
    select="$log-processing-path/configuration/cfg:logProcessing/cfg:syncLogsServer"/>

  <xsl:variable
    name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>

  <!-- check config sections -->
  <xsl:call-template name="STunnelServerConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="service-config" select="$sync-server-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

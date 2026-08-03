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

<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="service-path" select="$xpath"/>

  <xsl:choose>
    <xsl:when test="count($service-path) = 0">
      <xsl:message terminate="yes"> ClickhouseUploader: Can't find service element(XPATH parameter) </xsl:message>
    </xsl:when>
    <xsl:when test="count($cluster-path) = 0">
      <xsl:message terminate="yes"> ClickhouseUploader: Can't find cluster path </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>

  <xsl:variable name="service-config"
    select="$service-path/configuration/cfg:clickhouseUploader"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="batch"><xsl:value-of select="$service-config/@batch"/>
    <xsl:if test="count($service-config/@batch) = 0">1000</xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-conn"><xsl:value-of select="$service-config/@clickhouse_conn"/>
  </xsl:variable>

{
  "pid_file": "<xsl:value-of select="concat($workspace-root, '/run/ClickhouseUploader.pid')"/>",
  "clickhouse_conn": "<xsl:value-of select="$clickhouse-conn"/>",
  "check_roots": [
    "<xsl:value-of select="concat($workspace-root, '/log/Predictor/ResearchLogs/ResearchImpression')"/>",
    "<xsl:value-of select="concat($workspace-root, '/log/Predictor/ResearchLogs/ResearchClick')"/>",
    "<xsl:value-of select="concat($workspace-root, '/log/Predictor/ResearchLogs/RActionClickhouse')"/>"
  ],
  "error_root": "<xsl:value-of select="concat($workspace-root, '/log/ClickhouseUploader/Error')"/>",
  "batch": <xsl:value-of select="$batch"/>
}

</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="cfg colo dyn">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="predictor-path" select="$xpath"/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="generator-config"
    select="$predictor-path/configuration/cfg:ctrPredictModelGenerator"/>
  <xsl:variable name="clickhouse-config"
    select="$xpath/../service[@descriptor = 'AdCluster/BackendSubCluster/ClickhouseUploader']/configuration/cfg:clickhouseUploader"/>
  <xsl:variable name="workspace-root"><xsl:value-of
    select="$env-config/@workspace_root"/><xsl:if
    test="count($env-config/@workspace_root) = 0"><xsl:value-of
    select="$def-workspace-root"/></xsl:if></xsl:variable>

{
  "pid_file": "<xsl:value-of select="concat($workspace-root, '/run/CTRPredictModelGenerator.pid')"/>",
  "workspace_root": "<xsl:value-of select="$workspace-root"/>",
  "clickhouse_conn": "<xsl:value-of select="$clickhouse-config/@clickhouse_conn"/>",
  "generate_period": <xsl:value-of select="$generator-config/@generate_period"/><xsl:if
    test="count($generator-config/@generate_period) = 0">86400</xsl:if>,
  "train_rows": <xsl:value-of select="$generator-config/@train_rows"/><xsl:if
    test="count($generator-config/@train_rows) = 0">1000000</xsl:if>,
  "data_delay": <xsl:value-of select="$generator-config/@data_delay"/>,
  "algorithm_id": "<xsl:value-of select="$generator-config/@algorithm_id"/><xsl:if
    test="count($generator-config/@algorithm_id) = 0">catboost</xsl:if>"
}
</xsl:template>

</xsl:stylesheet>

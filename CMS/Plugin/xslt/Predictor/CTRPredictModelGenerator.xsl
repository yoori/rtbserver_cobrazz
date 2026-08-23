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
  <xsl:variable name="central-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:central"/>
  <xsl:variable name="generator-config"
    select="$predictor-path/configuration/cfg:ctrPredictModelGenerator"/>
  <xsl:variable name="clickhouse-config"
    select="$xpath/../service[@descriptor = 'AdCluster/BackendSubCluster/ClickhouseUploader']/configuration/cfg:clickhouseUploader"/>
  <xsl:variable name="workspace-root"><xsl:value-of
    select="$env-config/@workspace_root"/><xsl:if
    test="count($env-config/@workspace_root) = 0"><xsl:value-of
    select="$def-workspace-root"/></xsl:if></xsl:variable>
  <xsl:variable name="web-port"><xsl:value-of
    select="$generator-config/cfg:networkParams/@port"/><xsl:if
    test="count($generator-config/cfg:networkParams/@port) = 0"><xsl:value-of
    select="$def-ctr-predict-model-generator-port"/></xsl:if></xsl:variable>

{
  "pid_file": "<xsl:value-of select="concat($workspace-root, '/run/CTRPredictModelGenerator.pid')"/>",
  "workspace_root": "<xsl:value-of select="$workspace-root"/>",
  "clickhouse_conn": "<xsl:value-of select="$clickhouse-config/@clickhouse_conn"/>",
  "postgres_conn": "<xsl:value-of select="$central-config/cfg:pgConnection/@connection_string"/>",
  "web_server": {
    "host": "0.0.0.0",
    "port": <xsl:value-of select="$web-port"/>
  },
  "generate_period": <xsl:value-of select="$generator-config/@generate_period"/><xsl:if
    test="count($generator-config/@generate_period) = 0">86400</xsl:if>,
  "selection_chunk_rows": <xsl:value-of select="$generator-config/@selection_chunk_rows"/><xsl:if
    test="count($generator-config/@selection_chunk_rows) = 0">7000000</xsl:if>,
  "main_chunk_rows": <xsl:value-of select="$generator-config/@main_chunk_rows"/><xsl:if
    test="count($generator-config/@main_chunk_rows) = 0">10000000</xsl:if>,
  "validation_set_rows": <xsl:value-of select="$generator-config/@validation_set_rows"/><xsl:if
    test="count($generator-config/@validation_set_rows) = 0">200000</xsl:if>,
  "selection_validation_sets": <xsl:value-of select="$generator-config/@selection_validation_sets"/><xsl:if
    test="count($generator-config/@selection_validation_sets) = 0">3</xsl:if>,
  "training_validation_sets": <xsl:value-of select="$generator-config/@training_validation_sets"/><xsl:if
    test="count($generator-config/@training_validation_sets) = 0">3</xsl:if>,
  "final_test_sets": <xsl:value-of select="$generator-config/@final_test_sets"/><xsl:if
    test="count($generator-config/@final_test_sets) = 0">3</xsl:if>,
  "selection_fit_steps": <xsl:value-of select="$generator-config/@selection_fit_steps"/><xsl:if
    test="count($generator-config/@selection_fit_steps) = 0">10</xsl:if>,
  "training_fit_steps": <xsl:value-of select="$generator-config/@training_fit_steps"/><xsl:if
    test="count($generator-config/@training_fit_steps) = 0">30</xsl:if>,
  "fit_iterations": <xsl:value-of select="$generator-config/@fit_iterations"/><xsl:if
    test="count($generator-config/@fit_iterations) = 0">10</xsl:if>,
  "selection_patience": <xsl:value-of select="$generator-config/@selection_patience"/><xsl:if
    test="count($generator-config/@selection_patience) = 0">3</xsl:if>,
  "training_patience": <xsl:value-of select="$generator-config/@training_patience"/><xsl:if
    test="count($generator-config/@training_patience) = 0">5</xsl:if>,
  "data_delay": <xsl:value-of select="$generator-config/@data_delay"/>,
  "algorithm_id": "<xsl:value-of select="$generator-config/@algorithm_id"/><xsl:if
    test="count($generator-config/@algorithm_id) = 0">catboost</xsl:if>"
}
</xsl:template>

</xsl:stylesheet>

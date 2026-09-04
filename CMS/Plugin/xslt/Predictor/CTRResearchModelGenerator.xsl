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
  <xsl:variable name="research-config"
    select="$xpath/configuration/cfg:ctrResearchModelGenerator"/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="central-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:central"/>
  <xsl:variable name="colo-config"
    select="$cluster-path/configuration/cfg:cluster"/>
  <xsl:variable name="clickhouse-config"
    select="$xpath/../service[
      @descriptor = 'AdCluster/BackendSubCluster/ClickhouseUploader'
    ]/configuration/cfg:clickhouseUploader"/>
  <xsl:variable name="user-navigation-sampling-value"><xsl:value-of
    select="$colo-config/cfg:userNavigation/@sampling"/><xsl:if
    test="count($colo-config/cfg:userNavigation/@sampling) = 0">100</xsl:if></xsl:variable>
  <xsl:variable name="workspace-root"><xsl:value-of
    select="$env-config/@workspace_root"/><xsl:if
    test="count($env-config/@workspace_root) = 0"><xsl:value-of
    select="$def-workspace-root"/></xsl:if></xsl:variable>
{
  "pid_file": "<xsl:value-of
    select="concat($workspace-root, '/run/CTRResearchModelGenerator.pid')"/>",
  "workspace_root": "<xsl:value-of select="$workspace-root"/>",
  "clickhouse_conn": "<xsl:value-of select="$clickhouse-config/@clickhouse_conn"/>",
  "postgres_conn": "<xsl:value-of select="$central-config/cfg:pgConnection/@connection_string"/>",
  "generate_period": <xsl:value-of select="$research-config/@generate_period"/><xsl:if
    test="count($research-config/@generate_period) = 0">86400</xsl:if>,
  "selection_chunk_rows": <xsl:value-of select="$research-config/@selection_chunk_rows"/><xsl:if
    test="count($research-config/@selection_chunk_rows) = 0">7000000</xsl:if>,
  "main_chunk_rows": <xsl:value-of select="$research-config/@main_chunk_rows"/><xsl:if
    test="count($research-config/@main_chunk_rows) = 0">10000000</xsl:if>,
  "validation_set_rows": <xsl:value-of select="$research-config/@validation_set_rows"/><xsl:if
    test="count($research-config/@validation_set_rows) = 0">200000</xsl:if>,
  "ctr_generator_workers": <xsl:value-of
    select="$research-config/@ctr_generator_workers"/><xsl:if
    test="count($research-config/@ctr_generator_workers) = 0">1</xsl:if>,
  "selection_validation_sets": <xsl:value-of
    select="$research-config/@selection_validation_sets"/><xsl:if
    test="count($research-config/@selection_validation_sets) = 0">3</xsl:if>,
  "training_validation_sets": <xsl:value-of
    select="$research-config/@training_validation_sets"/><xsl:if
    test="count($research-config/@training_validation_sets) = 0">3</xsl:if>,
  "final_test_sets": <xsl:value-of select="$research-config/@final_test_sets"/><xsl:if
    test="count($research-config/@final_test_sets) = 0">3</xsl:if>,
  "selection_fit_steps": <xsl:value-of select="$research-config/@selection_fit_steps"/><xsl:if
    test="count($research-config/@selection_fit_steps) = 0">10</xsl:if>,
  "max_feature_selection_steps": <xsl:value-of
    select="$research-config/@max_feature_selection_steps"/><xsl:if
    test="count($research-config/@max_feature_selection_steps) = 0">3</xsl:if>,
  "feature_correlation_threshold": <xsl:value-of
    select="$research-config/@feature_correlation_threshold"/><xsl:if
    test="count($research-config/@feature_correlation_threshold) = 0">0.98</xsl:if>,
  "training_fit_steps": <xsl:value-of select="$research-config/@training_fit_steps"/><xsl:if
    test="count($research-config/@training_fit_steps) = 0">30</xsl:if>,
  "fit_iterations": <xsl:value-of select="$research-config/@fit_iterations"/><xsl:if
    test="count($research-config/@fit_iterations) = 0">10</xsl:if>,
  "training_patience": <xsl:value-of select="$research-config/@training_patience"/><xsl:if
    test="count($research-config/@training_patience) = 0">5</xsl:if>,
  "user_navigation_sampling": <xsl:value-of select="$user-navigation-sampling-value"/>,
  "data_delay": <xsl:value-of select="$research-config/@data_delay"/>
}
</xsl:template>

</xsl:stylesheet>

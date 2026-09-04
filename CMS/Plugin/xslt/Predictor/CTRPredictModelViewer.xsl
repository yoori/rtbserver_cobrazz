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
  <xsl:variable name="viewer-config"
    select="$xpath/configuration/cfg:ctrPredictModelViewer"/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="workspace-root"><xsl:value-of
    select="$env-config/@workspace_root"/><xsl:if
    test="count($env-config/@workspace_root) = 0"><xsl:value-of
    select="$def-workspace-root"/></xsl:if></xsl:variable>
  <xsl:variable name="web-port"><xsl:value-of
    select="$viewer-config/cfg:networkParams/@port"/><xsl:if
    test="count($viewer-config/cfg:networkParams/@port) = 0"><xsl:value-of
    select="$def-ctr-predict-model-viewer-port"/></xsl:if></xsl:variable>
  <xsl:variable name="url-path"><xsl:value-of
    select="$viewer-config/@url_path"/><xsl:if
    test="count($viewer-config/@url_path) = 0"><xsl:text>/</xsl:text></xsl:if></xsl:variable>

{
  "pid_file": "<xsl:value-of select="concat($workspace-root, '/run/CTRPredictModelViewer.pid')"/>",
  "model_root": "<xsl:value-of select="concat($workspace-root, '/log/Predictor/CTRConfig')"/>",
  "research_model_root": "<xsl:value-of
    select="concat($workspace-root, '/log/Predictor/CTRResearch')"/>",
  "web_server": {
    "host": "0.0.0.0",
    "port": <xsl:value-of select="$web-port"/>
  },
  "url_path": "<xsl:value-of select="$url-path"/>"
}
</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- Predictor merger config generate function -->
<xsl:template name="MergerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="predictor-config"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:BidCostPredictorMergerConfiguration>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Predictor/BidCostPredictorMergerConfig.xsd')"/></xsl:attribute>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$predictor-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $bidcost-predictor-merger-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-log-level"/>
    </xsl:call-template>

    <cfg:BidCost>
      <xsl:attribute name="path">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/BidCostStat]]></xsl:attribute>
      <xsl:attribute name="cache">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/BidCostStatAgg]]></xsl:attribute>

      <xsl:attribute name="model_path">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/BidCostConfig]]></xsl:attribute>
      <xsl:attribute name="tmp_model_path">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/BidCostConfigTmp]]></xsl:attribute>

      <xsl:attribute name="ctr_model_path">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/CTRTrivialConfig]]></xsl:attribute>
      <xsl:attribute name="tmp_ctr_model_path">
        <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/CTRTrivialConfigTmp]]></xsl:attribute>

      <xsl:attribute name="aggregate_period">
        <xsl:value-of select="$predictor-config/cfg:merger/@aggregate_period"/>
        <xsl:if test="count($predictor-config/cfg:merger/@aggregate_period) = 0"><xsl:value-of
          select="$def-bidcost-predictor-merger-aggregate-period"/></xsl:if>
      </xsl:attribute>
      <xsl:attribute name="model_period">
        <xsl:value-of select="$predictor-config/cfg:merger/@model_period"/>
        <xsl:if test="count($predictor-config/cfg:merger/@model_period) = 0"><xsl:value-of
          select="$def-bidcost-predictor-merger-generate-model-period"/></xsl:if>
      </xsl:attribute>
      <xsl:attribute name="analyze_days">
        <xsl:value-of select="$predictor-config/@analyze_days"/>
        <xsl:if test="count($predictor-config/@analyze_days) = 0"><xsl:value-of select="$def-bidcost-predictor-merger-analyze-days"/></xsl:if>
      </xsl:attribute>
    </cfg:BidCost>

    <cfg:Model>
       <xsl:attribute name="path">
         <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/BidCostModel]]></xsl:attribute>
    </cfg:Model>
  </cfg:BidCostPredictorMergerConfiguration>
</xsl:template>


<xsl:template match="/">

  <xsl:variable name="cluster-path" select="$xpath/../.."/>

  <xsl:variable name="bidcost-predictor-path" select="$xpath"/>

  <xsl:variable
    name="colo-config"
    select="$cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="bidcost-predictor-config"
    select="$bidcost-predictor-path/configuration/cfg:predictor"/>

  <xsl:call-template name="MergerConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="predictor-config" select="$bidcost-predictor-config"/>
  </xsl:call-template>
  
</xsl:template>

</xsl:stylesheet>

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

  <cfg:MergerConfiguration>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Predictor/PredictorMergerConfig.xsd')"/></xsl:attribute>
    <xsl:attribute name="output_path">
      <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/PRImpression]]></xsl:attribute>
    <xsl:attribute name="max_timeout">
      <xsl:value-of select="$predictor-config/cfg:merger/@max_timeout"/>
      <xsl:if test="count($predictor-config/cfg:merger/@max_timeout) = 0"><xsl:value-of select="$def-predictor-merger-timeout"/></xsl:if>
    </xsl:attribute>
    <xsl:attribute name="log_days_to_keep">
      <xsl:value-of select="$predictor-config/@log_days_to_keep"/>
      <xsl:if test="count($predictor-config/@log_days_to_keep) = 0"><xsl:value-of select="$def-predictor-keep-logs"/></xsl:if>
    </xsl:attribute>
    <xsl:attribute name="sleep_timeout">
      <xsl:value-of select="$predictor-config/cfg:merger/@sleep_timeout"/>
      <xsl:if test="count($predictor-config/cfg:merger/@sleep_timeout) = 0"><xsl:value-of select="$def-predictor-merger-sleep_timeout"/></xsl:if>
    </xsl:attribute>
    
    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$predictor-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $predictor-merger-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-log-level"/>
    </xsl:call-template>

    <cfg:Impression>
       <xsl:attribute name="path">
         <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/ResearchImpression]]></xsl:attribute>
       <xsl:attribute name="from">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:impression/@from"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:impression/@from) = 0"><xsl:value-of select="$def-predictor-merger-from"/></xsl:if>
       </xsl:attribute>
       <xsl:attribute name="to">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:impression/@to"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:impression/@to) = 0"><xsl:value-of select="$def-predictor-merger-imp-to"/></xsl:if>
       </xsl:attribute>
    </cfg:Impression>
    <cfg:Click>
       <xsl:attribute name="path">
         <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/ResearchClick]]></xsl:attribute>
       <xsl:attribute name="from">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:click/@from"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:click/@from) = 0"><xsl:value-of select="$def-predictor-merger-from"/></xsl:if>
       </xsl:attribute>
       <xsl:attribute name="to">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:click/@to"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:click/@to) = 0"><xsl:value-of select="$def-predictor-merger-to"/></xsl:if>
       </xsl:attribute>
    </cfg:Click>
    <cfg:Action>
       <xsl:attribute name="path">
         <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/ResearchAction]]></xsl:attribute>
       <xsl:attribute name="from">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:action/@from"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:action/@from) = 0"><xsl:value-of select="$def-predictor-merger-from"/></xsl:if>
       </xsl:attribute>
       <xsl:attribute name="to">
         <xsl:value-of select="$predictor-config/cfg:merger/cfg:action/@to"/>
         <xsl:if test="count($predictor-config/cfg:merger/cfg:action/@to) = 0"><xsl:value-of select="$def-predictor-merger-to"/></xsl:if>
       </xsl:attribute>
    </cfg:Action>
  </cfg:MergerConfiguration>
</xsl:template>

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
    name="predictor-config"
    select="$predictor-path/configuration/cfg:predictor"/>

  <xsl:call-template name="MergerConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="predictor-config" select="$predictor-config"/>
  </xsl:call-template>
  
</xsl:template>

</xsl:stylesheet>

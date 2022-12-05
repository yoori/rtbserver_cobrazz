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
<xsl:template name="SVMGeneratorConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="predictor-config"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:SVMGeneratorConfiguration>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Predictor/CTRPredictorSVMGeneratorConfig.xsd')"/></xsl:attribute>
   <xsl:attribute name="input_path">
      <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/PRImpression]]></xsl:attribute>
   <xsl:attribute name="output_path">
      <xsl:value-of select="$workspace-root"/><![CDATA[/log/Predictor/ResearchLogs/LibSVM]]></xsl:attribute>
    <xsl:attribute name="log_days_to_keep">
      <xsl:value-of select="$predictor-config/@log_days_to_keep"/>
      <xsl:if test="count($predictor-config/@log_days_to_keep) = 0"><xsl:value-of select="$def-predictor-keep-logs"/></xsl:if>
    </xsl:attribute>
    
    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$predictor-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $predictor-svm-generator-log-path)"/>
      <xsl:with-param name="default-log-level" select="$default-log-level"/>
    </xsl:call-template>

    <xsl:variable name="svm-generator-port">
      <xsl:value-of select="$predictor-config/cfg:svmGenerator/cfg:networkParams/@port"/>
      <xsl:if test="count($predictor-config/cfg:svmGenerator/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-svm-generator-port"/>
      </xsl:if>
    </xsl:variable>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool">
        <xsl:value-of select="1"/>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$svm-generator-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>
    
    <cfg:Model features_dimension="24">
      <cfg:Feature>
         <cfg:BasicFeature name="publisher"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="tag"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="sizeid"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="wd"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="hour"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="device"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="campaign"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="group"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="ccid"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="campaign_freq"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="campaign_freq_log"/>
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="geoch"/>      
      </cfg:Feature>
      <cfg:Feature>
        <cfg:BasicFeature name="userch"/>      
      </cfg:Feature>
    </cfg:Model>
  </cfg:SVMGeneratorConfiguration>
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

  <xsl:call-template name="SVMGeneratorConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="predictor-config" select="$predictor-config"/>
  </xsl:call-template>
  
</xsl:template>

</xsl:stylesheet>

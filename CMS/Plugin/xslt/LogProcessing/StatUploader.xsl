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

<xsl:include href="../Variables.xsl"/>

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="UploaderConfig">
  <xsl:param name="workspace-root"/>	
  <xsl:param name="list"/>
  <xsl:param name="stat-uploader-config"/>	
  <xsl:param name="stat-node"/>

  <xsl:variable name="newlist" select="concat(normalize-space($list), ' ')"/>
  <xsl:variable name="head" select="substring-before($newlist, ' ')"/>
  <xsl:variable name="tail" select="substring-after($newlist, ' ')"/>

  <xsl:variable name="stat-path" select="$stat-node/*[local-name() = $head]"/>

    {
      "upload_type": "<xsl:value-of select="$head"/>",
      "in_dir": "<xsl:value-of select="$workspace-root"/>/log/LogGeneralizer/Out/<xsl:value-of select="$head"/>",
      "failure_dir": "<xsl:value-of select="$workspace-root"/>/log/StatUploader/Failure/<xsl:value-of select="$head"/>"
    }

  <xsl:if test="$tail">
    ,
    <xsl:call-template name="UploaderConfig">
      <xsl:with-param name="workspace-root" select="$workspace-root"/>	    
      <xsl:with-param name="list" select="$tail"/>
      <xsl:with-param name="stat-uploader-config" select="$stat-uploader-config"/>
      <xsl:with-param name="stat-node" select="$stat-node"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!-- StatUploader config generate function -->
<xsl:template name="StatUploaderConfigGenerator">
  <xsl:param name="env-config"/>	
  <xsl:param name="stat-uploader-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

{
  "period": <xsl:value-of select="$stat-uploader-check-logs-period"/>,

  <xsl:if test="count($stat-uploader-config/cfg:clickhouse/@host) != 0">
  "ch_host": "<xsl:value-of select="$stat-uploader-config/cfg:clickhouse/@host"/>",
  </xsl:if>

  <xsl:if test="count($stat-uploader-config/cfg:logging/@verbosity) != 0">
  "verbosity": <xsl:value-of select="$stat-uploader-config/cfg:logging/@verbosity"/>,
  </xsl:if>

  "uploads": [
    <xsl:variable name="uploads-config" select="$stat-uploader-config/cfg:uploads"/>

    <xsl:call-template name="UploaderConfig">
      <xsl:with-param name="workspace-root" select="$workspace-root"/>	    
      <xsl:with-param name="list" select="'RequestStatsHourlyExtStat'"/>
      <xsl:with-param name="stat-uploader-config" select="$stat-uploader-config"/>
      <xsl:with-param name="stat-node" select="$uploads-config"/>
    </xsl:call-template>
  ]
}
</xsl:template>

<xsl:template match="/">

  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="stat-uploader-path"
    select="$xpath"/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="stat-uploader-config"
    select="$stat-uploader-path/configuration/cfg:statUploader"/>

  <xsl:call-template name="StatUploaderConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="stat-uploader-config" select="$stat-uploader-config"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>


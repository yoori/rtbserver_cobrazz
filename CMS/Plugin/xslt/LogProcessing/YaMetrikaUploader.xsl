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

<!-- YaMetrikaUploader config generate function -->
<xsl:template name="YaMetrikaUploaderConfigGenerator">
  <xsl:param name="env-config"/>	
  <xsl:param name="ya-metrika-uploader-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

{
  "period": <xsl:value-of select="$ya-metrika-uploader-check-logs-period"/>,

  "ch_host": "<xsl:value-of select="$ya-metrika-uploader-config/cfg:clickhouse/@host"/>",

  "pg_host": "<xsl:value-of select="$ya-metrika-uploader-config/cfg:postgres/@host"/>",
  "pg_db": "<xsl:value-of select="$ya-metrika-uploader-config/cfg:postgres/@db"/>",
  "pg_user": "<xsl:value-of select="$ya-metrika-uploader-config/cfg:postgres/@user"/>",
  "pg_pass": "<xsl:value-of select="$ya-metrika-uploader-config/cfg:postgres/@pass"/>",

  "tmp_dir": "<xsl:value-of select="$workspace-root"/>/log/YaMetrikaUploader/Temp",
  "out_dir": "<xsl:value-of select="$workspace-root"/>/log/YaMetrikaUploader/Out",

  <xsl:if test="count($ya-metrika-uploader-config/cfg:logging/@verbosity) != 0">
  "verbosity": <xsl:value-of select="$ya-metrika-uploader-config/cfg:logging/@verbosity"/>,
  </xsl:if>

  "days": <xsl:value-of select="$ya-metrika-uploader-config/cfg:params/@days"/>
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
    name="ya-metrika-uploader-path"
    select="$xpath"/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="ya-metrika-uploader-config"
    select="$ya-metrika-uploader-path/configuration/cfg:statUploader"/>

  <xsl:call-template name="YaMetrikaUploaderConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="ya-metrika-uploader-config" select="$ya-metrika-uploader-config"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>


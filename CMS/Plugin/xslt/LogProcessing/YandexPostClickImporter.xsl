<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn colo"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration">

<xsl:output method="text" indent="no" encoding="utf-8"/>
<xsl:include href="../Variables.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="config"
    select="$xpath/configuration/cfg:yandexPostClickImporter"/>
  <xsl:variable name="central-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:central"/>
  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of
      select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="period"><xsl:value-of select="$config/@period"/>
    <xsl:if test="count($config/@period) = 0">3600</xsl:if>
  </xsl:variable>
  <xsl:variable name="days"><xsl:value-of select="$config/@days"/>
    <xsl:if test="count($config/@days) = 0">4</xsl:if>
  </xsl:variable>
  <xsl:variable name="attribution"><xsl:value-of select="$config/@attribution"/>
    <xsl:if test="count($config/@attribution) = 0">lastsign</xsl:if>
  </xsl:variable>
  <xsl:variable name="chunks-count"><xsl:value-of select="$config/@chunks_count"/>
    <xsl:if test="count($config/@chunks_count) = 0">24</xsl:if>
  </xsl:variable>
  <xsl:variable name="request-timeout"><xsl:value-of select="$config/@request_timeout"/>
    <xsl:if test="count($config/@request_timeout) = 0">60</xsl:if>
  </xsl:variable>
  <xsl:variable name="ch-port"><xsl:value-of select="$config/cfg:clickhouse/@port"/>
    <xsl:if test="count($config/cfg:clickhouse/@port) = 0">8123</xsl:if>
  </xsl:variable>
  <xsl:variable name="ch-database"><xsl:value-of
    select="$config/cfg:clickhouse/@database"/>
    <xsl:if test="count($config/cfg:clickhouse/@database) = 0">default</xsl:if>
  </xsl:variable>
  <xsl:variable name="ch-user"><xsl:value-of select="$config/cfg:clickhouse/@user"/>
    <xsl:if test="count($config/cfg:clickhouse/@user) = 0">default</xsl:if>
  </xsl:variable>
  <xsl:variable name="secure"><xsl:choose>
    <xsl:when test="$config/cfg:clickhouse/@secure = 'true' or
      $config/cfg:clickhouse/@secure = '1'">true</xsl:when>
    <xsl:otherwise>false</xsl:otherwise>
  </xsl:choose></xsl:variable>
  <xsl:variable name="pg-dsn"><xsl:choose>
    <xsl:when test="count($config/cfg:postgres/@connection_string) != 0"><xsl:value-of
      select="$config/cfg:postgres/@connection_string"/></xsl:when>
    <xsl:otherwise><xsl:value-of
      select="$central-config/cfg:pgConnectionForLogProcessing/@connection_string"/></xsl:otherwise>
  </xsl:choose></xsl:variable>

{
  "period": <xsl:value-of select="$period"/>,
  "pid_file": "<xsl:value-of select="$workspace-root"/>/run/YandexPostClickImporter.pid",
  "tmp_dir": "<xsl:value-of select="$workspace-root"/>/log/YandexPostClickImporter/Temp",
  "out_dir": "<xsl:value-of select="$workspace-root"/>/log/YandexPostClickImporter/Out",
  "log_dir": "<xsl:value-of select="$workspace-root"/>/log/YandexPostClickImporter/Log",
  "days": <xsl:value-of select="$days"/>,
  "sources": "<xsl:value-of select="$config/@sources"/>",
  "attribution": "<xsl:value-of select="$attribution"/>",
  "chunks_count": <xsl:value-of select="$chunks-count"/>,
  "request_timeout": <xsl:value-of select="$request-timeout"/>,
  "pg_dsn": "<xsl:value-of select="$pg-dsn"/>",
  "ch_host": "<xsl:value-of select="$config/cfg:clickhouse/@host"/>",
  "ch_port": <xsl:value-of select="$ch-port"/>,
  "ch_database": "<xsl:value-of select="$ch-database"/>",
  "ch_user": "<xsl:value-of select="$ch-user"/>",
  "ch_pass": "<xsl:value-of select="$config/cfg:clickhouse/@pass"/>",
  "ch_secure": <xsl:value-of select="$secure"/>
}
</xsl:template>
</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:variable name="service" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="config" select="$service/configuration/cfg:segmentModelViewer"/>
  <xsl:variable name="environment" select="$service/../configuration/cfg:cluster/cfg:environment"/>
  <xsl:template match="/">
    <xsl:variable name="workspace-root"><xsl:choose>
      <xsl:when test="$environment/@workspace_root"><xsl:value-of select="$environment/@workspace_root"/></xsl:when>
      <xsl:otherwise>/u01/foros/server-ai/var</xsl:otherwise>
    </xsl:choose></xsl:variable>
{
  "pid_file": "<xsl:value-of select="$workspace-root"/>/run/SegmentModelViewer.pid",
  "model_root": "<xsl:value-of select="$workspace-root"/>/log/SegmentGenerator/Models",
  "web_server": {
    "host": "<xsl:choose><xsl:when test="$config/@listen_host"><xsl:value-of select="$config/@listen_host"/></xsl:when><xsl:otherwise>0.0.0.0</xsl:otherwise></xsl:choose>",
    "port": <xsl:choose><xsl:when test="$config/@port"><xsl:value-of select="$config/@port"/></xsl:when><xsl:otherwise>11437</xsl:otherwise></xsl:choose>
  },
  "url_path": "<xsl:choose><xsl:when test="$config/@url_path"><xsl:value-of select="$config/@url_path"/></xsl:when><xsl:otherwise>/</xsl:otherwise></xsl:choose>"
}
  </xsl:template>
</xsl:stylesheet>

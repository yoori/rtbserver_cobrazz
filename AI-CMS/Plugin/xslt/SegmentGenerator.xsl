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
  <xsl:variable name="environment" select="$service/../configuration/cfg:cluster/cfg:environment"/>
  <xsl:template match="/">{
  "pid_file": "<xsl:choose><xsl:when test="$environment/@workspace_root"><xsl:value-of select="$environment/@workspace_root"/></xsl:when><xsl:otherwise>/u01/foros/server-ai/var</xsl:otherwise></xsl:choose>/run/SegmentGenerator.pid"
}
</xsl:template>
</xsl:stylesheet>

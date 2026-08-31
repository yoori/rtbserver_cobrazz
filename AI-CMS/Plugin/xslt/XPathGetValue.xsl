<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:template match="/"><xsl:value-of select="dyn:evaluate($XPATH)"/></xsl:template>
</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration">
<xsl:output method="text" indent="no" encoding="utf-8"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:include href="Functions.xsl"/>
<xsl:template match="/">
  <xsl:variable name="hostlist">
    <xsl:for-each select="$xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="./@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="sorted-hosts">
    <xsl:for-each select="exsl:node-set($hostlist)//host">
      <xsl:sort select="."/>
      <host><xsl:value-of select="."/></host>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="hosts">
    <xsl:for-each select="exsl:node-set($sorted-hosts)//host">
      <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
        <xsl:value-of select="concat(., ' ')"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:value-of select="normalize-space($hosts)"/>

</xsl:template>
</xsl:stylesheet>

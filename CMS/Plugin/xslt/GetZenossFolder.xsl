<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl">
<xsl:output method="text" indent="no" encoding="utf-8"/>
<xsl:include href="Functions.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
  <xsl:variable name="zenoss-folder">
    <xsl:call-template name="ZenossFolder">
      <xsl:with-param name="app-xpath" select="$xpath"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="normalize-space($zenoss-folder)"/>
</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
      <xsl:message terminate="yes"> PerformanceTest:Campaigns: Can't find performance test config.</xsl:message>
    </xsl:when>
    <xsl:when test="count($xpath/cfg:campaigns/cfg:campaign) = 0">
      <xsl:message terminate="yes"> PerformanceTest:Campaigns: Can't find campaigns config for performance test.</xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:for-each select="$xpath/cfg:campaigns/cfg:campaign">
    <xsl:variable name="campaign-count" select="@count"/>
    <xsl:variable name="campaign-types">
      <xsl:for-each select="cfg:type">
        <xsl:value-of select="."/>
          <xsl:if test="position()!=last()">,</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="$campaign-types"/>: <xsl:value-of select="$campaign-count"/><xsl:if test="position()!=last()"><xsl:text>&#xa;</xsl:text></xsl:if>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

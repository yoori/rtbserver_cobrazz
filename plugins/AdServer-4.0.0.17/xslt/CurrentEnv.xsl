<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
>

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- CurrentEnvBe config generate function -->
<xsl:template name="CurrentEnvGenerator">
  <xsl:param name="port-xpath"/>

  <xsl:variable name="control-port">
    <xsl:value-of select="$port-xpath"/>
    <xsl:if test="count($port-xpath) = 0">
      <xsl:value-of select="dyn:evaluate(concat('$', $DEF_PORT_VAR_NAME))"/>
    </xsl:if>
  </xsl:variable>

<xsl:value-of select="$SERVICE_NAME"/>_port=<xsl:value-of select="$control-port"/>
export <xsl:value-of select="$SERVICE_NAME"/>_port

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="port-xpath" select="$xpath"/>
  
  <xsl:call-template name="CurrentEnvGenerator">
    <xsl:with-param name="port-xpath" select="$port-xpath"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

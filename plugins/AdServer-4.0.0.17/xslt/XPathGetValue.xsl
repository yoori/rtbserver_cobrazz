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
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:template match="/"><xsl:value-of select="$xpath"/></xsl:template>
</xsl:stylesheet>

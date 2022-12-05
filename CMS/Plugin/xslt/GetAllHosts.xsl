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
<xsl:template match="/">
    <xsl:for-each select="/colo:colocation/host">
      <xsl:value-of select="concat(./@hostName, '&#10;')"/>
    </xsl:for-each>
</xsl:template>
</xsl:stylesheet>

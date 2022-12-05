<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="var-root" select="'VALUE'"/>

<xsl:include href="../RelativeXSL2/variables.xsl"/>

  <xsl:template match="/">
  <Simple1>
    <xsl:value-of select="//title"/>
  </Simple1>
  <Simple2>
    <xsl:value-of select="//author"/>
  </Simple2>


  <xsl:value-of select="$current-major-version"/>

  </xsl:template>

  
</xsl:stylesheet>

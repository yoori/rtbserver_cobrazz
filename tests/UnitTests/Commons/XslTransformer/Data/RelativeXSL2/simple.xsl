<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="/">
  <HEADING1>
    <xsl:value-of select="//title"/>
  </HEADING1>
  <HEADING2>
    <xsl:value-of select="//author"/>
  </HEADING2>
  </xsl:template>

</xsl:stylesheet>

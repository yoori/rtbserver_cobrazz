<?xml version="1.0" encoding="windows-1251"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" indent="no" encoding="Windows-1251"/>

<xsl:strip-space elements="*"/>
<xsl:strip-space elements="block"/>
<xsl:strip-space elements="column"/>

<xsl:template match="/">
  <xsl:apply-templates/>
</xsl:template> 

<xsl:template match="section">
<xsl:apply-templates>
  <xsl:with-param name="space"></xsl:with-param>
</xsl:apply-templates>
</xsl:template> 

<xsl:template name="section" match="section">
<xsl:param name="space"/>
<xsl:value-of select="$space"/><xsl:value-of select="@name"/>: <xsl:text>
</xsl:text>
  <xsl:apply-templates>
    <xsl:with-param name="space"><xsl:text>  </xsl:text><xsl:value-of select="$space"/></xsl:with-param>
  </xsl:apply-templates>
</xsl:template>

<xsl:template name="column" match="column">
<xsl:param name="space"/>
<xsl:value-of select="$space"/><xsl:value-of select="@name"/> : <xsl:value-of select="@value"/>
<xsl:text>
</xsl:text>
</xsl:template> 

</xsl:stylesheet>

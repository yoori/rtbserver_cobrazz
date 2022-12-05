<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet 
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  >

<xsl:param name="test_xsl_parameter1">Default1
</xsl:param>
<xsl:param name="test_xsl_parameter2">Default2
</xsl:param>
<xsl:param name="test_xsl_parameter3">Default3
</xsl:param>
<xsl:param name="test_xsl_parameter4">Default4
</xsl:param>

  <xsl:output method="text" encoding="utf-8"/>

  <!-- Print XSLT parameters values -->

  <xsl:template match="/">
    <xsl:value-of select="$test_xsl_parameter1"/>
    <xsl:value-of select="$test_xsl_parameter2"/>
    <xsl:value-of select="$test_xsl_parameter3"/>
    <xsl:value-of select="$test_xsl_parameter4"/>
  </xsl:template>

</xsl:stylesheet>

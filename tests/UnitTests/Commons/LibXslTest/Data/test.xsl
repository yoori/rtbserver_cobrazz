<?xml version="1.0" encoding="windows-1251"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:external="http://foros.com/foros/xslt-template"
  exclude-result-prefixes="external">

<!--  xmlns:external="http://ExternalFunction.xalan-c++.xml.apache.org"-->

<xsl:param name="test_xsl_parameter1">Default1
</xsl:param>
<xsl:param name="test_xsl_parameter2">Default2
</xsl:param>
<xsl:param name="test_xsl_parameter3">Default3
</xsl:param>
<xsl:param name="test_xsl_parameter4">Default4
</xsl:param>

<xsl:output method="text" encoding="Windows-1251"/>

<xsl:template match="creative">

  <xsl:for-each select="token">
    <xsl:value-of select="@name"/>
    <xsl:value-of select="external:escape-js(@value)"/>
  </xsl:for-each>
</xsl:template>

  <xsl:template match="/">
    <xsl:value-of select="$test_xsl_parameter1"/>
    <xsl:value-of select="$test_xsl_parameter2"/>
    <xsl:value-of select="$test_xsl_parameter3"/>
    <xsl:value-of select="$test_xsl_parameter4"/>
  </xsl:template>

</xsl:stylesheet>




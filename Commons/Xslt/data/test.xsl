<?xml version="1.0" encoding="windows-1251"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:external="http://ExternalFunction.xalan-c++.xml.apache.org"
  exclude-result-prefixes="external">

<xsl:output method="text" encoding="Windows-1251"/>

<xsl:template match="creative">

  <xsl:for-each select="token">
    <xsl:value-of select="@name"/>
    <xsl:value-of select="external:escape-js(@value)"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

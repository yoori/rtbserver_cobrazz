<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:external="http://foros.com/foros/xslt-template"
  exclude-result-prefixes="external">

<xsl:output method="text" encoding="utf-8"/>

<xsl:template match="creative">

  <xsl:for-each select="token">
    <xsl:value-of select="@name"/>
    <xsl:value-of select="external:escape-js(string(@value))"/>
    <xsl:value-of select="external:escape-js('ü€€€€€')"/>
    <xsl:value-of select="external:escape-js-unicode('ü€€€€€')"/>
    <xsl:value-of select="external:escape-js('ù€€€€')"/>
    <xsl:value-of select="external:escape-js-unicode('ù€€€€')"/>
    <xsl:value-of select="external:escape-js('õ¿¿€')"/>
    <xsl:value-of select="external:escape-js-unicode('õ¿¿€')"/>
    <xsl:value-of select="external:escape-js('ð€€¿')"/>
    <xsl:value-of select="external:escape-js-unicode('ð€€¿')"/>
    <xsl:value-of select="external:escape-js('àŸ¿')"/>
    <xsl:value-of select="external:escape-js-unicode('àŸ¿')"/>
    <xsl:value-of select="external:escape-js('í €')"/>
    <xsl:value-of select="external:escape-js-unicode('í €')"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

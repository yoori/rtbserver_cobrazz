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
    <xsl:value-of select="external:escape-js('Russian:правильная UTF-8 строка')"/>
    <xsl:value-of select="external:escape-js('Deseret:𐐎𐐲𐐻 𐐮𐑆 𐐏𐐭𐑌𐐮𐐿𐐬𐐼')"/>
    <xsl:value-of select="external:escape-js-unicode('Russian:правильная UTF-8 строка')"/>
    <xsl:value-of select="external:escape-js-unicode('Deseret:𐐎𐐲𐐻 𐐮𐑆 𐐏𐐭𐑌𐐮𐐿𐐬𐐼')"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

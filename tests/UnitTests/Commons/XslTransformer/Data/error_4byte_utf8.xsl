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
    <xsl:value-of select="external:escape-js('א__Tא___א_'_א___א___א_'<א__פא_'_א__>א___א_'_א__נ, א__ץא___א_'<א__üא__תא__-א__>א_'%א_'Å א___א___א_':א__> א___א__נא_'_ א__א__Rא_'_ א_'_א___א_'<א__ףא_'%א_'Å. ')"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

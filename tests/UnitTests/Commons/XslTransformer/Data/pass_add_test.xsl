<?xml version="1.0" encoding="windows-1251"?>
<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration ../AdServer/ChannelSvcs/ChannelManagerControllerConfig.xsd">

<xsl:output method="xml" indent="yes" encoding="Windows-1251"/>

<xsl:variable name="xpath" select="/TTT/dbConnection"/>

<!--
<xsl:template name="elementPath">
  <xsl:for-each 
    select="(ancestor-or-self::*)">
    /*[<xsl:value-of select="1+count(preceding-sibling::*)"/>]
  </xsl:for-each>
</xsl:template>

<xsl:template match="*">
    <xsl:for-each select="ancestor-of-self">
      <xsl:value-of select="name()" /><xsl:text>/</xsl:text>
    </xsl:for-each>

  <xsl:apply-templates/>
</xsl:template>
-->

<xsl:template match="/">
  <ROOT>
    <xsl:for-each select="$xpath//.">
       <TR>
       <xsl:value-of select="name()"/>
       <xsl:value-of select="name()"/>
       <xsl:value-of select="@user"/>
       </TR>
    </xsl:for-each>
  </ROOT>
</xsl:template>

</xsl:stylesheet>

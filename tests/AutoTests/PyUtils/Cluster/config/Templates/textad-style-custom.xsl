<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:variable name="headline-color" select="/impression/token[@name='HEADLINE_COLOR']"/>
  <xsl:variable name="description-color" select="/impression/token[@name='DESCRIPTION_COLOR']"/>
  <xsl:variable name="url-color" select="/impression/token[@name='URL_COLOR']"/>
  <xsl:variable name="background-color" select="/impression/token[@name='BACKGROUND_COLOR']"/>
  <xsl:variable name="background-image" select="/impression/token[@name='BACKGROUND_IMAGE']"/>
  <xsl:variable name="eborder-color" select="/impression/token[@name='EXT_BORDER_COLOR']"/>
  <xsl:variable name="iborder-color" select="/impression/token[@name='INT_BORDER_COLOR']"/>
  <xsl:variable name="eborder-size" select="/impression/token[@name='EXT_BORDER_SIZE']"/>
  <xsl:variable name="iborder-size" select="/impression/token[@name='INT_BORDER_SIZE']"/>
  <xsl:variable name="font" select="/impression/token[@name='TA_FONT']"/>
  <xsl:variable name="font-size" select="/impression/token[@name='TA_FONT_SIZE']"/>
  <xsl:template name="custom-style">
    <xsl:if test="$background-color">
      body,#logo {background-color:#<xsl:value-of select="$background-color"/>;}
    </xsl:if>
    <xsl:if test="$background-image">
      .impression {background-image:url("<xsl:value-of select="$background-image"/>"); background-position:center; background-repeat:no-repeat;}
    </xsl:if>
    <xsl:if test="$eborder-size">
      body {border-width:<xsl:value-of select="$eborder-size"/>;}
    </xsl:if>
    <xsl:if test="$eborder-color">
      body {border-color:#<xsl:value-of select="$eborder-color"/>;}
    </xsl:if>
    <xsl:if test="$description-color">
      .creative td {color:#<xsl:value-of select="$description-color"/>;}
    </xsl:if>
    <xsl:if test="$font">
      .creative td {font-family:<xsl:value-of select="$font"/>;}
    </xsl:if>
    <xsl:if test="$font-size">
      .creative td {font-size:<xsl:value-of select="$font-size"/>;}
    </xsl:if>
    <xsl:if test="$iborder-size">
      .creative table {border-width:<xsl:value-of select="$iborder-size"/>; border-style:solid; border-color:gray;}
      <xsl:call-template name="image-padding"/>
    </xsl:if>
    <xsl:if test="$iborder-color">
      .creative table {border-color:#<xsl:value-of select="$iborder-color"/>;}
    </xsl:if>
    <xsl:if test="$headline-color">
      .HEADLINE {color:#<xsl:value-of select="$headline-color"/>;}
    </xsl:if>
    <xsl:if test="$url-color">
      .DISPLAY_URL {color:#<xsl:value-of select="$url-color"/>;}
    </xsl:if>
  </xsl:template>
  <xsl:template name="image-padding">
    <xsl:param name="top-padding" select="$image-top-padding - 2*number(substring-before($iborder-size, 'px')) + 1"/>
    <xsl:choose>
      <xsl:when test="$top-padding > 0">
        td.himage,td.one-image {padding-top:<xsl:value-of select="$top-padding"/>px !important;}
      </xsl:when>
      <xsl:otherwise>
        td.himage,td.one-image {padding-top:0 !important;}
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>

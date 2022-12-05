<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:include href="textad-style-korean.xsl"/>
  <xsl:include href="textad-style-mobile.xsl"/>
  <xsl:include href="textad-style-custom.xsl"/>
  <xsl:include href="textad-style-image.xsl"/>
  <xsl:variable name="base-font-size" select="10"/>
  <xsl:variable name="image-top-padding" select="4"/>
  <xsl:variable name="ext-border-size">
    <xsl:choose>
      <xsl:when test="//token[@name='EXT_BORDER_SIZE']">
        <xsl:value-of select="substring(/impression/token[@name='EXT_BORDER_SIZE'], 1, 1)"/>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="int-border-size">
    <xsl:choose>
      <xsl:when test="//token[@name='INT_BORDER_SIZE']">
        <xsl:value-of select="substring(/impression/token[@name='EXT_BORDER_SIZE'], 1, 1)"/>
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="cell-side-padding">
    <xsl:choose>
      <xsl:when test="2*$ext-border-size &lt; 5">
        <xsl:value-of select="5 - 2*$ext-border-size"/>
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="length">
    <xsl:choose>
      <xsl:when test="$size='728x90' or $size='468x60'">
        <xsl:value-of select="$width"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$height"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="creative-size" select="round(($length - 2*$ext-border-size) div $creative-count) - 2*$int-border-size"/>
  <xsl:variable name="last-creative-size" select="$length - 2*$ext-border-size - $creative-size*($creative-count - 1)"/>
  <xsl:template name="style">
    body {margin:0; padding:0; overflow:hidden; background-color:#FFFFFF; border:1px solid gray;}
    .impression {overflow:hidden; width:<xsl:value-of select="$width - 2*$ext-border-size"/>px; height:<xsl:value-of select="$height - 2*$ext-border-size"/>px;}
    .impression a {text-decoration:none; cursor:pointer;}
    .creative {float:left; width:100%; height:100%; overflow:hidden;}
    .creative table {width:100%; height:100%; margin:0; padding:0; border-collapse:collapse;}
    .creative td {padding:0.3em; height:100%; font-family:Arial,sans-serif; font-size:<xsl:value-of select="$base-font-size"/>px; color:black;}
    .creative td.one {width:100%; text-align:center; vertical-align:middle;}
    .wrap {word-wrap:break-word;}
    .HEADLINE {font-weight:bold; color:blue; text-decoration:underline;}
    .DISPLAY_URL {color:#008000; font-size:10px; word-break:break-all;}
    &#35;ibutton {width:12px; height:12px; position:absolute; right:5px; bottom:5px; z-index:10; border:1px solid #551A8B; font:bold 10px monospace; text-align:center; background-color:white; overflow:hidden;}
    &#35;ibutton a {text-decoration:none;}
    <xsl:if test="boolean(//token[@name='IMAGE_FILE'])">
      <xsl:call-template name="image-style"/>
    </xsl:if>
    <xsl:choose>
      <!-- HORIZONTAL BANNERS -->
      <xsl:when test="$size='234x60'">
        <xsl:call-template name="B234x60"/>
      </xsl:when>
      <xsl:when test="$size='728x90'">
        <xsl:call-template name="B728x90"/>
      </xsl:when>
      <xsl:when test="$size='468x60'">
        <xsl:call-template name="B468x60"/>
      </xsl:when>
      <!-- VERTICAL BANNERS -->
      <xsl:when test="$size='120x600'">
        <xsl:call-template name="B120x600"/>
      </xsl:when>
      <xsl:when test="$size='160x600'">
        <xsl:call-template name="B160x600"/>
      </xsl:when>
      <xsl:when test="$size='300x250' or $size='336x280'">
        <xsl:call-template name="B300x250"/>
      </xsl:when>
      <!-- KOREAN -->
      <xsl:when test="$size='580x150'">
        <xsl:call-template name="B580x150"/>
      </xsl:when>
      <xsl:when test="$size='250x250'">
        <xsl:call-template name="B250x250"/>
      </xsl:when>
      <xsl:when test="$size='560x200'">
        <xsl:call-template name="B560x200"/>
      </xsl:when>
      <xsl:when test="$size='545x150'">
        <xsl:call-template name="B545x150"/>
      </xsl:when>
      <!-- MOBILE -->
      <xsl:when test="$size=$mobile-size">
        <xsl:call-template name="mtop"/>
      </xsl:when>
      <!-- 234x90 OIX PREVIEW -->
      <xsl:otherwise>
        <xsl:call-template name="B234x90"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="custom-style"/>
  </xsl:template>
  <xsl:template name="B234x60">
    .creative td {vertical-align:top;}
  </xsl:template>
  <xsl:template name="B234x90">
    .creative td {vertical-align:top; text-align:left !important;}
  </xsl:template>
  <xsl:template name="B728x90">
    <xsl:call-template name="horizontal-sizes"/>
    .creative td {vertical-align:top;}
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+8"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+5"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=3">
        .creative td {font-size:<xsl:value-of select="$base-font-size+3"/>px;}
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="B468x60">
    <xsl:call-template name="horizontal-sizes"/>
    .creative td {vertical-align:top;}
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+2"/>px;}
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="B120x600">
    .creative td {text-align:center; vertical-align:middle; padding:0.3em 0; width:<xsl:value-of select="$width - 2*$ext-border-size"/>px;}
    .wrap {width:<xsl:value-of select="$width - 2*$ext-border-size - 2*$int-border-size - 2*$cell-side-padding"/>px; margin:0 auto;}
    <xsl:call-template name="vertical-sizes"/>
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+6"/>px;}
        .HEADLINE {padding-bottom:0.5em;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+4"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=3">
        .creative td {font-size:<xsl:value-of select="$base-font-size+2"/>px;}
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="B160x600">
    .creative td {text-align:center; vertical-align:middle; padding:0.3em 0; width:<xsl:value-of select="$width - 2*$ext-border-size"/>px;}
    .wrap {width:<xsl:value-of select="$width - 2*$ext-border-size - 2*$int-border-size - 2*$cell-side-padding"/>px; margin:0 auto;}
    <xsl:call-template name="vertical-sizes"/>
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+8"/>px;}
        .HEADLINE {padding-bottom:0.5em;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+6"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=3">
        .creative td {font-size:<xsl:value-of select="$base-font-size+5"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=4">
        .creative td {font-size:<xsl:value-of select="$base-font-size+3"/>px;}
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="B300x250">
    .creative td {vertical-align:middle; padding:0.3em <xsl:value-of select="$cell-side-padding"/>px;}
    <xsl:call-template name="vertical-sizes"/>
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+10"/>px;}
        .HEADLINE {padding-bottom:0.5em;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+6"/>px;}
      </xsl:when>
      <xsl:when test="$creative-count=3">
        .creative td {font-size:<xsl:value-of select="$base-font-size+2"/>px;}
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="horizontal-sizes">
    <xsl:if test="$creative-count>1">
      .creative {width:<xsl:value-of select="$creative-size"/>px;}
      .creative.last {width:<xsl:value-of select="$last-creative-size"/>px;}
    </xsl:if>
  </xsl:template>
  <xsl:template name="vertical-sizes">
    <xsl:if test="$creative-count>1">
      .creative {width:100%; height:<xsl:value-of select="$creative-size"/>px;}
      .creative.last {height:<xsl:value-of select="$last-creative-size"/>px;}
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>

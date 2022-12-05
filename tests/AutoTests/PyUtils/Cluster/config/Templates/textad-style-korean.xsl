<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:template name="B580x150">
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+8"/>px;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+4"/>px;}
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="korean-style"/>
  </xsl:template>
  <xsl:template name="B250x250">
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+8"/>px;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+4"/>px;}
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="korean-style"/>
  </xsl:template>
  <xsl:template name="B560x200">
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+10"/>px;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+6"/>px;}
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="korean-style"/>
  </xsl:template>
  <xsl:template name="B545x150">
    <xsl:choose>
      <xsl:when test="$creative-count=1">
        .creative td {text-align:center; font-size:<xsl:value-of select="$base-font-size+8"/>px;}
        .DISPLAY_URL {padding-top:1em;}
      </xsl:when>
      <xsl:when test="$creative-count=2">
        .creative td {font-size:<xsl:value-of select="$base-font-size+4"/>px;}
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="korean-style"/>
  </xsl:template>
  <xsl:template name="korean-style">
    <xsl:if test="$creative-count>1">
      .creative {height:<xsl:value-of select="$creative-size"/>px;}
      .creative.last {height:<xsl:value-of select="$last-creative-size"/>px;}
    </xsl:if>
    .HEADLINE {line-height:1.5em;}
    .creative td {font-family:serif; vertical-align:middle;}
  </xsl:template>
</xsl:stylesheet>

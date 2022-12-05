<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet >

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:data="urn:data"
  xmlns="http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
  extension-element-prefixes="data"
>

<xsl:output method="xml" encoding="UTF-8" indent="no"/>

<xsl:param name="max-favorites" select="7"/>
<xsl:variable name="id" select="'yandex-bar-services'"/>

<xsl:template match="data:services">
  <toolbarbutton type="menu-button"
                 statustext="http://www{$domain}"
                 oncommand="if (event.target == this) Ya.loadURI('http://www{$domain}', event, null, 4);"
                 label="label">
    <xsl:apply-templates select="@id"/>
    <menupopup>
      <xsl:apply-templates select="." mode="favorites"/>
      <xsl:apply-templates select="data:group[*]"/>
    </menupopup>
  </toolbarbutton>
</xsl:template>

<xsl:template match="data:services" mode="favorites">
  <xsl:variable name="services" select=".//data:service[@service-timestamp != 0]"/>

  <xsl:if test="count($services) &gt; 0">
    <xsl:for-each select="$services">
      <xsl:sort select="@service-timestamp" order="descending" data-type="number"/>
      <xsl:if test="position() &lt;= $max-favorites">
        <xsl:apply-templates select="." mode="top"/>
      </xsl:if>
    </xsl:for-each>
    <menuseparator />
  </xsl:if>
</xsl:template>

<xsl:template match="data:services/data:group">
  <menu>
    <xsl:apply-templates select="@*"/>
    <menupopup>
      <xsl:apply-templates select="data:service | data:complex"/>
    </menupopup>
  </menu>
</xsl:template>

<xsl:template match="data:service" mode="url">
  <xsl:attribute name="statustext">
    <xsl:choose>
      <xsl:when test="@host">
        <xsl:value-of select="@host"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>http://</xsl:text>
        <xsl:value-of select="concat(@id, $domain)"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>?yasoft=barff</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="oncommand">
    <xsl:text>Ya.loadService('</xsl:text>
    <xsl:value-of select="@id"/>
    <xsl:text>',this.getAttribute('statustext'),event);</xsl:text>
  </xsl:attribute>
</xsl:template>

<xsl:template match="data:service">
  <menuitem>
    <xsl:apply-templates select="@*"/>
    <xsl:apply-templates select="." mode="url"/>
  </menuitem>
</xsl:template>

<xsl:template match="data:service" mode="top">
  <menuitem>
    <xsl:apply-templates select="@*" mode="top"/>
    <xsl:apply-templates select="." mode="url"/>
  </menuitem>
</xsl:template>

<xsl:template match="@*" mode="top">
  <xsl:apply-templates select="."/>
</xsl:template>

<xsl:template match="data:service[parent::data:complex[@image != 'null']]/@image" mode="top">
  <xsl:attribute name="image">
    <xsl:value-of select="concat($images, parent::data:service/parent::data:complex/@image, '.png')"/>
  </xsl:attribute>
  <xsl:attribute name="class">
    <xsl:text>menuitem-iconic</xsl:text>
  </xsl:attribute>
</xsl:template>

<xsl:template match="data:service[parent::data:complex]/@label" mode="top">
  <xsl:attribute name="label">
    <xsl:value-of select="parent::data:service/parent::data:complex/@label"/>
    <xsl:text> / </xsl:text>
    <xsl:value-of select="."/>
  </xsl:attribute>
</xsl:template>

<xsl:template match="data:services/@image">
  <xsl:attribute name="class">
    <xsl:text>toolbarbutton-icon</xsl:text>
  </xsl:attribute>
  <xsl:apply-templates select="." mode="image"/>
</xsl:template>

</xsl:stylesheet>

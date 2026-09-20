<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn cfg colo">
  <xsl:include href="../Functions.xsl"/>
  <xsl:param name="XPATH"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <xsl:variable name="cluster" select="dyn:evaluate($XPATH)"/>
    <xsl:if test="count($cluster) != 1">
      <xsl:message terminate="yes">
        Expected exactly one AdCluster for frontend networking.
      </xsl:message>
    </xsl:if>
    <xsl:variable name="config" select="$cluster/configuration/cfg:cluster"/>
    <network>
      <xsl:if test="$config/cfg:frontendNetwork">
        <xsl:attribute name="enabled">true</xsl:attribute>
        <xsl:copy-of select="$config/cfg:frontendNetwork"/>
        <inventory><xsl:copy-of select="/colo:colocation/host"/></inventory>
        <frontends>
          <xsl:for-each select="$cluster/serviceGroup[@descriptor = $fe-cluster-descriptor]
            /service[@descriptor = $http-frontend-descriptor]">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
            </xsl:call-template>
          </xsl:for-each>
        </frontends>
        <xsl:for-each select="$config/cfg:coloParams/*[
          (self::cfg:virtualServer or self::cfg:secureVirtualServer) and
          (not(@enable) or @enable = 'true' or @enable = '1')]">
          <endpoint address="{@externalAddress}">
            <xsl:attribute name="port">
              <xsl:choose>
                <xsl:when test="@port"><xsl:value-of select="@port"/></xsl:when>
                <xsl:when test="self::cfg:virtualServer">80</xsl:when>
                <xsl:otherwise>443</xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
            <xsl:attribute name="internal_port">
              <xsl:choose>
                <xsl:when test="@internal_port"><xsl:value-of select="@internal_port"/></xsl:when>
                <xsl:when test="self::cfg:virtualServer">
                  <xsl:value-of select="$def-frontend-port"/>
                </xsl:when>
                <xsl:otherwise><xsl:value-of select="$def-secure-frontend-port"/></xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </endpoint>
        </xsl:for-each>
      </xsl:if>
    </network>
  </xsl:template>
</xsl:stylesheet>

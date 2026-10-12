<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  exclude-result-prefixes="exsl dyn colo cfg">

<xsl:output method="text" encoding="utf-8"/>
<xsl:include href="Functions.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="DbAccessServices">
  <xsl:param name="services"/>
  <xsl:param name="name"/>
  <xsl:param name="default-port"/>
  <xsl:for-each select="$services">
    <xsl:variable name="network" select="configuration/*/cfg:networkParams"/>
    <xsl:variable name="port">
      <xsl:choose>
        <xsl:when test="$network/@port"><xsl:value-of select="$network/@port"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$default-port"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="grpc-port">
      <xsl:call-template name="DbAccessGrpcPort">
        <xsl:with-param name="network" select="$network"/>
        <xsl:with-param name="port" select="$port"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'DBAccess: hosts resolving'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)//host[. = $HOST]">
      <service name="{$name}" endpoint="{concat(., ':', $grpc-port)}"/>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template>

<xsl:template name="DbAccessJsonString">
  <xsl:param name="value"/>
  <xsl:if test="string-length($value) > 0">
    <xsl:variable name="char" select="substring($value, 1, 1)"/>
    <xsl:choose>
      <xsl:when test="$char = '&quot;'"><xsl:text>\"</xsl:text></xsl:when>
      <xsl:when test="$char = '\'"><xsl:text>\\</xsl:text></xsl:when>
      <xsl:when test="$char = '&#10;'"><xsl:text>\n</xsl:text></xsl:when>
      <xsl:when test="$char = '&#13;'"><xsl:text>\r</xsl:text></xsl:when>
      <xsl:when test="$char = '&#9;'"><xsl:text>\t</xsl:text></xsl:when>
      <xsl:otherwise><xsl:value-of select="$char"/></xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="DbAccessJsonString">
      <xsl:with-param name="value" select="substring($value, 2)"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template match="/">
  <xsl:if test="count($xpath) != 1">
    <xsl:message terminate="yes">DBAccess: expected one cluster</xsl:message>
  </xsl:if>
  <xsl:variable name="services">
    <xsl:call-template name="DbAccessServices">
      <xsl:with-param name="services"
        select="$xpath/serviceGroup[@descriptor = $fe-cluster-descriptor]/service[
          @descriptor = $channel-server-descriptor]"/>
      <xsl:with-param name="name" select="'ChannelServer'"/>
      <xsl:with-param name="default-port" select="$def-channel-server-port"/>
    </xsl:call-template>
    <xsl:call-template name="DbAccessServices">
      <xsl:with-param name="services"
        select="$xpath/serviceGroup[@descriptor = $be-cluster-descriptor]/service[
          @descriptor = $campaign-server-descriptor]"/>
      <xsl:with-param name="name" select="'CampaignServer'"/>
      <xsl:with-param name="default-port" select="$def-campaign-server-port"/>
    </xsl:call-template>
    <xsl:call-template name="DbAccessServices">
      <xsl:with-param name="services"
        select="$xpath/serviceGroup[@descriptor = $be-cluster-descriptor]/service[
          @descriptor = $log-generalizer-descriptor]"/>
      <xsl:with-param name="name" select="'LogGeneralizer'"/>
      <xsl:with-param name="default-port" select="$def-log-generalizer-port"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:text>{"services": [</xsl:text>
  <xsl:for-each select="exsl:node-set($services)/service">
    <xsl:if test="position() > 1"><xsl:text>,</xsl:text></xsl:if>
    <xsl:text>&#10;  {"name": "</xsl:text>
    <xsl:call-template name="DbAccessJsonString">
      <xsl:with-param name="value" select="@name"/>
    </xsl:call-template>
    <xsl:text>", "endpoint": "</xsl:text>
    <xsl:call-template name="DbAccessJsonString">
      <xsl:with-param name="value" select="@endpoint"/>
    </xsl:call-template>
    <xsl:text>"}</xsl:text>
  </xsl:for-each>
  <xsl:text>&#10;]}&#10;</xsl:text>
</xsl:template>
</xsl:stylesheet>

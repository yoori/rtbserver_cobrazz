<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="Functions.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- ADSC-2930. Duplicate service's host validation  -->
<xsl:template name="ServicesHostsValidation">
  <xsl:param name="root-path"/>

  <xsl:for-each select="$root-path//service">
    <xsl:if test="generate-id(key('descriptor', @descriptor)[1])=generate-id(@descriptor)">
      <xsl:variable name="descriptor" select="@descriptor"/>
      <xsl:variable name="service-hosts">
        <xsl:for-each select="$root-path//service[@descriptor = $descriptor]">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix"  select="concat('Can not get hosts for service ', $descriptor)"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($service-hosts)//host">
        <xsl:variable name="host" select="."/>
        <xsl:if test="generate-id(key('host', .)[1])=generate-id(.)">
          <xsl:if test="count(exsl:node-set($service-hosts)//host[.=$host]) > 1">
             <xsl:message terminate="yes">Duplicate host '<xsl:value-of select="."/>' for service '<xsl:value-of select="$descriptor"/>'</xsl:message>
          </xsl:if>
        </xsl:if>
      </xsl:for-each>
    </xsl:if>
  </xsl:for-each>

</xsl:template>

<xsl:template match="/">
  <xsl:variable
    name="full-cluster-path"
    select="$xpath"/>

  <xsl:variable
    name="backend-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="frontend-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find XPATH element: $XPATH </xsl:message>
    </xsl:when>
    <xsl:when test="count($backend-cluster-path) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find backend path</xsl:message>
    </xsl:when>
    <xsl:when test="count($frontend-cluster-path) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find frontend path</xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="frontends"
    select="$frontend-cluster-path/service[@descriptor = $http-frontend-descriptor]"/>

  <xsl:variable
    name="campaign-managers"
    select="$frontend-cluster-path/service[@descriptor = $campaign-manager-descriptor]"/>

  <xsl:variable
    name="log-generalizer"
    select="$backend-cluster-path/service[@descriptor = $log-generalizer-descriptor]"/>

  <xsl:variable
    name="request-info-manager"
    select="$backend-cluster-path/service[@descriptor = $request-info-manager-descriptor]"/>

  <xsl:variable
    name="expression-matcher"
    select="$backend-cluster-path/service[@descriptor = $expression-matcher-descriptor]"/>

  <xsl:choose>
    <xsl:when test="(count($frontends) > 0) and count($campaign-managers) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find campaign managers</xsl:message>
    </xsl:when>
    <xsl:when test="count($log-generalizer) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find log generalizer</xsl:message>
    </xsl:when>
    <xsl:when test="count($request-info-manager) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find request info manager</xsl:message>
    </xsl:when>
    <xsl:when test="count($expression-matcher) = 0">
       <xsl:message terminate="yes"> ErrorsCheck: Can't find expression matcher</xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="ServicesHostsValidation">
    <xsl:with-param name="root-path" select="$xpath"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>

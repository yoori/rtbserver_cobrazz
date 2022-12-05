<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="protocol" select="$PROTOCOL"/>

<!-- virtual_servers config generate function -->
<xsl:template name="PageSenseConfigGenerator">
  <xsl:param name="colo-config"/>
  <xsl:param name="frontend-config"/>

  <xsl:variable name="request-domain-postfix">
    <xsl:choose>
      <xsl:when test="$protocol = 'http'"><xsl:if
        test="count($colo-config/cfg:coloParams/@external_http_port) > 0 and
        $colo-config/cfg:coloParams/@external_http_port != 80">:<xsl:value-of
        select="$colo-config/cfg:coloParams/@external_http_port"/></xsl:if>
      </xsl:when>
      <xsl:otherwise><xsl:if
        test="count($colo-config/cfg:coloParams/cfg:httpsParams/@port) > 0 and
        $colo-config/cfg:coloParams/cfg:httpsParams/@port != 443">:<xsl:value-of
        select="$colo-config/cfg:coloParams/cfg:httpsParams/@port"/></xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="creative-domain-postfix">
    <xsl:choose>
      <xsl:when test="$protocol = 'http'"><xsl:if
        test="count($colo-config/cfg:coloParams/@creative_http_port) > 0 and
        $colo-config/cfg:coloParams/@creative_http_port != 80">:<xsl:value-of
        select="$colo-config/cfg:coloParams/@creative_http_port"/></xsl:if>
      </xsl:when>
      <xsl:otherwise><xsl:if
        test="count($colo-config/cfg:coloParams/cfg:httpsParams/@creative_port) > 0 and
        $colo-config/cfg:coloParams/cfg:httpsParams/@creative_port != 443">:<xsl:value-of
        select="$colo-config/cfg:coloParams/cfg:httpsParams/@creative_port"/></xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="request-domain"><xsl:value-of
    select="$colo-config/cfg:coloParams/cfg:domain[1]/@name"/><xsl:value-of
    select="$request-domain-postfix"/></xsl:variable>

  <xsl:variable name="creative-domain"><xsl:value-of
    select="$colo-config/cfg:coloParams/cfg:creative_domain[1]/@name"/><xsl:value-of
    select="$creative-domain-postfix"/></xsl:variable>

  <xsl:variable name="pagesense-module" select="$frontend-config/cfg:pageSenseModule"/>

  <xsl:variable name="pagesense-domain"><xsl:value-of
    select="$request-domain"/></xsl:variable>

  <xsl:choose>
    <xsl:when test="count($pagesense-module) > 0">
      <xsl:choose>
        <xsl:when test="$protocol = 'http'">
          %CONFIG = (%CONFIG,
            'adsrv_host_name' => 'http://<xsl:value-of select="$request-domain"/>',
            'ps_host' => 'http://<xsl:value-of select="$pagesense-domain"/>',
            'adimage_server' => 'http://<xsl:value-of select="$creative-domain"/>',
          );
        </xsl:when>
        <xsl:otherwise>
          %CONFIG = (%CONFIG,
            'adsrv_host_name' => 'https://<xsl:value-of select="$request-domain"/>',
            'ps_host' => 'https://<xsl:value-of select="$pagesense-domain"/>',
            'adimage_server' => 'https://<xsl:value-of select="$creative-domain"/>',
          );
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>

    1;

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="full-cluster-path" select="$xpath/../.. | $xpath/.."/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
      <xsl:message terminate="yes"> PageSenseConfig: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
      <xsl:message terminate="yes"> PageSenseConfig: Can't find full cluster group </xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->

  <xsl:variable
    name="frontend-config" select="$xpath/configuration/cfg:frontend"/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
      <xsl:message terminate="yes"> PageSenseConfig: Can't find full cluster config </xsl:message>
    </xsl:when>
    <xsl:when test="count($frontend-config) = 0">
      <xsl:message terminate="yes"> PageSenseConfig: Can't find frontend config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="PageSenseConfigGenerator">
    <xsl:with-param name="colo-config" select="$colo-config"/>
    <xsl:with-param name="frontend-config" select="$frontend-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="RequestInfoVariables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- RIChunksConfig generate function -->
<xsl:template name="RequestInfoChunksConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="colocation-path"/>

  <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
    <xsl:if test="count($env-config/@cache_root[1]) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="user-action-chunks-root"
    select="concat($cache-root, '/', $user-action-dir-name, '/')"/>
  <xsl:variable name="user-campaign-reach-chunks-root"
    select="concat($cache-root, '/', $user-campaign-reach-dir-name, '/')"/>
  <xsl:variable name="request-chunks-root"
    select="concat($cache-root, '/', $request-dir-name, '/')"/>

  <xsl:variable
    name="colocation-hosts"
    select="$colocation-path/host"/>

  @colocation_hosts = (
    <xsl:for-each select="$colocation-hosts">'<xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="@name"/>
      <xsl:with-param name="error-prefix" select="'RequestInfoChunksConfigurator'"/>
    </xsl:call-template>', </xsl:for-each>
  );

  %ri_chunks = (
    host =>
      <xsl:for-each select="$be-cluster-path/service[@descriptor = $request-info-manager-descriptor]">
        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
             <xsl:with-param name="hosts" select="@host"/>
             <xsl:with-param name="error-prefix"
                select="'RequestInfoChunksConfigurator'"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:for-each select="exsl:node-set($hosts)//host">
        [ '<xsl:value-of select="."/>', ],
        </xsl:for-each>
      </xsl:for-each>
    chunks => {
                'user_action_chunks' => [
                  '<xsl:value-of select="$user-action-prefix"/>',
                  '<xsl:value-of select="$user-action-chunks-root"/>', ],
                'user_campaign_reach_chunks' => [
                  '<xsl:value-of select="$user-campaign-reach-prefix"/>',
                  '<xsl:value-of select="$user-campaign-reach-chunks-root"/>', ],
                'request_chunks' => [
                  '<xsl:value-of select="$request-prefix"/>',
                  '<xsl:value-of select="$request-chunks-root"/>', ],
              },
  );

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="be-cluster-path" select="$xpath"/>
  <xsl:variable name="full-cluster-path" select="$xpath/.."/>
  <xsl:variable name="colocation-path" select="$xpath/../../.."/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($be-cluster-path) = 0">
      <xsl:message terminate="yes"> RequestInfoChunksConfigurator: Can't find be-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:call-template name="RequestInfoChunksConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
    <xsl:with-param name="colocation-path" select="$colocation-path"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

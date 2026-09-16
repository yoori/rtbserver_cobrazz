<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:exsl="http://exslt.org/common"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl cfg colo">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="queue">
  <xsl:param name="service"/>
  <xsl:param name="name"/>
  <xsl:param name="workspace-root"/>
  {
    "service": "<xsl:value-of select="$service"/>",
    "name": "<xsl:value-of select="$name"/>",
    "path": "<xsl:value-of
      select="concat($workspace-root, '/log/', $service, '/In/', $name)"/>"
  }
</xsl:template>

<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath"/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of
      select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of
      select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of
      select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="expression-matcher-hosts">
    <xsl:for-each select="$cluster-path//service[
        @descriptor = $expression-matcher-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="request-info-manager-hosts">
    <xsl:for-each select="$cluster-path//service[
        @descriptor = $request-info-manager-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="expression-matcher-on-host"
    select="count(exsl:node-set($expression-matcher-hosts)/host[. = $HOST]) &gt; 0"/>
  <xsl:variable name="request-info-manager-on-host"
    select="count(exsl:node-set($request-info-manager-hosts)/host[. = $HOST]) &gt; 0"/>

{
  "measurement": "rtb_queue",
  "period_seconds": 60,
  "queues": [
    <xsl:if test="$expression-matcher-on-host">
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'ExpressionMatcher'"/>
        <xsl:with-param name="name" select="'RequestBasicChannels'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'ExpressionMatcher'"/>
        <xsl:with-param name="name" select="'Request'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'ExpressionMatcher'"/>
        <xsl:with-param name="name" select="'Impression'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'ExpressionMatcher'"/>
        <xsl:with-param name="name" select="'Click'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'ExpressionMatcher'"/>
        <xsl:with-param name="name" select="'TagRequest'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="$expression-matcher-on-host and $request-info-manager-on-host">,</xsl:if>
    <xsl:if test="$request-info-manager-on-host">
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'AdvertiserAction'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'Click'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'Impression'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'PassbackImpression'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'PostClickAction'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'Request'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'RequestOperation'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>,
      <xsl:call-template name="queue">
        <xsl:with-param name="service" select="'RequestInfoManager'"/>
        <xsl:with-param name="name" select="'TagRequest'"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>
    </xsl:if>
  ],
  "process_roots": ["<xsl:value-of select="$server-root"/>"],
  "instance_roots": ["<xsl:value-of select="concat($config-root, '/', $OUT_DIR)"/>"],
  "excluded_commands": ["StatCollector", "telegraf"],
  "processes": []
}
</xsl:template>

</xsl:stylesheet>

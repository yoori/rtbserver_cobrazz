<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="xml" indent="yes" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:param name="COLOCATION_NAME"/>
  <xsl:param name="VERSION"/>
  <xsl:param name="RELEASE"/>
  <xsl:variable name="application" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="cluster" select="$application/serviceGroup[@descriptor = 'AICluster']"/>
  <xsl:variable name="app-env" select="$application/configuration/cfg:environment"/>
  <xsl:variable name="cluster-env" select="$cluster/configuration/cfg:cluster/cfg:environment"/>

  <xsl:template match="/">
    <xsl:variable name="user"><xsl:choose>
      <xsl:when test="$app-env/cfg:forosZoneManagement/@user_name"><xsl:value-of select="$app-env/cfg:forosZoneManagement/@user_name"/></xsl:when>
      <xsl:otherwise>aduser</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="ssh-key"><xsl:choose>
      <xsl:when test="$app-env/cfg:forosZoneManagement/@ssh_key"><xsl:value-of select="$app-env/cfg:forosZoneManagement/@ssh_key"/></xsl:when>
      <xsl:otherwise>/home/<xsl:value-of select="$user"/>/.ssh/adkey</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="config-root"><xsl:choose>
      <xsl:when test="$cluster-env/@config_root"><xsl:value-of select="$cluster-env/@config_root"/></xsl:when>
      <xsl:otherwise>/opt/foros/server-ai/etc</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="server-root"><xsl:choose>
      <xsl:when test="$cluster-env/@server_root"><xsl:value-of select="$cluster-env/@server_root"/></xsl:when>
      <xsl:otherwise>/opt/foros/server-ai</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="server-bin-root"><xsl:choose>
      <xsl:when test="$cluster-env/@server_bin_root"><xsl:value-of select="$cluster-env/@server_bin_root"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$server-root"/>/bin</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <AIServer sms-version="3"
              init="%%SUDO%%"
              env="aicluster"
              brief="AI services {$VERSION}-{$RELEASE} configuration for {$COLOCATION_NAME}"
              user="{$user}">
      <xsl:attribute name="transport">ssh -o "BatchMode yes" -i <xsl:value-of select="$ssh-key"/> %%HOST%% ". <xsl:value-of select="$config-root"/>/<xsl:value-of select="$COLOCATION_NAME"/>/aicluster/environment.sh &amp;&amp; $SMS_TEXT"</xsl:attribute>
      <Services start="%%START_LOOP_PRINT%%"
                stop="%%STOP_LOOP_PRINT%%"
                status="%%STATUS_PRINT%%"
                timeout="600"
                start_cmd="%%cmd%% start"
                stop_cmd="%%cmd%% stop"
                status_cmd="%%cmd%% is_alive"
                env="aicluster">
        <xsl:attribute name="cmd"><xsl:value-of select="$server-bin-root"/>/aiserver.pl %%HOST%% %%TYPE%%</xsl:attribute>
        <xsl:for-each select="$cluster/service[@descriptor = 'AICluster/AIAgent']">
          <Service name="ai-AIAgent" type="AICluster::AIAgent" host="{@host}"/>
        </xsl:for-each>
        <xsl:for-each select="$cluster/service[@descriptor = 'AICluster/SegmentGenerator']">
          <Service name="ai-SegmentGenerator" type="AICluster::SegmentGenerator" host="{@host}"/>
        </xsl:for-each>
      </Services>
    </AIServer>
  </xsl:template>
</xsl:stylesheet>

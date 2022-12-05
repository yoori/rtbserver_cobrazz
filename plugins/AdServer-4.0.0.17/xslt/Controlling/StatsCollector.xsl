<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="exsl dyn">

<xsl:include href="../Functions.xsl"/>

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- ChannelController config generate function -->
<xsl:template name="StatsCollectorConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="be-config"/>
  <xsl:param name="service-path"/>
  <xsl:param name="server-root"/>

  <xsl:variable
    name="w-search"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$w-search/@workspace_root[1]"/>
    <xsl:if test="count($w-search/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable
    name="service-config"
    select="$service-path/configuration/cfg:statsCollector"/>

  <xsl:variable name="service-port">
    <xsl:value-of select="$service-config/cfg:networkParams/@port"/>
    <xsl:if test="count($service-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-stats-collector-port"/>
    </xsl:if>
  </xsl:variable>
    <exsl:document href="statsCollector.port"
      method="text" omit-xml-declaration="yes"
      >  ['statsCollector', <xsl:copy-of select="$service-port"/>],</exsl:document>

  <xsl:variable name="snmp-stats-enabled">
    <xsl:if test="count($colo-config/cfg:snmpStats) > 0">
      <xsl:value-of select="string($colo-config/cfg:snmpStats/@enable)"/>
    </xsl:if>
  </xsl:variable>

  <cfg:StatsCollectorConfig>
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$service-config/cfg:threadParams/@min"/>
        <xsl:if test="count($service-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-stats-collector-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$service-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="StatsCollector" name="StatsCollector"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>
    <xsl:if test="$snmp-stats-enabled = 'true'">
      <xsl:variable name="mib-root"><xsl:if
        test="count($env-config/@mib_root) = 0"><xsl:value-of
        select="$server-root"/>/mibs</xsl:if><xsl:value-of
        select="$env-config/@mib_root"/></xsl:variable>
      <xsl:variable name="mib-path" select="concat('/usr/share/snmp/mibs:',
        $mib-root, ':', $mib-root, '/Controlling')"/>
      <cfg:SNMPConfig mib_dirs="{$mib-path}"
        index="{$colo-config/cfg:snmpStats/@oid_suffix}"/>
    </xsl:if>
    <cfg:Rules>
      <cfg:Rule prefix="historyMatch" type="Measurable"/>
      <cfg:Rule prefix="usersMerge" type="Measurable"/>
      <cfg:Rule prefix="triggersMatch" type="Measurable"/>
      <cfg:Rule prefix="requestParsing" type="Measurable"/>
      <cfg:Rule prefix="creativeSelection" type="Measurable"/>
      <cfg:Rule prefix="creativeSelectionLocal" type="Measurable"/>
      <cfg:Rule name="adRequest-Count" prefix="adRequest-Count" type="Countable"/>
      <cfg:Rule name="adRequestWithImpression-OptInUser-Count" prefix="adRequestWithImpression-OptInUser-Count" type="Countable"/>
      <cfg:Rule name="adRequestWithImpression-nonOptInUser-Count" prefix="adRequestWithImpression-nonOptInUser-Count" type="Countable"/>
      <cfg:Rule name="adRequestWithPassback-OptInUser-Count" prefix="adRequestWithPassback-OptInUser-Count" type="Countable"/>
      <cfg:Rule name="adRequestWithPassback-nonOptInUser-Count" prefix="adRequestWithPassback-nonOptInUser-Count" type="Countable"/>
      <cfg:Rule name="campaignSelect-OptOutUser-Count" prefix="campaignSelect-OptOutUser-Count" type="Countable"/>
      <cfg:Rule name="campaignSelect-OptInUser-Count" prefix="campaignSelect-OptInUser-Count" type="Countable"/>
      <cfg:Rule name="campaignSelect-ProbeUser-Count" prefix="campaignSelect-ProbeUser-Count" type="Countable"/>
      <cfg:Rule name="campaignSelect-TemporaryUser-Count" prefix="campaignSelect-TemporaryUser-Count" type="Countable"/>
      <cfg:Rule name="campaignSelect-UndefinedUser-Count" prefix="campaignSelect-UndefinedUser-Count" type="Countable"/>
      <cfg:Rule name="passbackRequest-Count" prefix="passbackRequest-Count" type="Countable"/>
      <cfg:Rule name="profilingRequest-ReceivedTriggers-Count" prefix="profilingRequest-ReceivedTriggers-Count" type="Countable"/>
      <cfg:Rule name="profilingRequest-MatchedTriggers-Count" prefix="profilingRequest-MatchedTriggers-Count" type="Countable"/>
      <cfg:Rule name="profilingRequest-TriggeredChannels-Count" prefix="profilingRequest-TriggeredChannels-Count" type="Countable"/>
      <cfg:Rule name="profilingRequest-Count" prefix="profilingRequest-Count" type="Countable"/>
      <cfg:Rule name="ooRequest-InOp-Count" prefix="ooRequest-InOp-Count" type="Countable"/>
      <cfg:Rule name="ooRequest-OutOp-Count" prefix="ooRequest-OutOp-Count" type="Countable"/>
      <cfg:Rule name="ooRequest-StatusOp-Count" prefix="ooRequest-StatusOp-Count" type="Countable"/>
      <cfg:Rule name="actionRequest-OptInUser-Count" prefix="actionRequest-OptInUser-Count" type="Countable"/>
      <cfg:Rule name="actionRequest-nonOptInUser-Count" prefix="actionRequest-nonOptInUser-Count" type="Countable"/>
      <cfg:Rule name="userProfiles" prefix="userProfiles" type="Countable" variable1="Set"/>
      <cfg:Rule name="dailyUsers-Count" prefix="DailyUsers" type="Countable" variable1="Set"/>
      <cfg:Rule name="userBaseProfiles-AreaSize" prefix="userBaseProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userTempProfiles-AreaSize" prefix="userTempProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userAdditionalProfiles-AreaSize" prefix="userAdditionalProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userHistoryProfiles-AreaSize" prefix="userHistoryProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userFreqCapProfiles-AreaSize" prefix="userFreqCapProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userWDProfiles-AreaSize" prefix="userWDProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userProfiles-AreaSize" prefix="userProfiles-AreaSize" type="Countable" variable1="Set"/>
      <cfg:Rule name="userProfiles-AdChannels" prefix="userProfiles-AdChannels" type="Countable" variable1="Set"/>
      <cfg:Rule name="userProfiles-DiscoverChannels" prefix="userProfiles-DiscoverChannels" type="Countable" variable1="Set"/>
      <cfg:Rule name="userProfiles-AdChannels-Avg" prefix="userProfiles-AdChannels"
        variable1="userProfiles-AdChannels" variable2="userProfiles" type="Average"/>
      <cfg:Rule name="userProfiles-DiscoverChannels-Avg" prefix="userProfiles-DiscoverChannels"
        variable1="userProfiles-DiscoverChannels" variable2="userProfiles" type="Average"/>

      <cfg:Rule name="rtbRequestCount" prefix="rtbRequestCount" type="Countable"/>
      <cfg:Rule name="rtbRequestTimeCounter" prefix="rtbRequestTimeCounter" type="Countable"/>

      <cfg:Rule name="rtbRequestBidCount" prefix="rtbRequestBidCount" type="Countable"/>
      <cfg:Rule name="rtbRequestTanxCount" prefix="rtbRequestTanxCount" type="Countable"/>
      <cfg:Rule name="rtbRequestTanxBidCount" prefix="rtbRequestTanxBidCount" type="Countable"/>
      <cfg:Rule name="rtbRequestBaiduCount" prefix="rtbRequestBaiduCount" type="Countable"/>
      <cfg:Rule name="rtbRequestBaiduBidCount" prefix="rtbRequestBaiduBidCount" type="Countable"/>
      <cfg:Rule name="rtbRequestOpenRTBCount" prefix="rtbRequestOpenRTBCount" type="Countable"/>
      <cfg:Rule name="rtbRequestOpenRTBBidCount" prefix="rtbRequestOpenRTBBidCount" type="Countable"/>
      <cfg:Rule name="rtbRequestOtherCount" prefix="rtbRequestOtherCount" type="Countable"/>
      <cfg:Rule name="rtbRequestOtherBidCount" prefix="rtbRequestOtherBidCount" type="Countable"/>

      <cfg:Rule name="rtbRequestTimeoutCount" prefix="rtbRequestTimeoutCount" type="Countable"/>
      <cfg:Rule name="rtbRequestSkipCount" prefix="rtbRequestSkipCount" type="Countable"/>
    </cfg:Rules>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$service-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $stats-collector-log-path)"/>
      <xsl:with-param name="default-log-level" select="$stats-collector-log-level"/>
    </xsl:call-template>

  </cfg:StatsCollectorConfig>
</xsl:template>

<xsl:template match="/">

  <xsl:variable name="stat-service-path" select="$xpath"/>

  <xsl:variable name="be-cluster-path" select="$xpath/.."/>

  <xsl:variable name="full-cluster-path" select="$xpath/../.. | $xpath/.."/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:choose>
    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> StatsCollector: Can't find be-cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> StatsCollector: Can't find full-cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> StatsCollector: Can't find colo config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="server-install-root"
    select="$colo-config/cfg:environment/@server_root"/>

  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Controlling/StatsCollectorConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="StatsCollectorConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="be-config" select="$be-config"/>
      <xsl:with-param name="service-path" select="$stat-service-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>
</xsl:template>

</xsl:stylesheet>

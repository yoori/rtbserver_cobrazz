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

<xsl:template name="OutputLogsConfigs">
  <xsl:param name="list"/>
  <xsl:param name="upload_type"/>
  <xsl:param name="upload-tasks"/>
  <xsl:param name="deferred-period"/>
  <xsl:param name="check-logs-period"/>
  <xsl:param name="flush-logs-period"/>
  <xsl:param name="flush-logs-size"/>
  <xsl:param name="stat-node"/>
  <xsl:param name="file-prefix"/>
  <xsl:param name="filtering-count"/>
  <xsl:param name="distrib_count"/>

  <xsl:variable name="newlist" select="concat(normalize-space($list), ' ')"/>
  <xsl:variable name="head" select="substring-before($newlist, ' ')"/>
  <xsl:variable name="tail" select="substring-after($newlist, ' ')"/>

  <xsl:variable name="stat-path" select="$stat-node/*[local-name() = $head]"/>
  <xsl:element name="cfg:{$head}">
    <xsl:if test="string-length($upload_type) > 0">
      <xsl:attribute name="upload_type"><xsl:value-of select="$upload_type"/></xsl:attribute>
    </xsl:if>
    <xsl:attribute name="check_logs_period">
      <xsl:choose>
        <xsl:when test="count($stat-path/@file_check_period) = 0">
          <xsl:value-of select="$check-logs-period"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$stat-path/@file_check_period"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:if test="string-length($deferred-period) > 0">
      <xsl:attribute name="check_deferred_logs_period">
        <xsl:value-of select="$deferred-period"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:attribute name="max_time">
      <xsl:choose>
        <xsl:when test="count($stat-path/@flush_period) = 0">
          <xsl:value-of select="$flush-logs-period"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$stat-path/@flush_period"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="max_size">
      <xsl:choose>
        <xsl:when test="count($stat-path/@flush_size) = 0">
          <xsl:value-of select="$flush-logs-size"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$stat-path/@flush_size"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="max_upload_task_count">
      <xsl:choose>
        <xsl:when test="string-length($upload-tasks) > 0">
          <xsl:value-of select="$upload-tasks"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$log-generalizer-upload-tasks"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="backup_mode"><xsl:value-of select="'false'"/></xsl:attribute>
    <xsl:if test="string-length($distrib_count) > 0">
      <xsl:attribute name="distrib_count"><xsl:value-of select="$distrib_count"/></xsl:attribute>
    </xsl:if>
    <xsl:if test="string-length($filtering-count) > 0">
      <cfg:HitsFiltering table_size="10485760" days_to_keep="7">
        <xsl:attribute name="min_count"><xsl:value-of select="$filtering-count"/></xsl:attribute>
        <xsl:attribute name="file_prefix"><xsl:value-of select="$file-prefix"/></xsl:attribute>
      </cfg:HitsFiltering>
    </xsl:if>
  </xsl:element>

  <xsl:if test="$tail">
    <xsl:call-template name="OutputLogsConfigs">
      <xsl:with-param name="list" select="$tail"/>
      <xsl:with-param name="upload_type" select="$upload_type"/>
      <xsl:with-param name="upload-tasks" select="$upload-tasks"/>
      <xsl:with-param name="deferred-period" select="$deferred-period"/>
      <xsl:with-param name="check-logs-period" select="$check-logs-period"/>
      <xsl:with-param name="flush-logs-period" select="$flush-logs-period"/>
      <xsl:with-param name="flush-logs-size" select="$flush-logs-size"/>
      <xsl:with-param name="stat-node" select="$stat-node"/>
      <xsl:with-param name="distrib_count" select="$distrib_count"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="ConvertLogGeneralizerDBConnection">
  <xsl:param name="pg-connection"/>

  <cfg:DBConnection>
    <cfg:Postgres>
      <xsl:attribute name="connection_string"><xsl:value-of select="$pg-connection/@connection_string"/></xsl:attribute>
    </cfg:Postgres>
  </cfg:DBConnection>
</xsl:template>

<!-- LogGeneralizer config generate function -->
<xsl:template name="LogGeneralizerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="central-colo-config"/>
  <xsl:param name="log-generalizer-config"/>
  <xsl:param name="server-root"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <cfg:LogGeneralizerConfig
    input_logs_dir="{$workspace-root}/log/LogGeneralizer/In"
    output_logs_dir="{$workspace-root}/log/LogGeneralizer/Out">

    <xsl:variable name="log-generalizer-port">
      <xsl:value-of select="$log-generalizer-config/cfg:networkParams/@port"/>
      <xsl:if test="count($log-generalizer-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-log-generalizer-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="logGeneralizer.port"
      method="text" omit-xml-declaration="yes"
      >  ['logGeneralizer', <xsl:copy-of select="$log-generalizer-port"/>],</exsl:document>

    <xsl:variable name="log-generalizer-logging" select="$log-generalizer-config/cfg:logging"/>
    <xsl:variable name="log-level"><xsl:value-of select="$log-generalizer-logging/@log_level"/>
      <xsl:if test="count($log-generalizer-logging/@log_level) = 0">
        <xsl:value-of select="$log-generalizer-log-level"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="stat-config" select="$log-generalizer-config/cfg:statLogging"/>
    <xsl:variable name="check-logs-period"><xsl:value-of select="$stat-config/@file_check_period"/>
      <xsl:if test="count($stat-config/@file_check_period) = 0">
        <xsl:value-of select="$log-generalizer-check-logs-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="check-deferred-logs-period">
      <xsl:value-of select="$log-generalizer-check-deferred-logs-period"/>
    </xsl:variable>
    <xsl:variable name="flush-logs-period"><xsl:value-of select="$stat-config/@flush_period"/>
      <xsl:if test="count($stat-config/@flush_period) = 0">
        <xsl:value-of select="$log-generalizer-flush-logs-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="flush-logs-size"><xsl:value-of select="$stat-config/@flush_size"/>
      <xsl:if test="count($stat-config/@flush_size) = 0">
        <xsl:value-of select="$log-generalizer-flush-logs-size"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="search-term-count-threshold">
      <xsl:value-of select="$stat-config/@search_term_count_threshold"/>
      <xsl:if test="count($stat-config/@search_term_count_threshold) = 0">
        <xsl:value-of select="$log-generalizer-search-term-count-threshold"/>
      </xsl:if>
    </xsl:variable>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$log-generalizer-config/cfg:threadParams/@min"/>
        <xsl:if test="count($log-generalizer-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-log-generalizer-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$log-generalizer-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
        <cfg:Object servant="LogGeneralizer" name="LogGeneralizer"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:variable name="snmp-stats-enabled">
      <xsl:if test="count($colo-config/cfg:snmpStats) > 0">
        <xsl:value-of select="string($colo-config/cfg:snmpStats/@enable)"/>
      </xsl:if>
    </xsl:variable>

    <xsl:if test="$snmp-stats-enabled = 'true'">
      <xsl:variable name="mib-root"><xsl:if
        test="count($env-config/@mib_root) = 0"><xsl:value-of
        select="$server-root"/>/mibs</xsl:if><xsl:value-of
        select="$env-config/@mib_root"/></xsl:variable>
      <cfg:SNMPConfig mib_dirs="{concat('/usr/share/snmp/mibs:',
        $mib-root, ':', $mib-root, '/LogProcessing/LogGeneralizer')}"
        index="{$colo-config/cfg:snmpStats/@oid_suffix}"/>
    </xsl:if>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$log-generalizer-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $log-generalizer-log-path)"/>
      <xsl:with-param name="default-log-level" select="$log-generalizer-log-level"/>
    </xsl:call-template>

    <xsl:variable name="pg-connection" select="$central-colo-config/cfg:pgConnectionForLogProcessing"/>
    <xsl:if test="count($pg-connection) > 0">
      <xsl:call-template name="ConvertLogGeneralizerDBConnection">
        <xsl:with-param name="pg-connection" select="$pg-connection"/>
      </xsl:call-template>
    </xsl:if>

    <cfg:LogProcessing>

      <xsl:variable name="max-upload-task-count">
        <xsl:value-of select="$stat-config/@max_upload_task_count"/>
        <xsl:if test="count($stat-config/@max_upload_task_count) = 0">
          <xsl:value-of select="$log-generalizer-upload-tasks"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="distrib_count">
        <xsl:if test="count($pg-connection) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:variable>

      <xsl:call-template name="OutputLogsConfigs">
        <xsl:with-param name="list" select="'CMPStat CreativeStat'"/>
        <xsl:with-param name="check-logs-period" select="$check-logs-period"/>
        <xsl:with-param name="flush-logs-period" select="$flush-logs-period"/>
        <xsl:with-param name="flush-logs-size" select="$flush-logs-size"/>
        <xsl:with-param name="stat-node" select="$stat-config"/>
        <xsl:with-param name="upload-tasks" select="$max-upload-task-count"/>
        <xsl:with-param name="deferred-period" select="$check-deferred-logs-period"/>
        <xsl:with-param name="upload_type" select="'postgres_csv'"/>
        <xsl:with-param name="distrib_count" select="$distrib_count"/>
      </xsl:call-template>

      <xsl:call-template name="OutputLogsConfigs">
        <xsl:with-param name="list" select="'SearchTermStat'"/>
        <xsl:with-param name="check-logs-period" select="$check-logs-period"/>
        <xsl:with-param name="flush-logs-period" select="$flush-logs-period"/>
        <xsl:with-param name="flush-logs-size" select="$flush-logs-size"/>
        <xsl:with-param name="stat-node" select="$stat-config"/>
        <xsl:with-param name="file-prefix" select="concat($workspace-root, '/cache/SearchPhrases/SearchPhrases')"/>
        <xsl:with-param name="filtering-count" select="$search-term-count-threshold"/>
        <xsl:with-param name="distrib_count" select="$distrib_count"/>
      </xsl:call-template>

      <xsl:variable name="standard-logs">
        CampaignStat ChannelCountStat ColoUsers
        SiteStat WebStat
        AdvertiserUserStat CCStat CCGStat CCGKeywordStat CCGUserStat CCUserStat
        CampaignUserStat ColoUserStat GlobalColoUserStat
        PageLoadsDailyStat PassbackStat SiteUserStat
        ActionRequest ActionStat ColoUpdateStat
        CCGSelectionFailureStat ChannelHitStat ChannelImpInventory ChannelInventory
        ChannelInventoryEstimationStat ChannelOverlapUserStat ChannelPerformance
        ChannelPriceRange ChannelTriggerImpStat ChannelTriggerStat DeviceChannelCountStat
        ExpressionPerformance SearchEngineStat
        SiteReferrerStat CampaignReferrerStat TagAuctionStat TagPositionStat UserAgentStat
        UserProperties
      </xsl:variable>

      <xsl:call-template name="OutputLogsConfigs">
        <xsl:with-param name="list" select="$standard-logs"/>
        <xsl:with-param name="check-logs-period" select="$check-logs-period"/>
        <xsl:with-param name="flush-logs-period" select="$flush-logs-period"/>
        <xsl:with-param name="flush-logs-size" select="$flush-logs-size"/>
        <xsl:with-param name="stat-node" select="$stat-config"/>
        <xsl:with-param name="distrib_count" select="$distrib_count"/>
      </xsl:call-template>

    </cfg:LogProcessing>

  </cfg:LogGeneralizerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="log-generalizer-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> LogGeneralizer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> LogGeneralizer: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> LogGeneralizer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($log-generalizer-path) = 0">
       <xsl:message terminate="yes"> LogGeneralizer: Can't find log generalizer node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="central-colo-config"
    select="$colo-config/cfg:central"/>

  <xsl:variable
    name="log-generalizer-config"
    select="$log-generalizer-path/configuration/cfg:logGeneralizer"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root[1]"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> LogGeneralizer: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/LogProcessing/LogGeneralizerConfig.xsd')}">
    <xsl:call-template name="LogGeneralizerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="central-colo-config" select="$central-colo-config"/>
      <xsl:with-param name="log-generalizer-config" select="$log-generalizer-config"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

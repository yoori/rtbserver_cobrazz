<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="service-id" select="$SERVICE_ID"/>
<xsl:variable name="service-count" select="$SERVICE_COUNT"/>

<!-- ExpressionMatcher config generate function -->
<xsl:template name="ExpressionMatcherConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="expression-matcher-config"/>
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
      <xsl:if test="count($env-config/@cache_root[1]) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="inventory-users-percentage-value"><xsl:value-of select="$colo-config/cfg:inventoryStats/@simplifying"/>
      <xsl:if test="count($colo-config/cfg:inventoryStats) = 0">
        <xsl:value-of select="$inventory-users-percentage"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="expression-matcher-port">
      <xsl:value-of select="$expression-matcher-config/cfg:networkParams/@port"/>
      <xsl:if test="count($expression-matcher-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-expression-matcher-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="expressionMatcher.port"
      method="text" omit-xml-declaration="yes"
      >  ['expressionMatcher', <xsl:copy-of select="$expression-matcher-port"/>],</exsl:document>

    <xsl:variable name="expression-matcher-logging" select="$expression-matcher-config/cfg:logging"/>
    <xsl:variable name="log-level"><xsl:value-of select="$expression-matcher-config/cfg:logging/@log_level"/>
      <xsl:if test="count($expression-matcher-logging/@log_level) = 0">
        <xsl:value-of select="$expression-matcher-log-level"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="processing-config" select="$expression-matcher-config/cfg:processing"/>
    <xsl:variable name="processing-threads">
      <xsl:value-of select="$processing-config/@threads"/>
      <xsl:if test="count($processing-config/@threads) = 0">
        <xsl:value-of select="$expression-matcher-processing-threads"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="stat-config" select="$expression-matcher-config/cfg:statLogging"/>
    <xsl:variable name="check-logs-period"><xsl:value-of select="$stat-config/@file_check_period"/>
      <xsl:if test="count($stat-config/@file_check_period) = 0">
        <xsl:value-of select="$expression-matcher-check-logs-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="flush-logs-period"><xsl:value-of select="$stat-config/@flush_period"/>
      <xsl:if test="count($stat-config/@flush_period) = 0">
        <xsl:value-of select="$expression-matcher-flush-logs-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="activity-flush-logs-period"><xsl:value-of select="$stat-config/@activity_flush_period"/>
      <xsl:if test="count($stat-config/@activity_flush_period) = 0">
        <xsl:value-of select="$expression-matcher-activity-flush-logs-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="inventory-flush-logs-period"><xsl:value-of select="$stat-config/@inventory_ecpm_flush_period"/>
      <xsl:if test="count($stat-config/@inventory_ecpm_flush_period) = 0">
        <xsl:value-of select="$expression-matcher-inventory-flush-logs-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="update-config" select="$expression-matcher-config/cfg:updateParams"/>
    <xsl:variable name="update-period"><xsl:value-of select="$update-config/@period"/>
      <xsl:if test="count($update-config/@period) = 0">
        <xsl:value-of select="$expression-matcher-update-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="chunks-root" select="concat($cache-root, '/ExpressionMatcher/')"/>

    <xsl:variable name="in-logs-dir" select="concat($workspace-root, '/log/ExpressionMatcher/In/')"/>
    <xsl:variable name="out-logs-dir" select="concat($workspace-root, '/log/ExpressionMatcher/Out/')"/>

    <xsl:variable name="expression-matcher-host-port-set">
      <xsl:for-each select="$be-cluster-path/service[@descriptor = $expression-matcher-descriptor]">
        <xsl:variable name="expression-matcher-host-subset">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'ExpressionMatcher'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="port">
          <xsl:value-of select="./configuration/cfg:expressionMatcher/cfg:networkParams/@port"/>
          <xsl:if test="count(./configuration/cfg:expressionMatcher/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-expression-matcher-port"/>
          </xsl:if>
        </xsl:variable>
        <xsl:for-each select="exsl:node-set($expression-matcher-host-subset)/host">
          <host port="{$port}"><xsl:value-of select="."/></host>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="expression-matcher-host-port-sorted-set">
      <xsl:for-each select="exsl:node-set($expression-matcher-host-port-set)/host">
        <xsl:sort select="."/>
        <host port="{@port}"><xsl:value-of select="."/></host>
      </xsl:for-each>
    </xsl:variable>


  <cfg:ExpressionMatcherConfig
    log_root="{concat($workspace-root, '/log/ExpressionMatcher')}"
    update_period="{$update-period}"
    inventory_users_percentage="{$inventory-users-percentage-value}"
    colo_id="{$colo-id}"
    service_index="{count(exsl:node-set(
      $expression-matcher-host-port-sorted-set)/host[. = $HOST]/preceding-sibling::host)}"
    service_host_name="{$HOST}">

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$expression-matcher-config/cfg:threadParams/@min"/>
        <xsl:if test="count($expression-matcher-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-expression-matcher-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$expression-matcher-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ExpressionMatcher" name="ExpressionMatcher"/>
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
      <xsl:variable name="mib-path" select="concat('/usr/share/snmp/mibs:',
        $mib-root, ':',
        $mib-root, '/RequestInfoSvcs/ExpressionMatcher')"/>
      <cfg:SNMPConfig mib_dirs="{$mib-path}"
        index="{$colo-config/cfg:snmpStats/@oid_suffix}"/>
    </xsl:if>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$expression-matcher-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $expression-matcher-log-path)"/>
      <xsl:with-param name="default-log-level" select="$expression-matcher-log-level"/>
    </xsl:call-template>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'ExpressionMatcher'"/>
    </xsl:call-template>

    <xsl:variable name="distrib-count"><xsl:value-of select="$colo-config/cfg:inventoryStats/@distrib_count"/>
      <xsl:if test="count($colo-config/cfg:inventoryStats/@distrib_count) = 0">
        <xsl:value-of select="$default-distrib-count"/>
      </xsl:if>
    </xsl:variable>

    <cfg:ExpressionMatcherGroup>
      <xsl:for-each select="exsl:node-set($expression-matcher-host-port-sorted-set)/host">
        <cfg:Ref ref="{concat('corbaloc:iiop:', ., ':', @port, '/ExpressionMatcher')}"/>
      </xsl:for-each>
    </cfg:ExpressionMatcherGroup>

    <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
      <cfg:UserInfoManagerControllerGroup name="UserInfoManagerControllers">
        <xsl:for-each select="./service[@descriptor = $user-info-manager-controller-descriptor]">
          <xsl:variable
            name="user-info-manager-controller-config"
            select="./configuration/cfg:userInfoManagerController"/>

          <xsl:variable name="hosts">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
              <xsl:with-param name="error-prefix"  select="'AdFrontend:UserInfoManagerController'"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:variable name="user-info-manager-controller-port">
            <xsl:value-of select="$user-info-manager-controller-config/cfg:networkParams/@port"/>
            <xsl:if test="count($user-info-manager-controller-config/cfg:networkParams/@port) = 0">
              <xsl:value-of select="$def-user-info-manager-controller-port"/>
            </xsl:if>
          </xsl:variable>

          <xsl:for-each select="exsl:node-set($hosts)/host">
            <cfg:Ref ref="{concat('corbaloc:iiop:', ., ':',
              $user-info-manager-controller-port, '/', $current-user-info-manager-controller-obj)}"/>
          </xsl:for-each>
        </xsl:for-each>
      </cfg:UserInfoManagerControllerGroup>
    </xsl:for-each>

    <xsl:variable name="inventory_days_to_keep">
      <xsl:choose>
        <xsl:when test="count($expression-matcher-config/cfg:profilesCleanupParams/@inventory_days_to_keep) > 0">
          <xsl:value-of select="$expression-matcher-config/cfg:profilesCleanupParams/@inventory_days_to_keep"/>
        </xsl:when>
        <xsl:otherwise>4</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <cfg:ChunksDistribution
      distribution_file_path="{concat($workspace-root, '/run/ExpressionMatcherDistribution.xml')}"
      distribution_file_schema="{concat($server-root, '/xsd/AdServerCommons/HostDistributionFile.xsd')}"/>

    <cfg:Storage>
      <xsl:attribute name="min_free_space"><xsl:value-of select="$expression-matcher-config/cfg:storage/@min_free_space"/>
        <xsl:if test="count($expression-matcher-config/cfg:storage/@min_free_space) = 0">0</xsl:if>
      </xsl:attribute>
    </cfg:Storage>

    <xsl:variable name="em-count">
      <xsl:value-of select="count(exsl:node-set($expression-matcher-host-port-sorted-set)/host)"/>
    </xsl:variable>

    <xsl:variable name="rwlevel-max-size">
      <xsl:value-of select='format-number(ceiling(1024 * 1024 * 1024 * $em-count div $distrib-count), "#")'/>
    </xsl:variable>

    <xsl:variable name="max-undumped-size">
      <xsl:value-of select='format-number(5 * $rwlevel-max-size, "#")'/>
    </xsl:variable>

    <xsl:variable name="max-inventory-undumped-size"><xsl:value-of select="$expression-matcher-config/cfg:storage/@max_inventory_undumped_size"/>
      <xsl:if test="count($expression-matcher-config/cfg:storage/@max_inventory_undumped_size) = 0">5000</xsl:if>
    </xsl:variable>

    <xsl:variable name="rwlevel-max-inventory-size">
      <xsl:value-of select='format-number(ceiling($max-inventory-undumped-size
      * 1024 * 1024 * $em-count div $distrib-count), "#")'/>
    </xsl:variable>

    <xsl:variable name="max-user-trigger-undumped-size"><xsl:value-of select="$expression-matcher-config/cfg:storage/@max_user_trigger_undumped_size"/>
      <xsl:if test="count($expression-matcher-config/cfg:storage/@max_user_trigger_undumped_size) = 0">5000</xsl:if>
    </xsl:variable>

    <xsl:variable name="rwlevel-max-user-trigger-size">
      <xsl:value-of select='format-number(ceiling($max-user-trigger-undumped-size
      * 1024 * 1024 * $em-count div $distrib-count), "#")'/>
    </xsl:variable>

    <cfg:ChunksConfig chunks_prefix = "Inventory_"
      rw_buffer_size="10485760"
      rwlevel_max_size="{$rwlevel-max-inventory-size}"
      max_undumped_size='{format-number($max-inventory-undumped-size
        * 1024 * 1024, "#")}'
      max_levels0="20"
      chunks_root="{$chunks-root}"
      chunks_number="{$distrib-count}"
      days_to_keep="{$inventory_days_to_keep}"
      expire_time="2592000"/>

    <xsl:if test="count($colo-config/cfg:channelTriggerImpStats/@enable) = 0 or
      count($colo-config/cfg:channelTriggerImpStats[@enable = 'true']) > 0 or
      count($colo-config/cfg:channelTriggerImpStats[@enable = '1']) > 0">
      <cfg:TriggerImpsConfig
        positive_triggers_group_size="{$expression-matcher-config/@positive_triggers_group_size}"
        negative_triggers_group_size="{$expression-matcher-config/@negative_triggers_group_size}"
        max_trigger_visits="{$expression-matcher-config/@max_trigger_visits}">
        <cfg:UserChunksConfig
          chunks_prefix="UserTriggerMatch_"
          rw_buffer_size="10485760"
          rwlevel_max_size="{$rwlevel-max-user-trigger-size}"
          max_undumped_size='{format-number($max-user-trigger-undumped-size
            * 1024 * 1024, "#")}'
          max_levels0="20"
          chunks_root="{$chunks-root}"
          chunks_number="{$distrib-count}">
          <xsl:attribute name="expire_time">
            <xsl:choose>
              <xsl:when test="count($expression-matcher-config/cfg:profilesCleanupParams/@trigger_match_life_time) > 0">
                <xsl:value-of select="$expression-matcher-config/cfg:profilesCleanupParams/@trigger_match_life_time*1440"/>
              </xsl:when>
              <xsl:otherwise>2592000</xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </cfg:UserChunksConfig>

        <cfg:TempUserChunksConfig
          chunks_prefix="TempUserTriggerMatch_"
          rw_buffer_size="10485760"
          rwlevel_max_size="{$rwlevel-max-size}"
          max_undumped_size="{$max-undumped-size}"
          max_levels0="20"
          chunks_root="{$chunks-root}"
          chunks_number="{$distrib-count}">
          <xsl:attribute name="expire_time"><xsl:value-of
            select="$expression-matcher-config/cfg:profilesCleanupParams/@temp_trigger_match_life_time"/><xsl:if
            test="count($expression-matcher-config/cfg:profilesCleanupParams/@temp_trigger_match_life_time) = 0">3600</xsl:if></xsl:attribute>
        </cfg:TempUserChunksConfig>

        <cfg:RequestChunksConfig chunks_prefix="RequestTriggerMatch_"
          rw_buffer_size="10485760"
          rwlevel_max_size="{$rwlevel-max-size}"
          max_undumped_size="{$max-undumped-size}"
          max_levels0="20"
          chunks_root="{concat($cache-root, '/RequestTriggerMatch/')}"
          expire_time="172800"/>
      </cfg:TriggerImpsConfig>
    </xsl:if>

    <cfg:HouseholdColoReachChunksConfig chunks_prefix="HouseholdColoReach_"
      rw_buffer_size="10485760"
      rwlevel_max_size="{$rwlevel-max-size}"
      max_undumped_size="{$max-undumped-size}"
      max_levels0="20"
      chunks_root="{$chunks-root}"
      chunks_number="{$distrib-count}"
      expire_time="5184000"/>

    <cfg:LogProcessing threads="{$processing-threads}"
      channel_match_cache_size="{$expression-matcher-config/@channel_match_cache_size}">

      <xsl:attribute name="adrequest_anonymize"><xsl:choose>
          <xsl:when test="$colo-config/cfg:coloParams/@ad_request_profiling =
            'ad-request profiling and stats collection disabled'">true</xsl:when>
          <xsl:otherwise>false</xsl:otherwise>
        </xsl:choose></xsl:attribute>

      <xsl:attribute name="cache_blocks"><xsl:value-of select="$processing-config/@cache_blocks"/>
        <xsl:if test="count($processing-config/@cache_blocks) = 0">1000</xsl:if>
      </xsl:attribute>

      <cfg:InLogs log_root="{$in-logs-dir}" check_logs_period="{$check-logs-period}">
        <cfg:RequestBasicChannels/>
        <cfg:ConsiderClick/>
        <cfg:ConsiderImpression/>
      </cfg:InLogs>

      <cfg:OutLogs log_root="{$out-logs-dir}">
        <cfg:ChannelInventory period="{$flush-logs-period}"/>
        <cfg:ChannelImpInventory period="{$flush-logs-period}"/>
        <cfg:ChannelPriceRange period="{$inventory-flush-logs-period}"/>
        <cfg:ChannelInventoryActivity period="{$activity-flush-logs-period}"/>
        <cfg:ChannelPerformance period="{$flush-logs-period}"/>
        <cfg:ChannelTriggerImpStat period="{$flush-logs-period}"/>
        <cfg:GlobalColoUserStat period="{$flush-logs-period}"/>
        <cfg:ColoUserStat period="{$flush-logs-period}"/>
      </cfg:OutLogs>
    </cfg:LogProcessing>

    <cfg:DailyProcessing processing_time="00:01" thread_pool_size="5"/>
  </cfg:ExpressionMatcherConfig>

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
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>

  <xsl:variable
    name="expression-matcher-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($expression-matcher-path) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find log expression matcher node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="expression-matcher-config"
    select="$expression-matcher-path/configuration/cfg:expressionMatcher"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> ExpressionMatcher: Can't find colo config config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/LogProcessing/ExpressionMatcherConfig.xsd')}">
    <xsl:call-template name="ExpressionMatcherConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="expression-matcher-config" select="$expression-matcher-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

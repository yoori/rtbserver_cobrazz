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
  exclude-result-prefixes="exsl dyn">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:include href="RequestInfoVariables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- RequestInfoManager config generate function -->
<xsl:template name="RequestInfoManagerConfigGenerator">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="request-info-manager-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="server-root"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of
       select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
    <xsl:if test="count($env-config/@cache_root[1]) = 0"><xsl:value-of
       select="$def-cache-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>
  <xsl:variable name="request-info-manager-host-port-set">
    <xsl:for-each select="$be-cluster-path/service[@descriptor = $request-info-manager-descriptor]">
      <xsl:variable name="request-info-manager-host-subset">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix" select="'RequestInfoManager'"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="port">
      <xsl:value-of select="./configuration/cfg:requestInfoManager/cfg:networkParams/@port"/>
      <xsl:if test="count(./configuration/cfg:requestInfoManager/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-request-info-manager-port"/>
      </xsl:if>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($request-info-manager-host-subset)/host">
        <host port="{$port}"><xsl:value-of select="."/></host>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="request-info-manager-host-port-sorted-set">
    <xsl:for-each select="exsl:node-set($request-info-manager-host-port-set)/host">
      <xsl:sort select="."/>
      <host port="{@port}"><xsl:value-of select="."/></host>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="request-info-manager-port">
    <xsl:value-of select="$request-info-manager-config/cfg:networkParams/@port"/>
    <xsl:if test="count($request-info-manager-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-request-info-manager-port"/>
    </xsl:if>
  </xsl:variable>

  <exsl:document href="requestInfoManager.port"
    method="text" omit-xml-declaration="yes"
    >  ['requestInfoManager', <xsl:copy-of select="$request-info-manager-port"/>],</exsl:document>

  <xsl:variable name="request-info-manager-logging" select="$request-info-manager-config/cfg:logging"/>
  <xsl:variable name="log-level">
    <xsl:value-of select="$request-info-manager-config/cfg:logging/@log_level"/>
    <xsl:if test="count($request-info-manager-logging/cfg:logging/@log_level) = 0">
      <xsl:value-of select="$request-info-manager-log-level"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="stat-config" select="$request-info-manager-config/cfg:statLogging"/>
  <xsl:variable name="check-logs-period"><xsl:value-of select="$stat-config/@file_check_period"/>
    <xsl:if test="count($stat-config/@file_check_period) = 0">
      <xsl:value-of select="$request-info-manager-check-logs-period"/>
    </xsl:if>
  </xsl:variable>
  <xsl:variable name="flush-logs-period"><xsl:value-of select="$stat-config/@flush_period"/>
    <xsl:if test="count($stat-config/@flush_period) = 0">
      <xsl:value-of select="$request-info-manager-flush-logs-period"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="user-action-info-chunks-root"
    select="concat($cache-root, '/', $user-action-dir-name, '/')"/>
  <xsl:variable name="user-campaign-reach-chunks-root"
    select="concat($cache-root, '/', $user-campaign-reach-dir-name, '/')"/>
  <xsl:variable name="passback-chunks-root"
    select="concat($cache-root, '/', $passback-dir-name, '/')"/>
  <xsl:variable name="request-chunks-root"
    select="concat($cache-root, '/', $request-dir-name, '/')"/>
  <xsl:variable name="bid-chunks-root"
    select="concat($cache-root, '/Bid/')"/>
  <xsl:variable name="user-fraud-protection-chunks-root"
    select="concat($cache-root, '/', $user-fraud-protection-dir-name, '/')"/>

  <xsl:variable name="processing-config" select="$request-info-manager-config/cfg:processing"/>
  <xsl:variable name="processing-threads">
    <xsl:value-of select="$processing-config/@threads"/>
    <xsl:if test="count($processing-config/@threads) = 0">
      <xsl:value-of select="$request-info-manager-processing-threads"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="in-logs-dir" select="concat($workspace-root, '/log/RequestInfoManager/In/')"/>
  <xsl:variable name="out-logs-dir" select="concat($workspace-root, '/log/RequestInfoManager/Out/')"/>

  <xsl:variable name="ignore-action-time-value"><xsl:value-of select="$request-info-manager-config/cfg:actionProcessing/@ignore_action_time"/>
    <xsl:if test="count($request-info-manager-config/cfg:actionProcessing/@ignore_action_time) = 0"><xsl:value-of select="$def-ignore-action-time"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="use-referrer-site-referrer-stats">
    <xsl:call-template name="GetReferrerLoggingValue">
      <xsl:with-param name="referrer-logging" select="$colo-config/@referrer_logging_site_referrer_stats"/>
    </xsl:call-template>
  </xsl:variable>

  <cfg:RequestInfoManagerConfig
    colo_id="{$colo-id}"
    service_index="{count(exsl:node-set(
      $request-info-manager-host-port-sorted-set)/host[. = $HOST]/preceding-sibling::host)}"
    services_count="{count(exsl:node-set(
      $request-info-manager-host-port-sorted-set)/host)}"
    distrib_count="24"
    action_ignore_time="{$ignore-action-time-value}"
    use_referrer_site_referrer_stats="{$use-referrer-site-referrer-stats}">

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$request-info-manager-config/cfg:threadParams/@min"/>
        <xsl:if test="count($request-info-manager-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-request-info-manager-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$request-info-manager-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
        <cfg:Object servant="RequestInfoManager" name="RequestInfoManager"/>
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
        $mib-root, '/RequestInfoSvcs/RequestInfoManager')"/>
      <cfg:SNMPConfig mib_dirs="{$mib-path}"
        index="{$colo-config/cfg:snmpStats/@oid_suffix}"/>
    </xsl:if>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$request-info-manager-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $request-info-manager-log-path)"/>
      <xsl:with-param name="default-log-level" select="$request-info-manager-log-level"/>
    </xsl:call-template>

    <cfg:UserActionChunksConfig chunks_prefix="{$user-action-prefix}"
      chunks_root="{$user-action-info-chunks-root}"/>

    <cfg:UserCampaignReachChunksConfig chunks_prefix="{$user-campaign-reach-prefix}"
      chunks_root="{$user-campaign-reach-chunks-root}"/>

    <cfg:UserFraudProtectionChunksConfig chunks_prefix="{$user-fraud-protection-prefix}"
      chunks_root="{$user-fraud-protection-chunks-root}"/>

    <cfg:PassbackChunksConfig chunks_prefix="{$passback-prefix}"
      chunks_root="{$passback-chunks-root}">
      <xsl:attribute name="expire_time"><xsl:value-of
        select="$request-info-manager-config/cfg:passbackProcessing/@expire_time"/>
        <xsl:if test="count($request-info-manager-config/cfg:passbackProcessing/@expire_time) = 0"><xsl:value-of
          select="$def-passback-request-expire-time"/></xsl:if>
      </xsl:attribute>
    </cfg:PassbackChunksConfig>

    <cfg:UserSiteReachChunksConfig chunks_prefix="{$user-site-reach-prefix}"
      chunks_root="{concat($cache-root, '/', $user-site-reach-chunks-root, '/')}"/>

    <xsl:if test="count($colo-config/cfg:tagGroupStats/@enable) = 0 or
      $colo-config/cfg:tagGroupStats/@enable = 'true' or
      $colo-config/cfg:tagGroupStats/@enable = '1'">
      <cfg:TagRequestGroupingConfig merge_time_bound="1" chunks_prefix="{$user-tag-request-group-prefix}"
        chunks_root="{concat($cache-root, '/', $user-tag-request-group-chunks-root, '/')}"/>
    </xsl:if>

    <cfg:ChunksConfig chunks_prefix="{$request-prefix}"
      chunks_root="{$request-chunks-root}">
      <xsl:attribute name="expire_time"><xsl:value-of
        select="$request-info-manager-config/cfg:requestProcessing/@expire_time"/>
        <xsl:if test="count($request-info-manager-config/cfg:requestProcessing/@expire_time) = 0"><xsl:value-of
          select="$def-request-expire-time"/></xsl:if>
      </xsl:attribute>
    </cfg:ChunksConfig>

    <cfg:BidChunksConfig chunks_prefix="Bid_"
      chunks_root="{$bid-chunks-root}">
      <xsl:attribute name="expire_time"><xsl:value-of
        select="$request-info-manager-config/cfg:requestProcessing/@expire_time"/>
        <xsl:if test="count($request-info-manager-config/cfg:requestProcessing/@expire_time) = 0"><xsl:value-of
          select="$def-request-expire-time"/></xsl:if>
      </xsl:attribute>
    </cfg:BidChunksConfig>

    <xsl:if test="count($colo-config/cfg:WebIndexOnRTB/@enable_profiling) > 0 and
      ($colo-config/cfg:WebIndexOnRTB/@enable_profiling = 'true' or
       $colo-config/cfg:WebIndexOnRTB/@enable_profiling = '1') and
      count($colo-config/cfg:WebIndexOnRTB/cfg:Endpoint) > 0">
      <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
        <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of
          select="$def-config-root"/></xsl:if>/<xsl:value-of select="$out-dir"/>
      </xsl:variable>
      <xsl:variable name="unixcommons-root"><xsl:value-of select="$env-config/@unixcommons_root"/>
        <xsl:if test="count($env-config/@unixcommons_root) = 0"><xsl:value-of select="$def-unixcommons-root"/></xsl:if>
      </xsl:variable>
      <xsl:variable name="uid-key-path">
        <xsl:choose>
          <xsl:when test="count($colo-config/cfg:coloParams/cfg:uid_key[. != '']) != 0">
            <xsl:value-of select="concat($config-root, '/cert/')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($unixcommons-root, '/share/uuid_keys/')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <cfg:Profiling user_id_private_key="{concat($uid-key-path, 'private.der')}"
        threads="1">
        <xsl:attribute name="sending_window"><xsl:value-of
          select="$request-info-manager-config/cfg:profiling/@sending_window"/>
          <xsl:if test="count($request-info-manager-config/cfg:profiling/@sending_window) = 0">1200</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="max_pool_size"><xsl:value-of
          select="$request-info-manager-config/cfg:profiling/@max_pool_size"/>
          <xsl:if test="count($request-info-manager-config/cfg:profiling/@max_pool_size) = 0">1048576</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="repeat_trigger_timeout"><xsl:value-of
          select="$colo-config/cfg:userProfiling/@repeat_trigger_timeout"/>
          <xsl:if test="count($colo-config/cfg:userProfiling/@repeat_trigger_timeout) = 0">
           <xsl:value-of select="$def-repeat-trigger-timeout"/>
          </xsl:if>
        </xsl:attribute>
        <xsl:for-each select="$colo-config/cfg:WebIndexOnRTB/cfg:Endpoint">
          <cfg:Endpoint url="{@url}"/>
        </xsl:for-each>
      </cfg:Profiling>
    </xsl:if>

    <cfg:Billing dump_period="600" send_delayed_period="300" threads="10">
      <xsl:attribute name="storage_root"><xsl:value-of
        select="concat($cache-root, '/RequestInfoManager/Amount')"/>
      </xsl:attribute>
      <cfg:BillingServerCorbaRef name="BillingServer">
        <xsl:call-template name="BillingServerCorbaRefs">
          <xsl:with-param name="billing-servers"
            select="$full-cluster-path//service[@descriptor = $billing-server-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'RequestInfoManager'"/>
        </xsl:call-template>
      </cfg:BillingServerCorbaRef>
    </cfg:Billing>

    <cfg:LogProcessing>
      <xsl:attribute name="cache_blocks"><xsl:value-of select="$processing-config/@cache_blocks"/>
        <xsl:if test="count($processing-config/@cache_blocks) = 0">1000</xsl:if>
      </xsl:attribute>

      <cfg:InLogs threads="{$processing-threads}" log_root="{$in-logs-dir}"
        check_logs_period="{$check-logs-period}">

        <cfg:Request priority="2"/>
        <cfg:Impression priority="2"/>
        <cfg:Click priority="2"/>
        <cfg:AdvertiserAction priority="2"/>
        <cfg:PassbackImpression priority="1"/>
        <cfg:TagRequest priority="1"/>
        <cfg:RequestOperation priority="2"/>
      </cfg:InLogs>

      <cfg:OutLogs log_root="{$out-logs-dir}">
        <xsl:attribute name="notify_impressions">
          <xsl:choose>
            <xsl:when test="count($colo-config/cfg:channelTriggerImpStats/@enable) = 0 or
              count($colo-config/cfg:channelTriggerImpStats[@enable = 'true']) > 0 or
              count($colo-config/cfg:channelTriggerImpStats[@enable = '1']) > 0 or
              count($colo-config/cfg:inventoryStats/@simplifying) = 0 or
              count($colo-config/cfg:inventoryStats[@simplifying = '0']) = 0">true</xsl:when>
            <xsl:otherwise>false</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:attribute name="notify_revenue">
          <xsl:choose>
            <xsl:when test="count($colo-config/cfg:inventoryStats/@simplifying) = 0 or
              count($colo-config/cfg:inventoryStats[@simplifying = '0']) = 0">true</xsl:when>
            <xsl:otherwise>false</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:attribute name="distrib_count"><xsl:value-of select="$colo-config/cfg:inventoryStats/@distrib_count"/>
          <xsl:if test="count($colo-config/cfg:inventoryStats/@distrib_count) = 0">
            <xsl:value-of select="$default-distrib-count"/>
          </xsl:if>
        </xsl:attribute>

        <xsl:variable name="predictor-service" 
          select="$full-cluster-path//service[@descriptor = $predictor-descriptor]"/>

        <cfg:CreativeStat period="{$flush-logs-period}"/>
        <cfg:UserProperties period="{$flush-logs-period}"/>
        <cfg:ChannelPerformance period="{$flush-logs-period}"/>
        <cfg:SiteChannelStat period="{$flush-logs-period}"/>
        <cfg:ExpressionPerformance period="{$flush-logs-period}"/>
        <cfg:CcgKeywordStat period="{$flush-logs-period}"/>
        <cfg:CmpStat period="{$flush-logs-period}"/>
        <cfg:ActionStat period="{$flush-logs-period}"/>
        <cfg:ChannelImpInventory period="{$flush-logs-period}"/>
        <cfg:CcgUserStat period="{$flush-logs-period}"/>
        <cfg:CcUserStat period="{$flush-logs-period}"/>
        <cfg:CampaignUserStat period="{$flush-logs-period}"/>
        <cfg:AdvertiserUserStat period="{$flush-logs-period}"/>
        <cfg:PassbackStat period="{$flush-logs-period}"/>
        <cfg:SiteUserStat period="{$flush-logs-period}"/>
        <cfg:SiteReferrerStat period="{$flush-logs-period}"/>
        <cfg:PageLoadsDailyStat period="{$flush-logs-period}"/>
        <cfg:TagPositionStat period="{$flush-logs-period}"/>
        <cfg:CampaignReferrerStat period="{$flush-logs-period}"/>
        <xsl:if test="count($predictor-service)> 0 or count($colo-config/cfg:predictorConfig/cfg:ref)> 0">
          <cfg:ResearchAction period="{$flush-logs-period}"/>
          <xsl:if test="$colo-config/cfg:predictorConfig/cfg:researchStat/@enable_bids = '1'
            or  $colo-config/cfg:predictorConfig/cfg:researchStat/@enable_bids = 'true'">
            <cfg:ResearchBid period="{$flush-logs-period}"/>
          </xsl:if>
          <cfg:ResearchImpression period="{$flush-logs-period}"/>
          <cfg:ResearchClick period="{$flush-logs-period}"/>
          <cfg:BidCostStat period="{$flush-logs-period}"/>
        </xsl:if>
        <cfg:RequestOperation chunks_count="24" period="{$flush-logs-period}"/>
        <cfg:ConsiderAction period="{$flush-logs-period}"/>
        <cfg:ConsiderClick period="{$flush-logs-period}"/>
        <cfg:ConsiderImpression period="{$flush-logs-period}"/>
        <cfg:ConsiderRequest period="{$flush-logs-period}"/>
      </cfg:OutLogs>
    </cfg:LogProcessing>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'RequestInfoManager'"/>
    </xsl:call-template>

    <xsl:call-template name="AddUserInfoManagerControllerGroups">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="error-prefix" select="AdFrontend"/>
    </xsl:call-template>

  </cfg:RequestInfoManagerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="user-profiling-path"
    select="$be-cluster-path/serviceGroup[@descriptor = $user-profiling-descriptor]"/>

  <xsl:variable
    name="request-info-manager-path"
    select="$xpath"/>

  <xsl:variable
    name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>

  <xsl:variable
    name="user-info-manager-controllers-path"
    select="$fe-cluster-path/service[@descriptor = $user-info-manager-controller-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> RequestInfoManager: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> RequestInfoManager: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> RequestInfoManager: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($request-info-manager-path) = 0">
       <xsl:message terminate="yes"> RequestInfoManager: Can't find request info manager node </xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-manager-controllers-path) = 0">
       <xsl:message terminate="yes"> RequestInfoManager: Can't find UserInfoManagerController node </xsl:message>
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
    name="request-info-manager-config"
    select="$request-info-manager-path/configuration/cfg:requestInfoManager"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root[1]"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration xsi:schemaLocation=
    "{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/RequestInfoSvcs/RequestInfoManagerConfig.xsd')}">
    <xsl:call-template name="RequestInfoManagerConfigGenerator">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="request-info-manager-config" select="$request-info-manager-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="user-info-manager-controllers-path"
        select="$user-info-manager-controllers-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

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
  exclude-result-prefixes="exsl dyn"
  xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration ../AdServer/UserInfoSvcs/UserInfoManagerConfig.xsd">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="cluster_id" select="$CLUSTER_ID"/>

<xsl:template name="ChunksConfigGenerator">
  <xsl:param name="rw-buffer-size"/>
  <xsl:param name="max-undumped-size-attr"/>
  <xsl:param name="max-undumped-size-default"/>
  <xsl:param name="max-levels0"/>
  <xsl:param name="expire-time"/>

  <xsl:attribute name="rw_buffer_size">
    <xsl:value-of select="$rw-buffer-size"/>
  </xsl:attribute>
  
  <xsl:variable name="max-undumped-size">
    <xsl:choose>
      <xsl:when test="count($max-undumped-size-attr) > 0">
        <xsl:value-of select="$max-undumped-size-attr * 1024 * 1024"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$max-undumped-size-default * 1024 * 1024"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:attribute name="rwlevel_max_size">
    <xsl:value-of select='format-number(ceiling($max-undumped-size div 3), "#")'/>
  </xsl:attribute>

  <xsl:attribute name="max_undumped_size">
    <xsl:value-of select='format-number($max-undumped-size, "#")'/>
  </xsl:attribute>

  <xsl:attribute name="max_levels0">
    <xsl:value-of select="$max-levels0"/>
  </xsl:attribute>

  <xsl:attribute name="expire_time">
    <xsl:value-of select="$expire-time"/>
  </xsl:attribute>
</xsl:template>

<!-- UserInfoManager config generate function -->
<xsl:template name="UserInfoManagerConfigGenerator">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="user-info-manager-config"/>
  <xsl:param name="secure-files-root"/>
  <xsl:param name="stats-collector-path"/>
  <xsl:param name="stats-collector-config"/>

  <cfg:UserInfoManagerConfig
    host="{$HOST}"
    max_base_profile_waiters="8"
    max_temp_profile_waiters="8"
    max_freqcap_profile_waiters="8"
    service_index="{$SERVICE_ID}">

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="user-info-manager-port">
      <xsl:value-of select="$user-info-manager-config/cfg:networkParams/@port"/>
      <xsl:if test="count($user-info-manager-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-user-info-manager-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="userInfoManager.port"
      method="text" omit-xml-declaration="yes"
      >  ['userInfoManager', <xsl:copy-of select="$user-info-manager-port"/>],</exsl:document>

    <xsl:variable name="root-dir" select="concat($workspace-root, '/log/UserInfoManager/Out/')"/>

    <xsl:variable name="update-config" select="$user-info-manager-config/cfg:updateParams"/>
    <xsl:variable name="channels-update-period"><xsl:value-of select="$update-config/@update_period"/>
      <xsl:if test="count($update-config/@update_period) = 0">
        <xsl:value-of select="$user-info-manager-channels-update-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="profile-lifetime"><xsl:value-of select="$user-info-manager-config/cfg:profilesCleanupParams/@life_time"/>
      <xsl:if test="count($user-info-manager-config/cfg:profilesCleanupParams/@life_time) = 0">
        <xsl:value-of select="$def-profile-lifetime"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="temp-profile-lifetime"><xsl:value-of select="$user-info-manager-config/cfg:profilesCleanupParams/@temp_life_time"/>
      <xsl:if test="count($user-info-manager-config/cfg:profilesCleanupParams/@temp_life_time) = 0">
        <xsl:value-of select="$def-temp-profile-lifetime"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="session-timeout"><xsl:value-of select="$user-info-manager-config/cfg:matchParams/@session_timeout"/>
      <xsl:if test="count($user-info-manager-config/cfg:matchParams/@session_timeout) = 0">
        <xsl:value-of select="$def-session-timeout"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="repeat-trigger-timeout"><xsl:value-of select="$colo-config/cfg:userProfiling/@repeat_trigger_timeout"/>
      <xsl:if test="count($colo-config/cfg:userProfiling/@repeat_trigger_timeout) = 0">
        <xsl:value-of select="$def-repeat-trigger-timeout"/>
      </xsl:if>
    </xsl:variable>
   <xsl:variable name="history-optimization-period"><xsl:value-of select="$user-info-manager-config/cfg:matchParams/@history_optimization_period"/>
      <xsl:if test="count($user-info-manager-config/cfg:matchParams/@history_optimization_period) = 0">
        <xsl:value-of select="$def-history-optimization-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="user-info-exchanger-ref" select="$colo-config/cfg:userProfiling/cfg:userInfoExchangerRef"/>
    <xsl:variable name="global-secure-params" select="$colo-config/cfg:secureParams"/>

    <!-- start config generation -->
    <xsl:attribute name="channels_update_period"><xsl:value-of select="$channels-update-period"/></xsl:attribute>
    <xsl:attribute name="session_timeout"><xsl:value-of select="$session-timeout"/></xsl:attribute>
    <xsl:attribute name="repeat_trigger_timeout"><xsl:value-of select="$repeat-trigger-timeout"/></xsl:attribute>
    <xsl:attribute name="history_optimization_period"><xsl:value-of select="$history-optimization-period"/></xsl:attribute>
    <xsl:attribute name="root_dir"><xsl:value-of select="$root-dir"/></xsl:attribute>
    <xsl:attribute name="colo_id"><xsl:value-of select="$colo-id"/></xsl:attribute>

    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$user-info-manager-config/cfg:threadParams/@min"/>
        <xsl:if test="count($user-info-manager-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-user-info-manager-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$user-info-manager-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="UserInfoManager" name="UserInfoManager"/>
        <cfg:Object servant="UserInfoManagerControl" name="UserInfoManagerControl"/>
        <cfg:Object servant="UserInfoManagerStats" name="UserInfoManagerStats"/>
        <cfg:Object servant="UserInfoManager" name="UserInfoManager">
          <xsl:attribute name="name"><xsl:value-of select="$current-user-info-manager-obj"/></xsl:attribute>
        </cfg:Object>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$user-info-manager-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $user-info-manager-log-path)"/>
      <xsl:with-param name="default-log-level" select="$user-info-manager-log-level"/>
    </xsl:call-template>

    <cfg:Logging>
      <cfg:ChannelCountStat>
        <xsl:attribute name="flush_period">30</xsl:attribute>
      </cfg:ChannelCountStat>
    </cfg:Logging>

    <cfg:Storage>
      <xsl:attribute name="chunks_root">
        <xsl:value-of select="concat($cache-root, '/Users', $cluster_id, '/')"/>
      </xsl:attribute>

      <xsl:attribute name="min_free_space"><xsl:value-of select="$user-info-manager-config/cfg:storage/@min_free_space"/>
        <xsl:if test="count($user-info-manager-config/cfg:storage/@min_free_space) = 0">0</xsl:if>
      </xsl:attribute>

      <xsl:attribute name="common_chunks_number"><xsl:value-of select="$colo-config/cfg:userProfiling/@chunks_count"/>
        <xsl:if test="count($colo-config/cfg:userProfiling/@chunks_count) = 0">
          <xsl:value-of select="$user-info-manager-scale-chunks"/>
        </xsl:if>
      </xsl:attribute>

      <xsl:variable name="tiny-max-undumped-size" select='300'/>
      <xsl:variable name="huge-max-undumped-size" select='3072'/>

      <cfg:AddChunksConfig>
        <xsl:call-template name="ChunksConfigGenerator">
          <xsl:with-param name="rw-buffer-size" select="$def-storage-rw-buffer-size"/>
          <xsl:with-param name="max-undumped-size-attr" select="$user-info-manager-config/cfg:storage/@max_add_undumped_size"/>
          <xsl:with-param name="max-undumped-size-default" select="$tiny-max-undumped-size"/>
          <xsl:with-param name="max-levels0" select="$def-storage-max-levels0"/>
          <xsl:with-param name="expire-time" select="$profile-lifetime * 86400"/>
        </xsl:call-template>
      </cfg:AddChunksConfig>

      <cfg:TempChunksConfig>
        <xsl:call-template name="ChunksConfigGenerator">
          <xsl:with-param name="rw-buffer-size" select="$def-storage-rw-buffer-size"/>
          <xsl:with-param name="max-undumped-size-attr" select="$user-info-manager-config/cfg:storage/@max_temp_undumped_size"/>
          <xsl:with-param name="max-undumped-size-default" select="$tiny-max-undumped-size"/>
          <xsl:with-param name="max-levels0" select="$def-storage-max-levels0"/>
          <xsl:with-param name="expire-time" select="$temp-profile-lifetime * 60"/>
        </xsl:call-template>
      </cfg:TempChunksConfig>

      <cfg:HistoryChunksConfig>
        <xsl:call-template name="ChunksConfigGenerator">
          <xsl:with-param name="rw-buffer-size" select="$def-storage-rw-buffer-size"/>
          <xsl:with-param name="max-undumped-size-attr" select="$user-info-manager-config/cfg:storage/@max_history_undumped_size"/>
          <xsl:with-param name="max-undumped-size-default" select="$tiny-max-undumped-size"/>
          <xsl:with-param name="max-levels0" select="$def-storage-max-levels0"/>
          <xsl:with-param name="expire-time" select="$profile-lifetime * 86400"/>
        </xsl:call-template>
      </cfg:HistoryChunksConfig>

      <cfg:BaseChunksConfig>
        <xsl:call-template name="ChunksConfigGenerator">
          <xsl:with-param name="rw-buffer-size" select="$def-storage-rw-buffer-size"/>
          <xsl:with-param name="max-undumped-size-attr" select="$user-info-manager-config/cfg:storage/@max_base_undumped_size"/>
          <xsl:with-param name="max-undumped-size-default" select="$huge-max-undumped-size"/>
          <xsl:with-param name="max-levels0" select="$def-storage-max-levels0"/>
          <xsl:with-param name="expire-time" select="$profile-lifetime * 86400"/>
        </xsl:call-template>
      </cfg:BaseChunksConfig>

      <cfg:FreqCapChunksConfig>
        <xsl:call-template name="ChunksConfigGenerator">
          <xsl:with-param name="rw-buffer-size" select="$def-storage-rw-buffer-size"/>
          <xsl:with-param name="max-undumped-size-attr" select="$user-info-manager-config/cfg:storage/@max_freq_cap_undumped_size"/>
          <xsl:with-param name="max-undumped-size-default" select="$huge-max-undumped-size"/>
          <xsl:with-param name="max-levels0" select="$def-storage-max-levels0"/>
          <xsl:with-param name="expire-time" select="$profile-lifetime * 86400"/>
        </xsl:call-template>
      </cfg:FreqCapChunksConfig>
    </cfg:Storage>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'UserInfoManager'"/>
    </xsl:call-template>

    <xsl:call-template name="AddStatsDumper">
      <xsl:with-param name="stats-collector-path" select="$stats-collector-path"/>
      <xsl:with-param name="stats-collector-config" select="$stats-collector-config"/>
      <xsl:with-param name="provide-channel-counters" select="'true'"/>
    </xsl:call-template>

    <xsl:if test="(count($user-info-manager-config/cfg:profilesCleanupParams/@clean_user_profiles) = 0 and
       $def-clean-user-profiles) or $user-info-manager-config/cfg:profilesCleanupParams/@clean_user_profiles = 'true' or
       $user-info-manager-config/cfg:profilesCleanupParams/@clean_user_profiles = '1'">
      <cfg:UserProfilesCleanup content_cleanup_time="43200" process_portion="100">
        <xsl:attribute name="start_time"><xsl:value-of select="$user-info-manager-config/cfg:profilesCleanupParams/@clean_time"/>
          <xsl:if test="count($user-info-manager-config/cfg:profilesCleanupParams/@clean_time) = 0">
            <xsl:value-of select="$def-profiles-cleanup-time"/>
          </xsl:if>
        </xsl:attribute>
      </cfg:UserProfilesCleanup>
    </xsl:if>

    <xsl:if test="count($user-info-exchanger-ref) > 0">
      <xsl:variable name="secure-params" select="$user-info-exchanger-ref/cfg:secureParams"/>
      <cfg:UserInfoExchangerParameters
        set_get_profiles_period="60"
        colo_request_timeout="600">
        <xsl:attribute name="customer_id"><xsl:value-of select="$colo-id"/></xsl:attribute>

        <xsl:variable name="user-info-exchanger-host" select="$user-info-exchanger-ref/@host"/>

        <cfg:UserInfoExchangerRef name="UserInfoExchanger">
          <xsl:choose>
            <xsl:when test="count($secure-params) > 0">
              <xsl:attribute name="ref">
                <xsl:value-of
                  select="concat('corbaloc:ssliop:', $user-info-exchanger-host, ':', $user-info-exchanger-ref/@port, '/UserInfoExchanger')"/>
              </xsl:attribute>
              <xsl:call-template name="ConvertSecureParams">
               <xsl:with-param name="secure-node" select="'true'"/>
               <xsl:with-param name="global-secure-node" select="$global-secure-params"/>
               <xsl:with-param name="config-root" select="$config-root"/>
               <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
             </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="ref">
                <xsl:value-of
                  select="concat('corbaloc:iiop:', $user-info-exchanger-host, ':', $user-info-exchanger-ref/@port, '/UserInfoExchanger')"/>
              </xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>
        </cfg:UserInfoExchangerRef>
      </cfg:UserInfoExchangerParameters>
    </xsl:if>

    <xsl:if test="($colo-config/cfg:coloParams/@backup_operations = 'true' or
                  $colo-config/cfg:coloParams/@backup_operations = '1') and
                  count($full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]) > 1">
      <cfg:UserOperationsBackup file_prefix="UserOp" rotate_period="60">
        <xsl:attribute name="dir"><xsl:value-of
          select="concat($workspace-root, '/log/UserInfoManager/Out/UserOp_', $cluster_id mod 2 + 1)"/></xsl:attribute>
      </cfg:UserOperationsBackup>
    </xsl:if>

    <cfg:UserOperationsLoad file_prefix="UserOp" check_period="60" threads="10">
      <xsl:attribute name="dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserInfoManager/In/UserOp_', $cluster_id)"/></xsl:attribute>
      <xsl:attribute name="unprocessed_dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserInfoManager/Out/UserOp_', $cluster_id)"/></xsl:attribute>
    </cfg:UserOperationsLoad>

    <cfg:ExternalUserOperationsLoad file_prefix="UserOp" check_period="60" threads="1">
      <xsl:attribute name="dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserInfoManager/In/ExternalUserOp')"/></xsl:attribute>
      <xsl:attribute name="unprocessed_dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserInfoManager/Out/ExternalUserOp')"/></xsl:attribute>
    </cfg:ExternalUserOperationsLoad>

    <cfg:FreqCaps>
      <xsl:attribute name="confirm_timeout"><xsl:if
        test="count($colo-config/cfg:freqCaps/@confirm_timeout) = 0">60</xsl:if><xsl:value-of
        select="$colo-config/cfg:freqCaps/@confirm_timeout"/>
      </xsl:attribute>
    </cfg:FreqCaps>

    <cfg:ReadWriteStats>
    </cfg:ReadWriteStats>
  </cfg:UserInfoManagerConfig>

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
    name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>

  <xsl:variable
    name="user-info-manager-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UserInfoManager: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoManager: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoManager: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-servers) = 0">
       <xsl:message terminate="yes"> UserInfoManager: Can't find campaign server node </xsl:message>
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
    name="stats-collector-path"
    select="$be-cluster-path/service[@descriptor = $stats-collector-descriptor]"/>

  <xsl:variable
    name="stats-collector-config"
    select="$stats-collector-path/configuration/cfg:statsCollector"/>

  <xsl:variable
    name="user-info-manager-config"
    select="$user-info-manager-path/configuration/cfg:userInfoManager"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root[1]"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="secure-files-root" select="concat('/', $out-dir, '/cert/')"/>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/UserInfoSvcs/UserInfoManagerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="UserInfoManagerConfigGenerator">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="user-info-manager-config" select="$user-info-manager-config"/>
      <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
      <xsl:with-param name="stats-collector-path" select="$stats-collector-path"/>
      <xsl:with-param name="stats-collector-config" select="$stats-collector-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

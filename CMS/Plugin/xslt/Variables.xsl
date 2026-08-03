<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:apd="http://foros.com/cms/applicationDescriptor"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  exclude-result-prefixes="dyn xsd cfg"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:variable name="app-version">
  <xsl:variable name="descriptor-doc" select="document('../AdServerAppDescriptor.xml')"/>
  <xsl:choose>
    <xsl:when test="count($descriptor-doc//apd:application) > 0">
      <xsl:value-of select="$descriptor-doc//apd:application/@version"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="alt-descriptor-doc" select="document('../../AdServerAppDescriptor.xml')"/>
      <xsl:value-of select="$alt-descriptor-doc//apd:application/@version"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:variable name="xsd-app-config-type"
  select="document('../xsd/AppConfigType.xsd')/xsd:schema/xsd:complexType[@name = 'AppConfigType']"/>
<xsl:variable name="xsd-snmp-config-type"
  select="document('../xsd/Commons/AdServerCommonsApp.xsd')/xsd:schema/xsd:complexType[@name = 'SNMPStatsConfigType']"/>
<xsl:variable name="xsd-zone-management-type"
  select="document('../xsd/AppConfigType.xsd')/xsd:schema/xsd:complexType[@name = 'ZoneManagementType']"/>
<xsl:variable name="xsd-isp-zone-management-type"
  select="document('../xsd/AppConfigType.xsd')/xsd:schema/xsd:complexType[@name = 'ispZoneManagementType']/xsd:complexContent/xsd:extension"/>
<xsl:variable name="xsd-webserver-params-type"
  select="document('../xsd/AdFrontendAppType.xsd')/xsd:schema/xsd:complexType[@name = 'WebServerParamsType']"/>
<xsl:variable name="xsd-frontend-logging-params-type"
  select="document('../xsd/AdFrontendAppType.xsd')/xsd:schema/xsd:complexType[@name='FrontendLoggingParamsType']"/>
<xsl:variable name="xsd-campaign-server-update-params-type"
  select="document('../xsd/CampaignManagement/CampaignServerAppType.xsd')/xsd:schema/xsd:complexType[@name='CampaignServerUpdateParamsType']"/>

<xsl:variable name="current-campaign-server-obj" select="'/CampaignServer_v360'"/>

<!-- these services used in tr zone and must have unique version for all changes -->
<xsl:variable name="app-version-num" select="translate($app-version, '0123456789.', '0123456789')"/>
<xsl:variable name="current-campaign-manager-obj" select="concat('CampaignManager_v', $app-version-num)"/>
<xsl:variable name="current-user-info-manager-obj" select="'UserInfoManager_v351'"/>
<xsl:variable name="current-user-bind-server-obj" select="concat('UserBindServer_v', $app-version-num)"/>

<xsl:variable name="colocation-name" select="/colo:colocation/@name"/>
<xsl:variable name="colo-name" select="translate($colocation-name,
  'ABCDEFGHIJKLMNOPQRSTUVWXYZ ',
  'abcdefghijklmnopqrstuvwxyz_')"/>

<xsl:variable name="ad-cluster-descriptor" select="'AdCluster'"/>
<xsl:variable name="ad-proxycluster-descriptor" select="'AdProxyCluster'"/>
<xsl:variable name="be-cluster-descriptor" select="'AdCluster/BackendSubCluster'"/>
<xsl:variable name="fe-cluster-descriptor" select="'AdCluster/FrontendSubCluster'"/>
<xsl:variable name="tests-descriptor" select="'AdCluster/Tests'"/>
<xsl:variable name="ad-profilingcluster-descriptor" select="'AdProfilingCluster'"/>

<xsl:variable name="log-processing-descriptor" select="'AdCluster/BackendSubCluster/LogProcessing'"/>
<xsl:variable name="local-proxy-descriptor" select="'AdCluster/BackendSubCluster/LocalProxy'"/>
<xsl:variable name="user-profiling-descriptor" select="'AdCluster/BackendSubCluster/UserInfoProfiling'"/>
<xsl:variable name="predictor-descriptor" select="'AdCluster/BackendSubCluster/Predictor'"/>

<xsl:variable name="campaign-server-descriptor"
  select="'AdCluster/BackendSubCluster/CampaignServer'"/>

<xsl:variable name="user-info-manager-descriptor"
  select="'AdCluster/FrontendSubCluster/UserInfoManager'"/>

<xsl:variable name="dictionary-provider-descriptor"
  select="'AdCluster/BackendSubCluster/DictionaryProvider'"/>

<xsl:variable name="user-bind-server-descriptor"
  select="'AdCluster/FrontendSubCluster/UserBindServer'"/>
  
<xsl:variable name="user-bind-controller-descriptor"
  select="'AdCluster/FrontendSubCluster/UserBindController'"/>

<xsl:variable name="user-info-controller-descriptor"
  select="'AdCluster/FrontendSubCluster/UserInfoController'"/>

<xsl:variable name="billing-server-descriptor"
  select="'AdCluster/FrontendSubCluster/BillingServer'"/>

<xsl:variable name="log-generalizer-descriptor" select="'AdCluster/BackendSubCluster/LogGeneralizer'"/>
<xsl:variable name="expression-matcher-descriptor" select="'AdCluster/BackendSubCluster/ExpressionMatcher'"/>
<xsl:variable name="request-info-manager-descriptor" select="'AdCluster/BackendSubCluster/RequestInfoManager'"/>
<xsl:variable name="clickhouse-uploader-descriptor" select="'AdCluster/BackendSubCluster/ClickhouseUploader'"/>
<xsl:variable name="stat-receiver-descriptor" select="'AdCluster/BackendSubCluster/StatReceiver'"/>

<xsl:variable name="campaign-manager-descriptor" select="'AdCluster/FrontendSubCluster/CampaignManager'"/>

<xsl:variable name="channel-server-descriptor" select="'AdCluster/FrontendSubCluster/ChannelServer'"/>
<xsl:variable name="channel-controller-descriptor" select="'AdCluster/FrontendSubCluster/ChannelController'"/>
<xsl:variable name="channel-search-service-descriptor" select="'AdCluster/FrontendSubCluster/ChannelSearchService'"/>

<xsl:variable name="default-kafka-threads" select="'20'"/>
<xsl:variable name="default-kafka-message-queue-size" select="'1000000'"/>
<xsl:variable name="http-frontend-descriptor" select="'AdCluster/FrontendSubCluster/HttpFrontend'"/>

<!-- remote specific descriptors -->
<xsl:variable name="remote-stunnel-client-descriptor" select="'AdCluster/BackendSubCluster/LocalProxy/STunnelClient'"/>

<!-- pbe specific descriptors -->
<xsl:variable name="pbe-campaign-server-descriptor" select="'AdProxyCluster/CampaignServer'"/>
<xsl:variable name="pbe-stunnel-server-descriptor" select="'AdProxyCluster/STunnelServer'"/>

<!-- logger specific params -->

<xsl:variable name="default-log-level" select="'7'"/>
<xsl:variable name="default-syslog-level" select="'4'"/>
<xsl:variable name="default-error-log-level" select="'4'"/>
<xsl:variable name="default-fcgiserver-log-level" select="'15'"/>
<xsl:variable name="def-sys-log" select="'true'"/>
<xsl:variable name="def-rotate-time" select="'1440'"/>
<xsl:variable name="def-rotate-size" select="'100'"/>

<xsl:variable name="campaign-manager-log-level" select="$default-log-level"/>
<xsl:variable name="campaign-manager-log-path" select="'/log/CampaignManager/CampaignManager'"/>
<xsl:variable name="campaign-server-log-level" select="$default-log-level"/>
<xsl:variable name="campaign-server-log-path" select="'/log/CampaignServer/CampaignServer'"/>
<xsl:variable name="billing-server-log-level" select="$default-log-level"/>
<xsl:variable name="billing-server-log-path" select="'/log/BillingServer/BillingServer'"/>
<xsl:variable name="dictionary-provider-log-path" select="'/log/DictionaryProvider/DictionaryProvider'"/>
<xsl:variable name="dictionary-provider-log-level" select="$default-log-level"/>
<xsl:variable name="user-bind-server-log-path" select="'/log/UserBindServer/UserBindServer'"/>
<xsl:variable name="user-bind-server-log-level" select="$default-log-level"/>
<xsl:variable name="user-bind-controller-log-path" select="'/log/UserBindController/UserBindController'"/>
<xsl:variable name="user-bind-controller-log-level" select="$default-log-level"/>
<xsl:variable name="channel-server-log-level" select="$default-log-level"/>
<xsl:variable name="channel-server-log-path" select="'/log/ChannelServer/ChannelServer'"/>
<xsl:variable name="channel-controller-log-level" select="$default-log-level"/>
<xsl:variable name="channel-controller-log-path" select="'/log/ChannelController/ChannelController'"/>
<xsl:variable name="channel-search-service-log-level" select="$default-log-level"/>
<xsl:variable name="channel-search-service-log-path" select="'/log/ChannelSearchService/ChannelSearchService'"/>
<xsl:variable name="sync-logs-log-level" select="$default-log-level"/>
<xsl:variable name="def-sync-logs-server-log-level" select="'1'"/>
<xsl:variable name="sync-logs-log-path" select="'/log/SyncLogs/SyncLogs'"/>
<xsl:variable name="expression-matcher-log-level" select="$default-log-level"/>
<xsl:variable name="expression-matcher-log-path" select="'/log/ExpressionMatcher/ExpressionMatcher'"/>
<xsl:variable name="log-generalizer-log-level" select="$default-log-level"/>
<xsl:variable name="log-generalizer-log-path" select="'/log/LogGeneralizer/LogGeneralizer'"/>
<xsl:variable name="request-info-manager-log-level" select="$default-log-level"/>
<xsl:variable name="request-info-manager-log-path" select="'/log/RequestInfoManager/RequestInfoManager'"/>
<xsl:variable name="clickhouse-uploader-log-path" select="'/log/ClickhouseUploader/ClickhouseUploader'"/>
<xsl:variable name="user-info-manager-log-level" select="$default-log-level"/>
<xsl:variable name="user-info-manager-log-path" select="'/log/UserInfoManager/UserInfoManager'"/>
<xsl:variable name="user-info-controller-log-path" select="'/log/UserInfoController/UserInfoController'"/>
<xsl:variable name="fcgi-adserver-log-path" select="'/log/FCGIAdServer/FCGIAdServer'"/>

<xsl:variable name="fcgi-trackserver-log-path" select="'/log/FCGITrackServer/FCGITrackServer'"/>
<xsl:variable name="fcgi-actionserver-log-path" select="'/log/FCGIActionServer/FCGIActionServer'"/>

<xsl:variable name="fcgi-rtbserver-log-path" select="'/log/FCGIRtbServer/FCGIRtbServer'"/>

<xsl:variable name="fcgi-userbindserver-log-path" select="'/log/FCGIUserBindServer/FCGIUserBindServer'"/>

<xsl:variable name="request-module-log-level" select="$default-log-level"/>
<xsl:variable name="adinst-module-log-level" select="$default-log-level"/>
<xsl:variable name="content-module-log-level" select="$default-log-level"/>
<xsl:variable name="impression-module-log-level" select="$default-log-level"/>
<xsl:variable name="click-module-log-level" select="$default-log-level"/>
<xsl:variable name="action-module-log-level" select="$default-log-level"/>
<xsl:variable name="optout-module-log-level" select="$default-log-level"/>
<xsl:variable name="pass-module-log-level" select="$default-log-level"/>
<xsl:variable name="pass-pixel-module-log-level" select="$default-log-level"/>
<xsl:variable name="pref-module-log-level" select="$default-log-level"/>
<xsl:variable name="adop-module-log-level" select="$default-log-level"/>
<xsl:variable name="webstat-module-log-level" select="$default-log-level"/>
<xsl:variable name="pubpixel-module-log-level" select="$default-log-level"/>
<xsl:variable name="bidding-module-log-level" select="$default-log-level"/>
<xsl:variable name="userbind-module-log-level" select="$default-log-level"/>
<xsl:variable name="profiling-module-log-level" select="$default-log-level"/>

<!-- threadParams specific params -->
<xsl:variable name="def-log-generalizer-threads" select="'10'"/>
<xsl:variable name="def-request-info-manager-threads" select="'10'"/>
<xsl:variable name="def-sync-logs-threads" select="'10'"/>
<xsl:variable name="def-expression-matcher-threads" select="'10'"/>
<xsl:variable name="def-campaign-server-threads" select="'10'"/>
<xsl:variable name="def-campaign-manager-threads" select="'40'"/>
<xsl:variable name="def-user-info-manager-controller-threads" select="'10'"/>
<xsl:variable name="def-user-info-manager-threads" select="'40'"/>
<xsl:variable name="def-channel-controller-threads" select="'10'"/>
<xsl:variable name="def-dictionary-provider-threads" select="'10'"/>
<xsl:variable name="def-user-bind-server-threads" select="'40'"/>
<xsl:variable name="def-user-bind-controller-threads" select="'40'"/>
<xsl:variable name="def-channel-search-threads" select="'10'"/>
<xsl:variable name="def-channel-server-threads" select="'40'"/>

<!-- updateParams specific params -->
<xsl:variable name="campaign-manager-update-period" select="'180'"/>
<xsl:variable name="campaign-manager-ecpm-update-period" select="'180'"/>
<xsl:variable name="campaign-manager-campaign-types" select="'all'"/>
<xsl:variable name="channel-server-update-memory-size" select="'2000'"/>
<xsl:variable name="channel-server-update-chunks-count" select="'32'"/>
<xsl:variable name="channel-server-update-channels-period" select="'30'"/>
<xsl:variable name="expression-matcher-update-period" select="'30'"/>
<xsl:variable name="user-info-manager-channels-update-period" select="'30'"/>

<!-- scaleParams specific params -->
<xsl:variable name="channel-serving-scale-chunks" select="'10'"/>
<xsl:variable name="user-info-manager-scale-chunks" select="'10'"/>
<xsl:variable name="user-bind-server-scale-chunks" select="'10'"/>

<!-- statLogging specific params -->
<xsl:variable name="campaign-manager-flush-loggers-period"
  select="document('../xsd/CampaignManagement/CampaignManagerAppType.xsd')/xsd:schema/xsd:complexType[
  @name='CampaignManagerStatLoggingType']/xsd:attribute[
  @name='flush_period']/@default"/>
<xsl:variable name="campaign-manager-flush-internal-loggers-period"
  select="document('../xsd/CampaignManagement/CampaignManagerAppType.xsd')/xsd:schema/xsd:complexType[
  @name='CampaignManagerStatLoggingType']/xsd:attribute[
  @name='internal_logs_flush_period']/@default"/>
<xsl:variable name="expression-matcher-flush-logs-period" select="'10'"/>
<xsl:variable name="expression-matcher-activity-flush-logs-period" select="'10'"/>
<xsl:variable name="expression-matcher-inventory-flush-logs-period"
  select="document('../xsd/LogProcessing/ExpressionMatcherAppType.xsd')/xsd:schema/xsd:complexType[
  @name='ExpressionMatcherStatLoggingType']/xsd:attribute[
  @name='inventory_ecpm_flush_period']/@default"/>
<xsl:variable name="expression-matcher-check-logs-period" select="'10'"/>
<xsl:variable name="log-generalizer-check-logs-period" select="'10'"/>
<xsl:variable name="log-generalizer-check-deferred-logs-period" select="'20'"/>
<xsl:variable name="log-generalizer-flush-logs-period" select="'10'"/>
<xsl:variable name="log-generalizer-flush-logs-size" select="'50000'"/>
<xsl:variable name="log-generalizer-search-term-count-threshold" select="'10'"/>
<xsl:variable name="log-generalizer-upload-tasks" select="'2'"/>
<xsl:variable name="request-info-manager-check-logs-period" select="'10'"/>
<xsl:variable name="request-info-manager-flush-logs-period" select="'10'"/>
<xsl:variable name="def-sync-log-content-check-period"
  select="document('../xsd/LogProcessing/SyncLogsAppType.xsd')/xsd:schema/xsd:complexType[
  @name='SyncLogsFileTransferringType']/xsd:attribute[
  @name='content_check_period']/@default"/>

<xsl:variable name="inventory-users-percentage" select="'100'"/>
<xsl:variable name="def-ignore-action-time" select="'30'"/>

<!-- processing specific params -->
<xsl:variable name="expression-matcher-processing-threads" select="'20'"/>
<xsl:variable name="request-info-manager-processing-threads" select="'10'"/>
<xsl:variable name="request-info-manager-fetch-threads" select="'10'"/>

<!-- environment specific params -->
<xsl:variable name="def-workspace-root" select="'/opt/foros/server/var'"/>
<xsl:variable name="def-config-root" select="'/opt/foros/server/etc'"/>
<xsl:variable name="def-cache-root" select="'/opt/foros/server/var/cache'"/>
<xsl:variable name="def-server-root" select="'/opt/foros/server'"/>
<xsl:variable name="def-unixcommons-root" select="'/opt/foros/server'"/>
<xsl:variable name="def-data-root" select="'/opt/foros/server/var/www'"/>
<xsl:variable name="def-user-name" select="'aduser'"/>
<xsl:variable name="def-user-group" select="'adgroup'"/>

<!-- web server specific params -->
<xsl:variable name="web-server-timeout" select="'3'"/>
<xsl:variable name="web-server-keep-alive" select="'true'"/>
<xsl:variable name="web-server-min-spare-threads" select="'1'"/>
<xsl:variable name="web-server-max-spare-threads" select="'60'"/>
<xsl:variable name="web-server-threads-per-process" select="'30'"/>
<xsl:variable name="web-server-request-headers-reading-timeout" select="'10'"/>
<xsl:variable name="web-server-request-body-reading-timeout" select="'10'"/>

<xsl:variable name="def-request-session-timeout" select="'30'"/>
<xsl:variable name="def-request-update-period" select="'60'"/>

<xsl:variable name="def-ad-module-threads" select="'128'"/>
<xsl:variable name="def-bidding-module-threads" select="'32'"/>
<xsl:variable name="def-impression-module-threads" select="'32'"/>
<xsl:variable name="def-impression-module-match-threads" select="'32'"/>

<xsl:variable name="def-userbind-module-threads" select="'32'"/>
<xsl:variable name="def-userbind-module-match-threads" select="'32'"/>
<xsl:variable name="def-userbind-module-match-task-limit" select="'1000'"/>
<xsl:variable name="def-userbind-module-bind-task-limit" select="'1000'"/>

<!-- default profile cleanup params -->
<xsl:variable name="def-profile-lifetime" select="'180'"/>
<xsl:variable name="def-temp-profile-lifetime" select="'30'"/>
<xsl:variable name="def-profiles-cleanup-time" select="'00:01'"/>
<xsl:variable name="def-clean-user-profiles" select="'true'"/>

<xsl:variable name="def-keyword-match-user-profile-life-time" select="'604800'"/>
<xsl:variable name="def-temporary-keyword-match-user-profile-life-time" select="'86400'"/>
<xsl:variable name="def-keyword-match-impression-profile-life-time" select="'86400'"/>

<!-- default match params -->
<xsl:variable name="def-session-timeout" select="'30'"/>
<xsl:variable name="def-history-optimization-period" select="'0'"/>
<xsl:variable name="def-status-check-period" select="'10'"/>
<xsl:variable name="def-repeat-trigger-timeout" select="'60'"/>

<!-- default file transfering params -->
<xsl:variable name="def-check-logs-period" select="'10'"/>
<xsl:variable name="def-host-check-period" select="'10'"/>

<!-- default ports for services -->
<xsl:variable name="configured-base-port"
  select="(//cfg:coloParams[@base_port] | //colo:coloParams[@base_port])[1]/@base_port"/>
<xsl:variable name="def-range-start">
  <xsl:choose>
    <xsl:when test="count($configured-base-port) > 0">
      <xsl:value-of select="$configured-base-port"/>
    </xsl:when>
    <xsl:otherwise>10100</xsl:otherwise>
  </xsl:choose>
</xsl:variable>
<xsl:variable name="def-user-info-manager-port" select="$def-range-start + 1"/>
<xsl:variable name="def-user-info-manager-controller-port" select="$def-range-start + 2"/>
<xsl:variable name="def-user-info-controller-port" select="$def-range-start + 36"/>
<xsl:variable name="def-user-info-controller-grpc-port" select="$def-range-start + 431"/>
<xsl:variable name="def-channel-server-port" select="$def-range-start + 3"/>
<xsl:variable name="def-channel-controller-port" select="$def-range-start + 430"/>
<xsl:variable name="def-channel-controller-grpc-port" select="$def-range-start + 430"/>
<xsl:variable name="def-campaign-server-port" select="$def-range-start + 6"/>
<xsl:variable name="def-campaign-manager-port" select="$def-range-start + 7"/>
<xsl:variable name="def-channel-search-service-port" select="$def-range-start + 9"/>
<xsl:variable name="def-dictionary-provider-port" select="$def-range-start + 10"/>
<xsl:variable name="def-user-bind-server-port" select="$def-range-start + 28"/>
<xsl:variable name="def-user-bind-controller-port" select="$def-range-start + 35"/>
<xsl:variable name="def-user-bind-controller-grpc-port" select="$def-range-start + 429"/>
<xsl:variable name="def-billing-server-port" select="$def-range-start + 31"/>
<xsl:variable name="def-billing-server-grpc-port" select="$def-range-start + 432"/>
<xsl:variable name="def-log-generalizer-port" select="$def-range-start + 11"/>
<!--xsl:variable name="def-log-delivering-port" select="concat($def-range-start, '112')"/-->
<xsl:variable name="def-sync-logs-port" select="$def-range-start + 12"/>
<xsl:variable name="def-expression-matcher-port" select="$def-range-start + 13"/>
<xsl:variable name="def-sync-logs-server-port" select="$def-range-start + 14"/>
<xsl:variable name="def-stat-receiver-port" select="$def-range-start + 15"/>
<xsl:variable name="def-request-info-manager-port" select="$def-range-start + 16"/>
<xsl:variable name="def-stunnel-client-port" select="$def-range-start + 21"/>
<xsl:variable name="def-predictor-sync-logs-server-port" select="$def-range-start + 68"/>

<xsl:variable name="def-frontend-port" select="$def-range-start + 180"/>
<xsl:variable name="def-secure-frontend-port" select="$def-range-start + 43"/>
<xsl:variable name="def-stunnel-server-port" select="$def-range-start + 100"/>

<!-- default ports for proxy services -->
<xsl:variable name="def-proxy-campaign-server-port" select="$def-range-start + 56"/>
<xsl:variable name="def-proxy-sync-logs-port" select="$def-range-start + 62"/>
<xsl:variable name="def-stunnel-server-internal-port" select="$def-range-start + 72"/>

<!-- default secure params -->
<xsl:variable name="default-secure-params-key" select="'skey.pem'"/>
<xsl:variable name="default-secure-params-certificate" select="'scert.pem'"/>
<xsl:variable name="default-secure-params-password" select="'adserver'"/>
<xsl:variable name="default-secure-params-ca" select="'ca.pem'"/>
<xsl:variable name="default-p3p-header" select="'CP=&quot;NON COR PSAo PSDo OUR BUS UNI STA PRE&quot;'"/>

<xsl:variable name="default-distrib-count" select="'4'"/>

<xsl:variable name="def-stat-files-receiver-port" select="$def-range-start + 773"/>

<xsl:variable name="def-rsync-server-port" select="$def-range-start + 14"/>

<!-- default frontend variables -->
<xsl:variable name="def-frontend-redirect-expire-time" select="'900'"/>

<!-- default taskbot params -->
<xsl:variable name="default-taskbot-db-host" select="'taskbot.ocslab.com'"/>
<xsl:variable name="default-taskbot-db-name" select="'taskbot'"/>
<xsl:variable name="default-taskbot-db-user" select="'taskbot'"/>
<xsl:variable name="default-taskbot-db-password" select="'taskbot'"/>

<!-- fcgi ports -->
<xsl:variable name="def-fcgi-adserver-port" select="$def-range-start + 76"/>
<xsl:variable name="def-fcgi-adserver-mon-port" select="$def-range-start + 176"/>

<xsl:variable name="def-fcgi-rtbserver1-port" select="$def-range-start + 77"/>
<xsl:variable name="def-fcgi-rtbserver1-mon-port" select="$def-range-start + 177"/>
<xsl:variable name="def-fcgi-rtbserver-port" select="$def-fcgi-rtbserver1-port"/>
<xsl:variable name="def-fcgi-rtbserver-mon-port" select="$def-fcgi-rtbserver1-mon-port"/>
<xsl:variable name="def-fcgi-rtbserver2-port" select="$def-range-start + 96"/>
<xsl:variable name="def-fcgi-rtbserver2-mon-port" select="$def-range-start + 196"/>

<xsl:variable name="def-fcgi-userbindserver-port" select="$def-range-start + 78"/>
<xsl:variable name="def-fcgi-userbindserver-mon-port" select="$def-range-start + 178"/>


<xsl:variable name="def-fcgi-trackserver-port" select="$def-range-start + 95"/>
<xsl:variable name="def-fcgi-trackserver-mon-port" select="$def-range-start + 195"/>
<xsl:variable name="def-fcgi-actionserver-mon-port" select="$def-range-start + 198"/>

<xsl:variable name="def-storage-rw-buffer-size" select="10485760"/>
<xsl:variable name="def-storage-rwlevel-max-size" select="104857600"/>
<xsl:variable name="def-storage-max-undumped-size" select="262144000"/>
<xsl:variable name="def-storage-max-levels0" select="20"/>

<xsl:variable name="def-profiling-log-sampling"
  select="document('../xsd/AdClusterAppType.xsd')/xsd:schema/xsd:complexType[
  @name='ResearchStatType']/xsd:attribute[
  @name='profiling_log_sampling']/@default"/>

<!-- Predictor commons -->
<xsl:variable name="def-predictor-keep-logs" select="90"/>

<!-- Predictor merger defaults -->
<xsl:variable name="def-predictor-merger-timeout" select="3600"/>
<xsl:variable name="def-predictor-merger-imp-to" select="-8"/>
<xsl:variable name="def-predictor-merger-from" select="-8"/>
<xsl:variable name="def-predictor-merger-to" select="8"/>
<xsl:variable name="predictor-merger-log-path" select="'/log/Predictor/Merger'"/>
<xsl:variable name="def-predictor-merger-sleep_timeout" select="300"/>

<!-- BidCost predictor merger defaults -->
<xsl:variable name="bidcost-predictor-merger-log-path" select="'/log/Predictor/BidCostPredictorMerger/BidCostPredictorMerger'"/>
<xsl:variable name="def-bidcost-predictor-merger-aggregate-period" select="3600"/>
<xsl:variable name="def-bidcost-predictor-merger-analyze-days" select="180"/>
<xsl:variable name="def-bidcost-predictor-merger-generate-model-period" select="86400"/>

<!-- Predictor SVM generator defaults -->
<xsl:variable name="def-svm-generator-port" select="$def-range-start + 69"/>
<xsl:variable name="predictor-svm-generator-log-path" select="'/log/Predictor/SVMGenerator'"/>

</xsl:stylesheet>

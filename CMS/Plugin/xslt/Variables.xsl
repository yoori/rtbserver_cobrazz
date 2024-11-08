<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:apd="http://foros.com/cms/applicationDescriptor"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="dyn xsd"
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

<xsl:variable name="xsd-zenoss-type"
  select="document('../xsd/AppConfigType.xsd')/xsd:schema/xsd:complexType[@name = 'ZenOSSType']"/>
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
<xsl:variable name="current-channel-proxy-obj" select="'/ChannelProxy_v33'"/>
<xsl:variable name="current-user-bind-controller-obj" select="'/UserBindController'"/>

<!-- these services used in tr zone and must have unique version for all changes -->
<xsl:variable name="app-version-num" select="translate($app-version, '0123456789.', '0123456789')"/>
<xsl:variable name="current-campaign-manager-obj" select="concat('CampaignManager_v', $app-version-num)"/>
<xsl:variable name="current-conv-server-obj" select="concat('ConvServer_v', $app-version-num)"/>
<xsl:variable name="current-user-info-manager-obj" select="'UserInfoManager_v351'"/>
<xsl:variable name="current-user-info-manager-controller-obj" select="'UserInfoManagerController_v351'"/>
<xsl:variable name="current-user-bind-server-obj" select="concat('UserBindServer_v', $app-version-num)"/>
<xsl:variable name="current-billing-server-obj" select="'BillingServer_v355'"/>

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

<xsl:variable name="user-info-manager-controller-descriptor"
  select="'AdCluster/FrontendSubCluster/UserInfoManagerController'"/>

<xsl:variable name="dictionary-provider-descriptor"
  select="'AdCluster/BackendSubCluster/DictionaryProvider'"/>

<xsl:variable name="user-operation-generator-descriptor"
  select="'AdCluster/BackendSubCluster/UserOperationGenerator'"/>

<xsl:variable name="user-bind-server-descriptor"
  select="'AdCluster/FrontendSubCluster/UserBindServer'"/>
  
<xsl:variable name="user-bind-controller-descriptor"
  select="'AdCluster/FrontendSubCluster/UserBindController'"/>

<xsl:variable name="billing-server-descriptor"
  select="'AdCluster/FrontendSubCluster/BillingServer'"/>

<xsl:variable name="log-generalizer-descriptor" select="'AdCluster/BackendSubCluster/LogGeneralizer'"/>
<xsl:variable name="stat-uploader-descriptor" select="'AdCluster/BackendSubCluster/StatUploader'"/>
<xsl:variable name="ya-metrika-uploader-descriptor" select="'AdCluster/BackendSubCluster/YaMetrikaUploader'"/>
<xsl:variable name="expression-matcher-descriptor" select="'AdCluster/BackendSubCluster/ExpressionMatcher'"/>
<xsl:variable name="request-info-manager-descriptor" select="'AdCluster/BackendSubCluster/RequestInfoManager'"/>
<xsl:variable name="stat-receiver-descriptor" select="'AdCluster/BackendSubCluster/StatReceiver'"/>
<xsl:variable name="stats-collector-descriptor" select="'AdCluster/BackendSubCluster/StatsCollector'"/>

<xsl:variable name="campaign-manager-descriptor" select="'AdCluster/FrontendSubCluster/CampaignManager'"/>
<xsl:variable name="conv-server-descriptor" select="'AdCluster/FrontendSubCluster/ConvServer'"/>

<xsl:variable name="channel-server-descriptor" select="'AdCluster/FrontendSubCluster/ChannelServer'"/>
<xsl:variable name="channel-controller-descriptor" select="'AdCluster/FrontendSubCluster/ChannelController'"/>
<xsl:variable name="channel-search-service-descriptor" select="'AdCluster/FrontendSubCluster/ChannelSearchService'"/>

<xsl:variable name="profiling-server-descriptor" select="'AdCluster/FrontendSubCluster/ProfilingServer'"/>
<xsl:variable name="uid-generator-adapter-descriptor" select="'AdCluster/FrontendSubCluster/UIDGeneratorAdapter'"/>
<xsl:variable name="zmq-profiling-balancer-descriptor" select="'AdCluster/FrontendSubCluster/ZmqProfilingBalancer'"/>
<xsl:variable name="default-uid-generator-topic" select="'uidgenerator'"/>
<xsl:variable name="default-click-stream-topic" select="'clickstream'"/>
<xsl:variable name="default-geo-topic" select="'geo'"/>
<xsl:variable name="default-ads-spaces-topic" select="'adsspaces'"/>
<xsl:variable name="default-match-topic" select="'match'"/>
<xsl:variable name="default-kafka-threads" select="'20'"/>
<xsl:variable name="default-kafka-message-queue-size" select="'1000000'"/>

<xsl:variable name="http-frontend-descriptor" select="'AdCluster/FrontendSubCluster/HttpFrontend'"/>

<!-- remote specific descriptors -->
<xsl:variable name="remote-channel-proxy-descriptor" select="'AdCluster/BackendSubCluster/LocalProxy/ChannelProxy'"/>
<xsl:variable name="remote-stunnel-client-descriptor" select="'AdCluster/BackendSubCluster/LocalProxy/STunnelClient'"/>

<!-- pbe specific descriptors -->
<xsl:variable name="pbe-campaign-server-descriptor" select="'AdProxyCluster/CampaignServer'"/>
<xsl:variable name="pbe-channel-proxy-descriptor" select="'AdProxyCluster/ChannelProxy'"/>
<xsl:variable name="pbe-stunnel-server-descriptor" select="'AdProxyCluster/STunnelServer'"/>
<xsl:variable name="pbe-user-info-exchanger-descriptor" select="'AdProxyCluster/UserInfoExchanger'"/>

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
<xsl:variable name="conv-server-log-level" select="$default-log-level"/>
<xsl:variable name="conv-server-log-path" select="'/log/ConvServer/ConvServer'"/>
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
<xsl:variable name="user-operation-generator-log-path" select="'/log/UserOperationGenerator/UserOperationGenerator'"/>

<xsl:variable name="channel-proxy-log-level" select="$default-log-level"/>
<xsl:variable name="channel-proxy-log-path" select="'/log/ChannelProxy/ChannelProxy'"/>
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
<xsl:variable name="user-info-manager-log-level" select="$default-log-level"/>
<xsl:variable name="user-info-manager-log-path" select="'/log/UserInfoManager/UserInfoManager'"/>
<xsl:variable name="user-info-manager-controller-log-level" select="$default-log-level"/>
<xsl:variable name="user-info-manager-controller-log-path" select="'/log/UserInfoManagerController/UserInfoManagerController'"/>
<xsl:variable name="user-info-exchanger-log-level" select="$default-log-level"/>
<xsl:variable name="user-info-exchanger-log-path" select="'/log/UserInfoExchanger/UserInfoExchanger'"/>
<xsl:variable name="stats-collector-log-path" select="'/log/StatsCollector/StatsCollector'"/>
<xsl:variable name="profiling-server-log-path" select="'/log/ProfilingServer/ProfilingServer'"/>
<xsl:variable name="zmq-profiling-balancer-log-path" select="'/log/ZmqProfilingBalancer/ZmqProfilingBalancer'"/>
<xsl:variable name="uid-generator-adapter-log-level" select="$default-log-level"/>
<xsl:variable name="uid-generator-adapter-log-path" select="'/log/UIDGeneratorAdapter/UIDGeneratorAdapter'"/>
<xsl:variable name="fcgi-adserver-log-path" select="'/log/FCGIAdServer/FCGIAdServer'"/>
<xsl:variable name="fcgi-trackserver-log-path" select="'/log/FCGITrackServer/FCGITrackServer'"/>
<xsl:variable name="fcgi-rtbserver1-log-path" select="'/log/FCGIRtbServer1/FCGIRtbServer1'"/>
<xsl:variable name="fcgi-rtbserver2-log-path" select="'/log/FCGIRtbServer2/FCGIRtbServer2'"/>

<xsl:variable name="http-adserver-log-path" select="'/log/HttpAdServer/HttpAdServer'"/>

<xsl:variable name="fcgi-userbindserver1-log-path" select="'/log/FCGIUserBindServer1/FCGIUserBindServer1'"/>
<xsl:variable name="fcgi-userbindserver2-log-path" select="'/log/FCGIUserBindServer2/FCGIUserBindServer2'"/>
<xsl:variable name="fcgi-userbindintserver-log-path" select="'/log/FCGIUserBindIntServer/FCGIUserBindIntServer'"/>
<xsl:variable name="fcgi-userbindaddserver-log-path" select="'/log/FCGIUserBindAddServer/FCGIUserBindAddServer'"/>

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

<xsl:variable name="stats-collector-log-level" select="$default-log-level"/>

<!-- threadParams specific params -->
<xsl:variable name="def-stats-collector-threads" select="'32'"/>
<xsl:variable name="def-log-generalizer-threads" select="'10'"/>
<xsl:variable name="def-request-info-manager-threads" select="'10'"/>
<xsl:variable name="def-sync-logs-threads" select="'10'"/>
<xsl:variable name="def-expression-matcher-threads" select="'10'"/>
<xsl:variable name="def-campaign-server-threads" select="'10'"/>
<xsl:variable name="def-campaign-manager-threads" select="'40'"/>
<xsl:variable name="def-conv-server-threads" select="'40'"/>
<xsl:variable name="def-user-info-exchanger-threads" select="'10'"/>
<xsl:variable name="def-user-info-manager-controller-threads" select="'10'"/>
<xsl:variable name="def-user-info-manager-threads" select="'40'"/>
<xsl:variable name="def-channel-controller-threads" select="'10'"/>
<xsl:variable name="def-channel-proxy-threads" select="'10'"/>
<xsl:variable name="def-dictionary-provider-threads" select="'10'"/>
<xsl:variable name="def-user-bind-server-threads" select="'40'"/>
<xsl:variable name="def-user-bind-controller-threads" select="'40'"/>
<xsl:variable name="def-channel-search-threads" select="'10'"/>
<xsl:variable name="def-channel-server-threads" select="'40'"/>

<!-- updateParams specific params -->
<xsl:variable name="campaign-manager-update-period" select="'180'"/>
<xsl:variable name="campaign-manager-ecpm-update-period" select="'180'"/>
<xsl:variable name="campaign-manager-campaign-types" select="'all'"/>
<xsl:variable name="channel-server-update-memory-size" select="'100'"/>
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
<xsl:variable name="campaign-manager-flush-loggers-archive" select="'no_compression'"/>
<xsl:variable name="expression-matcher-flush-logs-period" select="'10'"/>
<xsl:variable name="expression-matcher-flush-logs-archive" select="'no_compression'"/>
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
<xsl:variable name="stat-uploader-check-logs-period" select="'10'"/>
<xsl:variable name="ya-metrika-uploader-check-logs-period" select="'10'"/>
<xsl:variable name="request-info-manager-check-logs-period" select="'10'"/>
<xsl:variable name="request-info-manager-flush-logs-period" select="'10'"/>
<xsl:variable name="request-info-manager-flush-logs-archive" select="'no_compression'"/>
<xsl:variable name="def-sync-log-content-check-period"
  select="document('../xsd/LogProcessing/SyncLogsAppType.xsd')/xsd:schema/xsd:complexType[
  @name='SyncLogsFileTransferringType']/xsd:attribute[
  @name='content_check_period']/@default"/>

<xsl:variable name="inventory-users-percentage" select="'100'"/>
<xsl:variable name="def-ignore-action-time" select="'30'"/>

<!-- processing specific params -->
<xsl:variable name="expression-matcher-processing-threads" select="'20'"/>
<xsl:variable name="request-info-manager-processing-threads" select="'10'"/>

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

<xsl:variable name="def-bidding-grpc-enable" select="'true'"/>
<xsl:variable name="def-bidding-module-threads" select="'1000'"/>

<xsl:variable name="def-ad-frontend-grpc-enable" select="'true'"/>

<xsl:variable name="def-userbind-module-threads" select="'1000'"/>
<xsl:variable name="def-userbind-module-match-threads" select="'100'"/>
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
<xsl:variable name="def-history-optimization-period" select="'3600'"/>
<xsl:variable name="def-status-check-period" select="'10'"/>
<xsl:variable name="def-repeat-trigger-timeout" select="'60'"/>

<!-- default file transfering params -->
<xsl:variable name="def-check-logs-period" select="'10'"/>
<xsl:variable name="def-host-check-period" select="'10'"/>

<!-- default ports for services -->
<xsl:variable name="def-range-start" select="10100"/>
<xsl:variable name="def-user-info-manager-port" select="$def-range-start + 1"/>
<xsl:variable name="def-user-info-manager-controller-port" select="$def-range-start + 2"/>
<xsl:variable name="def-user-info-manager-grpc-port" select="$def-range-start + 201"/>
<xsl:variable name="def-campaign-manager-grpc-port" select="$def-range-start + 202"/>

<xsl:variable name="def-channel-server-port" select="$def-range-start + 3"/>
<xsl:variable name="def-channel-server-grpc-port" select="$def-range-start + 203"/>

<xsl:variable name="def-channel-controller-port" select="$def-range-start + 4"/>
<xsl:variable name="def-campaign-server-port" select="$def-range-start + 6"/>
<xsl:variable name="def-campaign-manager-port" select="$def-range-start + 7"/>
<xsl:variable name="def-conv-server-port" select="$def-range-start + 34"/>
<xsl:variable name="def-channel-search-service-port" select="$def-range-start + 9"/>
<xsl:variable name="def-dictionary-provider-port" select="$def-range-start + 10"/>

<xsl:variable name="def-user-bind-server-port" select="$def-range-start + 28"/>
<xsl:variable name="def-user-bind-server-grpc-port" select="$def-range-start + 228"/>
<xsl:variable name="def-user-bind-server-mon-port" select="$def-range-start + 328"/>

<xsl:variable name="def-user-bind-controller-port" select="$def-range-start + 29"/>
<xsl:variable name="def-user-operation-generator-port" select="$def-range-start + 30"/>
<xsl:variable name="def-billing-server-port" select="$def-range-start + 31"/>
<xsl:variable name="def-log-generalizer-port" select="$def-range-start + 11"/>
<!--xsl:variable name="def-log-delivering-port" select="concat($def-range-start, '112')"/-->
<xsl:variable name="def-sync-logs-port" select="$def-range-start + 12"/>

<xsl:variable name="def-expression-matcher-port" select="$def-range-start + 13"/>
<xsl:variable name="def-expression-matcher-mon-port" select="$def-range-start + 313"/>

<xsl:variable name="def-sync-logs-server-port" select="$def-range-start + 14"/>
<xsl:variable name="def-stat-receiver-port" select="$def-range-start +15"/>

<xsl:variable name="def-request-info-manager-port" select="$def-range-start + 16"/>
<xsl:variable name="def-request-info-manager-mon-port" select="$def-range-start + 316"/>

<!--xsl:variable name="def-local-channel-proxy-port" select="concat($def-range-start, '120')"/-->
<xsl:variable name="def-stunnel-client-port" select="$def-range-start + 21"/>
<xsl:variable name="def-uid-generator-adapter-port" select="$def-range-start + 32"/>
<xsl:variable name="def-uid-generator-adapter-input-port" select="$def-range-start + 33"/>
<xsl:variable name="def-predictor-sync-logs-server-port" select="$def-range-start + 68"/>
<xsl:variable name="def-frontend-port" select="$def-range-start + 180"/>
<xsl:variable name="def-secure-frontend-port" select="$def-range-start + 43"/>
<xsl:variable name="def-stunnel-server-port" select="'10200'"/>

<xsl:variable name="def-stats-collector-port" select="$def-range-start + 18"/>
<xsl:variable name="def-stats-dumping-period" select="'60'"/>

<!-- default ports for proxy services -->
<xsl:variable name="def-proxy-campaign-server-port" select="'10156'"/>
<xsl:variable name="def-user-info-exchanger-port" select="'10160'"/>
<xsl:variable name="def-channel-proxy-port" select="'10155'"/>
<xsl:variable name="def-proxy-sync-logs-port" select="'10162'"/>
<xsl:variable name="def-stunnel-server-internal-port" select="'10172'"/>

<!-- default secure params -->
<xsl:variable name="default-secure-params-key" select="'skey.pem'"/>
<xsl:variable name="default-secure-params-certificate" select="'scert.pem'"/>
<xsl:variable name="default-secure-params-password" select="'adserver'"/>
<xsl:variable name="default-secure-params-ca" select="'ca.pem'"/>
<xsl:variable name="default-p3p-header" select="'CP=&quot;NON COR PSAo PSDo OUR BUS UNI STA PRE&quot;'"/>

<xsl:variable name="default-distrib-count" select="'4'"/>

<xsl:variable name="def-stat-files-receiver-port" select="'10873'"/>

<xsl:variable name="def-rsync-server-port" select="'10114'"/>

<!-- default frontend variables -->
<xsl:variable name="def-frontend-redirect-expire-time" select="'900'"/>

<!-- default taskbot params -->
<xsl:variable name="default-taskbot-db-host" select="'taskbot.ocslab.com'"/>
<xsl:variable name="default-taskbot-db-name" select="'taskbot'"/>
<xsl:variable name="default-taskbot-db-user" select="'taskbot'"/>
<xsl:variable name="default-taskbot-db-password" select="'taskbot'"/>

<!-- default profiling cluster params -->
<xsl:variable name="def-zmq-profiling-balancer-port" select="$def-range-start + 74"/>
<xsl:variable name="def-zmq-profiling-balancer-profiling-info-port" select="$def-range-start + 88"/>
<xsl:variable name="def-zmq-profiling-balancer-anonymous-stats-port" select="$def-range-start + 89"/>
<xsl:variable name="def-zmq-profiling-balancer-dmp-profiling-info-port" select="$def-range-start + 91"/>

<xsl:variable name="def-profiling-server-port" select="$def-range-start + 75"/>
<xsl:variable name="def-zmq-profiling-server-profiling-info-port" select="$def-range-start + 86"/>
<xsl:variable name="def-zmq-profiling-server-anonymous-stats-port" select="$def-range-start + 87"/>
<xsl:variable name="def-zmq-profiling-server-dmp-profiling-info-port" select="$def-range-start + 90"/>

<!-- fcgi ports -->
<xsl:variable name="def-fcgi-adserver-port" select="$def-range-start + 76"/>
<xsl:variable name="def-fcgi-adserver-mon-port" select="$def-range-start + 376"/>

<xsl:variable name="def-fcgi-rtbserver1-port" select="$def-range-start + 77"/>
<xsl:variable name="def-fcgi-rtbserver1-mon-port" select="$def-range-start + 377"/>
<xsl:variable name="def-fcgi-rtbserver2-port" select="$def-range-start + 96"/>
<xsl:variable name="def-fcgi-rtbserver2-mon-port" select="$def-range-start + 396"/>

<xsl:variable name="def-fcgi-userbindserver1-port" select="$def-range-start + 78"/>
<xsl:variable name="def-fcgi-userbindserver1-mon-port" select="$def-range-start + 378"/>
<xsl:variable name="def-fcgi-userbindserver2-port" select="$def-range-start + 94"/>
<xsl:variable name="def-fcgi-userbindserver2-mon-port" select="$def-range-start + 394"/>

<xsl:variable name="def-fcgi-userbindintserver-port" select="$def-range-start + 79"/>
<xsl:variable name="def-fcgi-userbindintserver-mon-port" select="$def-range-start + 379"/>
<xsl:variable name="def-fcgi-userbindaddserver-port" select="$def-range-start + 93"/>
<xsl:variable name="def-fcgi-userbindaddserver-mon-port" select="$def-range-start + 393"/>
<xsl:variable name="def-fcgi-trackserver-port" select="$def-range-start + 95"/>
<xsl:variable name="def-fcgi-trackserver-mon-port" select="$def-range-start + 395"/>

<xsl:variable name="def-fcgi-userinfomanager-mon-port" select="$def-range-start + 196"/>

<!-- http ports -->
<xsl:variable name="def-http-adserver-port" select="$def-range-start + 99"/>
<xsl:variable name="def-http-adserver-mon-port" select="$def-range-start + 199"/>

<xsl:variable name="def-storage-rw-buffer-size" select="10485760"/>
<xsl:variable name="def-storage-rwlevel-max-size" select="104857600"/>
<xsl:variable name="def-storage-max-undumped-size" select="262144000"/>
<xsl:variable name="def-storage-max-levels0" select="20"/>

<xsl:variable name="def-is-level-enable" select="'true'"/>
<xsl:variable name="def-is-rocksdb-enable" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb" select="128"/>
<xsl:variable name="def-rocksdb-compaction-style" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads" select="3"/>

<xsl:variable name="def-is-rocksdb-enable-user-campaign-reach" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-user-campaign-reach" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-user-campaign-reach" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-user-campaign-reach" select="10"/>
<xsl:variable name="def-expire-time-user-campaign-reach" select="3888000"/>

<xsl:variable name="def-is-rocksdb-enable-user-fraud-protection" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-user-fraud-protection" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-user-fraud-protection" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-user-fraud-protection" select="10"/>
<xsl:variable name="def-expire-time-user-fraud-protection" select="3888000"/>

<xsl:variable name="def-is-rocksdb-enable-user-site-reach" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-user-site-reach" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-user-site-reach" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-user-site-reach" select="10"/>
<xsl:variable name="def-expire-time-user-site-reach" select="3888000"/>

<xsl:variable name="def-is-rocksdb-enable-tag-request-grouping" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-tag-request-grouping" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-tag-request-grouping" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-tag-request-grouping" select="10"/>
<xsl:variable name="def-expire-time-tag-request-grouping" select="3888000"/>

<xsl:variable name="def-is-rocksdb-enable-user-action" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-user-action" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-user-action" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-user-action" select="10"/>
<xsl:variable name="def-expire-time-user-action" select="3888000"/>

<xsl:variable name="def-is-rocksdb-enable-request-trigger-match" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-request-trigger-match" select="512"/>
<xsl:variable name="def-rocksdb-compaction-style-request-trigger-match" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-request-trigger-match" select="10"/>
<xsl:variable name="def-expire-time-request-trigger-match" select="172800"/>

<xsl:variable name="def-is-rocksdb-enable-request-info" select="'true'"/>
<xsl:variable name="def-rocksdb-block-cache-size-mb-request-info" select="1024"/>
<xsl:variable name="def-rocksdb-compaction-style-request-info" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-rocksdb-number-background-threads-request-info" select="20"/>
<xsl:variable name="def-expire-time-request-info" select="7776000"/>

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

<!-- BidCost predictor -->
<xsl:variable name="bidcost-predictor-version" select="'1.0'"/>
<xsl:variable name="bidcost-predictor-description" select="'Bid cost predictor'"/>
<xsl:variable name="bidcost-predictor-log-path" select="'/log/Predictor/BidCostPredictor/bid_cost_predictor.log'"/>
<xsl:variable name="bidcost-predictor-pid-path" select="'/run/bid_cost_predictor.pid'"/>
<xsl:variable name="bidcost-predictor-archive" select="'gzip_default'"/>
<xsl:variable name="bidcost-predictor-model-input-path" select="'/log/Predictor/ResearchLogs/BidCostStatAgg'"/>
<xsl:variable name="bidcost-predictor-model-period" select="43200"/>
<xsl:variable name="bidcost-predictor-model-bid-cost-file-name" select="'bid_cost.csv'"/>
<xsl:variable name="bidcost-predictor-model-bid-cost-output-directory" select="'/log/Predictor/BidCostConfig'"/>
<xsl:variable name="bidcost-predictor-model-bid-cost-temp-directory" select="'/log/Predictor/BidCostConfigTmp'"/>
<xsl:variable name="bidcost-predictor-model-ctr-file-name" select="'trivial_ctr.csv'"/>
<xsl:variable name="bidcost-predictor-model-ctr-output-directory" select="'/log/Predictor/CTRTrivialConfig'"/>
<xsl:variable name="bidcost-predictor-model-ctr-temp-directory" select="'/log/Predictor/CTRTrivialConfigTmp'"/>
<xsl:variable name="bidcost-predictor-model-ctr-max-imps" select="100000"/>
<xsl:variable name="bidcost-predictor-model-ctr-tag-imps" select="100"/>
<xsl:variable name="bidcost-predictor-model-ctr-trust-imps" select="10000"/>
<xsl:variable name="bidcost-predictor-aggregator-input-directory" select="'/log/Predictor/ResearchLogs/BidCostStat'"/>
<xsl:variable name="bidcost-predictor-aggregator-output-directory" select="'/log/Predictor/ResearchLogs/BidCostStatAgg'"/>
<xsl:variable name="bidcost-predictor-aggregator-period" select="3600"/>
<xsl:variable name="bidcost-predictor-aggregator-dump-max-size" select="100000"/>
<xsl:variable name="bidcost-predictor-aggregator-max-process-files" select="10000"/>
<xsl:variable name="bidcost-predictor-reaggregator-input-directory" select="'/log/Predictor/ResearchLogs/BidCostStatAgg'"/>
<xsl:variable name="bidcost-predictor-reaggregator-output-directory" select="'/log/Predictor/ResearchLogs/BidCostStatAgg'"/>
<xsl:variable name="bidcost-predictor-reaggregator-period" select="3600"/>
<xsl:variable name="bidcost-predictor-pg-connection-host" select="'postdb00'"/>
<xsl:variable name="bidcost-predictor-pg-connection-port" select="5432"/>
<xsl:variable name="bidcost-predictor-pg-connection-dbname" select="'stat'"/>
<xsl:variable name="bidcost-predictor-pg-connection-user" select="'ro'"/>
<xsl:variable name="bidcost-predictor-pg-connection-password" select="'Q1oL6mm5hPTjnDQ'"/>

<!-- Predictor SVM generator defaults -->
<xsl:variable name="def-svm-generator-port" select="$def-range-start + 69"/>
<xsl:variable name="predictor-svm-generator-log-path" select="'/log/Predictor/SVMGenerator'"/>

<!-- Coroutine params -->
<xsl:variable name="coro-pool-initial-size" select="10000"/>
<xsl:variable name="coro-pool-max-size" select="500000"/>
<xsl:variable name="coro-pool-stack-size" select="262144"/>
<xsl:variable name="event-thread-pool-number-threads" select="15"/>
<xsl:variable name="event-thread-pool-name" select="'event-worker'"/>
<xsl:variable name="event-thread-pool-ev-default-loop-disabled" select="'true'"/>
<xsl:variable name="event-thread-pool-defer-events" select="'false'"/>
<xsl:variable name="main-task-processor-name" select="'main_task_processor'"/>
<xsl:variable name="main-task-processor-number-threads" select="50"/>
<xsl:variable name="main-task-processor-should-guess-cpu-limit" select="'true'"/>
<xsl:variable name="main-task-processor-overload-action" select="'ignore'"/>
<xsl:variable name="main-task-processor-wait-queue-length-limit" select="500000"/>
<xsl:variable name="main-task-processor-wait-queue-time-limit" select="0"/>
<xsl:variable name="main-task-processor-sensor-wait-queue-time-limit" select="10000"/>

<!-- Grpc server params -->
<xsl:variable name="grpc-server-ip" select="'0.0.0.0'"/>

<!-- Grpc client pool params -->
<xsl:variable name="grpc-pool-client-num-channels" select="200"/>
<xsl:variable name="grpc-pool-client-num-clients" select="200"/>
<xsl:variable name="grpc-pool-client-timeout" select="1500"/>
<xsl:variable name="grpc-pool-client-enable" select="'false'"/>

<!-- Http server params -->
<xsl:variable name="http-backlog" select="2048"/>
<xsl:variable name="http-max-connections" select="65536"/>
<xsl:variable name="http-keepalive-timeout-seconds" select="600"/>
<xsl:variable name="http-in-buffer-size" select="32768"/>

<xsl:variable name="channel-matcher-cache-rocksdb-enable" select="'true'"/>
<xsl:variable name="channel-matcher-cache-rocksdb-block-cache-size-mb" select="300"/>
<xsl:variable name="channel-matcher-cache-rocksdb-ttl" select="86400"/>
<xsl:variable name="channel-matcher-cache-recheck-period" select="86400"/>

<!-- UserBindServer params -->
<xsl:variable name="def-user-bind-server-two-layers-enable" select="'false'"/>
<xsl:variable name="def-user-bind-server-memory-days" select="3"/>
<xsl:variable name="def-user-bind-server-rocksdb-block-cache-size-mb" select="300"/>
<xsl:variable name="def-user-bind-server-rocksdb-compaction-style" select="'kCompactionStyleLevel'"/>
<xsl:variable name="def-user-bind-server-rocksdb-number-background-threads" select="10"/>
<xsl:variable name="def-user-bind-server-rocksdb-expire-time" select="3"/>

<!-- Click frontend params -->
<xsl:variable name="def-time-duration-grpc-client-mark-bad" select="20"/>

</xsl:stylesheet>

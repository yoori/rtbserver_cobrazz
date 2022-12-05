<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="xsl exsl dyn cfg colo xsd">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="cluster-name" select="$CLUSTER_NAME"/>

<xsl:key name="services-by-host" match="service" use="host"/>
<xsl:key name="hosts-by-current" match="host" use="." />

<xsl:template name="FrontendsOutput">
  <xsl:param name="colo-config"/>
  <xsl:param name="oid-suffix"/>
  <xsl:param name="fe-descriptor"/>

  <xsl:variable name="fe-ports-count"
    select="count($colo-config/cfg:coloParams/cfg:virtualServer[not(@internal_port =
  preceding-sibling::cfg:virtualServer/@internal_port)]) + count(
  $colo-config/cfg:coloParams/cfg:secureVirtualServer[
    not(@internal_port = preceding-sibling::cfg:virtualServer/@internal_port)])"/>

  <xsl:if test="$fe-ports-count > 0">
    <descr name="frontendConnections"
      suffix="{$oid-suffix}"><xsl:value-of select="$fe-descriptor"/></descr>
  </xsl:if>

</xsl:template>

<xsl:template name="GetUniqueHostsTree">
  <xsl:param name="services-xpath"/>
  <xsl:param name="error-prefix"/>

  <xsl:variable name="hosts">
  <xsl:for-each select="$services-xpath">
    <xsl:call-template name="GetHosts">
      <xsl:with-param name="hosts" select="@host"/>
      <xsl:with-param name="error-prefix" select="$error-prefix"/>
    </xsl:call-template>
  </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($hosts)//host[not(. = preceding-sibling::host/.)]">
  <host><xsl:value-of select="."/></host>
  </xsl:for-each>
</xsl:template>

<xsl:template name="GenerateZenossConfigFiles">
  <xsl:param name="cluster-xpath"/>

  <zendmd>
    <xsl:variable
      name="zenoss-path"><xsl:call-template
        name="ZenossFolder">
        <xsl:with-param name="app-xpath" select="$xpath"/>
      </xsl:call-template></xsl:variable>
      <xsl:variable name="common-paths" select="concat($zenoss-path,'/mibs')"/>
      <xsl:variable name="adcluster-paths" select="concat(
        ':', $zenoss-path,'/mibs/CampaignSvcs/CampaignServer:',
        $zenoss-path,'/mibs/LogProcessing/LogGeneralizer:',
        $zenoss-path,'/mibs/RequestInfoSvcs/ExpressionMatcher:',
        $zenoss-path,'/mibs/Controlling')"/>
      <xsl:variable name="zenoss-paths"
        select="concat($common-paths, $adcluster-paths)"/>
    <config mibpath="{$zenoss-paths}"/>
    <processes>
      <process name="AdFrontend" class="/OIX/AdServer"
        version="{$app-version}" pattern=".*?httpd.worker .* -k start" />
      <process name="CampaignServer" class="/OIX/AdServer"
        version="{$app-version}" pattern="CampaignServer .*" />
      <process name="CleanupLogs" class="/OIX/AdServer"
        version="{$app-version}" pattern=".*?CleanupLogs.pl -conf .*" />
      <process name="SyncLogs" class="/OIX/AdServer"
        version="{$app-version}" pattern="SyncLogs .*" />
      <xsl:if test="$cluster-name='adcluster'">
        <process name="CampaignManager" class="/OIX/AdServer"
          version="{$app-version}" pattern="CampaignManager .*" />
        <process name="ChannelController" class="/OIX/AdServer"
          version="{$app-version}" pattern="ChannelController .*" />
        <process name="ChannelSearchService" class="/OIX/AdServer"
          version="{$app-version}" pattern="ChannelSearchService .*" />
        <process name="ChannelServer" class="/OIX/AdServer"
          version="{$app-version}" pattern="ChannelServer .*" />
        <process name="DictionaryProvider" class="/OIX/AdServer"
          version="{$app-version}" pattern="DictionaryProvider .*" />
        <process name="ExpressionMatcher" class="/OIX/AdServer"
          version="{$app-version}" pattern="ExpressionMatcher .*" />
        <process name="LogGeneralizer" class="/OIX/AdServer"
          version="{$app-version}" pattern="LogGeneralizer .*" />
        <process name="RequestInfoManager" class="/OIX/AdServer"
          version="{$app-version}" pattern="RequestInfoManager .*" />
        <process name="StatsCollector" class="/OIX/AdServer"
          version="{$app-version}" pattern="StatsCollector .*" />
        <process name="SyncLogsServer" class="/OIX/AdServer"
          version="{$app-version}" pattern=".*?rsync .*synclogs_server.conf" />
        <process name="UserBindServer" class="/OIX/AdServer"
          version="{$app-version}" pattern="UserBindServer .*"/>
        <process name="UserInfoManager" class="/OIX/AdServer"
          version="{$app-version}" pattern="UserInfoManager .*" />
        <process name="UserInfoManagerController" class="/OIX/AdServer"
          version="{$app-version}" pattern="UserInfoManagerController .*" />
        <process name="BillingServer" class="/OIX/AdServer"
          version="{$app-version}" pattern="BillingServer .*" />
      </xsl:if>
    </processes>
    <hosts>
    <xsl:variable name="app-config" select="$xpath/configuration/cfg:environment/cfg:ZenOSS"/>

    <xsl:variable name="zenoss-collector">
      <xsl:value-of select="$app-config/@collector"/>
      <xsl:if test="count($app-config/@collector) = 0">
        <xsl:value-of select="$xsd-zenoss-type/xsd:attribute[@name='collector']/@default"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="zenoss-system-def"
      select="concat($colo-name, '/AdServer')"/>

    <xsl:variable name="zenoss-system">
      <xsl:value-of select="$app-config/@system"/>
      <xsl:if test="count($app-config/@system) = 0">
        <xsl:value-of select="$zenoss-system-def"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="zenoss-group">
      <xsl:value-of select="$app-config/@group"/>
      <xsl:if test="count($app-config/@group) = 0">
        <xsl:value-of select="$xsd-zenoss-type/xsd:attribute[@name='group']/@default"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="zenoss-location">
      <xsl:value-of select="$app-config/@location"/>
      <xsl:if test="count($app-config/@location) = 0">
        <xsl:value-of select="$xsd-zenoss-type/xsd:attribute[@name='location']/@default"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="zenoss-community">
      <xsl:value-of select="$app-config/@community"/>
      <xsl:if test="count($app-config/@community) = 0">
        <xsl:value-of select="$xsd-zenoss-type/xsd:attribute[@name='community']/@default"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="colo-config"
      select="$cluster-xpath/serviceGroup[@descriptor = $ad-cluster-descriptor]/configuration/cfg:cluster"/>
   
    <xsl:variable name="adproxy-clusters"
      select="$cluster-xpath/serviceGroup[@descriptor = $ad-proxycluster-descriptor]"/>

    <xsl:variable name="oid-suffix">
      <xsl:call-template name="GetAttr">
        <xsl:with-param name="node" select="$colo-config/cfg:snmpStats"/>
        <xsl:with-param name="name" select="'oid_suffix'"/>
        <xsl:with-param name="type" select="$xsd-snmp-config-type"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="monitored-services-descriptors">
      <!-- PBE clusters descriptors -->
      <xsl:for-each select="$adproxy-clusters">
        <xsl:variable name="adproxy-snmp-stats-enabled">
          <xsl:if test="count(./configuration/cfg:cluster/cfg:snmpStats) > 0">
            <xsl:value-of select="string(./configuration/cfg:cluster/cfg:snmpStats/@enable)"/>
          </xsl:if>
        </xsl:variable>
        <xsl:if test="$adproxy-snmp-stats-enabled = 'true'">
        <xsl:variable name="pbe-oid-suffix">
          <xsl:call-template name="GetAttr">
            <xsl:with-param name="node" select="./configuration/cfg:cluster/cfg:snmpStats"/>
            <xsl:with-param name="name" select="'oid_suffix'"/>
            <xsl:with-param name="type" select="$xsd-snmp-config-type"/>
          </xsl:call-template>
        </xsl:variable>
        <descr name="userInfoExchangerConnections" suffix="{$pbe-oid-suffix}"><xsl:value-of
          select="$pbe-user-info-exchanger-descriptor"/></descr>
        <descr name="campaignServerConnections" suffix="{$pbe-oid-suffix}"><xsl:value-of
          select="$pbe-campaign-server-descriptor"/></descr>
        <descr name="channelProxyConnections" suffix="{$pbe-oid-suffix}"><xsl:value-of
          select="$pbe-channel-proxy-descriptor"/></descr>
        </xsl:if>
      </xsl:for-each>

      <!-- AdCluster descriptors -->
      <descr name="OixServerLogGeneralizer" suffix="{$oid-suffix}"><xsl:value-of
        select="$log-generalizer-descriptor"/></descr>
      <xsl:if test="count($colo-config/cfg:snmpStats/@monitoring_tag_id) > 0">
        <descr name="OixServerMonitoringRequest" suffix="{$oid-suffix}"><xsl:value-of
          select="$http-frontend-descriptor"/></descr>
        <descr name="OixServerApache" suffix="{$oid-suffix}"><xsl:value-of
          select="$http-frontend-descriptor"/></descr>
      </xsl:if>
      <descr name="OixServerCommons" suffix="{$oid-suffix}"><xsl:value-of
        select="$stats-collector-descriptor"/></descr>
      <descr name="OixServerExpressionMatcher" suffix="{$oid-suffix}"><xsl:value-of
        select="$expression-matcher-descriptor"/></descr>
      <descr name="OixServerRequestInfoManager" suffix="{$oid-suffix}"><xsl:value-of
        select="$request-info-manager-descriptor"/></descr>
      <descr name="OixServerUserInfoManager" suffix="{$oid-suffix}"><xsl:value-of
        select="$user-info-manager-descriptor"/></descr>

      <descr name="campaignManagerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$campaign-manager-descriptor"/></descr>
      <descr name="campaignServerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$campaign-server-descriptor"/></descr>
      <descr name="channelControllerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$channel-controller-descriptor"/></descr>
      <descr name="channelSearchServiceConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$channel-search-service-descriptor"/></descr>
      <descr name="channelServerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$channel-server-descriptor"/></descr>
      <descr name="dictionaryProviderConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$dictionary-provider-descriptor"/></descr>
      <descr name="expressionMatcherConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$expression-matcher-descriptor"/></descr>
      <descr name="logGeneralizerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$log-generalizer-descriptor"/></descr>
      <descr name="statsCollectorConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$stats-collector-descriptor"/></descr>
      <descr name="userBindServerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$user-bind-server-descriptor"/></descr>
      <descr name="userInfoManagerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$user-info-manager-descriptor"/></descr>
      <descr name="userInfoManagerControllerConnections" suffix="{$oid-suffix}"><xsl:value-of
        select="$user-info-manager-controller-descriptor"/></descr>

      <xsl:call-template name="FrontendsOutput">
        <xsl:with-param name="colo-config" select="$colo-config"/>
        <xsl:with-param name="oid-suffix" select="$oid-suffix"/>
        <xsl:with-param name="fe-descriptor" select="$http-frontend-descriptor"/>
      </xsl:call-template>
      <!--descr Add other custom descriptors </descr-->
    </xsl:variable>

    <!-- Generate Temporary Tree Fragment service->hosts and revert mapping to host->services -->
    <xsl:variable name="service-hosts-map">
      <xsl:for-each select="exsl:node-set($monitored-services-descriptors)/descr">
        <xsl:variable name="descr" select="."/>
        <xsl:for-each select="$xpath//service[@descriptor = $descr]">
          <service name="{$descr/@name}" suffix="{$descr/@suffix}">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
              <xsl:with-param name="error-prefix" select="'CollectServicesAndHosts'"/>
            </xsl:call-template>
          </service>
        </xsl:for-each>
      </xsl:for-each>

      <xsl:for-each select="$adproxy-clusters">
        <xsl:variable name="adproxy-snmp-stats-enabled">
          <xsl:if test="count(./configuration/cfg:cluster/cfg:snmpStats) > 0">
            <xsl:value-of select="string(./configuration/cfg:cluster/cfg:snmpStats/@enable)"/>
          </xsl:if>
        </xsl:variable>
        <xsl:if test="$adproxy-snmp-stats-enabled = 'true'">
          <xsl:variable name="pbe-oid-suffix">
            <xsl:call-template name="GetAttr">
              <xsl:with-param name="node" select="./configuration/cfg:cluster/cfg:snmpStats"/>
              <xsl:with-param name="name" select="'oid_suffix'"/>
              <xsl:with-param name="type" select="$xsd-snmp-config-type"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="proxy-sync-logs-services"
            select=".//service[
              @descriptor = $pbe-channel-proxy-descriptor or
              @descriptor = $pbe-campaign-server-descriptor or
              @descriptor = $pbe-stunnel-server-descriptor
            ]"/>
          <service name="proxySyncLogsConnections" suffix="{$pbe-oid-suffix}">
          <xsl:call-template name="GetUniqueHostsTree">
            <xsl:with-param name="services-xpath" select="$proxy-sync-logs-services"/>
            <xsl:with-param name="error-prefix"
              select="'GetUniqueHostsTree::proxySyncLogs'"/>
          </xsl:call-template>
          </service>
        </xsl:if>
      </xsl:for-each>

      <xsl:variable name="sync-logs-services"
        select="$cluster-xpath/serviceGroup[@descriptor = $ad-cluster-descriptor]//service[
        @descriptor = $campaign-manager-descriptor or
        @descriptor = $http-frontend-descriptor or
        @descriptor = $log-generalizer-descriptor or
        @descriptor = $expression-matcher-descriptor or
        @descriptor = $request-info-manager-descriptor or
        @descriptor = $stat-receiver-descriptor ] |
        $cluster-xpath/serviceGroup[@descriptor =
          $ad-cluster-descriptor]//service[@descriptor = $user-info-manager-descriptor]"/>
      <service name="syncLogsConnections" suffix="{$oid-suffix}">
      <xsl:call-template name="GetUniqueHostsTree">
        <xsl:with-param name="services-xpath" select="$sync-logs-services"/>
        <xsl:with-param name="error-prefix"
          select="'GetUniqueHostsTree::syncLogs'"/>
      </xsl:call-template>
      </service>
    </xsl:variable>
      <!--xsl:message terminate="no"-->
      <!--/xsl:message-->

    <!-- Revert mapping: distinct hosts of services and iterate through it -->
    <xsl:for-each select="exsl:node-set($service-hosts-map)/service/host
                          [generate-id() =
                           generate-id(key('hosts-by-current', .)[1])]">
      <xsl:sort select="."/>
      <host name="{.}" monitor="{$zenoss-collector}"
                       group="{$zenoss-group}"
                       location="{$zenoss-location}"
                       system="{$zenoss-system}"
                       community="{$zenoss-community}" >
      <xsl:variable name="host" select="."/>
        <!-- Get services by host value -->
        <xsl:for-each select="key('services-by-host', .)">
          <service name="{@name}" suffix="{@suffix}" comment=""/>
        </xsl:for-each>
      </host>
    </xsl:for-each>
    </hosts>
  </zendmd>
</xsl:template>

<xsl:template match="/">

  <xsl:if test="count($xsd-zenoss-type) != 1">
    <xsl:message
      terminate="yes">Error: xsd:complexType 'ZenOSSType' not found in '../../xsd/AppConfigType.xsd'</xsl:message>
  </xsl:if>

  <xsl:variable name="zenoss-enabled">
    <xsl:call-template name="GetZenOSSEnabled">
      <xsl:with-param name="app-xpath" select="$xpath"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
    <xsl:call-template name="GenerateZenossConfigFiles">
      <xsl:with-param name="cluster-xpath" select="$xpath"/>
    </xsl:call-template>
  </xsl:if>

</xsl:template>

</xsl:stylesheet>

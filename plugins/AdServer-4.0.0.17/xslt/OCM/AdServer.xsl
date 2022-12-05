<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:str="http://exslt.org/strings"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="dyn"
>

<xsl:output method="xml" indent="yes" encoding="utf-8"/>
<xsl:key name="host" match="@host" use="."/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../UserInfoProfiling/UserInfoManagerStorageParams.xsl"/>
<xsl:include href="../LogProcessing/ExpressionMatcherDistribution.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:variable name="current-time" select="$CURRENT_TIME"/>
<xsl:variable name="isp-zone" select="$ISP_ZONE"/>
<xsl:variable name="app-path" select="$xpath"/>
<xsl:variable name="separate-isp-zone" select="
  string($app-path/configuration/cfg:environment/cfg:ispZoneManagement/@separate_isp_zone |
    $xsd-isp-zone-management-type/xsd:attribute[@name='separate_isp_zone']/@default)"/>
<xsl:variable name="nil-isp-zone">
  <xsl:if test="$separate-isp-zone = 'true'">0</xsl:if>
  <xsl:if test="$separate-isp-zone = 'false'">1</xsl:if>
</xsl:variable>

<xsl:variable name="tr-services" select="$xpath/
  serviceGroup[@descriptor = $ad-cluster-descriptor]/
  serviceGroup[@descriptor = $fe-cluster-descriptor]/service[
    @descriptor = $channel-server-descriptor or
    @descriptor = $channel-controller-descriptor or
    @descriptor = $http-frontend-descriptor]"/>

<xsl:variable name="tr-hosts">
  <xsl:for-each select="$tr-services">
    <xsl:call-template name="GetHosts">
      <xsl:with-param name="hosts" select="@host"/>
      <xsl:with-param name="error-prefix" select="'SMS-tr-all-services-hosts'"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:variable>

<!-- AddService: -->
<xsl:template name="AddService">
  <xsl:param name="service-path"/>
  <xsl:param name="service-name"/>
  <xsl:param name="service-type"/>

  <xsl:variable name="zone-prefix">
    <xsl:choose>
      <xsl:when test="$isp-zone = '1'"><xsl:value-of select="'tr'"/></xsl:when>
      <xsl:when test="substring($service-name, 1, 2) = 'tr'"/>
      <xsl:otherwise><xsl:value-of select="substring($service-name, 1, 2)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:if test="$nil-isp-zone = '1' or substring($service-name, 1, 2) = $zone-prefix">
    <xsl:for-each select="$service-path">
      <xsl:variable name="real-hosts">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix" select="'SMS'"/>
        </xsl:call-template>
      </xsl:variable>
    <xsl:for-each select="exsl:node-set($real-hosts)/host">
<xsl:text>
    </xsl:text>
      <Service name="{$service-name}" type="{$service-type}" host="{.}"/>
    </xsl:for-each>
  </xsl:for-each>
  </xsl:if>

</xsl:template>

<xsl:template name="AddSubAgentFrontendSubcluster">
  <xsl:param name="cluster-path"/>
  <xsl:param name="service-descriptor"/>
  <xsl:param name="service-name-prefix"/>
  <xsl:param name="checking-host"/>
  <xsl:param name="use-position"/>

  <xsl:for-each select="$cluster-path/serviceGroup[@descriptor =
    $fe-cluster-descriptor]/service[@descriptor = $service-descriptor]">
    <xsl:variable name="pos">
      <xsl:if test="$use-position != 'false'">
        <xsl:value-of select="position()"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="subcluster-hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'SMS-frontend-subcluster-hosts'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="count(exsl:node-set($subcluster-hosts)[host =
      $checking-host]) > 0">
<xsl:text>
    </xsl:text>
      <Service name="{concat($service-name-prefix, $pos, '-SubAgent')}"
        type="AdServer::Controlling::SubAgent" host="{$checking-host}"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<xsl:template name="AddSubAgentService">
  <xsl:param name="cluster-path"/>
  <xsl:param name="number"/>

  <xsl:variable name="colo-config"
    select="$cluster-path/configuration/cfg:cluster"/>

  <xsl:if test="$colo-config/cfg:snmpStats/@enable = '1' or
    $colo-config/cfg:snmpStats/@enable = 'true'">

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>
    <xsl:variable
      name="subagent-be-services"
      select="$be-cluster-path/service[
              @descriptor = $campaign-server-descriptor or
              @descriptor = $dictionary-provider-descriptor or
              @descriptor = $expression-matcher-descriptor or
              @descriptor = $log-generalizer-descriptor or
              @descriptor = $request-info-manager-descriptor or
              @descriptor = $stats-collector-descriptor or
              @descriptor = $user-bind-server-descriptor
              ]"/>

    <xsl:variable name="be-hosts">
      <xsl:for-each select="$subagent-be-services">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'service-name', '-hosts')"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable
      name="subagent-pbe-services"
      select="$cluster-path/service[
              @descriptor = $pbe-campaign-server-descriptor or
              @descriptor = $pbe-channel-proxy-descriptor or
              @descriptor = $pbe-stunnel-server-descriptor or
              @descriptor = $pbe-user-info-exchanger-descriptor]"/>
    <xsl:variable name="pbe-hosts">
      <xsl:for-each select="$subagent-pbe-services">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'service-name', '-hosts')"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="uim-hosts">
      <xsl:for-each select="$cluster-path//service[@descriptor =
        $user-info-manager-descriptor]">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix"
            select="concat('OCM-', 'user-info-manager', '-hosts')"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="count-uim-clusters"
      select="count($cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor])"/>
    <xsl:variable name="ui-backup">
      <xsl:if test="($colo-config/cfg:coloParams/@backup_operations != '1' and
      $colo-config/cfg:coloParams/@backup_operations != 'true') or $count-uim-clusters = 1">
        <xsl:value-of select="'false'"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="def-prefix">
      <xsl:choose>
        <xsl:when test="$ui-backup = 'false'">
          <xsl:value-of select="'ui'"/>
        </xsl:when>
        <xsl:otherwise>fe</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="subfe-hosts">
      <xsl:for-each select="$cluster-path/serviceGroup[@descriptor =
        $fe-cluster-descriptor]/service[@descriptor = $http-frontend-descriptor]">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'frontend', '-hosts')"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>

    <xsl:for-each select="/colo:colocation/host">
      <xsl:variable name="this-host">
        <xsl:call-template name="ResolveHostName">
          <xsl:with-param name="base-host" select="@name"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'service-name', '-hosts')"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="count(exsl:node-set($pbe-hosts)[host = $this-host]) > 0">
          <xsl:if test="$isp-zone = '0'">
<xsl:text>
    </xsl:text>
      <Service name="{concat('pbe', $number, '-SubAgent')}"
        type="AdServer::Controlling::SubAgent" host="{$this-host}"/>
          </xsl:if>
        </xsl:when>
        <xsl:when test="count(exsl:node-set($be-hosts)[host = $this-host]) > 0">
          <xsl:if test="$isp-zone = '0'">
<xsl:text>
    </xsl:text>
      <Service name="be-SubAgent"
        type="AdServer::Controlling::SubAgent" host="{$this-host}"/>
          </xsl:if>
        </xsl:when>
        <xsl:when test="count(exsl:node-set($uim-hosts)[host = $this-host]) > 0">
          <xsl:if test="$isp-zone = '0'">
            <xsl:call-template name="AddSubAgentFrontendSubcluster">
              <xsl:with-param name="cluster-path" select="$cluster-path"/>
              <xsl:with-param name="service-descriptor"
                select="$user-info-manager-descriptor"/>
              <xsl:with-param name="service-name-prefix" select="$def-prefix"/>
              <xsl:with-param name="checking-host" select="$this-host"/>
              <xsl:with-param name="use-position" select="$ui-backup"/>
            </xsl:call-template>
          </xsl:if>
        </xsl:when>
        <xsl:when test="count(exsl:node-set($subfe-hosts)[host =
          $this-host]) > 0">
          <xsl:variable name="prefix">
            <xsl:choose>
              <xsl:when test="$isp-zone = '1' and count(exsl:node-set($tr-hosts)[host = $this-host]) > 0">
                <xsl:value-of select="'tr'"/>
              </xsl:when>
              <xsl:otherwise><xsl:value-of select="$def-prefix"/></xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:if test="$nil-isp-zone = '1' or $isp-zone = '1' and count(exsl:node-set($tr-hosts)[host = $this-host]) > 0">
          <xsl:call-template name="AddSubAgentFrontendSubcluster">
            <xsl:with-param name="cluster-path" select="$cluster-path"/>
            <xsl:with-param name="service-descriptor"
              select="$http-frontend-descriptor"/>
            <xsl:with-param name="service-name-prefix" select="$prefix"/>
            <xsl:with-param name="checking-host" select="$this-host"/>
            <xsl:with-param name="use-position" select="$ui-backup"/>
          </xsl:call-template>
          </xsl:if>
        </xsl:when>
      </xsl:choose>
    </xsl:for-each>

  </xsl:if>
</xsl:template>

<xsl:template name="AddOneOnHostService">
  <xsl:param name="serv-path"/>
  <xsl:param name="service-name"/>
  <xsl:param name="service-type"/>

  <xsl:variable name="real-hosts">
    <xsl:for-each select="$serv-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="concat('SMS-', $service-name, '-hosts')"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="/colo:colocation/host">
    <xsl:variable name="this-host">
      <xsl:call-template name="ResolveHostName">
        <xsl:with-param name="base-host" select="@name"/>
        <xsl:with-param name="error-prefix"
          select="concat('SMS-', $service-name, '-hosts')"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="count(exsl:node-set($real-hosts)[host = $this-host]) > 0">
<xsl:text>
    </xsl:text>
      <Service name="{$service-name}" type="{$service-type}" host="{$this-host}"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<xsl:template name="AddOneOnHostTrService">
  <xsl:param name="serv-path"/>
  <xsl:param name="tr-services-path"/>
  <xsl:param name="service-name-prefix"/>
  <xsl:param name="service-name-suffix"/>
  <xsl:param name="service-type"/>

  <xsl:variable name="real-hosts">
    <xsl:for-each select="$serv-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="concat('SMS-lp-', $service-name-suffix, '-hosts')"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="real-tr-hosts">
    <xsl:for-each select="$tr-services-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="concat('SMS-tr-', $service-name-suffix, '-hosts')"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="/colo:colocation/host">
    <xsl:variable name="this-host">
      <xsl:call-template name="ResolveHostName">
        <xsl:with-param name="base-host" select="@name"/>
        <xsl:with-param name="error-prefix"
          select="concat('SMS-', $service-name-suffix, '-hosts')"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="count(exsl:node-set($real-hosts)[host = $this-host]) > 0">
        <xsl:if test="$isp-zone = '0'">
<xsl:text>
    </xsl:text>
      <Service name="{$service-name-prefix}-{$service-name-suffix}" type="{$service-type}" host="{$this-host}"/>
        </xsl:if>
      </xsl:when>
      <xsl:when test="$isp-zone = '1' and count(exsl:node-set($real-tr-hosts)[host = $this-host]) > 0">
<xsl:text>
    </xsl:text>
      <Service name="tr-{$service-name-suffix}" type="{$service-type}" host="{$this-host}"/>
      </xsl:when>
    </xsl:choose>

  </xsl:for-each>
</xsl:template>


<xsl:template name="AdDBServices">
  <xsl:param name="cluster-path"/>
  <xsl:param name="number"/>

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

    <xsl:if test="count($be-cluster-path) > 0">

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $log-generalizer-descriptor]"/>
        <xsl:with-param name="service-name" select="'lp-LogGeneralizer'"/>
        <xsl:with-param name="service-type" select="'AdServer::LogProcessing::LogGeneralizer'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>
        <xsl:with-param name="service-name" select="'be-CampaignServer'"/>
        <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::CampaignServer'"/>
      </xsl:call-template>

    </xsl:if>

    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">

      <xsl:variable name="pos" select="position()"/>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $channel-server-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('tr', $pos, '-ChannelServer')"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelServer'"/>
      </xsl:call-template>
    </xsl:for-each>
</xsl:template>

<xsl:template name="AdServices">
  <xsl:param name="cluster-path"/>
  <xsl:param name="number"/>

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>
    <xsl:variable
      name="fe-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

    <xsl:variable name="colo-config" select="$cluster-path/configuration/cfg:cluster"/>

    <xsl:variable name="count-uim-clusters"
      select="count($cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor])"/>

    <xsl:if test="count($be-cluster-path) > 0">

      <xsl:variable
        name="localproxy-path"
        select="$be-cluster-path/serviceGroup[@descriptor = $local-proxy-descriptor]"/>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $stat-receiver-descriptor]"/>
        <xsl:with-param name="service-name" select="'lp-StatReceiver'"/>
        <xsl:with-param name="service-type" select="'AdServer::LogProcessing::StatReceiver'"/>
      </xsl:call-template>

      <xsl:if test="$isp-zone = '0'">
        <xsl:call-template name="AddOneOnHostService">
          <xsl:with-param name="serv-path"
            select="$be-cluster-path/service[@descriptor = $log-generalizer-descriptor or
                @descriptor = $expression-matcher-descriptor or
                @descriptor = $request-info-manager-descriptor] |
              $cluster-path//service[@descriptor = $user-info-manager-descriptor] |
              $be-cluster-path//service[@descriptor = $remote-stunnel-client-descriptor]"/>
          <xsl:with-param name="service-name" select="'lp-SyncLogsServer'"/>
          <xsl:with-param name="service-type" select="'AdServer::LogProcessing::SyncLogsServer'"/>
        </xsl:call-template>
        <xsl:if test="count($be-cluster-path/service[@descriptor = $predictor-descriptor]) > 0">
          <xsl:call-template name="AddOneOnHostService">
            <xsl:with-param name="serv-path"
              select="$be-cluster-path/service[@descriptor = $predictor-descriptor]"/>
            <xsl:with-param name="service-name" select="'be-Predictor-SyncLogsServer'"/>
            <xsl:with-param name="service-type" select="'AdServer::Predictor::SyncLogsServer'"/>
          </xsl:call-template>
          <xsl:call-template name="AddOneOnHostService">
            <xsl:with-param name="serv-path"
              select="$be-cluster-path/service[@descriptor = $predictor-descriptor]"/>
            <xsl:with-param name="service-name" select="'be-Predictor-Merger'"/>
            <xsl:with-param name="service-type" select="'AdServer::Predictor::Merger'"/>
          </xsl:call-template>
          <xsl:call-template name="AddOneOnHostService">
            <xsl:with-param name="serv-path"
              select="$be-cluster-path/service[@descriptor = $predictor-descriptor]"/>
            <xsl:with-param name="service-name" select="'be-Predictor-SVMGenerator'"/>
            <xsl:with-param name="service-type" select="'AdServer::Predictor::SVMGenerator'"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:if>

      <xsl:choose>
        <xsl:when test="$nil-isp-zone = '1'">
          <xsl:call-template name="AddOneOnHostService">
            <xsl:with-param name="serv-path" select="$cluster-path//service[
                @descriptor = $campaign-manager-descriptor or
                @descriptor = $http-frontend-descriptor or
                @descriptor = $log-generalizer-descriptor or
                @descriptor = $expression-matcher-descriptor or
                @descriptor = $request-info-manager-descriptor or
                @descriptor = $stat-receiver-descriptor ] |
              $cluster-path//service[@descriptor = $user-info-manager-descriptor]"/>
            <xsl:with-param name="service-name" select="'lp-SyncLogs'"/>
            <xsl:with-param name="service-type" select="'AdServer::LogProcessing::SyncLogs'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="AddOneOnHostTrService">
            <xsl:with-param name="serv-path" select="$cluster-path//service[
                @descriptor = $campaign-manager-descriptor or
                @descriptor = $log-generalizer-descriptor or
                @descriptor = $expression-matcher-descriptor or
                @descriptor = $request-info-manager-descriptor or
                @descriptor = $stat-receiver-descriptor ] |
              $cluster-path//service[@descriptor = $user-info-manager-descriptor]"/>
            <xsl:with-param name="tr-services-path"
              select="$cluster-path//service[@descriptor = $http-frontend-descriptor]"/>
            <xsl:with-param name="service-name-prefix" select="'lp'"/>
            <xsl:with-param name="service-name-suffix" select="'SyncLogs'"/>
            <xsl:with-param name="service-type" select="'AdServer::LogProcessing::SyncLogs'"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $expression-matcher-descriptor]"/>
        <xsl:with-param name="service-name" select="'lp-ExpressionMatcher'"/>
        <xsl:with-param name="service-type" select="'AdServer::LogProcessing::ExpressionMatcher'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $request-info-manager-descriptor]"/>
        <xsl:with-param name="service-name" select="'lp-RequestInfoManager'"/>
        <xsl:with-param name="service-type" select="'AdServer::RequestInfoSvcs::RequestInfoManager'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $dictionary-provider-descriptor]"/>
        <xsl:with-param name="service-name" select="'be-DictionaryProvider'"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::DictionaryProvider'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $user-operation-generator-descriptor]"/>
        <xsl:with-param name="service-name" select="'be-UserOperationGenerator'"/>
        <xsl:with-param name="service-type" select="'AdServer::UserInfoSvcs::UserOperationGenerator'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = $stats-collector-descriptor]"/>
        <xsl:with-param name="service-name" select="'be-StatsCollector'"/>
        <xsl:with-param name="service-type" select="'AdServer::Controlling::StatsCollector'"/>
      </xsl:call-template>

      <xsl:if test="count($localproxy-path//service) > 0" >
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="$localproxy-path/service[@descriptor = $remote-channel-proxy-descriptor]"/>
          <xsl:with-param name="service-name" select="'localproxy-ChannelProxy'"/>
          <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelProxy'"/>
        </xsl:call-template>

        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="$localproxy-path/service[@descriptor = $remote-stunnel-client-descriptor]"/>
          <xsl:with-param name="service-name" select="'localproxy-STunnel'"/>
          <xsl:with-param name="service-type" select="'AdServer::LogProcessing::STunnel'"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:if>

    <xsl:call-template name="AddSubAgentService">
      <xsl:with-param name="cluster-path" select="$cluster-path"/>
      <xsl:with-param name="number" select="$number"/>
    </xsl:call-template>

    <xsl:variable name="group-prefix"><xsl:choose><xsl:when
      test="$cluster-path/@descriptor = $ad-proxycluster-descriptor">
      <xsl:value-of select="concat('pbe', $number)"/></xsl:when>
      <xsl:otherwise>lp</xsl:otherwise></xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$nil-isp-zone = '1'">
        <xsl:call-template name="AddOneOnHostService">
          <xsl:with-param name="serv-path" select="$cluster-path//service"/>
          <xsl:with-param name="service-name" select="concat($group-prefix, '-CleanupLogs')"/>
          <xsl:with-param name="service-type" select="'AdServer::LogProcessing::CleanupLogs'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="AddOneOnHostTrService">
          <xsl:with-param name="serv-path" select="$cluster-path//service[not(@descriptor = $tr-services/@descriptor)]"/>
          <xsl:with-param name="tr-services-path"
              select="$cluster-path//service[@descriptor = $tr-services/@descriptor]"/>
          <xsl:with-param name="service-name-prefix" select="$group-prefix"/>
          <xsl:with-param name="service-name-suffix" select="'CleanupLogs'"/>
          <xsl:with-param name="service-type" select="'AdServer::LogProcessing::CleanupLogs'"/>
        </xsl:call-template>
     </xsl:otherwise>
    </xsl:choose>

    <xsl:variable name="ui-backup">
      <xsl:if test="($colo-config/cfg:coloParams/@backup_operations != '1' and
      $colo-config/cfg:coloParams/@backup_operations != 'true') or $count-uim-clusters = 1">
        <xsl:value-of select="'false'"/>
      </xsl:if>
    </xsl:variable>

    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">

      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="ui-comp">
        <xsl:choose>
          <xsl:when test="$ui-backup = 'false'">
            <xsl:value-of select="'ui'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('fe', $pos)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $user-info-manager-descriptor]"/>
        <xsl:with-param name="service-name" select="concat($ui-comp, '-UserInfoManager')"/>
        <xsl:with-param name="service-type" select="'AdServer::UserInfoSvcs::UserInfoManager'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $user-info-manager-controller-descriptor]"/>
        <xsl:with-param name="service-name" select="concat($ui-comp, '-UserInfoManagerController')"/>
        <xsl:with-param name="service-type"
          select="'AdServer::UserInfoSvcs::UserInfoManagerController'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $channel-controller-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('tr', $pos, '-ChannelController')"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelController'"/>
      </xsl:call-template>

      <xsl:if test="count(./service[@descriptor = $user-bind-server-descriptor]) > 0">
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $user-bind-server-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('fe', $pos,
            '-UserBindServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::UserInfoSvcs::UserBindServer'"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $user-bind-controller-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos,
          '-UserBindController')"/>
        <xsl:with-param name="service-type" select="'AdServer::UserInfoSvcs::UserBindController'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $billing-server-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos,
          '-BillingServer')"/>
        <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::BillingServer'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $channel-search-service-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-ChannelSearchService')"/>
        <xsl:with-param name="service-type"
          select="'AdServer::ChannelSearchSvcs::ChannelSearchService'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $campaign-manager-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-CampaignManager')"/>
        <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::CampaignManager'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $conv-server-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-ConvServer')"/>
        <xsl:with-param name="service-type" select="'AdServer::Frontends::ConvServer'"/>
      </xsl:call-template>

      <xsl:if test="count($fe-cluster-path/service[@descriptor = $profiling-server-descriptor]) > 0">
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $profiling-server-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-ProfilingServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::ProfilingServer'"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:if test="count($fe-cluster-path/service[@descriptor = $http-frontend-descriptor]) > 0">
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-FCGIAdServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::FCGIAdServer'"/>
        </xsl:call-template>

        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-FCGIRtbServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::FCGIRtbServer'"/>
        </xsl:call-template>

        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-FCGIUserBindServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::FCGIUserBindServer'"/>
        </xsl:call-template>

        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-FCGIUserBindIntServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::FCGIUserBindIntServer'"/>
        </xsl:call-template>

        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-FCGIUserBindAddServer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::FCGIUserBindAddServer'"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:if test="count($fe-cluster-path/service[@descriptor = $uid-generator-adapter-descriptor]) > 0">
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $uid-generator-adapter-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-UIDGeneratorAdapter')"/>
          <xsl:with-param name="service-type" select="'AdServer::Frontends::UIDGeneratorAdapter'"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:if test="count($fe-cluster-path/service[@descriptor = $zmq-profiling-balancer-descriptor]) > 0">
        <xsl:call-template name="AddService">
          <xsl:with-param name="service-path"
            select="./service[@descriptor = $zmq-profiling-balancer-descriptor]"/>
          <xsl:with-param name="service-name" select="concat('tr', $pos, '-ZmqProfilingBalancer')"/>
          <xsl:with-param name="service-type" select="'AdServer::Utils::ZmqProfilingBalancer'"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = $http-frontend-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('tr', $pos, '-HttpFrontend')"/>
        <xsl:with-param name="service-type" select="'AdServer::HttpFrontend'"/>
      </xsl:call-template>
    </xsl:for-each>

    <!--proxy cluster services-->
    <xsl:variable name="group"><xsl:value-of select="concat('pbe', $number, '-')"/></xsl:variable>

    <xsl:call-template name="AddService">
      <xsl:with-param name="service-path"
        select="$cluster-path/service[@descriptor = $pbe-user-info-exchanger-descriptor]"/>
      <xsl:with-param name="service-name" select="concat($group, 'UserInfoExchanger')"/>
      <xsl:with-param name="service-type" select="'AdServer::UserInfoSvcs::UserInfoExchanger'"/>
    </xsl:call-template>

    <xsl:call-template name="AddService">
      <xsl:with-param name="service-path"
        select="$cluster-path/service[@descriptor = $pbe-channel-proxy-descriptor]"/>
      <xsl:with-param name="service-name" select="concat($group, 'ChannelProxy')"/>
      <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelProxy'"/>
    </xsl:call-template>

    <xsl:call-template name="AddOneOnHostService">
      <xsl:with-param name="serv-path"
        select="$cluster-path/service[
          @descriptor = $pbe-channel-proxy-descriptor or
          @descriptor = $pbe-campaign-server-descriptor or
          @descriptor = $pbe-stunnel-server-descriptor]"/>
      <xsl:with-param name="service-name" select="concat($group, 'SyncLogs')"/>
      <xsl:with-param name="service-type" select="'AdServer::LogProcessing::SyncLogs'"/>
    </xsl:call-template>

    <xsl:call-template name="AddService">
      <xsl:with-param name="service-path"
        select="$cluster-path/service[@descriptor = $pbe-campaign-server-descriptor]"/>
      <xsl:with-param name="service-name" select="concat($group, 'CampaignServer')"/>
      <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::CampaignServer'"/>
    </xsl:call-template>

    <xsl:call-template name="AddService">
      <xsl:with-param name="service-path"
        select="$cluster-path/service[@descriptor = $pbe-stunnel-server-descriptor]"/>
      <xsl:with-param name="service-name" select="concat($group, 'STunnelServer')"/>
      <xsl:with-param name="service-type" select="'AdServer::LogProcessing::STunnelServer'"/>
    </xsl:call-template>
</xsl:template>


<xsl:template name="Services">
  <xsl:param name="env"/>
  <xsl:param name="cluster-path"/>
  <xsl:param name="script"/>
  <xsl:param name="number"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:if test="count($cluster-path/configuration/cfg:cluster/cfg:central) > 0">
    <Services start="%%START_LOOP_PRINT%%" stop="%%STOP_LOOP_PRINT%%"
      SMS_STATUS="%%SMS_STATUS2%%" status="%%STATUS_PRINT%%" timeout="600" cmd="{concat($script, ' %%HOST%% %%TYPE%%')}"
      start_cmd="%%cmd%% start" stop_cmd="%%cmd%% stop" status_cmd="%%cmd%% is_alive" status2_cmd="%%cmd%% db_status" env="{$env}">
      <xsl:call-template name="AdDBServices">
        <xsl:with-param name="cluster-path" select="$cluster-path"/>
        <xsl:with-param name="number" select="$number"/>
      </xsl:call-template>
    </Services>
  </xsl:if>

  <Services start="%%START_LOOP_PRINT%%" stop="%%STOP_LOOP_PRINT%%"
    status="%%STATUS_PRINT%%" timeout="600" cmd="{concat($script, ' %%HOST%% %%TYPE%%')}"
    start_cmd="%%cmd%% start" stop_cmd="%%cmd%% stop" status_cmd="%%cmd%% is_alive" env="{$env}">

    <xsl:if test="count($cluster-path/configuration/cfg:cluster/cfg:central) = 0">
      <xsl:call-template name="AdDBServices">
        <xsl:with-param name="cluster-path" select="$cluster-path"/>
        <xsl:with-param name="number" select="$number"/>
      </xsl:call-template>
    </xsl:if>

    <!--ad cluster services-->
    <xsl:call-template name="AdServices">
      <xsl:with-param name="cluster-path" select="$cluster-path"/>
      <xsl:with-param name="number" select="$number"/>
    </xsl:call-template>
  </Services>

</xsl:template>

<xsl:template name="Tasks">
  <xsl:param name="env"/>
  <xsl:param name="app-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="cluster-path"/>
  <xsl:param name="script"/>
  <xsl:param name="number"/>
  <xsl:param name="server-root"/>
  <xsl:param name="config-root"/>
  <xsl:param name="workspace-root"/>
  <xsl:param name="ssh-key"/>

  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>

  <xsl:variable
    name="be-cluster-path"
    select="$cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <Services start="%%START_NOLOOP_DONE%%" start_cmd="%%cmd%% start"
    cmd="{concat($script, ' %%HOST%% %%TYPE%%')}" env="{$env}" nostate="">

    <Service name="keyscan-Scanner" host="localhost"
      start="%%START_NOLOOP_DONE%%" start_cmd="%%SMS_KEYSCAN%%" transport="/bin/sh -c &quot;$SMS_TEXT&quot;">
      <xsl:attribute name="keyscan_hosts">
        <xsl:choose>
        <xsl:when test="$nil-isp-zone = '1'">
          <xsl:call-template name="GetUniqueHosts">
              <xsl:with-param name="services-xpath" select="$cluster-path//service"/>
            <xsl:with-param name="error-prefix" select="'OCM::KeyScan'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$isp-zone = '0'">
          <xsl:call-template name="GetUniqueHosts">
              <xsl:with-param name="services-xpath" select="$cluster-path//service[not(. = $tr-services)]"/>
            <xsl:with-param name="error-prefix" select="'OCM::KeyScan'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="GetUniqueHosts">
              <xsl:with-param name="services-xpath" select="$tr-services"/>
            <xsl:with-param name="error-prefix" select="'OCM::KeyScan'"/>
          </xsl:call-template>
        </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </Service>

    <xsl:variable name="dbaccess-hosts">
      <xsl:choose>
        <xsl:when test="$nil-isp-zone = '1'">
          <xsl:for-each select="$cluster-path//service[@descriptor = $log-generalizer-descriptor or
            @descriptor = $channel-server-descriptor or
            @descriptor = $campaign-server-descriptor]">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
              <xsl:with-param name="error-prefix" select="'OCM::DBAccess'"/>
            </xsl:call-template>
          </xsl:for-each>
        </xsl:when>
        <xsl:when test="$isp-zone = '0'">
      <xsl:for-each select="$cluster-path//service[@descriptor = $log-generalizer-descriptor or
        @descriptor = $campaign-server-descriptor]">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix" select="'OCM::DBAccess-foros'"/>
        </xsl:call-template>
      </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
      <xsl:for-each select="$cluster-path//service[@descriptor = $channel-server-descriptor]">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
          <xsl:with-param name="error-prefix" select="'OCM::DBAccess-isp'"/>
        </xsl:call-template>
      </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:for-each select="/colo:colocation/host">
      <xsl:variable name="check-host">
        <xsl:call-template name="ResolveHostName">
          <xsl:with-param name="base-host" select="@name"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'check-dbaccess-hosts ', @name)"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:if test="count(exsl:node-set($dbaccess-hosts)[host = $check-host]) > 0">
        <Service name="dbaccess-DBAccess" type="AdServer::DBAccess"
          stop_cmd="%%cmd%% stop" stop="%%STOP_NOLOOP_PRINT%%" host="{$check-host}"/>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path" select="./service[@descriptor = $http-frontend-descriptor]"/>
        <xsl:with-param name="service-name" select="concat('tr', position(), '-PSConfigurator')"/>
        <xsl:with-param name="service-type" select="'AdServer::PSConfigurator'"/>
      </xsl:call-template>
    </xsl:for-each>
    <xsl:variable name="group"><xsl:value-of select="concat('pbe', $number, '-')"/></xsl:variable>
    <xsl:for-each select="$cluster-path/service[@descriptor = $pbe-stunnel-server-descriptor]">
      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path" select="."/>
        <xsl:with-param name="service-name" select="concat($group, 'LogProxyConfigurator')"/>
        <xsl:with-param name="service-type" select="'AdServer::LogProcessing::LogProxyConfigurator'"/>
      </xsl:call-template>
    </xsl:for-each>
  </Services>

  <!-- distribution services -->
  <xsl:variable name="server-mgr-root">/opt/foros/server/manager/<xsl:value-of
    select="$PRODUCT_IDENTIFIER"/></xsl:variable>

  <xsl:variable name="server-mgr-lib-root">
    <xsl:choose>
      <xsl:when test="count($app-config/cfg:develParams/@management_lib_root) > 0">
        <xsl:value-of select="$app-config/cfg:develParams/@management_lib_root"/></xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$server-mgr-root"/>/lib</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="server-mgr-bin-root">
    <xsl:choose>
      <xsl:when test="count($app-config/cfg:develParams/@management_bin_root) > 0"><xsl:value-of
        select="$app-config/cfg:develParams/@management_bin_root[1]"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$server-mgr-root"/>/bin</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="count-uim-clusters"
    select="count($cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor])"/>
  <xsl:variable name="ui-backup">
    <xsl:choose>
      <xsl:when test="$count-uim-clusters = 1">
        <xsl:value-of select="'false'"/>
      </xsl:when>
      <xsl:when test="$colo-config/cfg:coloParams/@backup_operations = '0'">
        <xsl:value-of select="'false'"/>
      </xsl:when>
      <xsl:when test="$colo-config/cfg:coloParams/@backup_operations = 'false'">
        <xsl:value-of select="'false'"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="'true'"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:if test="$isp-zone = '0'">
  <xsl:variable name="all-user-info-manager-hosts">
    <xsl:for-each select="$cluster-path/serviceGroup[
      @descriptor = $fe-cluster-descriptor]/service[@descriptor = $user-info-manager-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
    <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="sync-logs-hosts">
    <xsl:for-each select="$cluster-path//service[
        @descriptor = $campaign-manager-descriptor or
        @descriptor = $log-generalizer-descriptor or
        @descriptor = $expression-matcher-descriptor or
        @descriptor = $request-info-manager-descriptor] |
      $cluster-path//service[@descriptor = $user-info-manager-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="chunk-configurator"><xsl:value-of
    select="concat('export PERL5LIB=$PERL5LIB:', $server-mgr-lib-root, ' ; ',
      $server-mgr-bin-root, '/ConfigureChunks.pl')"/>
  </xsl:variable>

  <xsl:variable name="all-user-bind-server-hosts">
    <xsl:for-each select="$cluster-path/serviceGroup[
      @descriptor = $fe-cluster-descriptor]/service[@descriptor = $user-bind-server-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
    <xsl:variable name="cluster-id" select="position()"/>
    <xsl:variable name="fe-cluster-path" select="."/>

    <Services transport="/bin/sh -c &quot;$SMS_TEXT&quot;" start="%%START_NOLOOP_DONE%%" force="0" check-hosts="" nostate="">

      <xsl:variable name="current-fe-component">
        <xsl:value-of select="concat('fe', $cluster-id)"/>
      </xsl:variable>

      <!-- UserBind distribution -->
      <xsl:variable name="user-bind-server-hosts">
        <xsl:for-each select="$fe-cluster-path/service[@descriptor = $user-bind-server-descriptor]">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:variable>

      <xsl:if test="count(exsl:node-set($user-bind-server-hosts)//host)> 0">
        <xsl:variable name="user-bind-chunks-count">
          <xsl:value-of select="$colo-config/cfg:userProfiling/@bind_chunks_count"/>                         
          <xsl:if test="count($colo-config/cfg:userProfiling/@bind_chunks_count)= 0">
            <xsl:value-of select="$user-bind-server-scale-chunks"/>
          </xsl:if>
        </xsl:variable>

        <xsl:variable name="user-bind-configurator-args">--chunks-count=<xsl:value-of
          select="$user-bind-chunks-count"/> --target-hosts='<xsl:for-each
          select="exsl:node-set($user-bind-server-hosts)//host">
            <xsl:value-of select="."/>,</xsl:for-each>' --modifier 'UserBind::Modifier' --chunks-root='<xsl:value-of
          select="concat($cache-root, '/UserBind', $cluster-id, '/')"/>' --transport='ssh:<xsl:value-of
          select="$ssh-key"/>' --environment='<xsl:value-of select="$config-root"/>/<xsl:value-of
          select="$colo-name"/>/adcluster/environment.sh' </xsl:variable>

        <xsl:variable name="user-bind-distribution-file"
          select="concat('/tmp/UserBindDistribution.', $cluster-id, '.xml.%%SMS_UNIQUE%%')"/>
        <xsl:variable name="user-bind-distribution-file-propagate-command">
          <xsl:for-each select="exsl:node-set($sync-logs-hosts)//host[not(. = preceding-sibling::host/.)]"
            > &amp;&amp; ssh -i '<xsl:value-of select="$ssh-key"/>' '<xsl:value-of select="."/>' "mkdir -p <xsl:value-of
            select="$workspace-root"/>/run/" &amp;&amp; rsync -e 'ssh -i <xsl:value-of
            select="$ssh-key"/>' <xsl:value-of
            select="$user-bind-distribution-file"/> <xsl:value-of select="concat(' ', .)"/>:<xsl:value-of
            select="$workspace-root"/>/run/UserBindDistribution.<xsl:value-of select="$cluster-id"/>.xml</xsl:for-each>
        </xsl:variable>

        <Service type="AdServer::UserInfoSvcs::UserBindChecker"
          host="localhost" name="{concat($current-fe-component, '-UserBindChecker')}">
          <xsl:attribute name="start_cmd">mv <xsl:value-of select="concat($cache-root,
            '/UserBind', ' ', $cache-root, '/UserBind1/Chunk_0_1')"/> 2>/dev/null ; <xsl:value-of
            select="$chunk-configurator"/> check <xsl:value-of
            select="$user-bind-configurator-args"/> --xml-out=<xsl:value-of
            select="$user-bind-distribution-file"/><xsl:value-of
            select="concat(' ', $user-bind-distribution-file-propagate-command)"
            /> || ( EX=$? ; if [ $EX -eq 1 ] ; then eval 'echo -e %%SMS_COLOR_ERROR%%redistribution required%%SMS_COLOR_PLAIN%%' &amp;&amp; exit 1 ; else exit -1 ; fi ; )</xsl:attribute>
        </Service>
        <Service type="AdServer::UserInfoSvcs::UserBindDistributor"
          host="localhost" name="{concat($current-fe-component, '-UserBindDistributor')}">
          <xsl:attribute name="start_cmd"><xsl:value-of select="$chunk-configurator"/> reconf <xsl:value-of
            select="$user-bind-configurator-args"/> --force=%%force%% --check-hosts='<xsl:for-each
            select="exsl:node-set($all-user-bind-server-hosts)//host">
              <xsl:value-of select="."/>,</xsl:for-each>%%check-hosts%%'</xsl:attribute>
        </Service>
      </xsl:if>

      <!-- UserInfo distribution -->
      <xsl:variable name="user-info-manager-hosts">
        <xsl:for-each select="$fe-cluster-path/service[@descriptor = $user-info-manager-descriptor]">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:variable>

      <xsl:variable name="ui-storage-params">
        <xsl:call-template name="UserInfoManagerStorageParams">
          <xsl:with-param name="cache-root" select="$cache-root"/>
          <xsl:with-param name="cluster-id" select="$cluster-id"/>
          <xsl:with-param name="colo-config" select="$colo-config"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="ui-configurator-args">--chunks-count=<xsl:value-of
        select="exsl:node-set($ui-storage-params)/chunks-count"/> --target-hosts='<xsl:for-each
        select="exsl:node-set($user-info-manager-hosts)//host">
          <xsl:value-of select="."/>,</xsl:for-each>' --modifier 'UserInfo::Modifier' --chunks-root='<xsl:value-of
        select="exsl:node-set($ui-storage-params)/chunks-root"/>' --transport='ssh:<xsl:value-of
        select="$ssh-key"/>' --environment='<xsl:value-of select="$config-root"/>/<xsl:value-of
        select="$colo-name"/>/adcluster/environment.sh' </xsl:variable>

      <xsl:variable name="user-info-distribution-file"
        select="concat('/tmp/UserInfoDistribution.', $cluster-id, '.xml.%%SMS_UNIQUE%%')"/>

      <!-- propagate distribution to all unique sync logs service hosts -->
      <xsl:variable name="user-info-distribution-file-propagate-command">
        <xsl:for-each select="exsl:node-set($sync-logs-hosts)//host[not(. = preceding-sibling::host/.)]"
          > &amp;&amp; ssh -i '<xsl:value-of select="$ssh-key"/>' '<xsl:value-of select="."/>' "mkdir -p <xsl:value-of
          select="$workspace-root"/>/run/" &amp;&amp; rsync -e 'ssh -i <xsl:value-of
          select="$ssh-key"/>' <xsl:value-of
          select="$user-info-distribution-file"/> <xsl:value-of select="concat(' ', .)"/>:<xsl:value-of
          select="$workspace-root"/>/run/UserInfoDistribution.<xsl:value-of select="$cluster-id"/>.xml</xsl:for-each>
      </xsl:variable>
      <xsl:variable name="ui-comp">
        <xsl:choose>
          <xsl:when test="$ui-backup = 'true'">
            <xsl:value-of select="concat('fe', $cluster-id)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'ui'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <Service type="AdServer::UserInfoSvcs::UserInfoChecker"
        host="localhost" name="{concat($ui-comp, '-UserInfoChecker')}">
        <xsl:attribute name="start_cmd"><xsl:value-of select="$chunk-configurator"/> check <xsl:value-of
          select="$ui-configurator-args"/> --xml-out=<xsl:value-of
          select="$user-info-distribution-file"/><xsl:value-of select="concat(' ', $user-info-distribution-file-propagate-command)"
          /> || ( EX=$? ; if [ $EX -eq 1 ] ; then eval 'echo -e %%SMS_COLOR_ERROR%%redistribution required%%SMS_COLOR_PLAIN%%' &amp;&amp; exit 1 ; else exit -1 ; fi ; )</xsl:attribute>
      </Service>
      <Service type="AdServer::UserInfoSvcs::UserInfoDistributor"
        host="localhost" name="{concat($ui-comp, '-UserInfoDistributor')}">
        <xsl:attribute name="start_cmd"><xsl:value-of select="$chunk-configurator"/> reconf <xsl:value-of
          select="$ui-configurator-args"/> --force=%%force%% --check-hosts='<xsl:for-each
          select="exsl:node-set($all-user-info-manager-hosts)//host">
            <xsl:value-of select="."/>,</xsl:for-each>%%check-hosts%%'</xsl:attribute>
      </Service>
    </Services>
  </xsl:for-each>

  <xsl:if test="count($be-cluster-path) > 0">
    <xsl:variable name="all-expression-matcher-hosts">
      <xsl:for-each select="$cluster-path/serviceGroup[
        @descriptor = $be-cluster-descriptor]/service[@descriptor = $expression-matcher-descriptor]">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>

    <Services transport="/bin/sh -c &quot;$SMS_TEXT&quot;" start="%%START_NOLOOP_DONE%%" force="0" check-hosts="">
      <xsl:call-template name="ExpressionMatcherServices">
        <xsl:with-param name="colo-config" select="$colo-config"/>
        <xsl:with-param name="cache-root" select="$cache-root"/>
        <xsl:with-param name="all-hosts" select="$all-expression-matcher-hosts"/>
        <xsl:with-param name="ssh-key" select="$ssh-key"/>
        <xsl:with-param name="config-root" select="$config-root"/>
        <xsl:with-param name="colo-name" select="$colo-name"/>
        <xsl:with-param name="sync-logs-hosts" select="$sync-logs-hosts"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
        <xsl:with-param name="chunk-configurator" select="$chunk-configurator"/>
      </xsl:call-template>
    </Services>
  </xsl:if>

  </xsl:if>

  <xsl:if test="$nil-isp-zone = '1' or $isp-zone = '1'">
  <xsl:variable
    name="fe-cluster-path"
    select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable name="prestart-timeout">
    <xsl:value-of select="$fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@timeout"/>
      <xsl:if test="count($fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@timeout) = 0">600</xsl:if>
  </xsl:variable>

  <Services start="%%START_LOOP_PRINT%%" status="%%STATUS_PRINT%%" cmd="{concat($script, ' %%HOST%% %%TYPE%%')}"
    start_cmd="%%cmd%% start" status_cmd="%%cmd%% is_alive" env="{$env}" timeout="{$prestart-timeout}" nostate="">

    <xsl:variable
      name="start-params-wait-loading"><xsl:value-of
      select="$fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@wait_loading"/>
      <xsl:if test="count($fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@wait_loading) = 0">
        <xsl:value-of select="'true'"/></xsl:if>
    </xsl:variable>

    <xsl:if test="$start-params-wait-loading = 'true' or $start-params-wait-loading = '1'">
    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
      <xsl:variable name="pos" select="position()"/>
        <Service name="{concat('tr', $pos, '-PreStart')}" type="AdServer::PreStart" host="localhost"
          start="%%START_LOOP_PRINT%%"
          status="%%STATUS_PRINT%%"
          transport="/bin/sh -c &quot;$SMS_TEXT&quot;"
          start_cmd="true"
          stop_cmd="true">
          <xsl:attribute name="status_cmd">
            <xsl:call-template name="GeneratePreStartCommand">
              <xsl:with-param name="fe-cluster" select="."/>
              <xsl:with-param name="config-root" select="$config-root"/>
              <xsl:with-param name="ssh-key" select="$ssh-key"/>
              <xsl:with-param name="server-mgr-lib-root" select="$server-mgr-lib-root"/>
              <xsl:with-param name="server-mgr-bin-root" select="$server-mgr-bin-root"/>
              <xsl:with-param name="ui-backup" select="$ui-backup"/>
            </xsl:call-template>
          </xsl:attribute>
        </Service>
    </xsl:for-each>
    </xsl:if>

  </Services>
  </xsl:if>

  <xsl:variable name="all-adfe-hosts">
    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor =
      $fe-cluster-descriptor]/service[@descriptor = $http-frontend-descriptor]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'all-adfe-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>
  <Services start="%%START_NOLOOP_DONE%%" op="install" transport="ssh -o 'BatchMode yes' $SMS_HOST &#34;$SMS_TEXT&#34;">
    <xsl:for-each select="/colo:colocation/host">
      <xsl:variable name="check-host">
        <xsl:call-template name="ResolveHostName">
          <xsl:with-param name="base-host" select="@name"/>
          <xsl:with-param name="error-prefix"
            select="concat('SMS-', 'check-host ', @name)"/>
        </xsl:call-template>
      </xsl:variable>
      <Service name="test-upgrade-zdt1" host="{$check-host}" start_cmd="sudo yum -d1 -y --downloadonly %%op%% foros-config-server-{$PRODUCT_IDENTIFIER}"/>
      <Service name="upgrade-zdt1" host="{$check-host}" start_cmd="sudo yum -C -d1 -y %%op%% foros-config-server-{$PRODUCT_IDENTIFIER}"/>
      <Service name="list-packages-zdt1" host="{$check-host}" start_cmd="rpm -qa foros-*server*"/>
      <xsl:if test="count(exsl:node-set($all-adfe-hosts)[host = $check-host]) > 0">
        <Service name="list-packages-pagesense" host="{$check-host}" start_cmd="rpm -qa foros-*pagesense*"/>
      </xsl:if>
    </xsl:for-each>
  </Services>

</xsl:template>

<xsl:template name="GeneratePreStartRefList">
  <xsl:param name="services"/>
  <xsl:param name="service-config"/>
  <xsl:param name="def-port"/>
  <xsl:param name="obj-name"/>
  <xsl:param name="services-name"/>

  <xsl:for-each select="$services">
    <xsl:variable name="config" select="dyn:evaluate($service-config)"/>
    <xsl:variable name="port">
      <xsl:value-of select="$config/cfg:networkParams/@port"/>
      <xsl:if test="count($config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-port"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="resolved-hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="./@host"/>
        <xsl:with-param name="error-prefix">OCM Config: <xsl:value-of
          select="$services-name"/> hosts resolving</xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($resolved-hosts)//host">
      <xsl:value-of select="."/>#corbaloc::localhost:<xsl:value-of
        select="$port"/>/<xsl:value-of select="$obj-name"/>,</xsl:for-each>
  </xsl:for-each>
</xsl:template>

<xsl:template name="GeneratePreStartCommand">
  <xsl:param name="fe-cluster"/>
  <xsl:param name="config-root"/>
  <xsl:param name="ssh-key"/>
  <xsl:param name="server-mgr-lib-root"/>
  <xsl:param name="server-mgr-bin-root"/>
  <xsl:param name="ui-backup"/>

  <xsl:variable name="prestart-checker"><xsl:value-of
    select="concat('export PERL5LIB=$PERL5LIB:', $server-mgr-lib-root, ' ; ',
      $server-mgr-bin-root, '/PreStartChecker.pl')"/></xsl:variable>

  <xsl:variable name="user-info-manager-arg">
    <xsl:call-template name="GeneratePreStartRefList">
      <xsl:with-param name="services"
        select="$fe-cluster/service[@descriptor = $user-info-manager-controller-descriptor]"/>
      <xsl:with-param name="service-config"
        select="'./configuration/cfg:userInfoManagerController'"/>
      <xsl:with-param name="def-port" select="$def-user-info-manager-controller-port"/>
      <xsl:with-param name="obj-name" select="'UserInfoClusterControl'"/>
      <xsl:with-param name="services-name" select="'UserInfoController'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="campaign-manager-arg">
    <xsl:call-template name="GeneratePreStartRefList">
      <xsl:with-param name="services"
        select="$fe-cluster/service[@descriptor = $campaign-manager-descriptor]"/>
      <xsl:with-param name="service-config"
        select="'./configuration/cfg:campaignManager'"/>
      <xsl:with-param name="def-port" select="$def-campaign-manager-port"/>
      <xsl:with-param name="obj-name" select="'ProcessControl'"/>
      <xsl:with-param name="services-name" select="'CampaignManager'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="channel-server-arg">
    <xsl:call-template name="GeneratePreStartRefList">
      <xsl:with-param name="services"
        select="$fe-cluster/service[@descriptor = $channel-controller-descriptor]"/>
      <xsl:with-param name="service-config"
        select="'./configuration/cfg:channelController'"/>
      <xsl:with-param name="def-port" select="$def-channel-controller-port"/>
      <xsl:with-param name="obj-name" select="'ChannelClusterControl'"/>
      <xsl:with-param name="services-name" select="'ChannelController'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="prestart-checker-args">--env=<xsl:value-of
    select="$config-root"/>/<xsl:value-of
    select="$colo-name"/>/%%env%%/environment.sh --ssh-identity=<xsl:value-of select="$ssh-key"
    /><xsl:if test="$ui-backup = 'true'"> --serv-UserInfoManager=<xsl:value-of
      select="$user-info-manager-arg"/></xsl:if><xsl:if
        test="string-length($campaign-manager-arg) > 0"> --serv-CampaignManager=<xsl:value-of
          select="$campaign-manager-arg"/></xsl:if
    > --serv-ChannelServer=<xsl:value-of select="$channel-server-arg"/></xsl:variable>

  <xsl:value-of select="concat($prestart-checker, ' ', $prestart-checker-args)"/>

</xsl:template>

<xsl:template name="Troubleshoot">
  <xsl:param name="server-root"/>
  <xsl:param name="config-root"/>
  <xsl:param name="workspace-root"/>
  <xsl:param name="ssh-argc"/>
  <xsl:param name="server-bin-root"/>
  <xsl:param name="isp-zone-specific-hosts"/>

  <Services start="%%START_NOLOOP_DONE%%" transport="/bin/sh -c &quot;$SMS_TEXT&quot;"
    report_dir="{concat('/opt/foros/manager/var/server-', $colo-name)}"
    cmd="REPORTFILE=%%report%%; treport_dir=`dirname $REPORTFILE`; ERROR_LOG=$treport_dir/error_description_%%SMS_UNIQUE%%; ">
    <xsl:for-each select="/colo:colocation/host">
      <xsl:variable name="this-host">
        <xsl:call-template name="ResolveHostName">
          <xsl:with-param name="base-host" select="@name"/>
          <xsl:with-param name="error-prefix" select="'SMS-tr-host'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="need-output">
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($isp-zone-specific-hosts)[host = $this-host]) > 0">
            <xsl:if test="$isp-zone = '1'">1</xsl:if>
          </xsl:when>
          <xsl:when test="$isp-zone = '0' or count(exsl:node-set($tr-hosts)[host = $this-host]) > 0">1</xsl:when>
        </xsl:choose>
      </xsl:variable>

      <xsl:if test="$need-output = '1'">
        <Service name="troubleshoot-TReport" type="AdServer::TReport" host="{$this-host}"
          start_cmd="{concat('%%cmd%% ssh ', $ssh-argc, ' &quot; PATH=$PATH:',
            $server-bin-root, '; ERROR_LOG=$ERROR_LOG; ', $server-root, '/bin/TReport.pl -w ', $workspace-root,
            ' -c ', $config-root, ' -b ', $server-root, ' -m ', $colo-name,
            ' -o ', $workspace-root, '/tr_report_', $this-host, '_%%SMS_UNIQUE%%.tar.gz&quot; 2>>$ERROR_LOG',
            ' || echo &quot;Error occured on generating report for host ',
            $this-host, '. Exit status = $?.&quot; >>$ERROR_LOG')}"/>
        <Service name="troubleshoot-Fetcher" type="AdServer::TReportFetcher" host="{$this-host}"
          start_cmd="{concat('%%cmd%% scp -q ', $ssh-argc, ':', $workspace-root, '/tr_report_', $this-host,
           '_%%SMS_UNIQUE%%.tar.gz ',
           '$treport_dir 2>>$ERROR_LOG || echo &quot;Error occured on getting report from ',
           $this-host, '. Exit status = $?.&quot; >>$ERROR_LOG')}"/>
        <Service name="troubleshoot-Cleaner" type="AdServer::TReportCleaner" host="{$this-host}"
          start_cmd="{concat('%%cmd%% ssh ', $ssh-argc, ' rm -f ', $workspace-root,
          '/tr_report_%%SMS_UNIQUE%%.tar.gz 2>/dev/null;  return 0')}"/>
        <xsl:variable name="xslt_dyspepsia">'{}'</xsl:variable>
        <Service name="clear-stat-backup-ClearLogBackup" type="AdServer::ClearLogBackup" host="{$this-host}"
          start_cmd="{concat('%%cmd%% ssh ', $ssh-argc, ' find ', $workspace-root,
            '/log -wholename &quot;*/*/Out/*_/*.*&quot; -exec &quot;rm ', $xslt_dyspepsia, ' \;&quot;')}"/>
      </xsl:if>
    </xsl:for-each>
    <Service name="troubleshoot-Cleaner" type="AdServer::TReportCleaner" host="localhost"
      start_cmd="%%cmd%% rm -f $ERROR_LOG"/>
    <Service name="troubleshoot-Printer" type="AdServer::TReportPrinter" host="localhost" start_cmd="%%SMS_TR_PRINT%%"/>
    <Service name="troubleshoot-Checker" type="AdServer::TReportChecker" host="localhost" start_cmd="%%SMS_TR_CHECK%%"/>
    <xsl:variable name="all_archives">
      <xsl:for-each select="/colo:colocation/host">
        <xsl:variable name="this-host">
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="@name"/>
            <xsl:with-param name="error-prefix" select="'SMS-tr-host'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($isp-zone-specific-hosts)[host = $this-host]) > 0">
            <xsl:if test="$isp-zone = '1'">
              <xsl:value-of select="concat('tr_report_', $this-host, '_%%SMS_UNIQUE%%.tar.gz ')"/>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$isp-zone = '0' or count(exsl:node-set($tr-hosts)[host = $this-host]) > 0">
            <xsl:value-of select="concat('tr_report_', $this-host, '_%%SMS_UNIQUE%%.tar.gz ')"/>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>
      <xsl:value-of select="'error_description_%%SMS_UNIQUE%%'"/>
    </xsl:variable>
    <Service name="troubleshoot-Collector" type="AdServer::TReportCollector" host="localhost"
      start_cmd="{concat('%%cmd%% tar --ignore-failed-read -cf $REPORTFILE --remove-files -C $treport_dir ',
      $all_archives, '; return 0')}"/>
  </Services>
</xsl:template>

<xsl:template name="AddGroup">
  <xsl:param name="group-name"/>
  <xsl:param name="group-mask"/>
  <xsl:param name="exclude-groups"/>
  <xsl:for-each select="str:tokenize($group-name, ' ')">
    <Group name="{.}">
      <xsl:variable name="real-mask">
        <xsl:choose>
          <xsl:when test="string-length($group-mask) > 0">
            <xsl:value-of select="$group-mask"/>
          </xsl:when>
          <xsl:when test="string-length($group-mask) = 0">
            <xsl:value-of select="concat(., '-*,*')"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:for-each select="str:tokenize($real-mask, ' ')">
        <Mask><xsl:value-of select="."/></Mask>
      </xsl:for-each>
      <xsl:for-each select="str:tokenize($exclude-groups, ' ')">
        <Group type="exclude"><xsl:value-of select="."/></Group>
      </xsl:for-each>
    </Group>
  </xsl:for-each>
</xsl:template>

<xsl:template name="AddLpGroups">

  <xsl:variable name="fe1-services" select="$xpath/
    serviceGroup[@descriptor = $ad-cluster-descriptor]/
    serviceGroup[@descriptor = $fe-cluster-descriptor][1]/service[
      @descriptor = $campaign-manager-descriptor or
      @descriptor = $channel-search-service-descriptor or
      @descriptor = $user-info-manager-descriptor or
      @descriptor = $user-info-manager-controller-descriptor]"/>

  <xsl:variable name="fe2-services" select="$xpath/
    serviceGroup[@descriptor = $ad-cluster-descriptor]/
    serviceGroup[@descriptor = $fe-cluster-descriptor][2]/service[
      @descriptor = $campaign-manager-descriptor or
      @descriptor = $channel-search-service-descriptor or
      @descriptor = $user-info-manager-descriptor or
      @descriptor = $user-info-manager-controller-descriptor]"/>

  <xsl:variable name="be-services" select="$xpath/
    serviceGroup[@descriptor = $ad-cluster-descriptor]/
    serviceGroup[@descriptor = $be-cluster-descriptor]//service"/>

  <xsl:variable name="fe1-hosts">
    <xsl:for-each select="$fe1-services">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'fe1-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="fe2-hosts">
    <xsl:for-each select="$fe2-services">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'fe2-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="be-hosts">
    <xsl:for-each select="$be-services">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'be-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="fe1-masks">
  <xsl:for-each select="exsl:node-set($fe1-hosts)/host[not(. =
    preceding-sibling::host/.)]"><xsl:value-of
      select="concat(' lp-*,', .)"/></xsl:for-each>
  </xsl:variable>

  <xsl:variable name="fe2-masks">
  <xsl:for-each select="exsl:node-set($fe2-hosts)/host[not(. =
    preceding-sibling::host/.)]"><xsl:value-of
      select="concat(' lp-*,', .)"/></xsl:for-each>
  </xsl:variable>

  <xsl:variable name="not-fe-hosts">
    <xsl:for-each select="exsl:node-set($be-hosts)/host[not(. =
      preceding-sibling::host/.)]">
      <xsl:variable name="this-host" select="."/>
      <xsl:if test="count(exsl:node-set($fe1-hosts)[host = $this-host]) = 0 and
        count(exsl:node-set($fe2-hosts)[host = $this-host]) = 0">
        <xsl:value-of select="concat(' lp-*,', $this-host)"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:if test="string-length($not-fe-hosts) > 0">
    <xsl:call-template name="AddGroup">
      <xsl:with-param name="group-name" select="'lp-be'"/>
      <xsl:with-param name="group-mask" select="$not-fe-hosts"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:if test="string-length($fe1-masks) > 0">
  <xsl:call-template name="AddGroup">
    <xsl:with-param name="group-name" select="'lp-fe1'"/>
    <xsl:with-param name="group-mask" select="$fe1-masks"/>
    <xsl:with-param name="exclude-groups" select="'em-distribution'"/>
  </xsl:call-template>
  </xsl:if>
  <xsl:if test="string-length($fe2-masks) > 0">
    <xsl:call-template name="AddGroup">
      <xsl:with-param name="group-name" select="'lp-fe2'"/>
      <xsl:with-param name="group-mask" select="$fe2-masks"/>
      <xsl:with-param name="exclude-groups" select="'em-distribution'"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="AddUIGroup">
  <xsl:param name="fe-comp"/>
  <xsl:param name="ui-comp"/>
  <xsl:call-template name="AddGroup">
    <xsl:with-param name="group-name" select="$ui-comp"/>
    <xsl:with-param name="group-mask" select="concat($fe-comp, '-UserInfo*,*')"/>
    <xsl:with-param name="exclude-groups" select="concat($ui-comp, '-distribution')"/>
  </xsl:call-template>
  <xsl:call-template name="AddGroup">
    <xsl:with-param name="group-name" select="concat($ui-comp, '-distribution')"/>
    <xsl:with-param name="group-mask" select="concat('*:', $fe-comp, '-UserInfoDistributor')"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="AddDependence">
  <xsl:param name="masters"/>
  <xsl:param name="slaves"/>
  <Dependence>
    <Slave>
    <xsl:for-each select="str:tokenize($slaves, ' ')">
      <Type><xsl:value-of select="."/></Type>
    </xsl:for-each>
    </Slave>
    <Master>
    <xsl:for-each select="str:tokenize($masters, ' ')">
      <Type><xsl:value-of select="."/></Type>
    </xsl:for-each>
    </Master>
  </Dependence>
</xsl:template>

<xsl:template name="PhormZoneCommonDependencies">
  <xsl:param name="proxycluster-path"/>
  <xsl:param name="be-cluster-path"/>

  <xsl:if test="count($proxycluster-path) > 0">
    <xsl:call-template name="AddDependence">
      <xsl:with-param name="masters" select="'AdServer::LogProcessing::STunnelServer'"/>
      <xsl:with-param name="slaves" select= "'AdServer::LogProcessing::LogProxyConfigurator'"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:call-template name="AddDependence">
    <xsl:with-param name="masters" select="'AdServer::TReportFetcher'"/>
    <xsl:with-param name="slaves" select= "'AdServer::TReportCollector'"/>
  </xsl:call-template>
  <xsl:call-template name="AddDependence">
    <xsl:with-param name="masters" select="'AdServer::TReport'"/>
    <xsl:with-param name="slaves" select= "'AdServer::TReportFetcher'"/>
  </xsl:call-template>
  <xsl:call-template name="AddDependence">
    <xsl:with-param name="masters" select="'AdServer::TReportCollector'"/>
    <xsl:with-param name="slaves" select= "'AdServer::TReportCleaner'"/>
  </xsl:call-template>
  <xsl:call-template name="AddDependence">
    <xsl:with-param name="masters" select="'AdServer::TReportCleaner'"/>
    <xsl:with-param name="slaves" select= "'AdServer::TReportPrinter'"/>
  </xsl:call-template>
  <xsl:call-template name="AddDependence">
    <xsl:with-param name="masters" select="'AdServer::TReportChecker'"/>
    <xsl:with-param name="slaves" select= "'AdServer::TReport'"/>
  </xsl:call-template>
</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath"/>

  <xsl:variable
    name="adcluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-cluster-descriptor]"/>

  <xsl:variable
    name="proxycluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-proxycluster-descriptor]"/>

  <xsl:variable
    name="be-cluster-path"
    select="$adcluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> SMS: Can't find XPATH element </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="colo-config"
    select="$adcluster-path/configuration/cfg:cluster |
      $proxycluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="app-config" select="$app-path/configuration/cfg:environment"/>

  <xsl:variable name="zone_condition"
    select="$isp-zone = '1' and $separate-isp-zone = 'true'"/>
  <xsl:variable name="app-zone-config"
    select="$app-config/cfg:ispZoneManagement[$zone_condition] |
      $app-config/cfg:forosZoneManagement[not($zone_condition)]"/>

  <xsl:variable name="foros-specific-hosts">
    <xsl:for-each select="$xpath/
      serviceGroup[@descriptor = $ad-cluster-descriptor]//service[not(@descriptor = $tr-services/@descriptor)]">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="'SMS-foros-hosts'"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="isp-zone-specific-hosts">
    <xsl:for-each select="exsl:node-set($tr-hosts)/*">
      <xsl:variable name="h" select="."/>
      <xsl:if test="count(exsl:node-set($foros-specific-hosts)/*[. = $h]) = 0">
        <host><xsl:value-of select="$h"/></host>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="server-root">
    <xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0">
      <xsl:value-of select="$def-server-root"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="server-bin-root">
    <xsl:value-of select="$env-config/@server_bin_root"/>
    <xsl:if test="count($env-config/@server_bin_root) = 0">
      <xsl:value-of select="$server-root"/>
    </xsl:if>
    <xsl:value-of select="'/bin'"/>
  </xsl:variable>

  <xsl:variable name="workspace-root">
    <xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0">
      <xsl:value-of select="$def-workspace-root"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root">
    <xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0">
      <xsl:value-of select="$def-config-root"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="state-root">
    <xsl:call-template name="GetAutoRestartFolder">
      <xsl:with-param name="app-env" select="$app-config"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="ssh-key">
    <xsl:call-template name="PrivateKeyFile">
      <xsl:with-param name="product-identifier" select="$PRODUCT_IDENTIFIER"/>
      <xsl:with-param name="app-env" select="$app-zone-config"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="ssh-argc"><xsl:value-of
    select="concat('-o &quot;BatchMode yes&quot; -i ', $ssh-key, ' %%HOST%%')"/>
  </xsl:variable>

  <xsl:variable name="ssh">ssh <xsl:value-of select="$ssh-argc"/></xsl:variable>

  <xsl:variable name="script"><xsl:value-of select="$server-root"/>/bin/adserver.pl</xsl:variable>
  <xsl:variable name="command">&quot;. <xsl:value-of
    select="$config-root"/>/<xsl:value-of
    select="$colo-name"/>/%%env%%/environment.sh &amp;&amp; $SMS_TEXT&quot;</xsl:variable>

<!-- Begin output-->
  <AdServer sms-version="3" init="%%SUDO%%"
    transport="{concat($ssh, ' ', $command)}" env="">
    <xsl:attribute name="brief">version <xsl:value-of select="$app-version"/> configuration for <xsl:value-of
      select="$colocation-name"/> colocation released at <xsl:value-of select="$current-time"/>
    </xsl:attribute>
    <xsl:attribute name="user">
       <xsl:call-template name="GetUserName">
         <xsl:with-param name="app-env" select="$app-zone-config"/>
       </xsl:call-template>
    </xsl:attribute>
    <xsl:if test="string-length($state-root) > 0">
    <xsl:attribute name="state_dir">
      <xsl:value-of select="$state-root"/>
    </xsl:attribute>
    <xsl:attribute name="transport_local">
      <xsl:value-of select="concat('/bin/sh -c ', $command)"/>
    </xsl:attribute>
    </xsl:if>

    <xsl:variable name="count-uim-clusters"
      select="count($adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor])"/>
    <xsl:variable name="ui-backup">
      <xsl:choose>
        <xsl:when test="$count-uim-clusters = 1">
          <xsl:value-of select="'false'"/>
        </xsl:when>
        <xsl:when test="$colo-config/cfg:coloParams/@backup_operations = '0'">
          <xsl:value-of select="'false'"/>
        </xsl:when>
        <xsl:when test="$colo-config/cfg:coloParams/@backup_operations = 'false'">
          <xsl:value-of select="'false'"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'true'"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="adfe-present"
      select="count($adcluster-path/serviceGroup[@descriptor =
        $fe-cluster-descriptor]/service[@descriptor = $http-frontend-descriptor]) > 0"/>

    <BatchCommands>
      <BatchCommand name="keyscan">
        <Command command="start">
          <Group>keyscan</Group>
        </Command>
      </BatchCommand>

      <BatchCommand name="test-upgrade-zdt1" batch_init=" ">
        <Command command="start">
          <Group>test-upgrade-zdt1</Group>
        </Command>
      </BatchCommand>

      <BatchCommand name="upgrade-zdt1" batch_init=" ">
        <Command command="start">
          <Group>upgrade-zdt1</Group>
        </Command>
      </BatchCommand>

      <BatchCommand name="list-packages-zdt1" batch_init=" ">
        <Command command="start" >
          <Group>list-packages-zdt1</Group>
        </Command>
      </BatchCommand>

      <xsl:if test="$adfe-present">
        <BatchCommand name="list-packages-pagesense" batch_init=" ">
          <Command command="start">
            <Group>list-packages-pagesense</Group>
          </Command>
        </BatchCommand>
      </xsl:if>

      <xsl:if test="count($adcluster-path) > 0">

        <!-- UserInfo redistribution belong Phorm zone -->
        <xsl:if test="$isp-zone = '0'">
          <xsl:choose>
            <xsl:when test="$ui-backup = 'false'">
              <BatchCommand name="redistribute-ui">
                <Command command="stop">
                  <Group>ui</Group>
                </Command>
                <Command command="start">
                  <Group>ui-distribution</Group>
                </Command>
              </BatchCommand>
            </xsl:when>
            <xsl:otherwise>
              <xsl:for-each select="$adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
                <xsl:variable name="pos" select="position()"/>
                <BatchCommand name="{concat('redistribute-fe', $pos)}">
                  <Command command="stop">
                    <Group>ui<xsl:value-of select="$pos"/></Group>
                  </Command>
                  <Command command="start">
                    <Group>ui<xsl:value-of select="$pos"/>-distribution</Group>
                  </Command>
                </BatchCommand>
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:for-each select="$adcluster-path/serviceGroup[@descriptor
            = $fe-cluster-descriptor]">
            <xsl:if test="count(./service[@descriptor = $user-bind-server-descriptor]) > 0">
              <xsl:variable name="pos" select="position()"/>
              <BatchCommand name="{concat('redistribute-ub', $pos)}">
                <Command command="stop">
                  <Group>ub<xsl:value-of select="$pos"/></Group>
                </Command>
                <Command command="start">
                  <Group>ub<xsl:value-of select="$pos"/>-distribution</Group>
                </Command>
              </BatchCommand>
            </xsl:if>
          </xsl:for-each>

          <BatchCommand name="redistribute-em">
            <Command command="stop">
              <Group>em</Group>
            </Command>
            <Command command="start">
              <Group>em-distribution</Group>
            </Command>
          </BatchCommand>
        </xsl:if>

        <BatchCommand name="clear-stat-backup">
          <Command command="start">
            <Group>clear-stat-backup</Group>
          </Command>
        </BatchCommand>
      </xsl:if>
      <xsl:if test="count($adcluster-path) > 0 or
          count($proxycluster-path) > 0">
        <BatchCommand name="troubleshoot">
          <Command command="start">
            <Group>troubleshoot</Group>
          </Command>
        </BatchCommand>
      </xsl:if>
    </BatchCommands>

    <xsl:if test="count($adcluster-path) > 0">
      <xsl:call-template name="Services">
        <xsl:with-param name="env" select="'adcluster'"/>
        <xsl:with-param name="cluster-path" select="$adcluster-path"/>
        <xsl:with-param name="script" select="$script"/>
      </xsl:call-template>

      <xsl:call-template name="Tasks">
        <xsl:with-param name="env" select="'adcluster'"/>
        <xsl:with-param name="app-config" select="$app-config"/>
        <xsl:with-param name="colo-config" select="$adcluster-path/configuration/cfg:cluster"/>
        <xsl:with-param name="cluster-path" select="$adcluster-path"/>
        <xsl:with-param name="script" select="$script"/>
        <xsl:with-param name="server-root" select="$server-root"/>
        <xsl:with-param name="config-root" select="$config-root"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
        <xsl:with-param name="ssh-key" select="$ssh-key"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="$isp-zone = '0'">
    <xsl:for-each select="$proxycluster-path">
      <xsl:variable name="env">adproxycluster-<xsl:value-of select="position()"/></xsl:variable>
      <xsl:call-template name="Services">
        <xsl:with-param name="env" select="$env"/>
        <xsl:with-param name="cluster-path" select="."/>
        <xsl:with-param name="script" select="$script"/>
        <xsl:with-param name="number" select="position()"/>
      </xsl:call-template>
      <xsl:call-template name="Tasks">
        <xsl:with-param name="env" select="$env"/>
        <xsl:with-param name="app-config" select="$app-config"/>
        <xsl:with-param name="colo-config" select="$proxycluster-path/configuration/cfg:cluster"/>
        <xsl:with-param name="cluster-path" select="."/>
        <xsl:with-param name="script" select="$script"/>
        <xsl:with-param name="number" select="position()"/>
        <xsl:with-param name="server-root" select="$server-root"/>
        <xsl:with-param name="config-root" select="$config-root"/>
        <xsl:with-param name="workspace-root" select="$workspace-root"/>
      </xsl:call-template>
    </xsl:for-each>
    </xsl:if>
    <xsl:call-template name="Troubleshoot">
      <xsl:with-param name="cluster-path"
        select="$adcluster-path | $proxycluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
      <xsl:with-param name="config-root" select="$config-root"/>
      <xsl:with-param name="workspace-root" select="$workspace-root"/>
      <xsl:with-param name="ssh-argc" select="$ssh-argc"/>
      <xsl:with-param name="server-bin-root" select="$server-bin-root"/>
      <xsl:with-param name="isp-zone-specific-hosts" select="$isp-zone-specific-hosts"/>
    </xsl:call-template>
    <Groups>
      <xsl:if test="count($adcluster-path) > 0 and $isp-zone = '0'">
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'be dbaccess'"/>
        </xsl:call-template>
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'lp'"/>
          <xsl:with-param name="exclude-groups" select="'em-distribution'"/>
        </xsl:call-template>
        <xsl:call-template name="AddLpGroups"/>
        <xsl:variable name="localproxy-path" select="$adcluster-path/
          serviceGroup[@descriptor = $be-cluster-descriptor]/
          serviceGroup[@descriptor = $local-proxy-descriptor]"/>
        <xsl:if test="count($localproxy-path) > 0">
          <xsl:call-template name="AddGroup">
            <xsl:with-param name="group-name" select="'localproxy'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="$ui-backup = 'false'">
          <xsl:call-template name="AddUIGroup">
            <xsl:with-param name="fe-comp" select="'ui'"/>
            <xsl:with-param name="ui-comp" select="'ui'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'em'"/>
          <xsl:with-param name="group-mask" select="'lp-ExpressionMatcher*,*'"/>
          <xsl:with-param name="exclude-groups" select="'em-distribution'"/>
        </xsl:call-template>
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'em-distribution'"/>
          <xsl:with-param name="group-mask" select="'*:lp-ExpressionMatcherDistributor'"/>
        </xsl:call-template>
        <xsl:for-each select="$adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
          <xsl:variable name="tr-comp" select="concat('tr', position())"/>
          <xsl:variable name="ui-comp" select="concat('ui', position())"/>
          <xsl:variable name="ub-comp" select="concat('ub', position())"/>
          <xsl:variable name="fe-comp" select="concat('fe', position())"/>
          <xsl:if test="count(./service[@descriptor = $user-bind-server-descriptor]) > 0">
            <xsl:call-template name="AddGroup">
              <xsl:with-param name="group-name" select="$ub-comp"/>
              <xsl:with-param name="group-mask" select="concat('*:', $fe-comp, '-UserBind*')"/>
              <xsl:with-param name="exclude-groups" select="concat($ub-comp, '-distribution')"/>
            </xsl:call-template>
            <xsl:call-template name="AddGroup">
              <xsl:with-param name="group-name" select="concat($ub-comp, '-distribution')"/>
              <xsl:with-param name="group-mask" select="concat('*:', $fe-comp, '-UserBindDistributor')"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="$ui-backup = 'true'">
              <xsl:call-template name="AddUIGroup">
                <xsl:with-param name="fe-comp" select="$fe-comp"/>
                <xsl:with-param name="ui-comp" select="$ui-comp"/>
              </xsl:call-template>
              <xsl:call-template name="AddGroup">
                <xsl:with-param name="group-name" select="$fe-comp"/>
                <xsl:with-param name="group-mask" select="concat($fe-comp, '-*,*')"/>
                <xsl:with-param name="exclude-groups" select="concat($ui-comp, '-distribution')"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="AddGroup">
                <xsl:with-param name="group-name" select="$fe-comp"/>
                <xsl:with-param name="group-mask" select="concat($fe-comp, '-*,*')"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:if test="$nil-isp-zone = '1'">
            <xsl:call-template name="AddGroup">
              <xsl:with-param name="group-name" select="$tr-comp"/>
              <xsl:with-param name="group-mask" select="concat($tr-comp, '-*,*')"/>
            </xsl:call-template>
          </xsl:if>
        </xsl:for-each>
      </xsl:if>
      <xsl:if test="count($adcluster-path) > 0 and $isp-zone = '1'">
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'dbaccess'"/>
        </xsl:call-template>
        <xsl:for-each select="$adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
          <Group name="{concat('tr', position())}">
            <Mask><xsl:value-of select="concat('tr', position())"/>-*,*</Mask>
          </Group>
        </xsl:for-each>
      </xsl:if>
      <xsl:if test="$isp-zone = '0'">
        <xsl:for-each select="$proxycluster-path">
          <xsl:call-template name="AddGroup">
            <xsl:with-param name="group-name" select="concat('pbe', position())"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:if>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'clear-stat-backup'"/>
      </xsl:call-template>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'keyscan'"/>
      </xsl:call-template>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'troubleshoot'"/>
      </xsl:call-template>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'test-upgrade-zdt1'"/>
        <xsl:with-param name="group-mask" select="'*:test-upgrade-zdt1'"/>
      </xsl:call-template>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'upgrade-zdt1'"/>
        <xsl:with-param name="group-mask" select="'*:upgrade-zdt1'"/>
      </xsl:call-template>
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'list-packages-zdt1'"/>
        <xsl:with-param name="group-mask" select="'*:list-packages-zdt1'"/>
      </xsl:call-template>

      <xsl:if test="$adfe-present">
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'list-packages-pagesense'"/>
          <xsl:with-param name="group-mask" select="'*:list-packages-pagesense'"/>
        </xsl:call-template>
      </xsl:if>
      <!--xsl:variable name="all-zones-exclude" select="'troubleshoot keyscan'"-->
      <xsl:variable name="exclude1">
        <xsl:value-of select="'troubleshoot keyscan clear-stat-backup test-upgrade-zdt1 upgrade-zdt1 list-packages-zdt1'"/>
        <xsl:if test="$adfe-present">
          <xsl:value-of select="' list-packages-pagesense'"/>
        </xsl:if>
        <xsl:if test="count($adcluster-path//service[@descriptor = $log-generalizer-descriptor or
          @descriptor = $channel-server-descriptor or @descriptor = $campaign-server-descriptor]) > 0">
            <xsl:value-of select="' dbaccess'"/>
        </xsl:if>
      </xsl:variable>
      <!-- Groups for Autorestart needs -->
      <xsl:for-each select="/colo:colocation/host">
        <xsl:variable name="real-host">
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="@name"/>
            <xsl:with-param name="error-prefix" select="'SMS'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($isp-zone-specific-hosts)[host = $real-host]) > 0">
            <xsl:if test="$isp-zone = '1'">
              <xsl:call-template name="AddGroup">
                <xsl:with-param name="group-name" select="$real-host"/>
                <xsl:with-param name="group-mask" select="concat('*,', $real-host)"/>
                <xsl:with-param name="exclude-groups" select="$exclude1"/>
              </xsl:call-template>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$isp-zone = '0' or count(exsl:node-set($tr-hosts)[host = $real-host]) > 0">
            <xsl:call-template name="AddGroup">
              <xsl:with-param name="group-name" select="$real-host"/>
              <xsl:with-param name="group-mask" select="concat('*,', $real-host)"/>
              <xsl:with-param name="exclude-groups" select="$exclude1"/>
            </xsl:call-template>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>
      <xsl:variable name="exclude2"><xsl:value-of
        select="$exclude1"/><xsl:if test="count($adcluster-path) > 0 and $isp-zone = '0'">
        <xsl:choose>
          <xsl:when test="$ui-backup = 'false'">
            <xsl:value-of select="' ui-distribution'"/>
          </xsl:when>
          <xsl:otherwise><xsl:for-each
          select="$adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"><xsl:value-of
            select="concat(' ui', position(), '-distribution')"/></xsl:for-each>
          </xsl:otherwise>
        </xsl:choose> em-distribution</xsl:if>
      </xsl:variable>
      <!-- This groups run 'by default', if empty command line -->
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'all'"/>
        <xsl:with-param name="group-mask" select="'*,*'"/>
        <xsl:with-param name="exclude-groups" select="$exclude2"/>
      </xsl:call-template>
    </Groups>

    <Dependences>

      <xsl:variable name="prestart-dep">
        <xsl:if test="count($adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]) -
          count($adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]/
            configuration/cfg:frontendCluster/cfg:startParams[
            @wait_loading = 'false' or @wait_loading = '0']) > 0"> AdServer::PreStart</xsl:if>
      </xsl:variable>
      <xsl:variable
        name="fe-cluster-path"
        select="$adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>
      <xsl:variable name="fcgi-dep">
        <xsl:value-of select="'AdServer::Frontends::FCGIAdServer
          AdServer::Frontends::FCGIUserBindServer
          AdServer::Frontends::FCGIUserBindIntServer
          AdServer::Frontends::FCGIUserBindAddServer AdServer::Frontends::FCGIRtbServer '"/>
      </xsl:variable>
      <xsl:variable name="fe-services">
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $profiling-server-descriptor]) > 0">
          <xsl:value-of select="'AdServer::Frontends::ProfilingServer '"/>
        </xsl:if>
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $http-frontend-descriptor]) > 0">
          <xsl:value-of select="$fcgi-dep"/><xsl:value-of select="' AdServer::HttpFrontend '"/>
        </xsl:if>
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $uid-generator-adapter-descriptor]) > 0">
          <xsl:value-of select="'AdServer::Frontends::UIDGeneratorAdapter '"/>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="campaign-manager-dep">
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $campaign-manager-descriptor]) > 0">
          <xsl:value-of select="'AdServer::CampaignSvcs::CampaignManager '"/>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="conv-server-dep">
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $conv-server-descriptor]) > 0">
          <xsl:value-of select="'AdServer::Frontends::ConvServer '"/>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="psconfig-dep">
        <xsl:if test="count($fe-cluster-path/service[@descriptor = $http-frontend-descriptor]) > 0">
          <xsl:value-of select="'AdServer::PSConfigurator '"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="sync-logs-dep">
        <xsl:choose>
          <xsl:when test="count($be-cluster-path/service[@descriptor = $predictor-descriptor]) > 0">
            <xsl:value-of select="'AdServer::LogProcessing::SyncLogsServer
              AdServer::LogProcessing::ExpressionMatcherChecker
              AdServer::Predictor::SyncLogsServer'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'AdServer::LogProcessing::SyncLogsServer
              AdServer::LogProcessing::ExpressionMatcherChecker'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="$nil-isp-zone = '1'">

        <xsl:if test="count($adcluster-path) > 0">
          <xsl:variable name="localproxy-path"
            select="$be-cluster-path/serviceGroup[@descriptor = $local-proxy-descriptor]"/>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer'"/>
            <xsl:with-param name="slaves" select=
              "concat($campaign-manager-dep, 'AdServer::ChannelSvcs::ChannelServer
                AdServer::UserInfoSvcs::UserInfoManager AdServer::LogProcessing::ExpressionMatcher')"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::BillingServer'"/>
            <xsl:with-param name="slaves" select="$campaign-manager-dep"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="slaves" select="'AdServer::ChannelSvcs::ChannelController'"/>
            <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer
              AdServer::ChannelSvcs::ChannelServer'"/>
          </xsl:call-template>
          <xsl:if test="count($adcluster-path/serviceGroup[@descriptor =
            $fe-cluster-descriptor]/service[@descriptor = $channel-search-service-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="slaves" select="'AdServer::ChannelSearchSvcs::ChannelSearchService'"/>
              <xsl:with-param name="masters" select=
                "concat($campaign-manager-dep, 'AdServer::ChannelSvcs::ChannelController')"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:if test="count($be-cluster-path/service[@descriptor = $dictionary-provider-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::DictionaryProvider'"/>
              <xsl:with-param name="slaves" select= "'AdServer::ChannelSvcs::ChannelServer'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::LogProcessing::LogGeneralizer'"/>
            <xsl:with-param name="slaves" select= "'AdServer::CampaignSvcs::CampaignServer'"/>
          </xsl:call-template>
          <xsl:if test="count($localproxy-path) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="slaves" select="'AdServer::ChannelSvcs::ChannelController'"/>
              <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::ChannelProxy'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select=
              "'AdServer::ChannelSvcs::ChannelServer
                AdServer::CampaignSvcs::CampaignServer AdServer::LogProcessing::LogGeneralizer'"/>
            <xsl:with-param name="slaves" select= "'AdServer::DBAccess'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserBindDistributor'"/>
            <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserBindChecker'"/>
          </xsl:call-template>
          <xsl:if test="count($fe-cluster-path/service[@descriptor = $user-bind-server-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserBindChecker'"/>
              <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserBindServer'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select= "'AdServer::UserInfoSvcs::UserBindServer'"/>
            <xsl:with-param name="slaves" select="concat('AdServer::UserInfoSvcs::UserBindController',
              $prestart-dep)"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoDistributor'"/>
            <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserInfoChecker'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoChecker'"/>
            <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
          </xsl:call-template>
          <xsl:if test="count($be-cluster-path/service[@descriptor = $stats-collector-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::Controlling::StatsCollector'"/>
              <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
            <xsl:with-param name="slaves" select="concat('AdServer::UserInfoSvcs::UserInfoManagerController',
              $prestart-dep)"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoManagerController
              AdServer::UserInfoSvcs::UserBindController 
              AdServer::LogProcessing::ExpressionMatcherChecker'"/>
            <xsl:with-param name="slaves" select= "concat('AdServer::LogProcessing::ExpressionMatcher
              AdServer::RequestInfoSvcs::RequestInfoManager ', $fe-services)"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::LogProcessing::ExpressionMatcher'"/>
            <xsl:with-param name="slaves" select= "'AdServer::RequestInfoSvcs::RequestInfoManager'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="$campaign-manager-dep"/>
            <xsl:with-param name="slaves" select= "$conv-server-dep"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="concat('AdServer::ChannelSvcs::ChannelController ',
              $campaign-manager-dep,
              $conv-server-dep)"/>
            <xsl:with-param name="slaves" select= "concat($fe-services, $prestart-dep)"/>
          </xsl:call-template>
          <xsl:if test="count($be-cluster-path/service[@descriptor = $stats-collector-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::Controlling::StatsCollector'"/>
              <xsl:with-param name="slaves" select= "$fe-services"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="concat($psconfig-dep, $prestart-dep)"/>
            <xsl:with-param name="slaves" select= "$fe-services"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="$sync-logs-dep"/>
            <xsl:with-param name="slaves" select= "'AdServer::LogProcessing::SyncLogs'"/>
          </xsl:call-template>
          <xsl:if test="count($be-cluster-path/service[@descriptor = $predictor-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::Predictor::SyncLogsServer'"/>
              <xsl:with-param name="slaves" select= "'AdServer::Predictor::Merger'"/>
            </xsl:call-template>
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::Predictor::Merger'"/>
              <xsl:with-param name="slaves" select= "'AdServer::Predictor::SVMGenerator'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:if test="count($be-cluster-path/service[@descriptor = $user-operation-generator-descriptor]) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoChecker'"/>
              <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserOperationGenerator'"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters"
            select="'AdServer::Frontends::FCGIAdServer
            AdServer::Frontends::FCGIUserBindServer
            AdServer::Frontends::FCGIUserBindIntServer
            AdServer::Frontends::FCGIUserBindAddServer AdServer::Frontends::FCGIRtbServer'"/>
            <xsl:with-param name="slaves" select="'AdServer::HttpFrontend'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:call-template name="PhormZoneCommonDependencies">
          <xsl:with-param name="proxycluster-path" select="$proxycluster-path"/>
          <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
        </xsl:call-template>

        <xsl:if test="count($fe-cluster-path/service[@descriptor = $zmq-profiling-balancer-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Frontends::ProfilingServer'"/>
            <xsl:with-param name="slaves" select= "'AdServer::Utils::ZmqProfilingBalancer'"/>
          </xsl:call-template>
        </xsl:if>

        </xsl:when>
        <xsl:when test="$isp-zone = '0'">
      <xsl:if test="count($adcluster-path) > 0">
        <xsl:variable name="localproxy-path"
          select="$be-cluster-path/serviceGroup[@descriptor = $local-proxy-descriptor]"/>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer'"/>
          <xsl:with-param name="slaves" select=
            "concat($campaign-manager-dep, 'AdServer::CampaignSvcs::BillingServer 
               AdServer::UserInfoSvcs::UserInfoManager 
               AdServer::LogProcessing::ExpressionMatcher')"/>
        </xsl:call-template>
        <xsl:if test="count($adcluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]/service[@descriptor = $channel-search-service-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="slaves" select="'AdServer::ChannelSearchSvcs::ChannelSearchService'"/>
            <xsl:with-param name="masters" select="$campaign-manager-dep"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::LogProcessing::LogGeneralizer'"/>
          <xsl:with-param name="slaves" select= "'AdServer::CampaignSvcs::CampaignServer'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer 
            AdServer::LogProcessing::LogGeneralizer'"/>
          <xsl:with-param name="slaves" select= "'AdServer::DBAccess'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoChecker'"/>
          <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
        </xsl:call-template>
        <xsl:if test="count($be-cluster-path/service[@descriptor = $stats-collector-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Controlling::StatsCollector'"/>
            <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select= "'AdServer::UserInfoSvcs::UserInfoManager'"/>
          <xsl:with-param name="slaves" select="'AdServer::UserInfoSvcs::UserInfoManagerController'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoManagerController
            AdServer::LogProcessing::ExpressionMatcherChecker'"/>
          <xsl:with-param name="slaves" select= "'AdServer::LogProcessing::ExpressionMatcher AdServer::RequestInfoSvcs::RequestInfoManager'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::LogProcessing::ExpressionMatcher'"/>
          <xsl:with-param name="slaves" select= "'AdServer::RequestInfoSvcs::RequestInfoManager'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="$sync-logs-dep"/>
          <xsl:with-param name="slaves" select= "'AdServer::LogProcessing::SyncLogs'"/>
        </xsl:call-template>
        <xsl:if test="count($be-cluster-path/service[@descriptor = $predictor-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Predictor::SyncLogsServer'"/>
            <xsl:with-param name="slaves" select= "'AdServer::Predictor::Merger'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Predictor::Merger'"/>
            <xsl:with-param name="slaves" select= "'AdServer::Predictor::SVMGenerator'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="count($be-cluster-path/service[@descriptor = $user-operation-generator-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::UserInfoSvcs::UserInfoChecker'"/>
            <xsl:with-param name="slaves" select= "'AdServer::UserInfoSvcs::UserOperationGenerator'"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:if>
      <xsl:call-template name="PhormZoneCommonDependencies">
        <xsl:with-param name="proxycluster-path" select="$proxycluster-path"/>
        <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      </xsl:call-template>

        </xsl:when>

        <xsl:otherwise>

      <xsl:if test="count($adcluster-path) > 0">

        <xsl:call-template name="AddDependence">
          <xsl:with-param name="slaves" select="'AdServer::ChannelSvcs::ChannelController'"/>
          <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::ChannelServer'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select=
            "'AdServer::ChannelSvcs::ChannelServer'"/>
          <xsl:with-param name="slaves" select= "'AdServer::DBAccess'"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::ChannelController'"/>
          <xsl:with-param name="slaves" select= "$prestart-dep"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::ChannelController'"/>
          <xsl:with-param name="slaves" select= "$fe-services"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="concat($psconfig-dep, $prestart-dep)"/>
          <xsl:with-param name="slaves" select= "$fe-services"/>
        </xsl:call-template>
        <xsl:call-template name="AddDependence">
          <xsl:with-param name="masters" select="$fcgi-dep"/>
          <xsl:with-param name="slaves" select="'AdServer::HttpFrontend'"/>
        </xsl:call-template>

        <xsl:if test="count($fe-cluster-path/service[@descriptor = $zmq-profiling-balancer-descriptor]) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Frontends::ProfilingServer'"/>
            <xsl:with-param name="slaves" select= "'AdServer::Utils::ZmqProfilingBalancer'"/>
          </xsl:call-template>
        </xsl:if>

      </xsl:if>

        </xsl:otherwise>
      </xsl:choose>

    </Dependences>
  </AdServer>
</xsl:template>

</xsl:stylesheet>

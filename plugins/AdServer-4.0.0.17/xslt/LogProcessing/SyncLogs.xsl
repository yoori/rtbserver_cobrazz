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

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="HostsStringGenerator">
  <xsl:param name="service-path"/>
  <xsl:param name="error-prefix"/>

  <xsl:variable name="hosts">
    <xsl:for-each select="$service-path">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="$error-prefix"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="sorted-hosts">
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <xsl:sort select="."/>
      <host><xsl:value-of select="."/></host>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($sorted-hosts)//host">
    <xsl:variable name="host" select="text()"/>
    <xsl:if test="count(preceding-sibling::host[text() = $host]) = 0">
      <xsl:if test="position() != 1">,</xsl:if><xsl:value-of select="$host"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- Route -->
<xsl:template name="Route">
  <xsl:param name="type"/>
  <xsl:param name="source-path-base"/>
  <xsl:param name="destination-path-base"/>
  <xsl:param name="source-hosts"/>
  <xsl:param name="destination-hosts"/>
  <xsl:param name="dirs"/>
  <xsl:param name="pattern"/>

  <xsl:if test="string-length($source-hosts) > 0">
    <cfg:Route post_command="touch ##SRC_DIR##/~##FILE_NAME##.commit.##DST_HOST##"
      type="{$type}">

      <xsl:for-each select="exsl:node-set($dirs)//dir">
        <xsl:variable name="dir" select="text()"/>
        <xsl:variable name="source"
          select="concat($source-path-base, 'Out/', $dir, '/', $dir, '.2*')"/>
        <xsl:variable name="destination"
          select="concat($destination-path-base, $dir)"/>

        <cfg:files source="{$source}" destination="{$destination}">
          <xsl:if test="string-length($pattern) > 0">
            <xsl:attribute name="pattern">
              <xsl:value-of select="$pattern"/>
            </xsl:attribute>
          </xsl:if>
        </cfg:files>
      </xsl:for-each>

      <cfg:hosts source="{$source-hosts}" destination="{$destination-hosts}"/>
    </cfg:Route>

    <!-- route for commit files -->
    <cfg:Route type="HostName" pattern="\.commit\.(.*)$">
      <xsl:for-each select="exsl:node-set($dirs)//dir">
        <xsl:variable name="dir" select="text()"/>
        <xsl:variable name="source"
          select="concat($source-path-base, 'Out/', $dir, '/~', $dir, '.2*.commit.*')"/>
        <xsl:variable name="destination"
          select="concat($destination-path-base, $dir)"/>

        <cfg:files source="{$source}" destination="{$destination}"/>
      </xsl:for-each>

      <cfg:hosts source="{$source-hosts}" destination="{$destination-hosts}"/>
    </cfg:Route>
  </xsl:if>
</xsl:template>


<!-- SyncLogs config generate function -->
<xsl:template name="SyncLogsConfigGenerator">
  <xsl:param name="log-processing-config"/>

  <xsl:param name="app-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="env-config"/>

  <xsl:param name="full-cluster-path"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="be-cluster-path"/>

  <xsl:param name="proxy-config"/>
  <xsl:param name="channel-serving-path"/>

  <!-- optional -->
  <xsl:param name="stunnel-client-config"/>
  <xsl:param name="stunnel-client-path"/>

    <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
      <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="server-config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="colo-config-root">
      <xsl:value-of select="$server-config-root"/>/<xsl:value-of select="$colo-name"/>
    </xsl:variable>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
      <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="sync-logs-config" select="$log-processing-config/cfg:syncLogs"/>
    <xsl:variable name="sync-logs-server-config" select="$log-processing-config/cfg:syncLogsServer"/>

    <xsl:variable name="sync-logs-port">
      <xsl:value-of select="$sync-logs-config/cfg:networkParams/@port"/>
      <xsl:if test="count($sync-logs-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-sync-logs-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="syncLogs.port"
      method="text" omit-xml-declaration="yes"
      >  ['syncLogs', <xsl:copy-of select="$sync-logs-port"/>],</exsl:document>

    <xsl:variable name="check-logs-period"><xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@file_check_period"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@file_check_period) = 0">
        <xsl:value-of select="$def-check-logs-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="host-check-period"><xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@host_check_period"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@host_check_period) = 0">
        <xsl:value-of select="$def-host-check-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="content-check-period"><xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@content_check_period"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@content_check_period) = 0">
        <xsl:value-of select="$def-sync-log-content-check-period"/>
      </xsl:if>
    </xsl:variable>


    <xsl:variable name="internal-content-check-period"><xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@internal_content_check_period"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@internal_content_check_period) = 0">
        <xsl:value-of select="$def-sync-log-content-check-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="log-root-dir" select="concat($workspace-root, '/log/SyncLogs')"/>
    <xsl:variable name="log-files-root-dir" select="concat($workspace-root, '/log')"/>

  <cfg:SyncLogsConfig
    log_root="{$log-root-dir}"
    check_logs_period="{$check-logs-period}"
    host_check_period="{$host-check-period}"
    hostname="{$HOST}">

    <xsl:variable name="remote-dest-port">
      <xsl:value-of select="$sync-logs-server-config/cfg:networkParams/@port"/>
      <xsl:if test="count($sync-logs-server-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-sync-logs-server-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="logs-backup">
      <xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@logs_backup"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@logs_backup) = 0">
        <xsl:value-of select="'false'"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="data-source-ref" select="$colo-config/cfg:central/cfg:dataSourceRef"/>

    <xsl:variable name="backup-command-prefix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup != 'disable')">
          <xsl:value-of select="$colo-config-root"/><![CDATA[/copy_and_backup.sh ']]></xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="backup-command-postfix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup != 'disable')"><![CDATA[' '##SRC_PATH##']]></xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="local-copy-command"><xsl:value-of
        select="concat($backup-command-prefix,
        '/usr/bin/rsync -t -z --timeout=55 --log-format=%f --ignore-existing ##SRC_PATH## ',
        $log-files-root-dir, '##DST_PATH##', $backup-command-postfix)"/>
    </xsl:variable>

    <xsl:variable name="remote-copy-command">
      <xsl:value-of select="concat($backup-command-prefix,
        '/usr/bin/rsync -t -z --timeout=55 --log-format=%f ##SRC_PATH## rsync://##DST_HOST##:',
        $remote-dest-port, '/ad-logs##DST_PATH##', $backup-command-postfix)"/>
    </xsl:variable>

    <!-- check that defined all needed parameters -->
    <xsl:choose>
      <xsl:when test="count($colo-config/cfg:central) > 0 and count($data-source-ref) = 0">
        <xsl:message terminate="yes">SyncLogs: dataSourceRef undefined.</xsl:message>
      </xsl:when>
    </xsl:choose>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$sync-logs-config/cfg:threadParams/@min"/>
        <xsl:if test="count($sync-logs-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-sync-logs-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$sync-logs-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$sync-logs-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $sync-logs-log-path)"/>
      <xsl:with-param name="default-log-level" select="$sync-logs-log-level"/>
    </xsl:call-template>

    <cfg:ClusterConfig
      root_logs_dir="{$log-files-root-dir}"
      definite_hash_schema="{concat($server-root,
        '/xsd/AdServerCommons/HostDistributionFile.xsd')}">

      <!-- init host lists -->
      <xsl:variable name="user-info-manager-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$fe-cluster-path/service[@descriptor = $user-info-manager-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'UserInfoManager hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="user-operation-generator-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $user-operation-generator-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'UserOperationGenerator hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <!-- Predictor sync log server -->
      <xsl:variable name="predictor-path"
          select="$be-cluster-path/service[@descriptor = $predictor-descriptor]"/>

      <xsl:variable name="predictor-hosts">
        <xsl:for-each select="$predictor-path">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'Predictor'"/>
          </xsl:call-template>
        </xsl:for-each>
      </xsl:variable>

      <xsl:variable name="predictor-sync-logs-port">
        <xsl:choose>
          <xsl:when test="count($predictor-path) != 0">
            <xsl:value-of select="$predictor-path//cfg:syncServer/cfg:networkParams/@port"/>
            <xsl:if test="count($predictor-path//cfg:syncServer/cfg:networkParams/@port) = 0">
              <xsl:value-of select="$def-predictor-sync-logs-server-port"/>
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:ref/@port"/>
            <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:ref/@port) = 0">
              <xsl:value-of select="$def-predictor-sync-logs-server-port"/>
            </xsl:if>
          </xsl:otherwise>           
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="predictor-sync-logs-host">
        <xsl:value-of select="exsl:node-set($predictor-hosts)/host[1]"/>
        <xsl:if test="count(exsl:node-set($predictor-hosts)/host) = 0">
          <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:ref/@host"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="predictor-ctr-path">
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($predictor-hosts)/host) > 0">ctr</xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:CTRConfig/@ctr_path"/>
            <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:CTRConfig/@ctr_path) = 0">ctr</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="predictor-conv-path">
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($predictor-hosts)/host) > 0">conv</xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:CTRConfig/@conv_path"/>
            <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:CTRConfig/@conv_path) = 0">conv</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="research-stat-receiver-path">
        <xsl:choose>
          <xsl:when test="count(exsl:node-set($predictor-hosts)/host) > 0">logs</xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$colo-config/cfg:predictorConfig/cfg:researchStat/@log_path"/>
            <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:researchStat/@log_path) = 0">logs</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
    
      <xsl:variable name="user-bind-server-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$fe-cluster-path/service[@descriptor = $user-bind-server-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'UserBindServer hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="campaign-manager-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$fe-cluster-path/service[@descriptor = $campaign-manager-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'CampaignManager hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="campaign-manager-and-adfrontend-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$fe-cluster-path/service[
              @descriptor = $campaign-manager-descriptor or
              @descriptor = $http-frontend-descriptor]"/>
          <xsl:with-param name="error-prefix"
            select="'CampaignManager, AdFrontend hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="campaign-server-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'CampaignServer hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="request-info-manager-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $request-info-manager-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'LogGeneralizer hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="expression-matcher-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $expression-matcher-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'ExpressionMatcher hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="log-generalizer-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $log-generalizer-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'LogGeneralizer hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="stat-receiver-hosts">
        <xsl:call-template name="HostsStringGenerator">
          <xsl:with-param name="service-path"
            select="$be-cluster-path/service[@descriptor = $stat-receiver-descriptor]"/>
          <xsl:with-param name="error-prefix" select="'StatReceiver hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="channel-server-hosts">
        <xsl:call-template name="HostsStringGenerator">
           <xsl:with-param name="service-path"
             select="$channel-serving-path/service[@descriptor = $channel-server-descriptor]"/>
           <xsl:with-param name="error-prefix" select="'ChannelServer hosts resolving'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="expression-matcher-distribution">
        <xsl:value-of select="$workspace-root"/>/run/ExpressionMatcherDistribution.xml</xsl:variable>

      <cfg:FeedRouteGroup
        pool_threads="50"
        local_copy_command_type="rsync"
        remote_copy_command_type="rsync"
        tries_per_file="2"
        local_copy_command="{$local-copy-command}"
        remote_copy_command="{$remote-copy-command}"
        >

        <xsl:variable name="campaign-manager-route1">
          <dirs>
            <dir>CreativeStat</dir>
            <dir>OptOutStat</dir>
            <dir>WebStat</dir>
            <dir>ChannelTriggerStat</dir>
            <dir>ChannelHitStat</dir>
            <dir>ActionRequest</dir>
            <dir>PublisherInventory</dir>
            <dir>UserProperties</dir>
            <dir>CCGStat</dir>
            <dir>CCStat</dir>
            <dir>TagAuctionStat</dir>
            <dir>TagPositionStat</dir>
            <dir>PassbackStat</dir>
            <dir>SearchEngineStat</dir>
            <dir>UserAgentStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'CampaignManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$campaign-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$campaign-manager-route1"/>
        </xsl:call-template>

        <xsl:variable name="search-term-stat-route">
          <dirs>
            <dir>SearchTermStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'Hash'"/>
          <xsl:with-param name="source-path-base" select="'CampaignManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$campaign-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$search-term-stat-route"/>
          <xsl:with-param name="pattern" select="'.*\.##HASH##'"/>
        </xsl:call-template>

        <xsl:variable name="colo-update-stat-route">
          <dirs>
            <dir>ColoUpdateStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'CampaignServer/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$campaign-server-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$colo-update-stat-route"/>
        </xsl:call-template>

        <xsl:variable name="request-info-manager-route1">
          <dirs>
            <dir>Request</dir>
            <dir>Impression</dir>
            <dir>Click</dir>
            <dir>AdvertiserAction</dir>
            <dir>PassbackOpportunity</dir>
            <dir>PassbackImpression</dir>
            <dir>TagRequest</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'Hash'"/>
          <xsl:with-param name="source-path-base" select="'CampaignManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/RequestInfoManager/In/'"/>
          <xsl:with-param name="source-hosts" select="$campaign-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$request-info-manager-hosts"/>
          <xsl:with-param name="dirs" select="$request-info-manager-route1"/>
          <xsl:with-param name="pattern" select="'.*\.##HASH##'"/>
        </xsl:call-template>

        <xsl:variable name="request-info-manager-route2">
          <dirs>
            <dir>CreativeStat</dir>
            <dir>UserProperties</dir>
            <dir>ChannelPerformance</dir>
            <dir>ExpressionPerformance</dir>
            <dir>CCGKeywordStat</dir>
            <dir>SiteChannelStat</dir>
            <dir>AdvertiserUserStat</dir>
            <dir>ActionStat</dir>
            <dir>CMPStat</dir>
            <dir>PassbackStat</dir>
            <dir>ChannelImpInventory</dir>
            <dir>SiteUserStat</dir>
            <dir>SiteReferrerStat</dir>
            <dir>PageLoadsDailyStat</dir>
            <dir>CCGUserStat</dir>
            <dir>CCUserStat</dir>
            <dir>CampaignUserStat</dir>
            <dir>TagPositionStat</dir>
            <dir>CampaignReferrerStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'RequestInfoManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$request-info-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$request-info-manager-route2"/>
        </xsl:call-template>

        <xsl:variable name="request-info-manager-exchange-logs">
          <dirs>
            <dir>RequestOperation</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'Hash'"/>
          <xsl:with-param name="source-path-base" select="'RequestInfoManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/RequestInfoManager/In/'"/>
          <xsl:with-param name="source-hosts" select="$request-info-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$request-info-manager-hosts"/>
          <xsl:with-param name="dirs" select="$request-info-manager-exchange-logs"/>
          <xsl:with-param name="pattern" select="'.*\.##HASH##'"/>
        </xsl:call-template>

        <xsl:variable name="consider-logs">
          <dirs>
            <dir>ConsiderAction</dir>
            <dir>ConsiderClick</dir>
            <dir>ConsiderImpression</dir>
            <dir>ConsiderRequest</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'DefiniteHash'"/>
          <xsl:with-param name="source-path-base" select="'RequestInfoManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/ExpressionMatcher/In/'"/>
          <xsl:with-param name="source-hosts" select="$request-info-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$expression-matcher-distribution"/>
          <xsl:with-param name="dirs" select="$consider-logs"/>
          <xsl:with-param name="pattern" select="'.*\.##HASH##'"/>
        </xsl:call-template>

        <xsl:variable name="channel-server-route">
          <dirs>
            <dir>ColoUpdateStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'ChannelServer/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$channel-server-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$channel-server-route"/>
        </xsl:call-template>

        <xsl:variable name="expression-matcher-route">
          <dirs>
            <dir>ChannelImpInventory</dir>
            <dir>ChannelInventory</dir>
            <dir>ChannelPriceRange</dir>
            <dir>ChannelInventoryEstimationStat</dir>
            <dir>ChannelPerformance</dir>
            <dir>ChannelTriggerImpStat</dir>
            <dir>GlobalColoUserStat</dir>
            <dir>ColoUserStat</dir>
            <dir>ChannelOverlapUserStat</dir>
            <dir>CCGSelectionFailureStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'ExpressionMatcher/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$expression-matcher-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$expression-matcher-route"/>
        </xsl:call-template>

        <xsl:variable name="user-info-manager-route">
          <dirs>
            <dir>ChannelCountStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-path-base" select="'UserInfoManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/LogGeneralizer/In/'"/>
          <xsl:with-param name="source-hosts" select="$user-info-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$user-info-manager-route"/>
        </xsl:call-template>

        <cfg:Route type="DefiniteHash">
          <cfg:files pattern="UserOp(?:\.\d+){{5}}\.##HASH##">
            <xsl:attribute name="source">UserOperationGenerator/Out/UserOp/UserOp.*</xsl:attribute>
            <xsl:attribute name="destination">/UserInfoManager/In/ExternalUserOp/</xsl:attribute>
          </cfg:files>

          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$user-operation-generator-hosts"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of
              select="$workspace-root"/>/run/UserInfoDistribution.1.xml</xsl:attribute>
          </cfg:hosts>
        </cfg:Route>

        <cfg:Route type="DefiniteHash">
          <cfg:files pattern="UserOp(?:\.\d+){{5}}\.##HASH##">
            <xsl:attribute name="source">UserInfoManager/Out/ExternalUserOp/UserOp.*</xsl:attribute>
            <xsl:attribute name="destination">/UserInfoManager/In/ExternalUserOp/</xsl:attribute>
          </cfg:files>

          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of
              select="$user-info-manager-hosts"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of
              select="$workspace-root"/>/run/UserInfoDistribution.1.xml</xsl:attribute>
          </cfg:hosts>
        </cfg:Route>

        <xsl:comment>
          user operations exchange (internal cluster)
        </xsl:comment>

        <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
          <xsl:variable name="dest-cluster-id" select="position()"/>
          <cfg:Route type="DefiniteHash">
            <cfg:files pattern="UserOp(?:\.\d+){{5}}\.##HASH##">
              <xsl:attribute name="source">UserInfoManager/Out/UserOp_<xsl:value-of
                select="concat($dest-cluster-id, '/UserOp.2*.*')"/></xsl:attribute>
              <xsl:attribute name="destination">/UserInfoManager/In/UserOp_<xsl:value-of
                select="$dest-cluster-id"/>/</xsl:attribute>
            </cfg:files>

            <cfg:hosts>
              <xsl:attribute name="source"><xsl:value-of select="$user-info-manager-hosts"/></xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of
                select="$workspace-root"/>/run/UserInfoDistribution.<xsl:value-of
                select="$dest-cluster-id"/>.xml</xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </xsl:for-each>

        <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
          <xsl:variable name="dest-cluster-id" select="position()"/>
          <cfg:Route type="DefiniteHash">
            <cfg:files pattern="UserBindOp(?:\.\d+){{5}}\.##HASH##">
              <xsl:attribute name="source">UserBindServer/Out/UserBindOp_<xsl:value-of
                select="concat($dest-cluster-id, '/UserBindOp.2*.*')"/></xsl:attribute>
              <xsl:attribute name="destination">/UserBindServer/In/UserBindOp_<xsl:value-of
                select="$dest-cluster-id"/>/</xsl:attribute>
            </cfg:files>

            <cfg:hosts>
              <xsl:attribute name="source"><xsl:value-of select="$user-bind-server-hosts"/></xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of
                select="$workspace-root"/>/run/UserBindDistribution.<xsl:value-of
                select="$dest-cluster-id"/>.xml</xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </xsl:for-each>
      </cfg:FeedRouteGroup>


      <xsl:variable name="rbc-backup-command-prefix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup = 'all')">
          <xsl:value-of select="$colo-config-root"/><![CDATA[/copy_and_backup.sh ']]></xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="rbc-backup-command-postfix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup = 'all')"><![CDATA[' '##SRC_PATH##']]></xsl:when>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="rbc-local-copy-command"><xsl:value-of
        select="concat($rbc-backup-command-prefix,
        '/usr/bin/rsync -t -z --timeout=55 --log-format=%f --ignore-existing ##SRC_PATH## ',
        $log-files-root-dir, '##DST_PATH##', $rbc-backup-command-postfix)"/>
      </xsl:variable>

      <xsl:variable name="rbc-remote-copy-command">
        <xsl:value-of select="concat($rbc-backup-command-prefix,
          '/usr/bin/rsync -t -z --timeout=55 --log-format=%f ##SRC_PATH## rsync://##DST_HOST##:',
          $remote-dest-port, '/ad-logs##DST_PATH##', $rbc-backup-command-postfix)"/>
      </xsl:variable>

      <cfg:FeedRouteGroup
        pool_threads="20"
        local_copy_command_type="rsync"
        remote_copy_command_type="rsync"
        tries_per_file="2"
        local_copy_command="{$rbc-local-copy-command}"
        remote_copy_command="{$rbc-remote-copy-command}">

        <xsl:variable name="to-expression-matcher-route">
          <dirs>
            <dir>RequestBasicChannels</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'DefiniteHash'"/>
          <xsl:with-param name="source-path-base" select="'CampaignManager/'"/>
          <xsl:with-param name="destination-path-base" select="'/ExpressionMatcher/In/'"/>
          <xsl:with-param name="source-hosts" select="$campaign-manager-hosts"/>
          <xsl:with-param name="destination-hosts" select="$expression-matcher-distribution"/>
          <xsl:with-param name="dirs" select="$to-expression-matcher-route"/>
          <xsl:with-param name="pattern" select="'.*\.##HASH##'"/>
        </xsl:call-template>

      </cfg:FeedRouteGroup>

      <!-- copy ad-content from local proxy (remote) or source host (central) to campaign manager hosts -->
      <xsl:variable name="data-source-host">
        <xsl:choose>
          <xsl:when test="count($stunnel-client-config) > 0">
            <!-- remote -->
            <xsl:call-template name="ResolveHostName">
              <xsl:with-param name="base-host" select="$stunnel-client-path/@host"/>
              <xsl:with-param name="error-prefix" select="'SyncLogs'"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <!-- central -->
            <xsl:value-of select="$data-source-ref/@host"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="data-source-port">
        <xsl:choose>
          <xsl:when test="count($stunnel-client-config) > 0">
            <xsl:value-of select="$remote-dest-port"/>
          </xsl:when>
          <xsl:otherwise>
            <!-- central -->
            <xsl:value-of
              select="$data-source-ref/@port"/><xsl:if test="count($data-source-ref/@port) = 0">8873</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="data-source-path">
        <xsl:choose>
          <xsl:when test="count($stunnel-client-config) > 0">ad-content</xsl:when>
          <xsl:otherwise>
            <xsl:value-of
              select="$data-source-ref/@path"/><xsl:if
              test="count($data-source-ref/@path) = 0">filesender</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:if test="string-length($campaign-manager-and-adfrontend-hosts) > 0">
        <cfg:FeedRouteGroup
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true"
          check_logs_period="{$internal-content-check-period}">
          <xsl:variable name="copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://##DST_HOST##:]]><xsl:value-of
            select="$data-source-port"/>/<xsl:value-of
            select="$data-source-path"/><![CDATA[/##SRC_PATH## ##DST_PATH##]]></xsl:variable>
          <xsl:attribute name="remote_copy_command"><xsl:value-of select="$copy_command"/></xsl:attribute>
          <xsl:attribute name="local_copy_command"><xsl:value-of select="$copy_command"/></xsl:attribute>
          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">/tags/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/tags/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Creatives/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Creatives/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/common/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/common/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Publ/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Publ/</xsl:attribute>
            </cfg:files>
            <cfg:hosts>
              <xsl:attribute name="source"><xsl:value-of
                select="$campaign-manager-and-adfrontend-hosts"/></xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-source-host"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>
      </xsl:if>

      <xsl:variable name="stat-files-ref" select="$colo-config/cfg:central/cfg:statFilesReceiverRef"/>

      <xsl:if test="count($stat-files-ref) > 0">
        <cfg:FeedRouteGroup
          local_copy_command="/bin/echo"
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2">
          <xsl:attribute name="remote_copy_command"><xsl:value-of
            select="$backup-command-prefix"/><![CDATA[/usr/bin/rsync -avz -t --timeout=55 --log-format=%f --delete-after ##SRC_PATH## rsync://]]><xsl:value-of
            select="$stat-files-ref/@host"/>:<xsl:value-of select="$stat-files-ref/@port"/><xsl:if
            test="count($stat-files-ref/@port) = 0"><xsl:value-of select="$def-stat-files-receiver-port"/></xsl:if>##DST_PATH##<xsl:value-of
            select="$backup-command-postfix"/></xsl:attribute>

          <cfg:Route type="RoundRobin">
            <cfg:files source="LogGeneralizer/Out/ActionRequest/ActionRequests_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ActionStat/ActionStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCStat/CcStatsDaily_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCGStat/CcgStatsDaily_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/AdvertiserUserStat/AdvertiserUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CampaignUserStat/CampaignUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCGKeywordStat/CcgKeywordStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCGUserStat/CcgUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCUserStat/CcUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CMPStat/CmpStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CreativeStat/RequestStatsHourly_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelCountStat/ChannelCountStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelHitStat/ChannelInventory_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelImpInventory/ChannelImpInventory_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelInventory/ChannelInventory_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelInventoryEstimationStat/ChannelInventoryEstimStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelOverlapUserStat/ChannelOverlapUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CCGSelectionFailureStat/CcgSelectionFailure_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelPerformance/ChannelPerformance_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelPriceRange/ChannelInventoryByCpm_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelTriggerImpStat/ChannelTriggerImpStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelTriggerStat/ChannelTriggerStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ChannelTriggerStat/ChannelTriggerStats-2_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ColoUpdateStat/ColoStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ColoUserStat/ColoUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/DeviceChannelCountStat/DeviceChannelCountStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/ExpressionPerformance/ExpressionPerformance_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/GlobalColoUserStat/GlobalColoUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/OptOutStat/OptOutStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/PageLoadsDailyStat/PageLoadsDaily_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/PassbackStat/PassbackStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/PublisherInventory/PublisherInventory_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/SearchEngineStat/SearchEngineStatsDaily_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/SearchTermStat/SearchTermStatsDaily_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/SiteChannelStat/SiteChannelStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/SiteReferrerStat/SiteReferrerStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/SiteUserStat/SiteUserStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/TagAuctionStat/TagAuctionStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/TagPositionStat/TagPositionStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/CampaignReferrerStat/CcgSiteReferrerStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/UserAgentStat/UserAgentStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/UserProperties/UserPropertyStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/WebStat/WebStats_*"
              destination="/csvlistener/"/>
            <cfg:files source="LogGeneralizer/Out/WebStat/WebStats-2_*"
              destination="/csvlistener/"/>
            <!-- use LogGeneralizer hosts as intermediate for ExtStat's from pbe -->
            <cfg:files source="LogGeneralizer/Out/ExtStat/*" destination="/csvlistener/"/>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>

          <xsl:if test="string-length($stat-receiver-hosts) > 0">
            <cfg:Route type="RoundRobin">
              <cfg:files source="StatReceiver/Out/ExtStat/*" destination="/csvlistener/"/>
              <cfg:hosts destination="-non-used-hostname">
                <xsl:attribute name="source"><xsl:value-of select="$stat-receiver-hosts"/></xsl:attribute>
              </cfg:hosts>
            </cfg:Route>
          </xsl:if>
        </cfg:FeedRouteGroup>
      </xsl:if>
    
    <xsl:variable name="research-command-prefix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup = '1' or $logs-backup = 'true')">
          <xsl:value-of select="$colo-config-root"/><![CDATA[/copy_and_backup.sh ']]></xsl:when>
        <xsl:when 
          test="count($colo-config/cfg:predictorConfig/cfg:auxiliaryRef) > 0 or
                $predictor-path//cfg:syncServer/@enable_backup = 'true' or 
                $predictor-path//cfg:syncServer/@enable_backup = '1'"><![CDATA[/bin/sh -c ']]></xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="research-command-postfix"><xsl:choose>
        <xsl:when test="count($logs-backup) > 0 and
          ($logs-backup = '1' or $logs-backup = 'true')"><![CDATA[' '##SRC_PATH##']]></xsl:when>
        <xsl:when 
           test="count($colo-config/cfg:predictorConfig/cfg:auxiliaryRef) > 0 or
                 $predictor-path//cfg:syncServer/@enable_backup = 'true' or 
                 $predictor-path//cfg:syncServer/@enable_backup = '1'"><![CDATA[']]></xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="count($predictor-sync-logs-host) > 0">
      <cfg:FeedRouteGroup
        local_copy_command="/bin/echo"
        local_copy_command_type="rsync"
        remote_copy_command_type="rsync"
        tries_per_file="2">
        <xsl:attribute name="remote_copy_command"><xsl:value-of
          select="$research-command-prefix"/><xsl:for-each 
          select="$colo-config/cfg:predictorConfig/cfg:auxiliaryRef"><![CDATA[(/usr/bin/rsync -avz -t --log-format=%f ##SRC_PATH## rsync://]]><xsl:value-of
          select="@host"/>:<xsl:value-of select="@port"/><xsl:if
          test="count(@port) = 0"><xsl:value-of 
          select="$def-research-stat-receiver-port"/></xsl:if><![CDATA[/]]><xsl:value-of select="$research-stat-receiver-path"/><![CDATA[##DST_PATH## || true) && ]]></xsl:for-each><xsl:if test="$predictor-path//cfg:syncServer/@enable_backup = 'true' or 
        $predictor-path//cfg:syncServer/@enable_backup = '1'"><![CDATA[(/usr/bin/rsync -az -t --timeout=55 --log-format=%f ##SRC_PATH##  rsync://]]><xsl:value-of
            select="$predictor-sync-logs-host"/>:<xsl:value-of select="$predictor-sync-logs-port"/><![CDATA[/backup##DST_PATH## || true) && ]]></xsl:if><![CDATA[/usr/bin/rsync -az -t --timeout=55 --log-format=%f --delete-after ##SRC_PATH## rsync://]]><xsl:value-of
            select="$predictor-sync-logs-host"/>:<xsl:value-of select="$predictor-sync-logs-port"/><![CDATA[/]]><xsl:value-of select="$research-stat-receiver-path"/>##DST_PATH##<xsl:value-of
            select="$research-command-postfix"/></xsl:attribute>

          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">RequestInfoManager/Out/ResearchBid/RBid_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchBid</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">RequestInfoManager/Out/ResearchImpression/RImpression_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchImpression</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">RequestInfoManager/Out/ResearchClick/RClick_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchClick</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">RequestInfoManager/Out/ResearchAction/RAction_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchAction</xsl:attribute>
            </cfg:files>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$request-info-manager-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>

          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">CampaignManager/Out/ResearchWebStat/RWebStat_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchWebStat</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">CampaignManager/Out/ResearchProf/RProf_*</xsl:attribute>
              <xsl:attribute name="destination"><![CDATA[/]]>ResearchProf</xsl:attribute>
            </cfg:files>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$campaign-manager-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>
      </xsl:if>

      <xsl:if test="count($stunnel-client-config) > 0">
        <xsl:variable name="stunnel-port">
          <xsl:value-of select="$stunnel-client-config/cfg:networkParams/@port"/>
          <xsl:if test="count($stunnel-client-config/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-stunnel-client-port"/>
          </xsl:if>
        </xsl:variable>
        <xsl:variable name="stunnel-host">
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="$stunnel-client-path/@host"/>
            <xsl:with-param name="error-prefix" select="'SyncLogs'"/>
          </xsl:call-template>
        </xsl:variable>
        <cfg:FeedRouteGroup
          local_copy_command="/usr/bin/rsync -t -z --timeout=55 --log-format=%f --ignore-existing ##SRC_PATH## ##DST_PATH##"
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          pool_threads="50">

          <xsl:attribute name="remote_copy_command"><xsl:value-of
            select="$backup-command-prefix"/>/usr/bin/rsync -t<xsl:if
              test="$stunnel-client-config/@compress = 'true'"><![CDATA[ -z]]></xsl:if><![CDATA[ --timeout=55 --log-format=%f --ignore-existing ##SRC_PATH## rsync://]]><xsl:value-of
            select="$stunnel-host"/>:<xsl:value-of select="$stunnel-port"/>/ad-logs##DST_PATH##<xsl:value-of
            select="$backup-command-postfix"/></xsl:attribute>

          <xsl:variable name="log-proxy-main-route">
            <dirs>
              <dir>CreativeStat</dir>
              <dir>UserProperties</dir>
              <dir>ChannelPerformance</dir>
              <dir>ExpressionPerformance</dir>
              <dir>OptOutStat</dir>
              <dir>WebStat</dir>
              <dir>ChannelTriggerStat</dir>
              <dir>ChannelTriggerImpStat</dir>
              <dir>ChannelHitStat</dir>
              <dir>AdvertiserUserStat</dir>
              <dir>ChannelInventory</dir>
              <dir>ChannelPriceRange</dir>
              <dir>ChannelInventoryEstimationStat</dir>
              <dir>CCGKeywordStat</dir>
              <dir>SiteChannelStat</dir>
              <dir>SiteReferrerStat</dir>
              <dir>ActionStat</dir>
              <dir>PassbackStat</dir>
              <dir>SearchEngineStat</dir>
              <dir>SearchTermStat</dir>
              <dir>UserAgentStat</dir>
              <dir>CMPStat</dir>
              <dir>ActionRequest</dir>
              <dir>PublisherInventory</dir>
              <dir>ChannelImpInventory</dir>
              <dir>SiteUserStat</dir>
              <dir>PageLoadsDailyStat</dir>
              <dir>ChannelCountStat</dir>
              <dir>TagAuctionStat</dir>
              <dir>TagPositionStat</dir>
              <dir>CampaignReferrerStat</dir>
              <dir>CCGStat</dir>
              <dir>CCStat</dir>
              <dir>CCGUserStat</dir>
              <dir>CCUserStat</dir>
              <dir>CampaignUserStat</dir>
              <dir>GlobalColoUserStat</dir>
              <dir>ColoUserStat</dir>
              <dir>ChannelOverlapUserStat</dir>
              <dir>CCGSelectionFailureStat</dir>
            </dirs>
          </xsl:variable>

          <xsl:call-template name="Route">
            <xsl:with-param name="type" select="'RoundRobin'"/>
            <xsl:with-param name="source-path-base" select="'LogGeneralizer/'"/>
            <xsl:with-param name="destination-path-base" select="'/LogProxy/'"/>
            <xsl:with-param name="source-hosts" select="$log-generalizer-hosts"/>
            <xsl:with-param name="destination-hosts" select="'-non-used-hostname'"/>
            <xsl:with-param name="dirs" select="$log-proxy-main-route"/>
          </xsl:call-template>

          <cfg:Route type="RoundRobin">
            <cfg:files source="StatReceiver/Out/ExtStat/*" destination="/LogProxy/ExtStat"/>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$stat-receiver-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>

        <cfg:FeedRouteGroup
          local_copy_command="/bin/echo"
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true"
          pool_threads="50"
          check_logs_period="{$content-check-period}">
          <xsl:attribute name="remote_copy_command">/usr/bin/rsync --partial -av<xsl:if
            test="$stunnel-client-config/@compress = 'true'">z</xsl:if><![CDATA[ -t --timeout=55 --log-format=%f --delete-after rsync://]]><xsl:value-of
            select="$stunnel-host"/>:<xsl:value-of select="$stunnel-port"/><![CDATA[/ad-content##SRC_PATH## ##DST_PATH##]]></xsl:attribute>

          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">/tags/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/tags/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Creatives/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Creatives/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/common/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/common/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Publ/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Publ/</xsl:attribute>
            </cfg:files>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$stunnel-host"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>
      </xsl:if>

      <xsl:if test="count($predictor-sync-logs-host) > 0">

        <cfg:FeedRouteGroup
          local_copy_command="/bin/echo"
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true"
          pool_threads="50"
          check_logs_period="{$content-check-period}">
          <xsl:attribute name="remote_copy_command"><xsl:value-of select="$colo-config-root"/><![CDATA[/rsync_immutable.sh rsync://]]><xsl:value-of
            select="$predictor-sync-logs-host"/>:<xsl:value-of
            select="$predictor-sync-logs-port"/><![CDATA[##SRC_PATH## ##DST_PATH##]]></xsl:attribute>

          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source"><![CDATA[/]]><xsl:value-of select="$predictor-ctr-path"/><![CDATA[/]]></xsl:attribute>
              <xsl:attribute name="destination">CampaignManager/In/CTRConfig</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source"><![CDATA[/]]><xsl:value-of select="$predictor-conv-path"/><![CDATA[/]]></xsl:attribute>
              <xsl:attribute name="destination">CampaignManager/In/ConvRateConfig</xsl:attribute>
            </cfg:files>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$campaign-manager-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>
      </xsl:if>

      <xsl:if test="count($colo-config/cfg:audienceChannelConfig/cfg:sourceRef) > 0">
        <xsl:variable name="source-ref" select="$colo-config/cfg:audienceChannelConfig/cfg:sourceRef"/>

        <cfg:FeedRouteGroup
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true">
          <xsl:variable name="copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://]]><xsl:value-of
            select="$source-ref/@host"/>:<xsl:value-of
            select="$source-ref/@port"/><![CDATA[/channels##SRC_PATH## ##DST_PATH##]]></xsl:variable>
          <xsl:attribute name="remote_copy_command"><xsl:value-of select="$copy_command"/></xsl:attribute>
          <xsl:attribute name="local_copy_command"><xsl:value-of select="$copy_command"/></xsl:attribute>
          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">/</xsl:attribute>
              <xsl:attribute name="destination">UserOperationGenerator/In/Snapshot</xsl:attribute>
            </cfg:files>
            <cfg:hosts destination="-non-used-hostname">
              <xsl:attribute name="source"><xsl:value-of select="$user-operation-generator-hosts"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>
      </xsl:if>
    </cfg:ClusterConfig>
  </cfg:SyncLogsConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/../.."/>
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="be-cluster-path"
    select="$xpath"/>

  <xsl:variable
    name="log-processing-path"
    select="$xpath/serviceGroup[@descriptor = $log-processing-descriptor]"/>

  <xsl:variable
    name="stunnel-client-path"
    select="$be-cluster-path/serviceGroup[@descriptor = $local-proxy-descriptor]/service[
      @descriptor = $remote-stunnel-client-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find XPATH element <xsl:value-of select="$xpath"/></xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find full cluster group</xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find fe-cluster group</xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find be cluster group</xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable name="log-processing-config" select="$log-processing-path/configuration/cfg:logProcessing"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable
    name="stunnel-client-config"
    select="$stunnel-client-path/configuration/cfg:sTunnelClient"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find colo config</xsl:message>
    </xsl:when>
    <xsl:when test="count($log-processing-config) = 0">
       <xsl:message terminate="yes">SyncLogs: Can't find SyncLogs config</xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/LogProcessing/SyncLogsConfig.xsd')}">
    <xsl:call-template name="SyncLogsConfigGenerator">
      <xsl:with-param name="log-processing-config" select="$log-processing-config"/>
      <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>

      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="channel-serving-path" select="$fe-cluster-path"/>

      <xsl:with-param name="stunnel-client-path" select="$stunnel-client-path"/>
      <xsl:with-param name="stunnel-client-config" select="$stunnel-client-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

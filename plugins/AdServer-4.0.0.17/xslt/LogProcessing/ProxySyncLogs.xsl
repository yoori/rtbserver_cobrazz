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

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- Route -->
<xsl:template name="Route">
  <xsl:param name="type"/>
  <xsl:param name="source-hosts"/>
  <xsl:param name="destination-hosts"/>
  <xsl:param name="dirs"/>
  <xsl:param name="pattern"/>

  <cfg:Route post_command="touch ##SRC_DIR##/~##FILE_NAME##.commit.##DST_HOST##" fetch_type="commited">
    <xsl:attribute name="type">
      <xsl:value-of select="$type"/>
    </xsl:attribute>

    <xsl:for-each select="exsl:node-set($dirs)//dir">
      <xsl:variable name="dir" select="text()"/>
      <xsl:variable name="source"
        select="concat('LogProxy/', $dir, '/', $dir)"/>
      <xsl:variable name="destination"
        select="concat('/LogGeneralizer/In/', $dir)"/>

      <cfg:files>
        <xsl:attribute name="source">
          <xsl:value-of select="$source"/>
        </xsl:attribute>
        <xsl:attribute name="destination">
          <xsl:value-of select="$destination"/>
        </xsl:attribute>
        <xsl:if test="string-length($pattern) > 0">
          <xsl:attribute name="pattern">
            <xsl:value-of select="$pattern"/>
          </xsl:attribute>
        </xsl:if>
      </cfg:files>
    </xsl:for-each>

    <cfg:hosts>
      <xsl:attribute name="source"><xsl:value-of select="$source-hosts"/></xsl:attribute>
      <xsl:attribute name="destination"><xsl:value-of select="$destination-hosts"/></xsl:attribute>
    </cfg:hosts>
  </cfg:Route>

  <!-- route for commit files -->
  <cfg:Route type="HostName" pattern="\.commit\.(.*)$">
    <xsl:for-each select="exsl:node-set($dirs)//dir">
      <xsl:variable name="dir" select="text()"/>
      <xsl:variable name="source"
        select="concat('LogProxy/', $dir, '/Intermediate/~', $dir, '.2*')"/>
      <xsl:variable name="destination"
        select="concat('/LogGeneralizer/In/', $dir)"/>

      <cfg:files>
        <xsl:attribute name="source">
          <xsl:value-of select="$source"/>
        </xsl:attribute>
        <xsl:attribute name="destination">
          <xsl:value-of select="$destination"/>
        </xsl:attribute>
      </cfg:files>
    </xsl:for-each>

    <cfg:hosts>
      <xsl:attribute name="source"><xsl:value-of select="$source-hosts"/></xsl:attribute>
      <xsl:attribute name="destination"><xsl:value-of select="$destination-hosts"/></xsl:attribute>
    </cfg:hosts>
  </cfg:Route>
</xsl:template>

<!-- SyncLogs config generate function -->
<xsl:template name="ProxySyncLogsConfigGenerator">
  <xsl:param name="app-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>

  <xsl:param name="pbe-cluster-path"/>
  <xsl:param name="sync-logs-config"/>

    <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
      <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="server-config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="colo-config-root">
      <xsl:value-of select="$server-config-root"/>/<xsl:value-of select="$colo-name"/>
    </xsl:variable>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
      <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="sync-logs-port">
      <xsl:value-of select="$sync-logs-config/cfg:networkParams/@port"/>
      <xsl:if test="count($sync-logs-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-proxy-sync-logs-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="proxySyncLogs.port"
      method="text" omit-xml-declaration="yes"
      >  ['proxySyncLogs', <xsl:copy-of select="$sync-logs-port"/>],</exsl:document>

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

    <xsl:variable name="log-root-dir" select="concat($workspace-root, '/log/SyncLogs')"/>
    <xsl:variable name="log-files-root-dir" select="concat($workspace-root, '/log')"/>

  <cfg:SyncLogsConfig
    log_root="{$log-root-dir}"
    check_logs_period="{$check-logs-period}"
    host_check_period="{$host-check-period}"
    hostname="{$HOST}">

    <xsl:variable name="logs-backup">
      <xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@logs_backup"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@logs_backup) = 0">
        <xsl:value-of select="'false'"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="backup-command-prefix">
      <xsl:if test="count($logs-backup) > 0 and
        ($logs-backup = '1' or $logs-backup = 'true')">
        <xsl:value-of select="concat($colo-config-root,
          '/copy_and_backup.sh ')"/><![CDATA[']]></xsl:if>
    </xsl:variable>
    <xsl:variable name="backup-command-postfix">
      <xsl:if test="count($logs-backup) > 0 and
        ($logs-backup = '1' or $logs-backup = 'true')"><![CDATA[' '##SRC_PATH##']]></xsl:if>
    </xsl:variable>

    <xsl:variable name="remote-dest-port"><xsl:value-of select="$sync-logs-config/cfg:fileTransferring/@port"/>
      <xsl:if test="count($sync-logs-config/cfg:fileTransferring/@port) = 0">
        <xsl:value-of select="$def-rsync-server-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="remote-copy-command">
      <xsl:value-of select="concat($backup-command-prefix,
        '/usr/bin/rsync -t -z --timeout=55 --log-format=%f ##SRC_PATH## rsync://##DST_HOST##:',
        $remote-dest-port, '/ad-logs##DST_PATH##', $backup-command-postfix)"/>
    </xsl:variable>

    <!-- check that defined all needed parameters -->
    <xsl:choose>
      <xsl:when test="count($check-logs-period) = 0">
        <xsl:message terminate="yes"> Proxy SyncLogs: check logs period undefined.</xsl:message>
      </xsl:when>
      <xsl:when test="count($host-check-period) = 0">
        <xsl:message terminate="yes"> Proxy SyncLogs: host check period undefined.</xsl:message>
      </xsl:when>
    </xsl:choose>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$sync-logs-config/cfg:threadParams/@min"/>
        <xsl:if test="count($sync-logs-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-sync-logs-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$sync-logs-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$sync-logs-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $sync-logs-log-path)"/>
      <xsl:with-param name="default-log-level" select="$sync-logs-log-level"/>
    </xsl:call-template>

    <!-- get host from channelproxy it presents always -->
    <xsl:variable
      name="sync-logs-host-name"
      select="$pbe-cluster-path/service[@descriptor = $pbe-channel-proxy-descriptor]/@host"/>

    <xsl:variable name="sync-logs-host">
      <xsl:call-template name="ResolveHostName">
        <xsl:with-param name="base-host" select="$sync-logs-host-name"/>
        <xsl:with-param name="error-prefix" select="'ProxySyncLogs'"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="log-generalizer-hosts">
      <xsl:for-each select="$sync-logs-config/cfg:fileTransferring/cfg:logMove">
        <xsl:value-of select="@host"/>
        <xsl:if test="position()!=last()">,</xsl:if>
      </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="data-source-ref" select="$sync-logs-config/cfg:fileTransferring/cfg:dataSourceRef"/>

    <cfg:ClusterConfig>
      <xsl:attribute name="root_logs_dir"><xsl:value-of select="$log-files-root-dir"/></xsl:attribute>
      <xsl:attribute name="definite_hash_schema"><xsl:value-of select="concat($server-root, '/xsd/AdServerCommons/HostDistributionFile.xsd')"/></xsl:attribute>

      <cfg:FeedRouteGroup
        local_copy_command_type="rsync"
        remote_copy_command_type="rsync"
        tries_per_file="2"
        pool_threads="50">
        <xsl:attribute name="local_copy_command">
          <xsl:value-of select="$remote-copy-command"/>
        </xsl:attribute>
        <xsl:attribute name="remote_copy_command">
          <xsl:value-of select="$remote-copy-command"/>
        </xsl:attribute>

        <xsl:variable name="log-proxy-main">
          <dirs>
            <dir>CCStat</dir>
            <dir>CCGStat</dir>
            <dir>CampaignUserStat</dir>
            <dir>CCGUserStat</dir>
            <dir>CCUserStat</dir>
            <dir>AdvertiserUserStat</dir>
            <dir>GlobalColoUserStat</dir>
            <dir>ColoUserStat</dir>
            <dir>CreativeStat</dir>
            <dir>OptOutStat</dir>
            <dir>WebStat</dir>
            <dir>ChannelTriggerStat</dir>
            <dir>ChannelTriggerImpStat</dir>
            <dir>SiteReferrerStat</dir>
            <dir>UserProperties</dir>
            <dir>ChannelPerformance</dir>
            <dir>ExpressionPerformance</dir>
            <dir>CCGKeywordStat</dir>
            <dir>SiteChannelStat</dir>
            <dir>ChannelPriceRange</dir>
            <dir>ChannelInventory</dir>
            <dir>ChannelInventoryEstimationStat</dir>
            <dir>ActionStat</dir>
            <dir>ActionRequest</dir>
            <dir>ChannelHitStat</dir>
            <dir>PassbackStat</dir>
            <dir>UserAgentStat</dir>
            <dir>CMPStat</dir>
            <dir>PublisherInventory</dir>
            <dir>ChannelCountStat</dir>
            <dir>ChannelImpInventory</dir>
            <dir>SiteStat</dir>
            <dir>SiteUserStat</dir>
            <dir>PageLoadsDailyStat</dir>
            <dir>TagAuctionStat</dir>
            <dir>TagPositionStat</dir>
            <dir>CampaignReferrerStat</dir>
            <dir>ChannelOverlapUserStat</dir>
            <dir>SearchEngineStat</dir>
          </dirs>
        </xsl:variable>

        <xsl:call-template name="Route">
          <xsl:with-param name="type" select="'RoundRobin'"/>
          <xsl:with-param name="source-hosts" select="$sync-logs-host"/>
          <xsl:with-param name="destination-hosts" select="$log-generalizer-hosts"/>
          <xsl:with-param name="dirs" select="$log-proxy-main"/>
        </xsl:call-template>

        <cfg:Route post_command="touch ##SRC_DIR##/~##FILE_NAME##.commit.##DST_HOST##" type="RoundRobin">
          <cfg:files source="CampaignServer/Out/ColoUpdateStat/ColoUpdateStat.2*" destination="/LogGeneralizer/In/ColoUpdateStat"/>
          <cfg:files source="ChannelProxy/Out/ColoUpdateStat/ColoUpdateStat.2*" destination="/LogGeneralizer/In/ColoUpdateStat"/>
          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
          </cfg:hosts>
        </cfg:Route>
        <cfg:Route type="HostName" pattern="\.commit\.(.*)$">
          <cfg:files source="CampaignServer/Out/ColoUpdateStat/~ColoUpdateStat.2*" destination="/LogGeneralizer/In/ColoUpdateStat"/>
          <cfg:files source="ChannelProxy/Out/ColoUpdateStat/~ColoUpdateStat.2*" destination="/LogGeneralizer/In/ColoUpdateStat"/>
          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
          </cfg:hosts>
        </cfg:Route>

        <cfg:Route type="RoundRobin">
          <!-- use LogGeneralizer hosts as intermediate for ExtStat's -->
          <cfg:files source="LogProxy/ExtStat/*"
            destination="/LogGeneralizer/Out/ExtStat"/>
          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
          </cfg:hosts>
        </cfg:Route>

        <cfg:Route post_command="touch ##SRC_DIR##/~##FILE_NAME##.commit.##DST_HOST##" type="Hash" fetch_type="commited">
          <cfg:files source="LogProxy/SearchTermStat/SearchTermStat" destination="/LogGeneralizer/In/SearchTermStat" pattern=".*\.##HASH##"/>
          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
          </cfg:hosts>
        </cfg:Route>
        <cfg:Route type="HostName" pattern="\.commit\.(.*)$">
          <cfg:files source="LogProxy/SearchTermStat/Intermediate/~SearchTermStat.*" destination="/LogGeneralizer/In/SearchTermStat"/>
          <cfg:hosts>
            <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
            <xsl:attribute name="destination"><xsl:value-of select="$log-generalizer-hosts"/></xsl:attribute>
          </cfg:hosts>
        </cfg:Route>
      </cfg:FeedRouteGroup>

      <xsl:if test="count($data-source-ref) > 0">
        <xsl:variable name="data-source-host" select="$data-source-ref/@host"/>
        <xsl:variable name="data-source-port"><xsl:value-of
          select="$data-source-ref/@port"/><xsl:if test="count($data-source-ref/@port) = 0">8873</xsl:if>
        </xsl:variable>
        <xsl:variable name="data-source-path"><xsl:value-of
          select="$data-source-ref/@path"/><xsl:if test="count($data-source-ref/@path) = 0">filesender</xsl:if>
        </xsl:variable>

        <cfg:FeedRouteGroup
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true">
          <!-- need exclude directories created for 1.7.5 compatibility -->
          <xsl:attribute name="local_copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://##DST_HOST##:]]><xsl:value-of
            select="$data-source-port"/>/<xsl:value-of
            select="$data-source-path"/><![CDATA[/##SRC_PATH## ##DST_PATH## --exclude=/Templates --exclude=/templates --exclude=/creatives]]></xsl:attribute>
          <xsl:attribute name="remote_copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://##DST_HOST##:]]><xsl:value-of
            select="$data-source-port"/>/<xsl:value-of
            select="$data-source-path"/><![CDATA[/##SRC_PATH## ##DST_PATH## --exclude=/Templates --exclude=/templates --exclude=/creatives]]></xsl:attribute>
          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">/Creatives/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Creatives/</xsl:attribute>
            </cfg:files>
            <cfg:hosts>
              <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-source-host"/></xsl:attribute>
            </cfg:hosts>
          </cfg:Route>
        </cfg:FeedRouteGroup>

        <cfg:FeedRouteGroup
          local_copy_command_type="rsync"
          remote_copy_command_type="rsync"
          tries_per_file="2"
          parse_source="false"
          unlink_source="false"
          interruptible="true">
          <xsl:attribute name="local_copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://##DST_HOST##:]]><xsl:value-of
            select="$data-source-port"/>/<xsl:value-of
            select="$data-source-path"/><![CDATA[/##SRC_PATH## ##DST_PATH##]]></xsl:attribute>
          <xsl:attribute name="remote_copy_command"><![CDATA[/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after rsync://##DST_HOST##:]]><xsl:value-of
            select="$data-source-port"/>/<xsl:value-of
            select="$data-source-path"/><![CDATA[/##SRC_PATH## ##DST_PATH##]]></xsl:attribute>

          <cfg:Route type="RoundRobin">
            <cfg:files>
              <xsl:attribute name="source">/tags/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/tags/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/templates/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/templates/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/WebwiseDiscover/common/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/WebwiseDiscover/common/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Discover/Customizations/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Discover/Customizations/</xsl:attribute>
            </cfg:files>
            <cfg:files>
              <xsl:attribute name="source">/Publ/</xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-root"/>/Publ/</xsl:attribute>
            </cfg:files>

            <cfg:hosts>
              <xsl:attribute name="source"><xsl:value-of select="$sync-logs-host"/></xsl:attribute>
              <xsl:attribute name="destination"><xsl:value-of select="$data-source-host"/></xsl:attribute>
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
  <xsl:variable name="app-path" select="$xpath/.."/>

  <xsl:variable
    name="pbe-cluster-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($pbe-cluster-path) = 0">
       <xsl:message terminate="yes"> Proxy SyncLogs: Can't find full cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$pbe-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="sync-logs-config"
    select="$colo-config/cfg:syncLogs"/>


  <xsl:variable
    name="env-config"
    select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> Proxy SyncLogs: Can't find colo config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/LogProcessing/SyncLogsConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ProxySyncLogsConfigGenerator">
      <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="pbe-cluster-path" select="$pbe-cluster-path"/>
      <xsl:with-param name="sync-logs-config" select="$sync-logs-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

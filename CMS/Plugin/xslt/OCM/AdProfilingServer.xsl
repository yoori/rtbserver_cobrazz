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

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:variable name="current-time" select="$CURRENT_TIME"/>
<xsl:variable name="app-path" select="$xpath"/>

<xsl:variable name="tr-services" select="$xpath/
  serviceGroup[@descriptor = $ad-profilingcluster-descriptor]/
  serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']/service[
    @descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelServer' or
    @descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController' or
    @descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']"/>

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

</xsl:template>

<xsl:template name="AddSubAgentFrontendSubcluster">
  <xsl:param name="cluster-path"/>
  <xsl:param name="service-descriptor"/>
  <xsl:param name="service-name-prefix"/>
  <xsl:param name="checking-host"/>

  <xsl:for-each select="$cluster-path/serviceGroup[@descriptor =
    'AdProfilingCluster/FrontendSubCluster']/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']">
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
      <Service name="{concat($service-name-prefix, '-SubAgent')}"
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

    <xsl:variable name="def-prefix" select="fe"/>
    <xsl:variable name="subfe-hosts">
      <xsl:for-each select="$cluster-path/serviceGroup[@descriptor =
        'AdProfilingCluster/FrontendSubCluster']/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']">
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
        <xsl:when test="count(exsl:node-set($subfe-hosts)[host =
          $this-host]) > 0">
          <xsl:variable name="prefix">
            <xsl:choose>
              <xsl:when test="count(exsl:node-set($tr-hosts)[host = $this-host]) > 0">
                <xsl:value-of select="'tr'"/>
              </xsl:when>
              <xsl:otherwise><xsl:value-of select="$def-prefix"/></xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:call-template name="AddSubAgentFrontendSubcluster">
            <xsl:with-param name="cluster-path" select="$cluster-path"/>
            <xsl:with-param name="service-descriptor"
              select="$http-frontend-descriptor"/>
            <xsl:with-param name="service-name-prefix" select="$prefix"/>
            <xsl:with-param name="checking-host" select="$this-host"/>
          </xsl:call-template>
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

<xsl:template name="AdDBServices">
  <xsl:param name="cluster-path"/>
  <xsl:param name="number"/>

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster']"/>

    <xsl:if test="count($be-cluster-path) > 0">
      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer']"/>
        <xsl:with-param name="service-name" select="'be-CampaignServer'"/>
        <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::CampaignServer'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$be-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/DictionaryProvider']"/>
        <xsl:with-param name="service-name" select="'be-DictionaryProvider'"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::DictionaryProvider'"/>
      </xsl:call-template>
    </xsl:if>

    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']">

      <xsl:variable name="pos" select="position()"/>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelServer']"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-ChannelServer')"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelServer'"/>
      </xsl:call-template>
    </xsl:for-each>
</xsl:template>

<xsl:template name="AdServices">
  <xsl:param name="cluster-path"/>
  <xsl:param name="number"/>

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster']"/>
    <xsl:variable
      name="fe-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']"/>

    <xsl:if test="count($be-cluster-path) > 0">

      <xsl:variable
        name="localproxy-path"
        select="$be-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster/LocalProxy']"/>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="$localproxy-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/LocalProxy/ChannelProxy']"/>
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

    <xsl:call-template name="AddSubAgentService">
      <xsl:with-param name="cluster-path" select="$cluster-path"/>
      <xsl:with-param name="number" select="$number"/>
    </xsl:call-template>

    <xsl:call-template name="AddOneOnHostService">
      <xsl:with-param name="serv-path" select="$cluster-path//service"/>
      <xsl:with-param name="service-name" select="'lp-CleanupLogs'"/>
      <xsl:with-param name="service-type" select="'AdServer::LogProcessing::CleanupLogs'"/>
    </xsl:call-template>

    <xsl:for-each select="$fe-cluster-path">
      <xsl:variable name="pos" select="position()"/>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController']"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-ChannelController')"/>
        <xsl:with-param name="service-type" select="'AdServer::ChannelSvcs::ChannelController'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/CampaignManager']"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-CampaignManager')"/>
        <xsl:with-param name="service-type" select="'AdServer::CampaignSvcs::CampaignManager'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ZmqProfilingGateway']"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-ZmqProfilingGateway')"/>
        <xsl:with-param name="service-type" select="'AdServer::Utils::ZmqProfilingGateway'"/>
      </xsl:call-template>

      <xsl:call-template name="AddService">
        <xsl:with-param name="service-path"
          select="./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/Frontend']"/>
        <xsl:with-param name="service-name" select="concat('fe', $pos, '-AdFrontend')"/>
        <xsl:with-param name="service-type" select="'AdServer::AdFrontend'"/>
      </xsl:call-template>
    </xsl:for-each>
</xsl:template>


<xsl:template name="Services">
  <xsl:param name="env"/>
  <xsl:param name="cluster-path"/>
  <xsl:param name="script"/>
  <xsl:param name="number"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']"/>

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

  <Services start="%%START_NOLOOP_DONE%%" start_cmd="%%cmd%% start"
    cmd="{concat($script, ' %%HOST%% %%TYPE%%')}" env="{$env}" nostate="">

    <xsl:variable
      name="be-cluster-path"
      select="$cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

    <Service name="keyscan-Scanner" host="localhost"
      start="%%START_NOLOOP_DONE%%" start_cmd="%%SMS_KEYSCAN%%" transport="/bin/sh -c &quot;$SMS_TEXT&quot;">
      <xsl:attribute name="keyscan_hosts">
        <xsl:call-template name="GetUniqueHosts">
            <xsl:with-param name="services-xpath" select="$cluster-path//service"/>
          <xsl:with-param name="error-prefix" select="'OCM::KeyScan'"/>
        </xsl:call-template>
      </xsl:attribute>
    </Service>
  </Services>

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

  <xsl:variable
    name="fe-cluster-path"
    select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']"/>

  <xsl:variable name="prestart-timeout">
    <xsl:choose>
      <xsl:when test="count($fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@timeout) > 0">
        <xsl:value-of select="$fe-cluster-path/configuration/cfg:frontendCluster/cfg:startParams/@timeout"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="'600'"/>
      </xsl:otherwise>
    </xsl:choose>
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
    <xsl:for-each select="$cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']">
      <xsl:variable name="pos" select="position()"/>
        <Service name="{concat('fe', $pos, '-PreStart')}" type="AdServer::PreStart" host="localhost"
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
            </xsl:call-template>
          </xsl:attribute>
        </Service>
    </xsl:for-each>
    </xsl:if>

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

  <xsl:variable name="prestart-checker"><xsl:value-of
    select="concat('export PERL5LIB=$PERL5LIB:', $server-mgr-lib-root, ' ; ',
      $server-mgr-bin-root, '/PreStartChecker.pl')"/></xsl:variable>

  <xsl:variable name="campaign-manager-arg">
    <xsl:call-template name="GeneratePreStartRefList">
      <xsl:with-param name="services"
        select="$fe-cluster/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/CampaignManager']"/>
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
        select="$fe-cluster/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController']"/>
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
    /> --serv-CampaignManager=<xsl:value-of select="$campaign-manager-arg"
    /> --serv-ChannelServer=<xsl:value-of select="$channel-server-arg"/></xsl:variable>

  <xsl:value-of select="concat($prestart-checker, ' ', $prestart-checker-args)"/>
</xsl:template>

<xsl:template name="Troubleshoot">
  <xsl:param name="server-root"/>
  <xsl:param name="config-root"/>
  <xsl:param name="workspace-root"/>
  <xsl:param name="ssh-argc"/>
  <xsl:param name="server-bin-root"/>

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
        <xsl:value-of select="concat('tr_report_', $this-host, '_%%SMS_UNIQUE%%.tar.gz ')"/>
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

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath"/>

  <xsl:variable
    name="adcluster-path"
    select="$app-path/serviceGroup[@descriptor = $ad-profilingcluster-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> SMS: Can't find XPATH element </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="colo-config"
    select="$adcluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="app-config" select="$app-path/configuration/cfg:environment"/>
  <xsl:variable name="app-zone-config" select="$app-config/cfg:forosZoneManagement[1]"/>

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

    <BatchCommands>
      <BatchCommand name="keyscan">
        <Command command="start">
          <Group>keyscan</Group>
        </Command>
      </BatchCommand>

      <xsl:if test="count($adcluster-path) > 0">
        <BatchCommand name="clear-stat-backup">
          <Command command="start">
            <Group>clear-stat-backup</Group>
          </Command>
        </BatchCommand>

        <BatchCommand name="troubleshoot">
          <Command command="start">
            <Group>troubleshoot</Group>
          </Command>
        </BatchCommand>
      </xsl:if>
    </BatchCommands>

    <xsl:if test="count($adcluster-path) > 0">
      <xsl:call-template name="Services">
        <xsl:with-param name="env" select="'adprofilingcluster'"/>
        <xsl:with-param name="cluster-path" select="$adcluster-path"/>
        <xsl:with-param name="script" select="$script"/>
      </xsl:call-template>

      <xsl:call-template name="Tasks">
        <xsl:with-param name="env" select="'adprofilingcluster'"/>
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
    <xsl:call-template name="Troubleshoot">
      <xsl:with-param name="server-root" select="$server-root"/>
      <xsl:with-param name="config-root" select="$config-root"/>
      <xsl:with-param name="workspace-root" select="$workspace-root"/>
      <xsl:with-param name="ssh-argc" select="$ssh-argc"/>
      <xsl:with-param name="server-bin-root" select="$server-bin-root"/>
    </xsl:call-template>
    <Groups>
      <xsl:if test="count($adcluster-path) > 0">
        <xsl:call-template name="AddGroup">
          <xsl:with-param name="group-name" select="'be lp'"/>
        </xsl:call-template>
        <xsl:variable name="localproxy-path" select="$adcluster-path/
          serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster']/
          serviceGroup[@descriptor = 'AdProfilingCluster/BackendSubCluster/LocalProxy']"/>
        <xsl:if test="count($localproxy-path) > 0">
          <xsl:call-template name="AddGroup">
            <xsl:with-param name="group-name" select="'localproxy'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:for-each select="$adcluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']">
          <xsl:variable name="fe-comp" select="concat('fe', position())"/>
          <xsl:call-template name="AddGroup">
            <xsl:with-param name="group-name" select="$fe-comp"/>
            <xsl:with-param name="group-mask" select="concat($fe-comp, '-*,*')"/>
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
      <!--xsl:variable name="all-zones-exclude" select="'troubleshoot keyscan'"-->
      <xsl:variable name="exclude1" select="'troubleshoot clear-stat-backup keyscan'"/>
      <!-- Groups for Autorestart needs -->
      <xsl:for-each select="/colo:colocation/host">
        <xsl:variable name="real-host">
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="@name"/>
            <xsl:with-param name="error-prefix" select="'SMS'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="count(exsl:node-set($tr-hosts)[host = $real-host]) > 0">
          <xsl:call-template name="AddGroup">
            <xsl:with-param name="group-name" select="$real-host"/>
            <xsl:with-param name="group-mask" select="concat('*,', $real-host)"/>
            <xsl:with-param name="exclude-groups" select="$exclude1"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:for-each>
      <!-- This groups run 'by default', if empty command line -->
      <xsl:call-template name="AddGroup">
        <xsl:with-param name="group-name" select="'all'"/>
        <xsl:with-param name="group-mask" select="'*,*'"/>
        <xsl:with-param name="exclude-groups" select="$exclude1"/>
      </xsl:call-template>
    </Groups>

    <Dependences>

      <xsl:variable name="prestart-dep">
        <xsl:if test="count($adcluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']) -
          count($adcluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']/
            configuration/cfg:frontendCluster/cfg:startParams[
            @wait_loading = 'false' or @wait_loading = '0']) > 0"> AdServer::PreStart</xsl:if>
      </xsl:variable>
      <xsl:variable
        name="be-cluster-path"
        select="$adcluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

        <xsl:if test="count($adcluster-path) > 0">
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer'"/>
            <xsl:with-param name="slaves" select="concat('AdServer::CampaignSvcs::CampaignManager ', 'AdServer::ChannelSvcs::ChannelServer')"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="slaves" select="'AdServer::ChannelSvcs::ChannelController'"/>
            <xsl:with-param name="masters" select="'AdServer::CampaignSvcs::CampaignServer
              AdServer::ChannelSvcs::ChannelServer'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="slaves" select="'AdServer::ChannelSvcs::ChannelController'"/>
            <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::ChannelProxy'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="concat('AdServer::CampaignSvcs::CampaignManager ', 'AdServer::ChannelSvcs::ChannelController')"/>
            <xsl:with-param name="slaves" select= "concat('AdServer::AdFrontend ', $prestart-dep)"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="'AdServer::Utils::ZmqProfilingGateway'"/>
            <xsl:with-param name="slaves" select= "'AdServer::AdFrontend'"/>
          </xsl:call-template>
          <xsl:call-template name="AddDependence">
            <xsl:with-param name="masters" select="$prestart-dep"/>
            <xsl:with-param name="slaves" select= "'AdServer::AdFrontend'"/>
          </xsl:call-template>
          <xsl:if test="count($be-cluster-path/service[@descriptor = 'AdServer::ChannelSvcs::DictionaryProvider']) > 0">
            <xsl:call-template name="AddDependence">
              <xsl:with-param name="masters" select="'AdServer::ChannelSvcs::DictionaryProvider'"/>
              <xsl:with-param name="slaves" select= "'AdServer::ChannelSvcs::ChannelServer'"/>
            </xsl:call-template>
          </xsl:if>
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
    </Dependences>
  </AdServer>
</xsl:template>

</xsl:stylesheet>

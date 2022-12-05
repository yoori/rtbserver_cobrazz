<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="cluster-id" select="$CLUSTER_ID"/>
<xsl:variable name="mode" select="$MODE"/> <!--AD,PROFILING-->

<!-- CampaignManager config generate function -->
<xsl:template name="CampaignManagerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="campaign-servers"/>
  <xsl:param name="campaign-manager-path"/>
  <xsl:param name="campaign-manager-config"/>
  <xsl:param name="full-cluster-path"/>

    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if
      >/<xsl:value-of select="$out-dir"/>
    </xsl:variable>
    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>
    <xsl:variable name="www-root"><xsl:value-of select="$env-config/@data_root"/>
      <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="backend-content-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:virtualServer[
        count(cfg:thirdPartyContentDomain) > 0][1]"/>

    <xsl:variable name="backend-content-domain"><xsl:value-of
      select="$backend-content-domain-virtual-server/
        cfg:thirdPartyContentDomain[1]/@name"/><xsl:if
        test="count($backend-content-domain-virtual-server/@port) > 0 and
          $backend-content-domain-virtual-server/@port != 80">:<xsl:value-of
          select="$backend-content-domain-virtual-server/
            @port"/></xsl:if></xsl:variable>

    <!-- HTTP domains (with port suffixes) -->
    <xsl:variable name="adserving-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:virtualServer[
        count(cfg:adservingDomain) > 0][1]"/>

    <xsl:variable name="adserving-domain"><xsl:value-of
      select="$adserving-domain-virtual-server/
        cfg:adservingDomain[1]/@name"/><xsl:if
      test="count($adserving-domain-virtual-server/@port) > 0 and
        $adserving-domain-virtual-server/@port != 80">:<xsl:value-of
      select="$adserving-domain-virtual-server/@port"/></xsl:if></xsl:variable>

    <xsl:variable name="video-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:virtualServer[
        count(cfg:videoDomain) > 0][1]"/>

    <xsl:variable name="video-domain"><xsl:value-of
      select="$video-domain-virtual-server/
        cfg:videoDomain[1]/@name"/><xsl:if
      test="count($video-domain-virtual-server/@port) > 0 and
        $video-domain-virtual-server/@port != 80">:<xsl:value-of
      select="$video-domain-virtual-server/@port"/></xsl:if></xsl:variable>

    <xsl:variable name="click-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:virtualServer[
        count(cfg:clickDomain) > 0][1]"/>

    <xsl:variable name="click-domain-test"><xsl:value-of
      select="$click-domain-virtual-server/
        cfg:clickDomain[1]/@name"/><xsl:if
        test="count($click-domain-virtual-server/@port) > 0 and
          $click-domain-virtual-server/@port != 80">:<xsl:value-of
          select="$click-domain-virtual-server/
            @port"/></xsl:if></xsl:variable>

    <xsl:variable name="click-domain">
      <xsl:value-of select="$click-domain-test"/>
      <xsl:if test="string-length($click-domain-test) = 0"><xsl:value-of
        select="$adserving-domain"/></xsl:if></xsl:variable>

    <xsl:variable name="frontend-content-domain"><xsl:value-of
      select="$colo-config/cfg:coloParams/@CDN_domain"/><xsl:if
        test="string-length($colo-config/cfg:coloParams/@CDN_domain) = 0"><xsl:value-of
          select="$backend-content-domain"/></xsl:if></xsl:variable>

    <!-- HTTPS domains (with port suffixes) -->
    <xsl:variable name="secure-params-adserving-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[
        count(cfg:adservingDomain) > 0][1]"/>
    <xsl:variable name="secure-adserving-domain-virtual-server"
      select="($secure-params-adserving-domain-virtual-server |
        $colo-config/cfg:coloParams/cfg:secureVirtualServer[
        count($secure-params-adserving-domain-virtual-server) = 0 and
        count(cfg:adservingDomain) > 0])[1]"/>

    <xsl:variable name="secure-adserving-domain"><xsl:value-of
      select="$secure-adserving-domain-virtual-server/
        cfg:adservingDomain[1]/@name"/><xsl:if
      test="$secure-adserving-domain-virtual-server/
        @port != 443">:<xsl:value-of
      select="$secure-adserving-domain-virtual-server/
        @port"/></xsl:if></xsl:variable>

    <xsl:variable name="secure-params-backend-content-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[
        count(cfg:thirdPartyContentDomain) > 0][1]"/>
    <xsl:variable name="secure-backend-content-domain-virtual-server"
      select="($secure-params-backend-content-domain-virtual-server |
        $colo-config/cfg:coloParams/cfg:secureVirtualServer[
        count($secure-params-backend-content-domain-virtual-server) = 0 and
        count(cfg:thirdPartyContentDomain) > 0])[1]"/>

    <xsl:variable name="secure-backend-content-domain"><xsl:value-of
      select="$secure-backend-content-domain-virtual-server/
        cfg:thirdPartyContentDomain[1]/@name"/><xsl:if
        test="count($secure-backend-content-domain-virtual-server/@port) > 0 and
          $secure-backend-content-domain-virtual-server/@port != 443">:<xsl:value-of
          select="$secure-backend-content-domain-virtual-server/
            @port"/></xsl:if></xsl:variable>

    <xsl:variable name="secure-params-click-domain-virtual-server"
      select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[
        count(cfg:clickDomain) > 0][1]"/>
    <xsl:variable name="secure-click-domain-virtual-server"
      select="$secure-params-click-domain-virtual-server |
      $colo-config/cfg:coloParams/cfg:virtualServer[
        count($secure-params-click-domain-virtual-server) = 0 and
        count(cfg:clickDomain) > 0][1]"/>

    <xsl:variable name="secure-click-domain-test"><xsl:value-of
      select="$secure-click-domain-virtual-server/
        cfg:clickDomain[1]/@name"/><xsl:if
        test="count($secure-click-domain-virtual-server/@port) > 0 and
          $secure-click-domain-virtual-server/@port != 443">:<xsl:value-of
          select="$secure-click-domain-virtual-server/
            @port"/></xsl:if></xsl:variable>

    <xsl:variable name="secure-click-domain">
      <xsl:value-of select="$secure-click-domain-test"/>
      <xsl:if test="string-length($secure-click-domain-test) = 0"><xsl:value-of
        select="$secure-adserving-domain"/></xsl:if></xsl:variable>

    <xsl:variable name="secure-frontend-content-domain"><xsl:value-of
      select="$colo-config/cfg:coloParams/@https_CDN_domain"/><xsl:if
        test="string-length($colo-config/cfg:coloParams/@https_CDN_domain) = 0"><xsl:value-of
          select="$secure-backend-content-domain"/></xsl:if></xsl:variable>

    <xsl:variable name="inventory-users-percentage"><xsl:value-of
      select="$colo-config/cfg:inventoryStats/@simplifying"/><xsl:if
      test="count($colo-config/cfg:inventoryStats/@simplifying) = 0"><xsl:value-of
      select="$inventory-users-percentage"/></xsl:if></xsl:variable>

    <xsl:variable name="campaign-manager-port"><xsl:value-of select="$campaign-manager-config/cfg:networkParams/@port"/>
      <xsl:if test="count($campaign-manager-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-campaign-manager-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="campaignManager.port"
      method="text" omit-xml-declaration="yes"
      >  ['campaignManager', <xsl:copy-of select="$campaign-manager-port"/>],</exsl:document>

    <xsl:variable name="update-config" select="$campaign-manager-config/cfg:updateParams"/>
    <xsl:variable name="update-period"><xsl:value-of select="$update-config/@update_period"/>
      <xsl:if test="count($update-config/@update_period) = 0">
        <xsl:value-of select="$campaign-manager-update-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="ecpm-update-period"><xsl:value-of select="$update-config/@ecpm_update_period"/>
      <xsl:if test="count($update-config/@ecpm_update_period) = 0">
        <xsl:value-of select="$campaign-manager-ecpm-update-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="campaign-types"><xsl:value-of select="$update-config/@campaign_types"/>
      <xsl:if test="count($update-config/@campaign_types) = 0">
        <xsl:value-of select="$campaign-manager-campaign-types"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="stat-config" select="$campaign-manager-config/cfg:statLogging"/>
    <xsl:variable name="flush-loggers-period"><xsl:value-of select="$stat-config/@flush_period"/>
      <xsl:if test="count($stat-config/@flush_period) = 0">
        <xsl:value-of select="$campaign-manager-flush-loggers-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="flush-internal-loggers-period"><xsl:value-of select="$stat-config/@internal_logs_flush_period"/>
      <xsl:if test="count($stat-config/@internal_logs_flush_period) = 0">
        <xsl:value-of select="$campaign-manager-flush-internal-loggers-period"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="campaigns-update-timeout">
      <xsl:choose>
        <xsl:when test="count($colo-config/cfg:remote) = 0">0</xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="count($colo-config/cfg:remote/@connect_timeout) > 0"><xsl:value-of
              select="$colo-config/cfg:remote/@connect_timeout"/></xsl:when>
            <xsl:otherwise>86400</xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="adrequest-anonymize">
      <xsl:choose>
        <xsl:when test="$colo-config/cfg:coloParams/@ad_request_profiling =
          'ad-request profiling and stats collection disabled'">true</xsl:when>
        <xsl:otherwise>false</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="campaign-manager-host-port-set">
      <xsl:for-each select="$xpath/../service[@descriptor = $campaign-manager-descriptor]">
        <xsl:variable name="campaign-manager-host-subset">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'CampaignManager'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:for-each select="exsl:node-set($campaign-manager-host-subset)/host">
          <host><xsl:value-of select="."/></host>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="campaign-manager-host-port-sorted-set">
      <xsl:for-each select="exsl:node-set($campaign-manager-host-port-set)/host">
        <xsl:sort select="."/>
        <host><xsl:value-of select="."/></host>
      </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="service-id"><xsl:value-of
      select="count(exsl:node-set(
        $campaign-manager-host-port-sorted-set)//host[. = $HOST]/preceding-sibling::host) + 1"/>
    </xsl:variable>

    <xsl:variable name="log-root"><xsl:value-of
      select="$workspace-root"/>/log/CampaignManager</xsl:variable>

    <!-- start config generation -->
  <cfg:CampaignManager
    host="{$HOST}"
    log_root="{$log-root}"
    config_update_period="{$update-period}"
    campaigns_update_timeout="{$campaigns-update-timeout}"
    ecpm_update_period="{$ecpm-update-period}"
    campaigns_type="{$campaign-types}"
    colocation_id="{$colo-id}"
    domain_config_path="{$config-root}/DomainConfig.xml"
    service_index="{concat($cluster-id, '_', $service-id)}"
    rid_private_key="{concat($config-root, '/rid_private_key.der')}"
    >

    <!-- check that defined all needed parameters -->
    <xsl:choose>
      <xsl:when test="count($colo-id) = 0">
        <xsl:message terminate="yes"> CampaignManager: colo id undefined. </xsl:message>
      </xsl:when>
    </xsl:choose>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$campaign-manager-config/cfg:threadParams/@min"/>
        <xsl:if test="count($campaign-manager-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-campaign-manager-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*" port="{$campaign-manager-port}">
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <!-- this object can be used only in utils -->
        <cfg:Object servant="CampaignManager" name="CampaignManager"/>
        <cfg:Object servant="CampaignManager" name="{$current-campaign-manager-obj}"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$campaign-manager-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $campaign-manager-log-path)"/>
      <xsl:with-param name="default-log-level" select="$campaign-manager-log-level"/>
    </xsl:call-template>

    <cfg:Creative
      creative_file_dir="{$www-root}/Creatives"
      template_file_dir="{$www-root}/Templates"
      post_instantiate_script_template_file="{$config-root}/PostInstantiateScript.html"
      post_instantiate_script_mime_format="text/html"
      post_instantiate_iframe_template_file="{$config-root}/PostInstantiateIFrame.html"
      post_instantiate_iframe_mime_format="text/html"
      instantiate_track_html_file="{$config-root}/InstntiateJSTracker.html">

      <cfg:ContentCache root="{concat($www-root, '/Creatives/')}">
        <xsl:attribute name="size"><xsl:value-of select="$campaign-manager-config/cfg:contentCache/@size"/>
          <xsl:if test="count($campaign-manager-config/cfg:contentCache/@size) = 0">52428800</xsl:if>
        </xsl:attribute>
        <xsl:attribute name="timeout"><xsl:value-of select="$campaign-manager-config/cfg:contentCache/@timeout"/>
          <xsl:if test="count($campaign-manager-config/cfg:contentCache/@timeout) = 0">10</xsl:if>
        </xsl:attribute>
      </cfg:ContentCache>

      <cfg:CreativeRule name="unsecure" secure="false"
        ad_server="http://{$adserving-domain}"
        image_url="http://{$frontend-content-domain}/creatives"
        publ_url="http://{$frontend-content-domain}/publ"
        ad_image_server="http://{$frontend-content-domain}"
        local_passback_prefix="http://{$frontend-content-domain}/tags/"

        dynamic_creative_prefix="http://{$backend-content-domain}/services/dcreative"
        ad_click_url="http://{$click-domain}/click"
        track_pixel_url="http://{$adserving-domain}/track.png"
        passback_pixel_url="http://{$adserving-domain}/passback"
        action_pixel_url="http://{$adserving-domain}/services/ActionServer/SetCookie"
        passback_template_path_prefix="{$www-root}/Templates/Passback/pb."
        user_bind_url="http://{$adserving-domain}/userbind"
        pub_pixels_optin="http://{$backend-content-domain}/pubpixels?us=in"
        pub_pixels_optout="http://{$backend-content-domain}/pubpixels?us=out"
        script_instantiate_url="http://{$adserving-domain}/inst?format=js&amp;"
        iframe_instantiate_url="http://{$adserving-domain}/inst?format=html&amp;"
        direct_instantiate_url="http://{$adserving-domain}/inst?"
        video_instantiate_url="http://{$video-domain}/inst?"
        >
        <xsl:for-each select="$colo-config/cfg:coloParams/cfg:bindUrl">
          <cfg:Token name="{concat('BINDURL', position())}" value="{@template}"/>
        </xsl:for-each>
      </cfg:CreativeRule>

      <cfg:CreativeRule name="secure" secure="true"
        ad_server="https://{$secure-adserving-domain}"
        image_url="https://{$secure-frontend-content-domain}/creatives"
        publ_url="https://{$secure-frontend-content-domain}/publ"
        ad_image_server="https://{$secure-frontend-content-domain}"
        local_passback_prefix="https://{$secure-frontend-content-domain}/tags/"

        dynamic_creative_prefix="https://{$secure-backend-content-domain}/services/dcreative"
        ad_click_url="https://{$click-domain}/click"
        track_pixel_url="https://{$secure-adserving-domain}/track.png"
        passback_pixel_url="https://{$secure-adserving-domain}/passback"
        action_pixel_url="https://{$secure-adserving-domain}/services/ActionServer/SetCookie"
        passback_template_path_prefix="{$www-root}/Templates/Passback/pb."
        user_bind_url="https://{$secure-adserving-domain}/userbind"
        pub_pixels_optin="https://{$secure-backend-content-domain}/pubpixels?us=in"
        pub_pixels_optout="https://{$secure-backend-content-domain}/pubpixels?us=out"
        script_instantiate_url="https://{$secure-adserving-domain}/inst?format=js&amp;"
        iframe_instantiate_url="https://{$secure-adserving-domain}/inst?format=html&amp;"
        direct_instantiate_url="https://{$secure-adserving-domain}/inst?"
        video_instantiate_url="https://{$video-domain}/inst?"
        >
        <xsl:for-each select="$colo-config/cfg:coloParams/cfg:bindUrl">
          <cfg:Token name="{concat('BINDURL', position())}" value="{@template}"/>
        </xsl:for-each>
      </cfg:CreativeRule>

      <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTB/cfg:source">
        <xsl:if test="$colo-config/cfg:coloParams/cfg:RTB/cfg:source/@custom_click = 'true' or
          $colo-config/cfg:coloParams/cfg:RTB/cfg:source/@custom_click = 1">
          <cfg:SourceRule name="{@id}">
            <xsl:if test="count(@click_prefix) > 0">
              <xsl:attribute name="click_prefix">
                <xsl:value-of select="@click_prefix"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="count(@mime_encoded_click_prefix) > 0">
              <xsl:attribute name="mime_encoded_click_prefix">
                <xsl:value-of select="@mime_encoded_click_prefix"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="count(@preclick) > 0">
              <xsl:attribute name="preclick">
                <xsl:value-of select="@preclick"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="count(@mime_encoded_preclick) > 0">
              <xsl:attribute name="mime_encoded_preclick">
                <xsl:value-of select="@mime_encoded_preclick"/>
              </xsl:attribute>
            </xsl:if>
          </cfg:SourceRule>
        </xsl:if>
      </xsl:for-each>
    </cfg:Creative>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'CampaignManager'"/>
    </xsl:call-template>

    <xsl:if test="$mode = 'AD'">
      <cfg:Billing check_bids="true" confirm_bids="true"
        optimize_campaign_ctr="false">
        <cfg:BillingServerCorbaRef name="BillingServer">
          <xsl:call-template name="BillingServerCorbaRefs">
            <xsl:with-param name="billing-servers"
              select="$full-cluster-path//service[@descriptor = $billing-server-descriptor]"/>
            <xsl:with-param name="error-prefix" select="'CampaignManager'"/>
          </xsl:call-template>
        </cfg:BillingServerCorbaRef>
      </cfg:Billing>
    </xsl:if>

    <xsl:variable name="use-referrer-site-referrer-stats">
      <xsl:call-template name="GetReferrerLoggingValue">
        <xsl:with-param name="referrer-logging" select="$colo-config/@referrer_logging_site_referrer_stats"/>
      </xsl:call-template>
    </xsl:variable>

    <cfg:Logging
      inventory_users_percentage="{$inventory-users-percentage}"
      use_referrer_site_referrer_stats="{$use-referrer-site-referrer-stats}" >

      <xsl:attribute name="distrib_count"><xsl:value-of select="$colo-config/cfg:inventoryStats/@distrib_count"/>
        <xsl:if test="count($colo-config/cfg:inventoryStats/@distrib_count) = 0">
          <xsl:value-of select="$default-distrib-count"/>
        </xsl:if>
      </xsl:attribute>

      <xsl:variable name="predictor-service" 
          select="$full-cluster-path//service[@descriptor = $predictor-descriptor]"/>

      <cfg:ChannelTriggerStat flush_period="{$flush-loggers-period}"/>
      <cfg:ChannelHitStat flush_period="{$flush-loggers-period}"/>
      <cfg:RequestBasicChannels adrequest_anonymize="{$adrequest-anonymize}"
        flush_period="{$flush-internal-loggers-period}">
        <xsl:attribute name="dump_channel_triggers"><xsl:value-of
          select="$colo-config/cfg:channelTriggerImpStats/@enable"/><xsl:if
          test="count($colo-config/cfg:channelTriggerImpStats/@enable) = 0">true</xsl:if></xsl:attribute>
      </cfg:RequestBasicChannels>
      <cfg:WebStat flush_period="{$flush-loggers-period}"/>
      <xsl:if test="count($predictor-service)> 0 or count($colo-config/cfg:predictorConfig/cfg:ref)> 0">
        <cfg:ResearchWebStat flush_period="{$flush-loggers-period}"/>
        <cfg:ProfilingResearch flush_period="{$flush-loggers-period}"/>
      </xsl:if>
      <cfg:CreativeStat flush_period="{$flush-loggers-period}"/>
      <cfg:ActionRequest flush_period="{$flush-loggers-period}"/>
      <cfg:PassbackStat flush_period="{$flush-loggers-period}"/>
      <cfg:UserAgentStat flush_period="{$flush-loggers-period}"/>

      <cfg:Request flush_period="{$flush-internal-loggers-period}"/>
      <cfg:Impression flush_period="{$flush-internal-loggers-period}"/>
      <cfg:Click flush_period="{$flush-internal-loggers-period}"/>
      <cfg:AdvertiserAction flush_period="{$flush-internal-loggers-period}"/>
      <cfg:PassbackImpression flush_period="{$flush-loggers-period}"/>

      <cfg:UserProperties flush_period="{$flush-loggers-period}"/>
      <cfg:TagRequest flush_period="{$flush-internal-loggers-period}"/>
      <cfg:CcgStat flush_period="{$flush-loggers-period}"/>
      <cfg:CcStat flush_period="{$flush-loggers-period}"/>
      <xsl:if test="count($colo-config/@enable_search_term_stats) > 0 and (
        $colo-config/@enable_search_term_stats = 'true' or $colo-config/@enable_search_term_stats = '1')">
        <cfg:SearchTermStat flush_period="{$flush-loggers-period}"/>
      </xsl:if>
      <cfg:SearchEngineStat flush_period="{$flush-loggers-period}"/>
      <cfg:TagAuctionStat flush_period="{$flush-loggers-period}"/>
      <cfg:TagPositionStat flush_period="{$flush-loggers-period}"/>
    </cfg:Logging>

    <cfg:IpEncryptConfig>
      <xsl:attribute name="key">
        <xsl:choose>
          <xsl:when test="count($colo-config/cfg:coloParams/@ip_key) > 0">
            <xsl:value-of select="$colo-config/cfg:coloParams/@ip_key"/>
          </xsl:when>
          <xsl:otherwise>MFBOeuplH3LlQqfGSvNiew..</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </cfg:IpEncryptConfig>

    <xsl:if test="count($colo-config/cfg:coloParams/cfg:securityToken) > 0">
      <cfg:SecurityToken
        key0="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key0}"
        key1="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key1}"
        key2="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key2}"
        key3="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key3}"
        key4="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key4}"
        key5="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key5}"
        key6="{$colo-config/cfg:coloParams/cfg:securityToken[1]/@key6}"
        />
    </xsl:if>
    <cfg:CTRConfig root="{concat($log-root,'/In/CTRConfig')}"
      capture_root="{concat($log-root,'/In/CapturedCTRConfig')}"
      check_period="60">
      <xsl:attribute name="expire_timeout">
        <xsl:choose>
          <xsl:when test="count($colo-config/cfg:predictorConfig/cfg:CTRConfig/@expire_timeout) =
            0">86400</xsl:when>
          <xsl:otherwise><xsl:value-of
            select="$colo-config/cfg:predictorConfig/cfg:CTRConfig/@expire_timeout"/></xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </cfg:CTRConfig>
    <cfg:ConvRateConfig root="{concat($log-root,'/In/ConvRateConfig')}"
      capture_root="{concat($log-root,'/In/CapturedConvRateConfig')}"
      check_period="60"
      expire_timeout="0"/>
    <xsl:if test="count($colo-config/cfg:coloParams/@enabled_countries) > 0">
      <xsl:variable name="whitelist-country-tokens"
        select="str:tokenize($colo-config/cfg:coloParams/@enabled_countries, ',&#x9;&#xA;&#xD;&#x20;')"/>
      <xsl:if test="count($whitelist-country-tokens) > 0">
        <cfg:CountryWhitelist>
          <xsl:for-each select="$whitelist-country-tokens">
            <cfg:Country country_code="{.}"/>
          </xsl:for-each>
        </cfg:CountryWhitelist>
      </xsl:if>
    </xsl:if>

   <xsl:variable name="kafka-config" select="$colo-config/cfg:coloParams/cfg:kafkaStorage"/>
   <xsl:if test="count($kafka-config/cfg:adsSpaces) != 0">
      <cfg:KafkaAdsSpacesStorage>
        <xsl:call-template name="SaveKafkaTopic">
          <xsl:with-param name="topic-config" select="$kafka-config/cfg:adsSpaces"/>
          <xsl:with-param name="default-topic-name" select="$default-ads-spaces-topic"/>
          <xsl:with-param name="kafka-config" select="$kafka-config"/>
        </xsl:call-template>
      </cfg:KafkaAdsSpacesStorage>
    </xsl:if>

  </cfg:CampaignManager>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="fe-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor or
              @descriptor = 'AdProfilingCluster/BackendSubCluster']"/>

  <xsl:variable
    name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor or
             @descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer']"/>

  <xsl:variable
    name="campaign-manager-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find campaign manager node (XPATH) element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find fe cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find be cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-servers) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find campaign servers node</xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster/cfg:environment |
     $colo-config/cfg:environment"/>

  <xsl:variable name="service-config"
     select="$campaign-manager-path/configuration/cfg:campaignManager"/>

  <xsl:variable name="service-group-config"
     select="$campaign-manager-path/../configuration/cfg:campaignManager"/>

  <xsl:variable
    name="campaign-manager-config"
    select="$service-config[count($service-config) > 0] |
      $service-group-config[count($service-config) = 0]"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find colo config </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-manager-config) = 0">
       <xsl:message terminate="yes"> CampaignManager: Can't find campaign manager config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/CampaignSvcs/CampaignManagerConfig.xsd')}">
    <xsl:call-template name="CampaignManagerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="campaign-manager-path" select="$campaign-manager-path"/>
      <xsl:with-param name="campaign-manager-config" select="$campaign-manager-config"/>
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

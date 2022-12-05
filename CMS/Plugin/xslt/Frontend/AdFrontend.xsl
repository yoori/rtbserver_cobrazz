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
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../UserIdBlackList.xsl"/>
<xsl:include href="../CampaignManagement/CampaignServersCorbaRefs.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="conf-type" select="$CONF_TYPE"/>

<xsl:template name="GetUidKeyPath">
  <xsl:param name="unixcommons-root"/>
  <xsl:param name="config-root"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="uid-key"/>

  <xsl:choose>
    <xsl:when test="count($colo-config/cfg:coloParams/*[name() = $uid-key and . != '']) != 0">
      <xsl:value-of select="concat($config-root, '/cert/')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat($unixcommons-root, '/share/uuid_keys/')"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="ConvertFrontendLogger">
  <xsl:param name="logger-node"/>
  <xsl:param name="default-log-level"/>

  <xsl:variable name="log-level"><xsl:value-of select="$logger-node/@log_level"/>
    <xsl:if test="count($logger-node/@log_level) = 0">
      <xsl:value-of select="$default-log-level"/>
    </xsl:if>
  </xsl:variable>

  <cfg:Logger>
    <xsl:attribute name="log_level"><xsl:value-of select="$log-level"/></xsl:attribute>
  </cfg:Logger>
</xsl:template>

<xsl:template name="CookieConfigurator">
  <xsl:param name="cookies-path"/>
  <xsl:param name="name"/>
  <xsl:param name="default-domain"/>
  <xsl:param name="default-path"/>
  <xsl:param name="default-expires"/>

  <cfg:Cookie>
    <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
    <xsl:choose>
      <xsl:when test="count($cookies-path/cfg:Cookie[@name = $name]/@domain) > 0">
        <xsl:attribute name="domain"><xsl:value-of
          select="$cookies-path/cfg:Cookie[@name = $name]/@domain"/></xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="count($cookies-path/@domain) > 0">
            <xsl:attribute name="domain"><xsl:value-of
              select="$cookies-path/@domain"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="domain"><xsl:value-of select="$default-domain"/></xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="count($cookies-path/cfg:Cookie[@name = $name]/@path) > 0">
        <xsl:attribute name="path"><xsl:value-of
          select="$cookies-path/cfg:Cookie[@name = $name]/@path"/></xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="count($cookies-path/@path) > 0">
            <xsl:attribute name="path"><xsl:value-of select="$cookies-path/@path"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="path"><xsl:value-of select="$default-path"/></xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="count($cookies-path/cfg:Cookie[@name = $name]/@expires) > 0">
        <xsl:attribute name="expires"><xsl:value-of
          select="$cookies-path/cfg:Cookie[@name = $name]/@expires"/></xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="count($cookies-path/@expires) > 0">
            <xsl:attribute name="expires"><xsl:value-of
              select="$cookies-path/@expires"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="expires"><xsl:value-of select="$default-expires * 24 * 60 * 60"/></xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </cfg:Cookie>
</xsl:template>

<xsl:template name="RemoveCookieConfigurator">
  <xsl:param name="cookies-path"/>
  <xsl:param name="name"/>
  <xsl:param name="default-domain"/>
  <xsl:param name="default-path"/>

  <xsl:variable name="domain">
    <xsl:choose>
      <xsl:when test="count($cookies-path/cfg:Cookie[@name = $name]/@domain) > 0">
        <xsl:value-of select="$cookies-path/cfg:Cookie[@name = $name]/@domain"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="count($cookies-path/@domain) > 0">
            <xsl:value-of select="$cookies-path/@domain"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$default-domain"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="path">
    <xsl:choose>
      <xsl:when test="count($cookies-path/cfg:Cookie[@name = $name]/@path) > 0">
        <xsl:value-of select="$cookies-path/cfg:Cookie[@name = $name]/@path"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="count($cookies-path/@path) > 0">
            <xsl:value-of select="$cookies-path/@path"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$default-path"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <cfg:RemoveCookie>
    <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
    <xsl:attribute name="domain"><xsl:value-of select="$domain"/></xsl:attribute>
    <xsl:attribute name="path"><xsl:value-of select="$path"/></xsl:attribute>
  </cfg:RemoveCookie>

</xsl:template>

<xsl:template name="check_ip_mask">
  <xsl:param name="ip_mask"/>

  <xsl:if test="contains($ip_mask, '-') = 0 and contains($ip_mask, ' ') = 0">
  <xsl:variable name="ip1" select="substring-before($ip_mask, '.')"/>
  <xsl:if test="$ip1 &lt;= 255 and $ip1 >= 0">
    <xsl:variable name="ip2" select="
      substring-before(substring-after($ip_mask, concat($ip1, '.')), '.')"/>
    <xsl:if test="$ip2 &lt;= 255 and $ip2 >= 0">
      <xsl:variable name="ip3" select="
        substring-before(substring-after($ip_mask, concat($ip1, '.', $ip2, '.')), '.')"/>
      <xsl:if test="$ip3 &lt;= 255 and $ip3 >= 0">
        <xsl:variable name="ip4" select="substring-before(
          substring-after($ip_mask, concat($ip1, '.', $ip2, '.', $ip3, '.')), '/')"/>
        <xsl:if test="$ip4 &lt;= 255 and $ip4 >= 0">
          <xsl:variable name="mask" select="
            substring-after($ip_mask, concat($ip1, '.', $ip2, '.', $ip3, '.', $ip4, '/')) "/>
          <xsl:if test="$mask &lt;= 32 and $mask >= 0">
<xsl:value-of select='1'/>
          </xsl:if>
        </xsl:if>
      </xsl:if>
    </xsl:if>
  </xsl:if>
  </xsl:if>
</xsl:template>

<xsl:template name="check_ip_masks">
  <xsl:param name="ip_masks"/>

  <xsl:for-each select="str:split($ip_masks, '&#xA;')">
    <xsl:variable name="state"><xsl:call-template
      name="check_ip_mask">
      <xsl:with-param name="ip_mask"><xsl:value-of
        select="normalize-space(.)"/></xsl:with-param>
    </xsl:call-template></xsl:variable>
    <xsl:if test="$state != '1'">
      <xsl:message terminate="yes"><xsl:value-of
       select="concat('Invalid ip range value: ', .)"/></xsl:message>
    </xsl:if>
  </xsl:for-each>
  <xsl:value-of select="$ip_masks"/>
</xsl:template>

<xsl:template name="GenerateEncryptionKeys">
  <xsl:if test="count(@openx_encryption_key) > 0">
    <xsl:attribute name="openx_encryption_key">
      <xsl:value-of select="@openx_encryption_key"/>
    </xsl:attribute>
  </xsl:if>
  <xsl:if test="count(@openx_integrity_key) > 0">
    <xsl:attribute name="openx_integrity_key">
      <xsl:value-of select="@openx_integrity_key"/>
    </xsl:attribute>
  </xsl:if>
  <xsl:if test="count(@baidu_encryption_key) > 0">
    <xsl:attribute name="baidu_encryption_key">
      <xsl:value-of select="@baidu_encryption_key"/>
    </xsl:attribute>
  </xsl:if>
  <xsl:if test="count(@baidu_integrity_key) > 0">
    <xsl:attribute name="baidu_integrity_key">
      <xsl:value-of select="@baidu_integrity_key"/>
    </xsl:attribute>
  </xsl:if>
  <xsl:if test="count(@google_encryption_key) > 0">
    <xsl:attribute name="google_encryption_key">
      <xsl:value-of select="@google_encryption_key"/>
    </xsl:attribute>
  </xsl:if>
  <xsl:if test="count(@google_integrity_key) > 0">
    <xsl:attribute name="google_integrity_key">
      <xsl:value-of select="@google_integrity_key"/>
    </xsl:attribute>
  </xsl:if>
</xsl:template>

<!-- AdFrontend config generate function -->
<xsl:template name="AdFrontendConfigGenerator">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="server-root"/>
  <xsl:param name="unixcommons-root"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>

  <xsl:param name="campaign-servers"/>
  <xsl:param name="user-bind-servers"/>
  <xsl:param name="channel-controller-path"/>

  <xsl:param name="channel-server-count"/>

  <xsl:param name="stats-collector-path"/>
  <xsl:param name="stats-collector"/>

  <xsl:param name="frontend-config"/>

  <xsl:variable name="www-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if
      >/<xsl:value-of select="$out-dir"/>
  </xsl:variable>
  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="ps-res-data-root" select="concat($data-root, '/PageSense')"/>
  <xsl:variable name="debug-param" select="$frontend-config/cfg:debugInfo"/>
  <xsl:variable name="tags-data-root" select="concat($data-root, '/tags')"/>
  <xsl:variable name="request-module" select="$frontend-config/cfg:requestModule"/>
  <xsl:variable name="adinst-module" select="$frontend-config/cfg:adInstModule"/>
  <xsl:variable name="content-module" select="$frontend-config/cfg:contentModule"/>
  <xsl:variable name="impression-module" select="$frontend-config/cfg:impressionModule"/>
  <xsl:variable name="click-module" select="$frontend-config/cfg:clickModule"/>
  <xsl:variable name="action-module" select="$frontend-config/cfg:actionModule"/>
  <xsl:variable name="optout-module" select="$frontend-config/cfg:optoutModule"/>
  <xsl:variable name="pubpixel-module" select="$frontend-config/cfg:pubPixelModule"/>
  <xsl:variable name="pass-module" select="$frontend-config/cfg:passbackModule"/>
  <xsl:variable name="pass-pixel-module" select="$frontend-config/cfg:passbackPixelModule"/>
  <xsl:variable name="adop-module" select="$frontend-config/cfg:adOpModule"/>
  <xsl:variable name="webstat-module" select="$frontend-config/cfg:webStatModule"/>
  <xsl:variable name="bidding-module" select="$frontend-config/cfg:biddingModule"/>
  <xsl:variable name="userbind-module" select="$frontend-config/cfg:userBindModule"/>

  <xsl:variable name="request-module-logging" select="$request-module/cfg:logging"/>
  <xsl:variable name="request-session-timeout"><xsl:value-of select="$request-module/cfg:session/@timeout"/>
    <xsl:if test="count($request-module/cfg:session/@timeout) = 0">
      <xsl:value-of select="$def-request-session-timeout"/>
    </xsl:if>
  </xsl:variable>
  <xsl:variable name="request-update-period"><xsl:value-of
    select="$request-module/cfg:update/@period"/>
    <xsl:if test="count($request-module/cfg:update/@period) = 0">
      <xsl:value-of select="$def-request-update-period"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="request-profiling-log-sampling"><xsl:value-of
    select="$colo-config/cfg:predictorConfig/cfg:researchStat/@profiling_log_sampling"/>
    <xsl:if test="count($colo-config/cfg:predictorConfig/cfg:researchStat/@profiling_log_sampling) = 0">
      <xsl:value-of select="$def-profiling-log-sampling"/>
    </xsl:if>
  </xsl:variable>

  <!-- start config generation -->
  <!-- check that defined all needed parameters -->
  <xsl:choose>
    <xsl:when test="count($channel-controller-path[1]/@host) = 0">
      <xsl:message terminate="yes"> AdFrontend: channel controller host undefined. </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="virtual-servers-raw">
    <xsl:call-template name="GetVirtualServers">
      <xsl:with-param name="xpath" select="$colo-config/cfg:coloParams/cfg:virtualServer"/>
      <xsl:with-param name="config-type" select="'all'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="virtual-servers" select="exsl:node-set($virtual-servers-raw)/cfg:virtualServer"/>

  <xsl:variable name="backend-content-virtual-server"
    select="$virtual-servers[
      count(cfg:thirdPartyContentDomain) > 0][1]"/>

  <xsl:variable name="backend-content-domain"><xsl:value-of
    select="$backend-content-virtual-server/cfg:thirdPartyContentDomain[1]/@name"/><xsl:if
      test="count($backend-content-virtual-server/@port) > 0 and
        $backend-content-virtual-server/@port != 80">:<xsl:value-of
        select="$backend-content-virtual-server/@port"/></xsl:if></xsl:variable>

  <xsl:variable name="frontend-content-domain"><xsl:value-of
    select="$colo-config/cfg:coloParams/@CDN_domain"/><xsl:if
      test="string-length($colo-config/cfg:coloParams/@CDN_domain) = 0"><xsl:value-of
        select="$backend-content-domain"/></xsl:if></xsl:variable>

  <!-- HTTP domains (with port suffixes) -->
  <xsl:variable name="adserving-domain-virtual-server"
    select="$virtual-servers[
      count(cfg:adservingDomain) > 0][1]"/>

  <xsl:variable name="base-adserving-domain"><xsl:value-of
    select="$adserving-domain-virtual-server/cfg:adservingDomain[1]/@name"/></xsl:variable>

  <xsl:variable name="adserving-domain"><xsl:value-of
    select="$base-adserving-domain"/><xsl:if
    test="count($adserving-domain-virtual-server/@port) > 0 and
      $adserving-domain-virtual-server/@port != 80">:<xsl:value-of
    select="$adserving-domain-virtual-server/@port"/></xsl:if></xsl:variable>

  <!-- HTTPS domains (with port suffixes) -->
  <xsl:variable name="secure-adserving-domain-virtual-server"
    select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[
      count(cfg:adservingDomain) > 0][1]"/>

  <xsl:variable name="secure-adserving-domain"><xsl:value-of
    select="$secure-adserving-domain-virtual-server/
      cfg:adservingDomain[1]/@name"/><xsl:if
    test="$secure-adserving-domain-virtual-server/@port != 443">:<xsl:value-of
    select="$secure-adserving-domain-virtual-server/@port"/></xsl:if></xsl:variable>

  <xsl:variable name="secure-backend-content-domain-virtual-server"
    select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[
      count(cfg:thirdPartyContentDomain) > 0][1]"/>

  <xsl:variable name="secure-backend-content-domain"><xsl:value-of
    select="$secure-backend-content-domain-virtual-server/
      cfg:thirdPartyContentDomain[1]/@name"/><xsl:if
      test="count($secure-backend-content-domain-virtual-server/@port) > 0 and
        $secure-backend-content-domain-virtual-server/@port != 443">:<xsl:value-of
        select="$secure-backend-content-domain-virtual-server/@port"/></xsl:if></xsl:variable>

  <xsl:variable name="secure-frontend-content-domain"><xsl:value-of
    select="$colo-config/cfg:coloParams/@https_CDN_domain"/><xsl:if
      test="string-length($colo-config/cfg:coloParams/@https_CDN_domain) = 0"><xsl:value-of
        select="$secure-backend-content-domain"/></xsl:if></xsl:variable>

  <xsl:variable name="adfrontend-https-enabled">
    <xsl:if test="count($frontend-config/cfg:networkParams) = 0">true</xsl:if>
    <xsl:value-of select="string($frontend-config/cfg:networkParams/@https_enabled)"/>
  </xsl:variable>

  <cfg:CommonFeConfiguration user_agent_filter_path="user_agent_filter.txt"
    request_domain="{$adserving-domain}"
    colo_id="{$colo-config/cfg:coloParams/@colo_id}"
    channel_factory_threads="{$channel-server-count * 10}"
    domain_config_path="{concat($config-root, '/DomainConfig.xml')}"
    update_period="{$request-update-period}"
    profiling_log_sampling="{$request-profiling-log-sampling}"
    service_index="{
      concat(count($xpath/../preceding-sibling::serviceGroup) + 1, $SERVICE_ID)}"
    ip_salt="{$colo-config/cfg:predictorConfig/cfg:researchStat/@ip_salt}">

    <xsl:attribute name="ip_logging_enabled"><xsl:value-of
      select="$colo-config/cfg:predictorConfig/cfg:researchStat/@ip_logging_enabled"/><xsl:if
      test="count($colo-config/cfg:predictorConfig/cfg:researchStat/@ip_logging_enabled) =
      0">false</xsl:if>
    </xsl:attribute>

    <cfg:ResponseHeaders>
      <cfg:Header name="Cache-Control" value="no-store, no-cache"/>
      <cfg:Header name="Vary" value="Cookie"/>
    </cfg:ResponseHeaders>

    <cfg:GeoIP path="/usr/share/GeoIP/ipv4.csv"/>

    <xsl:call-template name="CampaignServerCorbaRefs">
      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="service-name" select="'AdFrontend'"/>
    </xsl:call-template>

    <xsl:if test="count($full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]) > 0 or
                  count($full-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']) > 0">
      <cfg:CampaignManagerRef name="CampaignManager">
        <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor] |
                              $full-cluster-path/serviceGroup[@descriptor = 'AdProfilingCluster/FrontendSubCluster']">

          <xsl:variable name="cluster-id" select="position()"/>

          <xsl:variable name="campaign-manager-host-port-set">
            <xsl:for-each select="./service[@descriptor = $campaign-manager-descriptor] |
                                  ./service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/CampaignManager']">
              <xsl:variable name="campaign-manager-host-subset">
                <xsl:call-template name="GetHosts">
                  <xsl:with-param name="hosts" select="@host"/>
                  <xsl:with-param name="error-prefix" select="'CampaignManager'"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:variable
                name="campaign-manager-config"
                select="./configuration/cfg:campaignManager"/>
              <xsl:if test="count($campaign-manager-config) = 0">
                <xsl:message terminate="yes"> AdFrontend: Can't find campaign manager config </xsl:message>
              </xsl:if>

              <xsl:variable name="campaign-manager-port">
                <xsl:value-of select="$campaign-manager-config/cfg:networkParams/@port"/>
                <xsl:if test="count($campaign-manager-config/cfg:networkParams/@port) = 0">
                  <xsl:value-of select="$def-campaign-manager-port"/>
                </xsl:if>
              </xsl:variable>
              <xsl:for-each select="exsl:node-set($campaign-manager-host-subset)/host">
                <host port="{$campaign-manager-port}"><xsl:value-of select="."/></host>
              </xsl:for-each>
            </xsl:for-each>
          </xsl:variable>

          <xsl:for-each select="exsl:node-set($campaign-manager-host-port-set)/host">
            <xsl:sort select="."/>
            <cfg:Ref service_index="{concat($cluster-id, '_', position())}"
              ref="{concat('corbaloc:iiop:', ., ':', @port, '/',
                $current-campaign-manager-obj)}"/>
          </xsl:for-each>

        </xsl:for-each>
      </cfg:CampaignManagerRef>
    </xsl:if>

    <xsl:call-template name="AddUserInfoManagerControllerGroups">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="error-prefix" select="AdFrontend"/>
    </xsl:call-template>

    <cfg:ChannelManagerControllerRefs name="ChannelManagerControllers">
      <xsl:for-each select="$channel-controller-path">

        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'AdFrontend:ChannelManagerController'"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:variable name="channel-controller-port">
          <xsl:value-of select="./configuration/cfg:channelController/cfg:networkParams/@port"/>
          <xsl:if test="count(./configuration/cfg:channelController/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-channel-controller-port"/>
          </xsl:if>
        </xsl:variable>

        <xsl:for-each select="exsl:node-set($hosts)/host">
        <cfg:Ref>
          <xsl:attribute name="ref">
            <xsl:value-of
              select="concat('corbaloc:iiop:', ., ':', $channel-controller-port,
                '/ChannelManagerController')"/>
          </xsl:attribute>
        </cfg:Ref>
        </xsl:for-each>
      </xsl:for-each>
    </cfg:ChannelManagerControllerRefs>

    <xsl:call-template name="AddUserBindControllerGroups">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="error-prefix" select="AdFrontend"/>
    </xsl:call-template>

    <cfg:Cookies>
      <xsl:variable name="old-cookie-domain"
        select="$base-adserving-domain"/>

      <xsl:variable name="cookie-domain">
        <xsl:choose>
          <xsl:when test="count($colo-config/cfg:Cookies/@domain) > 0">
            <xsl:choose>
              <xsl:when test="contains($colo-config/cfg:Cookies/@domain, '.')">.<xsl:value-of
                select="substring-after($colo-config/cfg:Cookies/@domain, '.')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$colo-config/cfg:Cookies/@domain"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <xsl:choose>
              <xsl:when test="contains($base-adserving-domain, '.')">.<xsl:value-of
                select="substring-after($base-adserving-domain, '.')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$base-adserving-domain"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="optout-cookie-domain">
        <xsl:choose>
          <xsl:when test="contains($base-adserving-domain, '.')">.<xsl:value-of
            select="substring-after($base-adserving-domain, '.')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$base-adserving-domain"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="default-path" select="'/'"/>

      <xsl:attribute name="domain"><xsl:value-of select="$cookie-domain"/></xsl:attribute>
      <xsl:attribute name="path"><xsl:value-of select="$default-path"/></xsl:attribute>
      <xsl:attribute name="expires"><xsl:value-of select="'31536000'"/></xsl:attribute>

      <cfg:ResponseHeader
        name="P3P">
         <xsl:attribute name="value"><xsl:value-of select="$request-module/@p3p_header"/>
          <xsl:if test="count($request-module/@p3p_header) = 0">
            <xsl:value-of select="$default-p3p-header"/>
          </xsl:if>
         </xsl:attribute>
      </cfg:ResponseHeader>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'lc'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'uid'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'uid2'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'trialoptin'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'730'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'oi_prompt'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'OPTED_IN'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'OPTED_OUT'"/>
        <xsl:with-param name="default-domain" select="$optout-cookie-domain"/>
        <xsl:with-param name="default-path" select="'/'"/>
        <xsl:with-param name="default-expires" select="'730'"/>
      </xsl:call-template>

      <xsl:call-template name="CookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'ap'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
        <xsl:with-param name="default-expires" select="'365'"/>
      </xsl:call-template>

      <!-- actual cookies -->
      <xsl:call-template name="RemoveCookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'lc'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
      </xsl:call-template>

      <xsl:call-template name="RemoveCookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'uid'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
      </xsl:call-template>

      <xsl:call-template name="RemoveCookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'uid2'"/>
        <xsl:with-param name="default-domain" select="$cookie-domain"/>
        <xsl:with-param name="default-path" select="$default-path"/>
      </xsl:call-template>

      <xsl:call-template name="RemoveCookieConfigurator">
        <xsl:with-param name="cookies-path" select="$colo-config/cfg:Cookies"/>
        <xsl:with-param name="name" select="'OPTED_OUT'"/>
        <xsl:with-param name="default-domain" select="$optout-cookie-domain"/>
        <xsl:with-param name="default-path" select="'/'"/>
      </xsl:call-template>

      <!-- outdated cookies -->
      <cfg:RemoveCookie name="MARef" domain="" path="/services/"/>
      <cfg:RemoveCookie name="MARKw" domain="" path="/services/"/>
      <cfg:RemoveCookie name="MASKw" domain="" path="/services/"/>
      <cfg:RemoveCookie name="MARKww" domain="" path="/services/"/>
      <cfg:RemoveCookie name="MASKww" domain="" path="/services/"/>

      <cfg:RemoveCookie name="sizenotsup" domain="" path="/services/"/>
      <cfg:RemoveCookie name="ct.pswnd" domain="" path="/services/"/>
      <cfg:RemoveCookie name="session_activity" domain="" path="/services/"/>
      <cfg:RemoveCookie name="freq_caps4" domain="" path="/services/"/>

      <cfg:RemoveCookie name="freq_caps5" domain="" path="/services/"/>
      <cfg:RemoveCookie name="session_info" domain="" path="/services/"/>
      <cfg:RemoveCookie name="state_change" domain="" path="/services/"/>
      <cfg:RemoveCookie name="action_info" domain="" path="/services/"/>
      <cfg:RemoveCookie name="action_info2" domain="" path="/services/"/>
      <cfg:RemoveCookie name="v_freq_caps" domain="" path="/services/"/>
      <cfg:RemoveCookie name="uc_freq_caps" domain="" path="/services/"/>
      <cfg:RemoveCookie name="last_colo" domain="" path="/services/"/>

      <cfg:RemoveCookie name="fc" domain="" path="/services/"/>
      <cfg:RemoveCookie name="ai" domain="" path="/services/"/>
      <cfg:RemoveCookie name="si" domain="" path="/services/"/>
      <cfg:RemoveCookie name="vfc" domain="" path="/services/"/>
      <cfg:RemoveCookie name="ucfc" domain="" path="/services/"/>
      <cfg:RemoveCookie name="sc" domain="" path="/services/"/>
    </cfg:Cookies>

    <cfg:OptOutRemoveCookies>
      <cfg:Cookie name="uid"/>
      <cfg:Cookie name="uid2"/>
      <cfg:Cookie name="OPTED_IN"/>
    </cfg:OptOutRemoveCookies>

    <cfg:OutdatedCookies>
      <cfg:Cookie name="session_activity"/>
      <cfg:Cookie name="freq_caps4"/>
      <cfg:Cookie name="freq_caps5"/>
      <cfg:Cookie name="uc_freq_caps"/>
      <cfg:Cookie name="session_info"/>
      <cfg:Cookie name="state_change"/>
      <cfg:Cookie name="last_colo"/>
    </cfg:OutdatedCookies>

    <xsl:variable name="uid-key-path">
      <xsl:call-template name="GetUidKeyPath">
        <xsl:with-param name="unixcommons-root" select="$unixcommons-root"/>
        <xsl:with-param name="config-root" select="$config-root"/>
        <xsl:with-param name="colo-config" select="$colo-config"/>
        <xsl:with-param name="uid-key" select="'configuration:uid_key'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="uid-temp-key-path">
      <xsl:call-template name="GetUidKeyPath">
        <xsl:with-param name="unixcommons-root" select="$unixcommons-root"/>
        <xsl:with-param name="config-root" select="$config-root"/>
        <xsl:with-param name="colo-config" select="$colo-config"/>
        <xsl:with-param name="uid-key" select="'configuration:temporary_uid_key'"/>
      </xsl:call-template>
    </xsl:variable>

    <cfg:UserIdConfig cache_size="100000" temp_cache_size="100000" ssp_cache_size="100000"
      private_key="{concat($uid-key-path, 'private.der')}"
      public_key="{concat($uid-key-path, 'public.der')}"
      temp_public_key="{concat($uid-temp-key-path, 'public_temp.der')}"
      ssp_private_key="{concat($unixcommons-root, '/share/uuid_keys/ssp_private_key.der')}"
      ssp_public_key="{concat($unixcommons-root, '/share/uuid_keys/ssp_public_key.der')}"
      ssp_uid_key="{$colo-config/cfg:coloParams/@ssp_uid_key}">
      <xsl:call-template name="FillUserIdBlackList">
        <xsl:with-param name="desc" select="$full-cluster-path/../@description"/>
      </xsl:call-template>
    </cfg:UserIdConfig>

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

    <cfg:TemplateRule name="unsecure">
      <xsl:attribute name="main_domain">http://<xsl:value-of select="$adserving-domain"/></xsl:attribute>

      <cfg:XsltToken name="ADIMAGE-SERVER">
        <xsl:attribute name="value">http://<xsl:value-of
          select="$frontend-content-domain"/></xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="ADIMAGE-PATH-PREFIX">
        <xsl:attribute name="value">http://<xsl:value-of
          select="$frontend-content-domain"/>/creatives</xsl:attribute>
      </cfg:XsltToken>

      <cfg:XsltToken name="ADIMAGE_SERVER">
        <xsl:attribute name="value">http://<xsl:value-of
          select="$frontend-content-domain"/></xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="PUBPIXELSOPTIN">
        <xsl:attribute name="value">http://<xsl:value-of
          select="$backend-content-domain"/>/pubpixels?us=in</xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="PUBPIXELSOPTOUT">
        <xsl:attribute name="value">http://<xsl:value-of
          select="$backend-content-domain"/>/pubpixels?us=out</xsl:attribute>
      </cfg:XsltToken>
    </cfg:TemplateRule>

    <cfg:TemplateRule name="secure">
      <xsl:attribute name="main_domain">https://<xsl:value-of
        select="$secure-adserving-domain"/></xsl:attribute>

      <cfg:XsltToken name="ADIMAGE-SERVER">
        <xsl:attribute name="value">https://<xsl:value-of
          select="$secure-frontend-content-domain"/></xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="ADIMAGE-PATH-PREFIX">
        <xsl:attribute name="value">https://<xsl:value-of
          select="$secure-frontend-content-domain"/>/creatives</xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="ADIMAGE_SERVER">
        <xsl:attribute name="value">https://<xsl:value-of
          select="$secure-frontend-content-domain"/></xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="PUBPIXELSOPTIN">
        <xsl:attribute name="value">https://<xsl:value-of
          select="$secure-backend-content-domain"/>/pubpixels?us=in</xsl:attribute>
      </cfg:XsltToken>
      <cfg:XsltToken name="PUBPIXELSOPTOUT">
        <xsl:attribute name="value">https://<xsl:value-of
          select="$secure-backend-content-domain"/>/pubpixels?us=out</xsl:attribute>
      </cfg:XsltToken>
    </cfg:TemplateRule>

    <xsl:call-template name="AddStatsDumper">
      <xsl:with-param name="stats-collector-path" select="$stats-collector-path"/>
      <xsl:with-param name="stats-collector-config" select="$stats-collector"/>
    </xsl:call-template>

    <cfg:DebugInfo>
      <xsl:if test="string-length($debug-param/@use_acl) > 0">
        <xsl:attribute name="use_acl"><xsl:value-of select="$debug-param/@use_acl"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="string-length($debug-param/@show_history_profile) > 0">
        <xsl:attribute name="show_history_matching"><xsl:value-of select="$debug-param/@show_history_profile"/></xsl:attribute>
      </xsl:if>
      <cfg:ips><xsl:if test="string-length($debug-param/@acl) > 0">
          <xsl:value-of select="$debug-param/@acl"/>
        </xsl:if></cfg:ips>
      <cfg:colocations><xsl:if test="string-length($debug-param/@colocations) > 0">
          <xsl:value-of select="$debug-param/@colocations"/>
        </xsl:if></cfg:colocations>
    </cfg:DebugInfo>

    <xsl:if test="count($colo-config/cfg:IPMapping/cfg:colo) > 0">
      <cfg:IPMapping>
        <xsl:for-each select="$colo-config/cfg:IPMapping/cfg:colo">
          <xsl:variable name="checked_ip_range">
            <xsl:choose>
              <xsl:when test="normalize-space(@ip_range) = ''">0.0.0.0/0</xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="check_ip_masks">
                  <xsl:with-param name="ip_masks" select="@ip_range"/>
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:variable name="webindex-profile"><xsl:if
            test="count(@webindex_profile) = 0">true</xsl:if><xsl:value-of select="@webindex_profile"/>
          </xsl:variable>
          <cfg:Rule ip_range="{$checked_ip_range}"
            cohorts="{@cohorts}" colo_id="{@colo_id}" profile_referer="{$webindex-profile}"/>
        </xsl:for-each>
      </cfg:IPMapping>
    </xsl:if>

    <xsl:if test="count($colo-config/cfg:coloParams/@enabled_countries) > 0">
      <xsl:variable name="filter-country-tokens"
        select="str:tokenize($colo-config/cfg:coloParams/@enabled_countries, ',&#x9;&#xA;&#xD;&#x20;')"/>
      <xsl:if test="count($filter-country-tokens) > 0">
        <cfg:CountryFiltering>
          <xsl:for-each select="$filter-country-tokens">
            <cfg:Country country_code="{.}"/>
          </xsl:for-each>
        </cfg:CountryFiltering>
      </xsl:if>
    </xsl:if>

    <xsl:comment>Comment the line below to disable support for Asian languages</xsl:comment>
    <xsl:choose>
      <xsl:when test="$colo-config/cfg:coloParams/@segmentor = 'Nlpir'">
        <cfg:Chinese name="Nlpir" base="/usr/share/NLPIR" country='cn'/>
      </xsl:when>
      <xsl:when test="$colo-config/cfg:coloParams/@segmentor = 'Polyglot'">
        <cfg:Polyglot name='Polyglot' base="/opt/foros/polyglot/dict/" country=''/>
      </xsl:when>
    </xsl:choose>
    <cfg:SkipExternalIds
      skip_external_ids="{$colo-config/cfg:coloParams/@skip_external_ids}">
      <cfg:Id value="00000000-0000-0000-0000-000000000000"/>
      <cfg:Id value="0:0"/>
      <cfg:Id value="0"/>
      <cfg:Id value="-1"/>
      <cfg:Id value="SMD0"/>
      <cfg:Id value="W"/>
      <cfg:Id value="W/W"/>
      <cfg:Id value="W/W/W"/>
      <cfg:Id value="{cap0}"/>
      <cfg:Id value="%7Bcap1%7D"/>
      <cfg:Id><xsl:attribute name="value">{UID}</xsl:attribute></cfg:Id>
    </cfg:SkipExternalIds>
  </cfg:CommonFeConfiguration>

  <xsl:variable name="inventory-users-percentage-value"><xsl:value-of select="$colo-config/cfg:inventoryStats/@simplifying"/>
    <xsl:if test="count($colo-config/cfg:inventoryStats) = 0">
      <xsl:value-of select="$inventory-users-percentage"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="use-referrer-action-stats">
    <xsl:call-template name="GetReferrerLoggingValue">
      <xsl:with-param name="referrer-logging" select="$colo-config/@referrer_logging_action_stats"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="use-referrer-site-referrer-stats">
    <xsl:call-template name="GetReferrerLoggingValue">
      <xsl:with-param name="referrer-logging" select="$colo-config/@referrer_logging_site_referrer_stats"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="not(count($request-module) = 0)">
    <cfg:AdFeConfiguration threads="1000" max_pending_tasks="100">
      <xsl:attribute name="ad_request_profiling">
        <xsl:choose>
          <xsl:when test="$colo-config/cfg:coloParams/@ad_request_profiling =
            'ad-request profiling and stats collection enabled'">true</xsl:when>
          <xsl:when test="count($colo-config/cfg:coloParams/@ad_request_profiling) = 0 or
            $colo-config/cfg:coloParams/@ad_request_profiling =
              'ad-request profiling disabled, stats collection enabled' or
            $colo-config/cfg:coloParams/@ad_request_profiling =
              'ad-request profiling and stats collection disabled'">false</xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">AdFrontend: unexpected ad_request_profiling value</xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:attribute name="set_uid">
        <xsl:choose>
          <xsl:when test="count($request-module/@set_uid) > 0"><xsl:value-of select="$request-module/@set_uid"/></xsl:when>
          <xsl:otherwise>always</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:attribute name="probe_uid">
        <xsl:choose>
          <xsl:when test="count($request-module/@probe_uid) > 0"><xsl:value-of select="$request-module/@probe_uid"/></xsl:when>
          <xsl:otherwise>false</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:attribute name="session_timeout"><xsl:value-of select="$request-session-timeout"/></xsl:attribute>
      <xsl:attribute name="inventory_users_percentage"><xsl:value-of select="$inventory-users-percentage"/></xsl:attribute>
      <xsl:attribute name="use_referrer_site_referrer_stats"><xsl:value-of select="$use-referrer-site-referrer-stats"/></xsl:attribute>
      <xsl:if test="count(
         $colo-config/cfg:WebIndex/@enable_direct_request_profiling) > 0 and
          ($colo-config/cfg:WebIndex/@enable_direct_request_profiling = 'true' or
             $colo-config/cfg:WebIndex/@enable_direct_request_profiling = '1')">
           <xsl:attribute name="enable_profile_referer">true</xsl:attribute>
      </xsl:if>
      <cfg:UriList>
        <cfg:Uri path="/services/nslookup"/>
        <cfg:Uri path="/get"/>
      </cfg:UriList>
      <xsl:call-template name="ConvertFrontendLogger">
        <xsl:with-param name="logger-node" select="$request-module/cfg:logging"/>
        <xsl:with-param name="default-log-level" select="$request-module-log-level"/>
      </xsl:call-template>
    </cfg:AdFeConfiguration>
 </xsl:if>

  <cfg:AdInstFeConfiguration threads="1000">
    <cfg:UriList>
      <cfg:Uri path="/services/inst"/>
      <cfg:Uri path="/inst"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$adinst-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$adinst-module-log-level"/>
    </xsl:call-template>
  </cfg:AdInstFeConfiguration>

  <cfg:ContentFeConfiguration threads="400">
    <cfg:UriList>
      <cfg:Uri path="/services/dcreative"/>
      <cfg:Uri path="/dcreative"/>
    </cfg:UriList>
    <cfg:DirectoryList>
      <cfg:Directory path="/tags" root="{$tags-data-root}"/>
    </cfg:DirectoryList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$content-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$content-module-log-level"/>
    </xsl:call-template>
    <cfg:TemplateCache root="{concat($www-root, '/Creatives/')}">
      <xsl:attribute name="size"><xsl:value-of select="$content-module/cfg:cache/@size"/>
        <xsl:if test="count($content-module/cfg:cache/@size) = 0">52428800</xsl:if>
      </xsl:attribute>
      <xsl:attribute name="timeout"><xsl:value-of select="$content-module/cfg:cache/@timeout"/>
        <xsl:if test="count($content-module/cfg:cache/@timeout) = 0">10</xsl:if>
      </xsl:attribute>
    </cfg:TemplateCache>
  </cfg:ContentFeConfiguration>

  <xsl:variable name="click-matching-threads"><xsl:value-of
    select="$click-module/@threads"/><xsl:if
      test="count($click-module/@threads) = 0"><xsl:call-template name="GetAttr">
    <xsl:with-param name="node" select="$frontend-config/cfg:webServerParams"/>
      <xsl:with-param name="name" select="'max_clients'"/>
      <xsl:with-param name="type" select="$xsd-webserver-params-type"/>
    </xsl:call-template></xsl:if>
  </xsl:variable>

  <xsl:variable name="match-task-limit"><xsl:value-of
    select="$click-module/@task_limit"/><xsl:if
      test="count($click-module/@task_limit) = 0"><xsl:value-of
        select="$click-matching-threads * 2"/></xsl:if>
  </xsl:variable>

  <cfg:ClickFeConfiguration threads="{$click-matching-threads}"
    match_task_limit="{$match-task-limit}"
    max_pending_tasks="0">
    <xsl:attribute name="template_file"><xsl:value-of
      select="$ps-res-data-root"/>/http/tag/click.html</xsl:attribute>
    <xsl:attribute name="set_uid">
      <xsl:choose>
        <xsl:when test="$click-module/@set_uid = 'false'">never</xsl:when>
        <xsl:otherwise>always</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="probe_uid">
      <xsl:choose>
        <xsl:when test="count($click-module/@probe_uid) > 0"><xsl:value-of select="$click-module/@probe_uid"/></xsl:when>
        <xsl:otherwise>false</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <cfg:PathUriList>
      <cfg:Uri path="/services/AdClickServer/"/>
      <cfg:Uri path="/click/"/>
    </cfg:PathUriList>
    <cfg:UriList>
      <cfg:Uri path="/click"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$click-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$click-module-log-level"/>
    </xsl:call-template>
  </cfg:ClickFeConfiguration>

  <xsl:variable name="impression-matching-threads"><xsl:value-of
    select="$impression-module/@threads"/><xsl:if
      test="count($impression-module/@threads) = 0"><xsl:call-template name="GetAttr">
    <xsl:with-param name="node" select="$frontend-config/cfg:webServerParams"/>
      <xsl:with-param name="name" select="'max_clients'"/>
      <xsl:with-param name="type" select="$xsd-webserver-params-type"/>
    </xsl:call-template></xsl:if>
  </xsl:variable>

  <xsl:variable name="impression-match-task-limit"><xsl:value-of
    select="$impression-module/@task_limit"/><xsl:if
      test="count($impression-module/@task_limit) = 0"><xsl:value-of
        select="$impression-matching-threads * 2"/></xsl:if>
  </xsl:variable>

  <cfg:ImprTrackFeConfiguration threads="400"
    match_threads="20"
    match_task_limit="{$impression-match-task-limit}"
    >
    <xsl:attribute name="track_pixel_path"><xsl:value-of select="concat($data-root, '/aux/pt.png')"/></xsl:attribute>
    <xsl:attribute name="track_pixel_content_type">image/png</xsl:attribute>
    <xsl:attribute name="template_file"><xsl:value-of
      select="$ps-res-data-root"/>/http/tag/track.html</xsl:attribute>
    <cfg:UriList>
      <cfg:Uri path="/services/ImprTrack/pt.gif"/>
      <cfg:Uri path="/track.gif"/>
      <cfg:Uri path="/track.png"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$impression-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$impression-module-log-level"/>
    </xsl:call-template>
    <cfg:DefaultKeys/>
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTBAccount">
      <cfg:AccountTraits account_id="{@account_id}">
        <xsl:call-template name="GenerateEncryptionKeys"/>
      </cfg:AccountTraits>
    </xsl:for-each>
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTBSite">
      <cfg:SiteTraits site_id="{@site_id}">
        <xsl:call-template name="GenerateEncryptionKeys"/>
      </cfg:SiteTraits>
    </xsl:for-each>
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:bindUrl">
      <cfg:BindURL template="{@template}" use_keywords="{@use_keywords}" keywords="{@keywords}"/>
    </xsl:for-each>
  </cfg:ImprTrackFeConfiguration>

  <xsl:variable name="action-matching-threads"><xsl:value-of
    select="$action-module/@threads"/><xsl:if
      test="count($action-module/@threads) = 0"><xsl:call-template name="GetAttr">
    <xsl:with-param name="node" select="$frontend-config/cfg:webServerParams"/>
      <xsl:with-param name="name" select="'max_clients'"/>
      <xsl:with-param name="type" select="$xsd-webserver-params-type"/>
    </xsl:call-template></xsl:if>
  </xsl:variable>

  <xsl:variable name="action-match-task-limit"><xsl:value-of
    select="$action-module/@task_limit"/><xsl:if
      test="count($action-module/@task_limit) = 0"><xsl:value-of
        select="$action-matching-threads * 2"/></xsl:if>
  </xsl:variable>

  <cfg:ActionFeConfiguration threads="{$action-matching-threads}"
    match_task_limit="{$action-match-task-limit}"
    max_pending_tasks="1000">
    <xsl:attribute name="track_html_path"><xsl:value-of select="$data-root"/>/aux/conv_track.html</xsl:attribute>
    <xsl:attribute name="track_pixel_path"><xsl:value-of select="concat($data-root, '/aux/pt.gif')"/></xsl:attribute>
    <xsl:attribute name="use_referrer_action_stats"><xsl:value-of select="$use-referrer-action-stats"/></xsl:attribute>
    <xsl:attribute name="set_uid"><xsl:value-of select=
      "$action-module/@set_uid"/><xsl:if test="count($action-module/@set_uid) = 0">true</xsl:if></xsl:attribute>
    <cfg:PathUriList>
      <cfg:Uri path="/services/ActionServer/"/>
      <cfg:Uri path="/action/"/>
    </cfg:PathUriList>
    <cfg:UriList>
      <cfg:Uri path="/conv.html"/>
    </cfg:UriList>
    <cfg:PixelUriList>
      <cfg:Uri path="/conv"/>
    </cfg:PixelUriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$action-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$action-module-log-level"/>
    </xsl:call-template>
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:bindUrl">
      <cfg:Redirect template="{@template}" use_keywords="{@use_keywords}" keywords="{@keywords}"/>
    </xsl:for-each>
  </cfg:ActionFeConfiguration>

  <cfg:PassFeConfiguration threads="200">
    <xsl:attribute name="domain">http://<xsl:value-of select="$backend-content-domain"/></xsl:attribute>
    <xsl:if test="$adfrontend-https-enabled = 'true'">
      <xsl:attribute name="secure_domain">https://<xsl:value-of select="$secure-backend-content-domain"/></xsl:attribute>
    </xsl:if>
    <cfg:UriList>
      <cfg:Uri path="/services/passback"/>
      <cfg:Uri path="/passback"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$pass-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$pass-module-log-level"/>
    </xsl:call-template>
  </cfg:PassFeConfiguration>

  <cfg:PassPixelFeConfiguration threads="200">
    <xsl:attribute name="track_pixel_path"><xsl:value-of select="concat($data-root, '/aux/pt.gif')"/></xsl:attribute>
    <cfg:UriList>
      <cfg:Uri path="/services/passback.gif"/>
      <cfg:Uri path="passback.gif"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$pass-pixel-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$pass-pixel-module-log-level"/>
    </xsl:call-template>
  </cfg:PassPixelFeConfiguration>
  
  <cfg:WebStatFeConfiguration threads="400">
    <xsl:attribute name="pixel_path"><xsl:value-of select="concat($data-root, '/aux/pt.gif')"/></xsl:attribute>
    <xsl:attribute name="rid_public_key"><xsl:value-of select="concat($config-root, '/rid_public_key.der')"/></xsl:attribute>
    <cfg:UriList>
      <cfg:Uri path="/services/sl.gif"/>
      <cfg:Uri path="/sl.gif"/>
    </cfg:UriList>
    <cfg:YandexNotificationUriList>
      <cfg:Uri path="/services/yandex_notification"/>
    </cfg:YandexNotificationUriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$webstat-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$webstat-module-log-level"/>
    </xsl:call-template>
  </cfg:WebStatFeConfiguration>

  <cfg:PubPixelFeConfiguration threads="400">
    <cfg:UriList>
      <cfg:Uri path="/pubpixels"/>
    </cfg:UriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$pubpixel-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$pubpixel-module-log-level"/>
    </xsl:call-template>
  </cfg:PubPixelFeConfiguration>

  <xsl:if test="$conf-type = 'nginx'">
    <cfg:BidFeConfiguration max_pending_tasks="10">
      <xsl:attribute name="threads"><xsl:value-of select="$bidding-module/@threads"/>
        <xsl:if test="count($bidding-module/@threads) = 0">
          <xsl:value-of select="$def-bidding-module-threads"/>
        </xsl:if>
      </xsl:attribute>
      <xsl:attribute name="request_timeout"><xsl:value-of select="$bidding-module/@max_bid_time"/>
        <xsl:if test="count($bidding-module/@max_bid_time) = 0">
          <xsl:value-of select="'50'"/>
        </xsl:if>
      </xsl:attribute>
      <xsl:attribute name="flush_period"><xsl:value-of
        select="$bidding-module/@flush_period"/><xsl:if
          test="count($bidding-module/@flush_period) =
            0">10</xsl:if>
      </xsl:attribute>

      <xsl:attribute name="intervalsBlacklist">
        <xsl:value-of select="$colo-config/cfg:coloParams/@intervalsBlacklist"/>
      </xsl:attribute>
      <xsl:if test="count(
       $colo-config/cfg:WebIndex/@enable_rtb_request_profiling) > 0 and
        ($colo-config/cfg:WebIndex/@enable_rtb_request_profiling = 'true' or
           $colo-config/cfg:WebIndex/@enable_rtb_request_profiling = '1')">
         <xsl:attribute name="enable_profile_referer">true</xsl:attribute>
      </xsl:if>

      <cfg:GoogleUriList>
        <cfg:Uri path="/google"/>
      </cfg:GoogleUriList>

      <cfg:OpenRtbUriList>
        <cfg:Uri path="/openrtb"/>
      </cfg:OpenRtbUriList>

      <cfg:AppNexusUriList>
        <cfg:Uri path="/appnexus"/>
      </cfg:AppNexusUriList>

      <cfg:AdXmlUriList>
        <cfg:Uri path="/adxml"/>
      </cfg:AdXmlUriList>

      <cfg:ClickStarUriList>
        <cfg:Uri path="/directnative"/>
      </cfg:ClickStarUriList>

      <cfg:DAOUriList>
        <cfg:Uri path="/dao"/>
      </cfg:DAOUriList>

      <xsl:call-template name="ConvertFrontendLogger">
        <xsl:with-param name="logger-node" select="$bidding-module/cfg:logging"/>
        <xsl:with-param name="default-log-level" select="$bidding-module-log-level"/>
      </xsl:call-template>

      <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTB/cfg:source">
        <cfg:Source id="{@id}">
          <xsl:variable name="instantiate-type">
            <xsl:if test="count(@instantiate_type) = 0">body</xsl:if><xsl:value-of select="@instantiate_type"/>
          </xsl:variable>

          <xsl:if test="count(@default_account_id) > 0">
          <xsl:attribute name="default_account_id"><xsl:value-of select="@default_account_id"/>
          </xsl:attribute></xsl:if>
          <xsl:attribute name="instantiate_type">
            <xsl:value-of select="$instantiate-type"/>
          </xsl:attribute>
          <xsl:if test="count(@request_type) > 0">
            <xsl:attribute name="request_type"><xsl:value-of select="@request_type"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="count(@seat) > 0">
            <xsl:attribute name="seat"><xsl:value-of select="@seat"/></xsl:attribute>
          </xsl:if>
          <xsl:attribute name="notice"><xsl:value-of
             select="@notice"/><xsl:if test="count(@notice)
             = 0">disabled</xsl:if></xsl:attribute>
          <xsl:if test="count(@notice_url) > 0">
            <xsl:attribute name="notice_url"><xsl:value-of select="@notice_url"/></xsl:attribute>
          </xsl:if>
          <xsl:attribute name="native_notice"><xsl:value-of
             select="@native_notice"/><xsl:if test="count(@native_notice)
             = 0">disabled</xsl:if></xsl:attribute>

          <xsl:attribute name="truncate_domain"><xsl:value-of
             select="@truncate_domain"/><xsl:if test="count(@truncate_domain)
             = 0">false</xsl:if></xsl:attribute>
          <xsl:if test="count(@appnexus_member_id) > 0">
            <xsl:attribute name="appnexus_member_id"><xsl:value-of
              select="@appnexus_member_id"/></xsl:attribute>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="count(@vast_instantiate_type) > 0">
              <xsl:attribute name="vast_instantiate_type">
                <xsl:value-of select="@vast_instantiate_type"/>
              </xsl:attribute>
              <xsl:attribute name="vast_notice">
                <xsl:value-of select="@vast_notice"/>
              </xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="vast_instantiate_type">
                <xsl:value-of select="$instantiate-type"/>
              </xsl:attribute>
              <xsl:attribute name="vast_notice">
                <xsl:value-of select="@notice"/>
              </xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:attribute name="fill_adid">
            <xsl:value-of select="@fill_adid"/>
            <xsl:if test="count(@fill_adid) = 0">false</xsl:if>
          </xsl:attribute>
          <xsl:attribute name="ipw_extension"><xsl:value-of
             select="@ipw_extension"/><xsl:if test="count(@ipw_extension)
             = 0">false</xsl:if></xsl:attribute>
          <xsl:if test="count(@max_bid_time) > 0">
            <xsl:attribute name="max_bid_time">
              <xsl:value-of select="@max_bid_time"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:attribute name="native_instantiate_type"><xsl:value-of 
            select="@native_instantiate_type"/><xsl:if 
              test="count(@native_instantiate_type) = 0">none</xsl:if>
          </xsl:attribute>
          <xsl:if test="count(@native_impression_tracker_type) > 0">
            <xsl:attribute name="native_impression_tracker_type"><xsl:value-of 
              select="@native_impression_tracker_type"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:attribute name="skip_ext_category"><xsl:value-of
             select="@skip_ext_category"/><xsl:if test="count(@skip_ext_category)
             = 0">false</xsl:if></xsl:attribute>
        </cfg:Source>
      </xsl:for-each>

      <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTBAccount[
        count(@max_cpm_value) > 0 or count(@google_display_billing_id) > 0 or count(@google_video_billing_id) > 0]">
        <cfg:Account account_id="{@account_id}">
          <xsl:if test="count(@max_cpm_value) > 0">
            <xsl:attribute name="max_cpm_value"><xsl:value-of select="@max_cpm_value"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="count(@google_display_billing_id) > 0">
            <xsl:attribute name="display_billing_id"><xsl:value-of select="@google_display_billing_id"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="count(@google_video_billing_id) > 0">
            <xsl:attribute name="video_billing_id"><xsl:value-of select="@google_video_billing_id"/></xsl:attribute>
          </xsl:if>
          <xsl:call-template name="GenerateEncryptionKeys"/>
        </cfg:Account>
      </xsl:for-each>

    </cfg:BidFeConfiguration>
  </xsl:if>

  <cfg:UserBindFeConfiguration enable_profiling="true">
    <xsl:if test="count($userbind-module/@nosecure_redirect) > 0">
      <xsl:attribute name="nosecure_redirect"><xsl:value-of
        select="$userbind-module/@nosecure_redirect"/></xsl:attribute>
    </xsl:if>

    <xsl:attribute name="set_uid">
      <xsl:if test="count($userbind-module/@set_uid) = 0">true</xsl:if><xsl:value-of
        select="$userbind-module/@set_uid"/>
    </xsl:attribute>
    <xsl:attribute name="create_profile">
      <xsl:if test="count($userbind-module/@create_profile) = 0">true</xsl:if><xsl:value-of
        select="$userbind-module/@create_profile"/>
    </xsl:attribute>
    <xsl:variable name="userbind-module-bind-threads"><xsl:value-of select="$userbind-module/@threads"/>
      <xsl:if test="count($userbind-module/@threads) = 0">
        <xsl:value-of select="$def-userbind-module-threads"/>
      </xsl:if>
    </xsl:variable>
    <xsl:attribute name="threads"><xsl:value-of select="$userbind-module-bind-threads"/></xsl:attribute>

    <xsl:variable name="userbind-module-match-threads"><xsl:value-of select="$userbind-module/@match_threads"/>
      <xsl:if test="count($userbind-module/@match_threads) = 0">
        <xsl:value-of select="$def-userbind-module-match-threads"/>
      </xsl:if>
    </xsl:variable>
    <xsl:attribute name="match_threads"><xsl:value-of select="$userbind-module-match-threads"/></xsl:attribute>

    <xsl:attribute name="max_request_time"><xsl:value-of select="$userbind-module/@max_request_time"/>
      <xsl:if test="count($userbind-module/@max_request_time) = 0">
        <xsl:value-of select="'400'"/>
      </xsl:if>
    </xsl:attribute>
    <xsl:attribute name="match_pending_task_limit"><xsl:value-of select="$userbind-module/@match_pending_task_limit"/>
      <xsl:if test="count($userbind-module/@match_pending_task_limit) = 0">
        <xsl:value-of select="$userbind-module-match-threads * 3"/>
      </xsl:if>
    </xsl:attribute>
    <xsl:attribute name="bind_pending_task_limit"><xsl:value-of select="$userbind-module/@bind_pending_task_limit"/>
      <xsl:if test="count($userbind-module/@bind_pending_task_limit) = 0">
        <xsl:value-of select="$userbind-module-bind-threads * 3"/>
      </xsl:if>
    </xsl:attribute>

    <xsl:attribute name="pixel_path">
      <xsl:value-of select="concat($data-root, '/aux/pt.png')"/>
    </xsl:attribute>
    <xsl:attribute name="pixel_content_type">image/png</xsl:attribute>

    <cfg:PathUriList>
      <cfg:Uri path="/userbind/"/>
    </cfg:PathUriList>
    <cfg:UriList>
      <cfg:Uri path="/userbind"/>
      <cfg:Uri path="/userbindint"/>
      <cfg:Uri path="/userbindadd"/>
      <cfg:Uri path="/"/>
    </cfg:UriList>
    <cfg:PixelUriList>
      <cfg:Uri path="/userbind.gif"/>
      <cfg:Uri path="/userbindint.gif"/>
      <cfg:Uri path="/userbindadd.gif"/>
      <cfg:Uri path="/userbind.png"/>
      <cfg:Uri path="/userbindint.png"/>
      <cfg:Uri path="/userbindadd.png"/>
    </cfg:PixelUriList>
    <xsl:call-template name="ConvertFrontendLogger">
      <xsl:with-param name="logger-node" select="$userbind-module/cfg:logging"/>
      <xsl:with-param name="default-log-level" select="$userbind-module-log-level"/>
    </xsl:call-template>

    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTB/cfg:allowedPassback">
      <cfg:AllowedPassback domain="{@domain}"/>
    </xsl:for-each>

    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:RTB/cfg:source">
      <xsl:variable name="source-id"><xsl:value-of select="@id"/></xsl:variable>
      <xsl:if test="count(cfg:userBind) > 0">
         <xsl:for-each select="cfg:userBind">
           <cfg:Source id="{$source-id}" passback="{@passback}">
             <xsl:attribute name="redirect"><xsl:value-of select="@redirect"/></xsl:attribute>
             <xsl:attribute name="weight"><xsl:value-of select="@weight"/><xsl:if
               test="count(@weight) = 0">1</xsl:if></xsl:attribute>
             <xsl:attribute name="redirect_empty_uid"><xsl:value-of select="@redirect_empty_uid"/><xsl:if
               test="count(@redirect_empty_uid) = 0">false</xsl:if></xsl:attribute>
           </cfg:Source>

           <xsl:for-each select="cfg:keywordRedirect">
             <cfg:Source redirect="{@redirect}"
                 keywords="{@keywords}"
                 passback="{@passback}">
               <xsl:attribute name="weight"><xsl:value-of select="@weight"/><xsl:if
                 test="count(@weight) = 0">1</xsl:if></xsl:attribute>
               <xsl:attribute name="redirect_empty_uid"><xsl:value-of select="@redirect_empty_uid"/><xsl:if
                 test="count(@redirect_empty_uid) = 0">false</xsl:if></xsl:attribute>
             </cfg:Source>
           </xsl:for-each>
        </xsl:for-each>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="count($colo-config/cfg:coloParams/cfg:RTB/@yandex_key) > 0"> 
      <cfg:Keys>
        <xsl:attribute name="yandex_key">
          <xsl:value-of select="$colo-config/cfg:coloParams/cfg:RTB/@yandex_key"/>
        </xsl:attribute>
      </cfg:Keys>
    </xsl:if>
  </cfg:UserBindFeConfiguration>

  <xsl:if test="count($optout-module) != 0
    or $optout-module/@enable = 'true' or $optout-module/@enable = '1'">
    <xsl:variable name="optout-log-ip">
      <xsl:choose>
        <xsl:when test="count($optout-module/cfg:logging/@log_ip) > 0"><xsl:value-of
          select="$optout-module/cfg:logging/@log_ip"/></xsl:when>
        <xsl:otherwise>false</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <cfg:OptOutFeConfiguration threads="100">
      <xsl:attribute name="log_ip"><xsl:value-of select="$optout-log-ip"/></xsl:attribute>
      <cfg:UriList>
        <cfg:Uri path="/services/OO"/>
        <cfg:Uri path="/OO"/>
      </cfg:UriList>
      <xsl:call-template name="ConvertFrontendLogger">
        <xsl:with-param name="logger-node" select="$optout-module/cfg:logging"/>
        <xsl:with-param name="default-log-level" select="$optout-module-log-level"/>
      </xsl:call-template>
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
      </cfg:OptOutFeConfiguration>
  </xsl:if>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="full-cluster-path" select="$xpath/../.."/>
  <xsl:variable name="fe-cluster-path" select="$xpath/.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor] |
            $full-cluster-path/serviceGroup[@descriptor ='AdProfilingCluster/BackendSubCluster']"/>

  <xsl:variable
    name="stats-collector-path"
    select="$be-cluster-path/service[@descriptor = $stats-collector-descriptor]"/>

  <xsl:variable
    name="channel-server-count"
    select="count($fe-cluster-path/service[@descriptor = $channel-server-descriptor] |
                  $fe-cluster-path/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelServer'])"/>

  <xsl:variable
    name="campaign-managers"
    select="$xpath/../service[@descriptor = $campaign-manager-descriptor]"/>

  <xsl:variable
    name="channel-controller-path"
    select="$fe-cluster-path/service[@descriptor = $channel-controller-descriptor] |
            $fe-cluster-path/service[@descriptor = 'AdProfilingCluster/FrontendSubCluster/ChannelController']"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find fe-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($channel-controller-path) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find channel controller service </xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$fe-cluster-path/../configuration/cfg:cluster"/>

  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="campaign-servers"
    select="$be-cluster-path/service[@descriptor = $campaign-server-descriptor] |
            $be-cluster-path/service[@descriptor = 'AdProfilingCluster/BackendSubCluster/CampaignServer']"/>

  <xsl:variable
    name="user-bind-servers"
    select="$be-cluster-path/service[@descriptor = $user-bind-server-descriptor]"/>

  <xsl:variable
    name="channel-serving-config"
    select="$fe-config/cfg:channelServing"/>

  <xsl:variable
    name="stats-collector-config"
    select="$stats-collector-path/configuration/cfg:statsCollector"/>

  <xsl:variable
    name="frontend-config" select="$xpath/configuration/cfg:frontend"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable
    name="unixcommons-install-root"
    select="$env-config/@unixcommons_root"/>

  <xsl:variable name="unixcommons-root"><xsl:value-of select="$unixcommons-install-root"/>
    <xsl:if test="count($unixcommons-install-root) = 0"><xsl:value-of select="$def-unixcommons-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> AdFrontend: Can't find colo config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:FeConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/Frontends/FeConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="AdFrontendConfigGenerator">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="server-root" select="$server-root"/>
      <xsl:with-param name="unixcommons-root" select="$unixcommons-root"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>

      <xsl:with-param name="campaign-servers" select="$campaign-servers"/>
      <xsl:with-param name="user-bind-servers" select="$user-bind-servers"/>
      <xsl:with-param name="channel-controller-path" select="$channel-controller-path"/>

      <xsl:with-param name="channel-server-count" select="$channel-server-count"/>
      <xsl:with-param name="stats-collector-path" select="$stats-collector-path"/>
      <xsl:with-param name="stats-collector" select="$stats-collector-config"/>

      <xsl:with-param name="frontend-config" select="$frontend-config"/>
    </xsl:call-template>
  </cfg:FeConfiguration>

</xsl:template>

</xsl:stylesheet>

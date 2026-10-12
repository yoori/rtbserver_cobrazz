<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="campaign-server-mode" select="$MODE"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="server-id" select="$SERVICE_ID"/>

<xsl:include href="CampaignServersGrpcRefs.xsl"/>

<xsl:template name="PrimaryCampaignServers">

  <cfg:CampaignServerGrpcRefs>
    <xsl:variable name="masters">
      <xsl:for-each select="//service[@descriptor = $campaign-server-descriptor]">

      <xsl:variable
        name="campaign-server-config"
        select="configuration/cfg:campaignServer"/>

      <xsl:variable name="is-primary-server" select="$campaign-server-config/cfg:updateParams/@primary |
        $xsd-campaign-server-update-params-type/xsd:attribute[@name='primary']/@default"/>

      <xsl:if test="string($is-primary-server) = 'true'">

        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix" select="'CampaignServers'"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:variable name="port">
          <xsl:call-template name="DbAccessGrpcPort">
            <xsl:with-param name="network" select="$campaign-server-config/cfg:networkParams"/>
            <xsl:with-param name="port">
              <xsl:choose>
                <xsl:when test="$campaign-server-config/cfg:networkParams/@port">
                  <xsl:value-of select="$campaign-server-config/cfg:networkParams/@port"/>
                </xsl:when>
                <xsl:otherwise><xsl:value-of select="$def-campaign-server-port"/></xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:variable>
        <xsl:for-each select="exsl:node-set($hosts)/host">
          <cfg:Ref endpoint="{concat(., ':', $port)}"/>
        </xsl:for-each>
      </xsl:if>

      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="count(exsl:node-set($masters)/cfg:Ref) = 0">
      <xsl:message terminate="yes"> CampaignServer(Central or Remote Mode): haven't got primary servers for slave CampaignServer. </xsl:message>
    </xsl:if>
    <xsl:copy-of select="$masters"/>

  </cfg:CampaignServerGrpcRefs>
</xsl:template>

<!-- CampaignServer config generate function (CENTRAL-OR-REMOTE-MODE, PROXY-MODE) -->
<xsl:template name="CampaignServerConfigGenerator">
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="be-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="central-colo-config"/>
  <xsl:param name="remote-colo-config"/>
  <xsl:param name="local-proxy-config"/>
  <xsl:param name="campaign-server-config"/>
  <xsl:param name="secure-files-root"/>
  <xsl:param name="server-root"/>


  <xsl:variable name="workspace-root"><xsl:value-of select="$colo-config/cfg:environment/@workspace_root[1]"/>
    <xsl:if test="count($colo-config/cfg:environment/@workspace_root[1]) = 0"><xsl:value-of
      select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$colo-config/cfg:environment/@config_root[1]"/>
    <xsl:if test="count($colo-config/cfg:environment/@config_root[1]) = 0"><xsl:value-of
      select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="campaign-server-port"><xsl:value-of select="$campaign-server-config/cfg:networkParams/@port"/>
    <xsl:if test="count($campaign-server-config/cfg:networkParams/@port) = 0">
      <xsl:choose>
        <xsl:when test="$campaign-server-mode != 'PROXY-MODE'">
          <xsl:value-of select="$def-campaign-server-port"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$def-proxy-campaign-server-port"/>
        </xsl:otherwise>
      </xsl:choose>
   </xsl:if>
  </xsl:variable>
  <exsl:document href="campaignServer.port"
    method="text" omit-xml-declaration="yes"
    >  ['campaignServer', <xsl:call-template name="DbAccessGrpcPort">
      <xsl:with-param name="network" select="$campaign-server-config/cfg:networkParams"/>
      <xsl:with-param name="port" select="$campaign-server-port"/>
    </xsl:call-template>],</exsl:document>

  <xsl:variable name="external-network-params" select="$campaign-server-config/cfg:externalNetworkParams"/>
  <xsl:variable name="global-secure-params" select="$colo-config/cfg:secureParams"/>

  <xsl:variable name="update-config" select="$campaign-server-config/cfg:updateParams"/>

  <xsl:variable name="primary-server" select="$update-config/@primary |
    $xsd-campaign-server-update-params-type/xsd:attribute[@name='primary']/@default"/>
  <xsl:variable name="config-update-period" select="$update-config/@update_period |
    $xsd-campaign-server-update-params-type/xsd:attribute[@name='update_period']/@default"/>
  <xsl:variable name="ecpm-update-period" select="$update-config/@ecpm_update_period |
    $xsd-campaign-server-update-params-type/xsd:attribute[@name='ecpm_update_period']/@default"/>

  <xsl:variable name="expiration-config" select="$campaign-server-config/cfg:expirationParams"/>

  <xsl:choose>
    <xsl:when test="count($colo-config/cfg:coloParams/@colo_id) = 0 and ($campaign-server-mode != 'PROXY-MODE' or count($remote-colo-config) > 0)">
      <xsl:message terminate="yes"> CampaignServer(Central or Remote Mode): undefined colo_id. </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>
  <xsl:variable name="country"><xsl:if
    test="count($colo-config/cfg:coloParams/@enabled_countries) > 0"><xsl:value-of
    select="translate($colo-config/cfg:coloParams/@enabled_countries,
      ',&#x9;&#xA;&#xD;', '    ')"/></xsl:if>
  </xsl:variable>

  <cfg:CampaignServer
    log_root="{concat($workspace-root, '/log/CampaignServer/Out')}"
    pid_file="{concat($workspace-root, '/run/CampaignServer.pid')}"
    config_update_period="{$config-update-period}"
    ecpm_update_period="{$ecpm-update-period}"
    server_id="{$server-id}">
    <xsl:attribute name="bill_stat_update_period"><xsl:value-of
      select="$update-config/@bill_stat_update_period |
        $xsd-campaign-server-update-params-type/xsd:attribute[@name='bill_stat_update_period']/@default"/></xsl:attribute>

    <xsl:if test="count($colo-id) > 0">
      <xsl:attribute name="colo_id"><xsl:value-of select="$colo-id"/></xsl:attribute>
      <xsl:attribute name="version"><xsl:value-of select="$app-version"/></xsl:attribute>
    </xsl:if>

    <xsl:choose>
      <xsl:when test="count($central-colo-config) > 0">
        <xsl:choose>
          <xsl:when test="count($central-colo-config/@campaign_statuses) > 0">
            <xsl:attribute name="campaign_statuses"><xsl:value-of
              select="$central-colo-config/@campaign_statuses"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="campaign_statuses">all</xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="count($central-colo-config/@channel_statuses) > 0">
            <xsl:attribute name="channel_statuses"><xsl:value-of
              select="$central-colo-config/@channel_statuses"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="channel_statuses">all</xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="count($remote-colo-config) > 0">
        <xsl:choose>
          <xsl:when test="count($remote-colo-config/@campaign_statuses) > 0">
            <xsl:attribute name="campaign_statuses"><xsl:value-of
              select="$remote-colo-config/@campaign_statuses"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="campaign_statuses">all</xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="count($remote-colo-config/@channel_statuses) > 0">
            <xsl:attribute name="channel_statuses"><xsl:value-of
              select="$remote-colo-config/@channel_statuses"/></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="channel_statuses">all</xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
    </xsl:choose>

    <xsl:if test="count($external-network-params) &gt; 0">
      <xsl:message terminate="yes">CampaignServer: external CORBA listener is retired;
        configure access to the gRPC endpoint explicitly.</xsl:message>
    </xsl:if>

    <xsl:variable name="snmp-stats-enabled">
      <xsl:if test="count($colo-config/cfg:snmpStats) > 0">
        <xsl:value-of select="string($colo-config/cfg:snmpStats/@enable)"/>
      </xsl:if>
    </xsl:variable>

    <xsl:if test="$snmp-stats-enabled = 'true'">
      <xsl:variable name="mib-root"><xsl:if
        test="count($env-config/@mib_root) = 0"><xsl:value-of
        select="$server-root"/>/mibs</xsl:if><xsl:value-of
        select="$env-config/@mib_root"/></xsl:variable>
      <xsl:variable name="mib-path" select="concat('/usr/share/snmp/mibs:',
        $mib-root, ':',
        $mib-root, '/CampaignSvcs/CampaignServer')"/>
      <cfg:SNMPConfig mib_dirs="{$mib-path}"
        index="{$colo-config/cfg:snmpStats/@oid_suffix}"/>
    </xsl:if>

    <cfg:GrpcConfig>
      <xsl:attribute name="process_threads">
        <xsl:choose>
          <xsl:when test="$campaign-server-config/cfg:threadParams/@min">
            <xsl:value-of select="$campaign-server-config/cfg:threadParams/@min"/>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="$def-campaign-server-threads"/></xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <cfg:Endpoint host="*">
        <xsl:attribute name="port">
          <xsl:call-template name="DbAccessGrpcPort">
            <xsl:with-param name="network" select="$campaign-server-config/cfg:networkParams"/>
            <xsl:with-param name="port" select="$campaign-server-port"/>
          </xsl:call-template>
        </xsl:attribute>
      </cfg:Endpoint>
    </cfg:GrpcConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$campaign-server-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $campaign-server-log-path)"/>
      <xsl:with-param name="default-log-level" select="$campaign-server-log-level"/>
    </xsl:call-template>

    <xsl:variable name="db-connection" select="$central-colo-config/cfg:pgConnection"/>

    <xsl:if test="count($db-connection) > 0 or $campaign-server-mode = 'PROXY-MODE'">
      <cfg:Logging>
        <cfg:ColoUpdateStat flush_period="30"/>
      </cfg:Logging>
    </xsl:if>

    <xsl:choose>
      <xsl:when test="$campaign-server-mode = 'CENTRAL-OR-REMOTE-MODE'">

        <xsl:choose>
          <xsl:when test="count($db-connection) > 0">
            <xsl:variable name="stat-sync-period"
              select="$update-config/@stat_sync_period |
              $xsd-campaign-server-update-params-type/xsd:attribute[@name='stat_sync_period']/@default"/>

            <cfg:ServerMode stat_stamp_sync_period="{$stat-sync-period}"
              enable_delivery_thresholds="false">
              <xsl:if test="count($expiration-config/@audience_expiration_time) > 0">
                <xsl:attribute name="audience_expiration_time">
                  <xsl:value-of select="$expiration-config/@audience_expiration_time"/>
                </xsl:attribute>
              </xsl:if>
              <xsl:if test="count($expiration-config/@pending_expire_time) > 0">
                <xsl:attribute name="pending_expire_time">
                  <xsl:value-of select="$expiration-config/@pending_expire_time"/>
                </xsl:attribute>
              </xsl:if>

              <xsl:choose>
                <xsl:when test="string($primary-server) = 'false'">
                  <xsl:call-template name="PrimaryCampaignServers"/>
                </xsl:when>
                <!-- Primary campaign server -->
                <xsl:otherwise>
                  <cfg:PGConnection
                    connection_string="{$db-connection/@connection_string}"/>
                </xsl:otherwise>
              </xsl:choose>

              <cfg:LogGeneralizerGrpcRefs>
                <xsl:for-each select="$be-cluster-path/service[@descriptor = $log-generalizer-descriptor]">

                  <xsl:variable name="port"><xsl:value-of select="configuration/cfg:logGeneralizer/cfg:networkParams/@port"/>
                    <xsl:if test="count(configuration/cfg:logGeneralizer/cfg:networkParams/@port) = 0">
                      <xsl:value-of select="$def-log-generalizer-port"/>
                    </xsl:if>
                  </xsl:variable>

                  <xsl:variable name="grpc-port">
                    <xsl:call-template name="DbAccessGrpcPort">
                      <xsl:with-param name="network"
                        select="configuration/cfg:logGeneralizer/cfg:networkParams"/>
                      <xsl:with-param name="port" select="$port"/>
                    </xsl:call-template>
                  </xsl:variable>
                  <xsl:variable name="hosts">
                    <xsl:call-template name="GetHosts">
                      <xsl:with-param name="hosts" select="@host"/>
                      <xsl:with-param name="error-prefix" select="'Resolving of LogGeneralizer'"/>
                    </xsl:call-template>
                  </xsl:variable>

                  <xsl:for-each select="exsl:node-set($hosts)/host">
                    <cfg:Ref endpoint="{concat(., ':', $grpc-port)}"/>
                  </xsl:for-each>
                </xsl:for-each>
              </cfg:LogGeneralizerGrpcRefs>
            </cfg:ServerMode>
          </xsl:when>
          <xsl:when test="count($remote-colo-config) > 0">
            <cfg:ProxyMode>
              <xsl:if test="count($country) > 0">
                <xsl:attribute name="load_country"><xsl:value-of select="$country"/></xsl:attribute>
              </xsl:if>

              <xsl:choose>
                <xsl:when test="string($primary-server) = 'false'">
                  <xsl:call-template name="PrimaryCampaignServers"/>
                </xsl:when>
                <xsl:otherwise>
                <!-- Primary campaign server -->

                <cfg:CampaignServerGrpcRefs>
                <xsl:for-each select="$remote-colo-config/cfg:campaignServerRef">
                  <xsl:variable name="campaign-server-host" select="@host"/>

                  <xsl:call-template name="CampaignServerExplicitGrpcRef"/>
                </xsl:for-each>
                </cfg:CampaignServerGrpcRefs>

                </xsl:otherwise>
              </xsl:choose>

            </cfg:ProxyMode>
          </xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes"> CampaignServer: can't determine colo type - db connection and remote section non defined. </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$campaign-server-mode = 'PROXY-MODE'">

        <cfg:ProxyMode>
          <!-- version & colo_id isn't defined for proxy host,
            -  for disable ColoUpdateStat logging
            -->
          <cfg:CampaignServerGrpcRefs>
            <xsl:variable name="campaign-server-ref" select="$local-proxy-config/cfg:campaignServerRef |
              $remote-colo-config/cfg:campaignServerRef | $colo-config/cfg:campaignServerRef"/>
            <xsl:choose>
              <xsl:when test="count($campaign-server-ref) &gt; 0">
                <xsl:for-each select="$campaign-server-ref">
                  <xsl:call-template name="CampaignServerExplicitGrpcRef"/>
                </xsl:for-each>
              </xsl:when>
              <xsl:otherwise>
                <xsl:variable name="refs">
                  <xsl:call-template name="CampaignServerGrpcRefs">
                    <xsl:with-param name="campaign-servers"
                      select="//service[@descriptor = $campaign-server-descriptor]"/>
                    <xsl:with-param name="service-name" select="'CampaignServer proxy'"/>
                  </xsl:call-template>
                </xsl:variable>
                <xsl:copy-of select="exsl:node-set($refs)/cfg:CampaignServerGrpcRefs/cfg:Ref"/>
              </xsl:otherwise>
            </xsl:choose>
          </cfg:CampaignServerGrpcRefs>
        </cfg:ProxyMode>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"> CampaignServer: unknown campaign server mode '<xsl:value-of select="$campaign-server-mode"/>'. </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </cfg:CampaignServer>

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.. | $xpath/.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="campaign-server-path"
    select="$xpath"/>

  <xsl:variable
    name="local-proxy-path"
    select="$xpath/../service[@descriptor = $local-proxy-descriptor]"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find full-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-server-path) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find campaign server node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="central-colo-config"
    select="$full-cluster-path/configuration/cfg:cluster/cfg:central"/>

  <xsl:variable
    name="remote-colo-config"
    select="$colo-config/cfg:remote"/>

  <xsl:variable
    name="local-proxy-config"
    select="$local-proxy-path/configuration/cfg:localProxy"/>

  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable name="campaign-server-config"
     select="$campaign-server-path/configuration/cfg:campaignServer"/>

  <xsl:variable
    name="server-install-root"
    select="$be-config/cfg:environment/@server_root | $colo-config/cfg:environment/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="secure-files-root" select="concat('/', $out-dir, '/cert/')"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find colo config </xsl:message>
    </xsl:when>

    <xsl:when test="count($campaign-server-config) = 0">
       <xsl:message terminate="yes"> CampaignServer: Can't find campaign server config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
     $server-root, '/xsd/CampaignSvcs/CampaignServerConfig.xsd')}">
    <xsl:call-template name="CampaignServerConfigGenerator">
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="be-config" select="$be-config"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="central-colo-config" select="$central-colo-config"/>
      <xsl:with-param name="remote-colo-config" select="$remote-colo-config"/>
      <xsl:with-param name="local-proxy-config" select="$local-proxy-config"/>
      <xsl:with-param name="campaign-server-config" select="$campaign-server-config"/>
      <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
      <xsl:with-param name="server-root" select="$server-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

<xsl:template name="CampaignServerExplicitGrpcRef">
  <xsl:if test="cfg:secureParams">
    <xsl:message terminate="yes">CampaignServer: CORBA secureParams cannot be used for gRPC;
      configure a protected gRPC endpoint explicitly.</xsl:message>
  </xsl:if>
  <xsl:variable name="port">
    <xsl:call-template name="DbAccessGrpcPort">
      <xsl:with-param name="network" select="."/>
      <xsl:with-param name="port" select="@port"/>
    </xsl:call-template>
  </xsl:variable>
  <cfg:Ref endpoint="{concat(@host, ':', $port)}"/>
</xsl:template>

</xsl:stylesheet>

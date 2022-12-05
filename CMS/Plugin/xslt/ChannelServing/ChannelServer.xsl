<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:include href="../Functions.xsl"/>

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="FillServicesRefs">
  <xsl:param name="services"/>
  <xsl:param name="def-port"/>
  <xsl:param name="error-prefix"/>

  <xsl:for-each select="$services">

    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="$error-prefix"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="port"><xsl:value-of select=".//cfg:networkParams/@port"/>
      <xsl:if test="count(.//cfg:networkParams/@port) = 0"><xsl:value-of select="$def-port"/></xsl:if>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <cfg:Ref>
        <xsl:attribute name="ref">
          <xsl:value-of
            select="concat('corbaloc:iiop:', ., ':', $port, '/DictionaryProvider')"/>
        </xsl:attribute>
      </cfg:Ref>
    </xsl:for-each>

  </xsl:for-each>
</xsl:template>

<!-- ChannelController config generate function -->
<xsl:template name="ChannelServerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="channel-server-config"/>
  <xsl:param name="be-cluster-path"/>
  <xsl:param name="fe-cluster-path"/>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="channel-server-port">
      <xsl:value-of select="$channel-server-config/cfg:networkParams/@port"/>
      <xsl:if test="count($channel-server-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-channel-server-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="channelServer.port"
      method="text" omit-xml-declaration="yes"
      >  ['channelServer', <xsl:copy-of select="$channel-server-port"/>],</exsl:document>

    <xsl:variable name="update-config" select="$channel-server-config/cfg:updateParams"/>
    <xsl:variable name="update-channels-period"><xsl:value-of select="$update-config/@period"/>
      <xsl:if test="count($update-config/@period) = 0">
        <xsl:value-of select="$channel-server-update-channels-period"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="update-logging" select="$update-config/@log_update"/>

    <xsl:variable name="update-chunks-count"><xsl:value-of select="$update-config/@chunks_count"/>
      <xsl:if test="count($update-config/@chunks_count) = 0">
        <xsl:value-of select="$channel-server-update-chunks-count"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="memory-size"><xsl:value-of select="$update-config/@memory_size"/>
      <xsl:if test="count($update-config/@memory_size) = 0">
        <xsl:value-of select="$channel-server-update-memory-size"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="have-channel-search">
      <xsl:value-of select="count($fe-cluster-path/service[@descriptor = $channel-search-service-descriptor]) > 0"/>
    </xsl:variable>

    <!-- start config generation -->
  <cfg:ChannelServerConfig
    count_chunks="{$update-chunks-count}"
    merge_size="{$memory-size}"
    update_period="{$update-channels-period}"
    log_root="{concat($workspace-root, '/log/ChannelServer/Out')}"
    service_index="{$SERVICE_ID}">

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$channel-server-config/cfg:threadParams/@min"/>
        <xsl:if test="count($channel-server-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-channel-server-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$channel-server-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="ChannelServerControl" name="ChannelServerControl"/>
        <cfg:Object servant="ChannelServer" name="ChannelServer"/>
        <cfg:Object servant="ChannelUpdate" name="ChannelUpdate"/>
        <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:choose>
      <xsl:when test="$channel-server-config/cfg:logging/@log_level > 7">
        <cfg:MatchOptions>
            <xsl:attribute name="match_logger_file">
            <xsl:value-of select="$workspace-root"/>/log/ChannelServer/MatchLogger.trace</xsl:attribute>
            <xsl:attribute name="nonstrict"><xsl:value-of select="$have-channel-search"/></xsl:attribute>
          <cfg:AllowPort>80</cfg:AllowPort>
        </cfg:MatchOptions>
      </xsl:when>
      <xsl:otherwise>
        <cfg:MatchOptions>
            <xsl:attribute name="nonstrict"><xsl:value-of select="$have-channel-search"/></xsl:attribute>
          <cfg:AllowPort>80</cfg:AllowPort>
        </cfg:MatchOptions>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:if test="count($be-cluster-path/service[@descriptor = $dictionary-provider-descriptor or
                                                 @descriptor = 'AdProfilingCluster/BackendSubCluster/DictionaryProvider']) > 0">
      <cfg:DictionaryRefs name="DictionaryProvider">
        <xsl:call-template name="FillServicesRefs">
          <xsl:with-param name="services" select="$be-cluster-path/service[@descriptor = $dictionary-provider-descriptor or
                                                                           @descriptor = 'AdProfilingCluster/BackendSubCluster/DictionaryProvider']"/>
          <xsl:with-param name="def-port" select="$def-dictionary-provider-port"/>
          <xsl:with-param name="error-prefix" select="'ChannelServer'" />
        </xsl:call-template>
      </cfg:DictionaryRefs>
    </xsl:if>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$channel-server-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $channel-server-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-server-log-level"/>
    </xsl:call-template>

    <xsl:if test="count($colo-config/cfg:central) > 0">
      <cfg:UpdateStatLogger>
        <xsl:attribute name="size">3</xsl:attribute>
        <xsl:attribute name="period">30</xsl:attribute>
        <xsl:attribute name="path">ColoUpdateStat</xsl:attribute>
      </cfg:UpdateStatLogger>
    </xsl:if>

    <xsl:variable name="matching-segmentor">
      <xsl:value-of select="$colo-config/cfg:coloParams/@segmentor"/><xsl:if
      test="string-length($colo-config/cfg:coloParams/@segmentor) = 0"><xsl:value-of
      select='None'/></xsl:if>
    </xsl:variable>
    <xsl:if test="count($colo-config/cfg:central/cfg:segmentorMapping) > 0 or $matching-segmentor != 'None'">
      <xsl:comment>Comment the lines below to disable support for Asian languages</xsl:comment>
      <cfg:Segmentors>
        <xsl:if test="$matching-segmentor != 'None'">
          <xsl:attribute name="matching_segmentor">
            <xsl:value-of select="$matching-segmentor"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="$matching-segmentor = 'Polyglot' and 
             count($colo-config/cfg:central/cfg:segmentorMapping/cfg:country[@segmentor = 'Polyglot'
               and @name='']) = 0">
            <cfg:Segmentor name="Polyglot" country='' base="/opt/foros/polyglot/dict/"/>
          </xsl:when>
          <xsl:when test="$matching-segmentor = 'Nlpir' and 
             count($colo-config/cfg:central/cfg:segmentorMapping/cfg:country[@segmentor = 'Nlpir'
               and @name='']) = 0">
            <cfg:Segmentor name="Nlpir" country='' base="/usr/share/NLPIR"/>
          </xsl:when>
        </xsl:choose>
        <xsl:for-each select="$colo-config/cfg:central/cfg:segmentorMapping/cfg:country">
          <xsl:choose>
            <xsl:when test="./@segmentor = 'Polyglot'">
              <cfg:Segmentor name="Polyglot" base="/opt/foros/polyglot/dict/"
                country="{./@name}"/>
            </xsl:when>
            <xsl:when test="./@segmentor = 'Nlpir'">
              <cfg:Segmentor name="Nlpir" base="/usr/share/NLPIR"
                country="{./@name}"/>
            </xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </cfg:Segmentors>
    </xsl:if>
  </cfg:ChannelServerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable
    name="fe-cluster-path"
    select="$xpath/.."/>

  <xsl:variable
    name="full-cluster-path"
    select="$fe-cluster-path/.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor] |
            $full-cluster-path/serviceGroup[@descriptor ='AdProfilingCluster/BackendSubCluster']"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> ChannelServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelServer: Can't find fe-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> ChannelServer: Can't find be-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="channel-serving-root-config"
    select="$fe-config/cfg:channelServer"/>

  <xsl:variable
    name="channel-server-config"
    select="$xpath/configuration/cfg:channelServer  | $channel-serving-root-config"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>

    <xsl:when test="count($channel-server-config) = 0">
       <xsl:message terminate="yes"> ChannelServer: Can't find channel server config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/ChannelServerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="ChannelServerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="channel-server-config" select="$channel-server-config"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

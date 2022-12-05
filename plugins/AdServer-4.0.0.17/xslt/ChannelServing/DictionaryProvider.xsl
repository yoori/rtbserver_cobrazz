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
  exclude-result-prefixes="dyn">

<xsl:include href="../Functions.xsl"/>

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- ChannelController config generate function -->
<xsl:template name="DictionaryProviderConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="service-config"/>

  <cfg:DictionaryProviderConfig>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>


    <xsl:variable name="service-port">
      <xsl:value-of select="$service-config/cfg:networkParams/@port"/>
      <xsl:if test="count($service-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-dictionary-provider-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="dictionaryProvider.port"
      method="text" omit-xml-declaration="yes"
      >  ['dictionaryProvider', <xsl:copy-of select="$service-port"/>],</exsl:document>

    <xsl:variable name="threads" select="$service-config/@threads"/>

    <!-- start config generation -->
    <xsl:attribute name="threads"><xsl:value-of select="$threads"/></xsl:attribute>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$service-config/cfg:threadParams/@min"/>
        <xsl:if test="count($service-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-dictionary-provider-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$service-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="DictionaryProvider" name="DictionaryProvider"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <cfg:Dictionary lang="en" file="/opt/foros/dictionaries/lexemes.en"/>
    <cfg:Dictionary lang="de" file="/opt/foros/dictionaries/lexemes.de"/>
    <cfg:Dictionary lang="es" file="/opt/foros/dictionaries/lexemes.es"/>
    <cfg:Dictionary lang="it" file="/opt/foros/dictionaries/lexemes.it"/>
    <cfg:Dictionary lang="ja" file="/opt/foros/dictionaries/lexemes.ja"/>
    <cfg:Dictionary lang="ko" file="/opt/foros/dictionaries/lexemes.ko"/>
    <cfg:Dictionary lang="pt" file="/opt/foros/dictionaries/lexemes.pt"/>
    <cfg:Dictionary lang="ro" file="/opt/foros/dictionaries/lexemes.ro"/>
    <cfg:Dictionary lang="ru" file="/opt/foros/dictionaries/lexemes.ru"/>
    <cfg:Dictionary lang="tr" file="/opt/foros/dictionaries/lexemes.tr"/>
    <cfg:Dictionary lang="zh" file="/opt/foros/dictionaries/lexemes.zh"/>
    <cfg:Dictionary lang="en" file="/opt/foros/dictionaries/lexemes-ext.en"/>
    <cfg:Dictionary lang="de" file="/opt/foros/dictionaries/lexemes-ext.de"/>
    <cfg:Dictionary lang="pt" file="/opt/foros/dictionaries/lexemes-ext.pt"/>
    <cfg:Dictionary lang="ru" file="/opt/foros/dictionaries/lexemes-ext.ru"/>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$service-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $dictionary-provider-log-path)"/>
      <xsl:with-param name="default-log-level" select="$channel-server-log-level"/>
    </xsl:call-template>

  </cfg:DictionaryProviderConfig>

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
    name="dictionary-provider-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> DictionaryProvider: Can't find XPATH element </xsl:message>
    </xsl:when>
    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> DictionaryProvider: Can't find be-cluster group </xsl:message>
    </xsl:when>
    <xsl:when test="count($dictionary-provider-path) = 0">
       <xsl:message terminate="yes"> DictionaryProvider: Can't find dictionary provider node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable name="service-config"
     select="$dictionary-provider-path/configuration/cfg:dictionaryProvider"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>

    <xsl:when test="count($service-config) = 0">
       <xsl:message terminate="yes"> DictionaryProvider: Can't find dictionary provider config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/ChannelSvcs/DictionaryProviderConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="DictionaryProviderConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="service-config" select="$service-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

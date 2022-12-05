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
  exclude-result-prefixes="exsl dyn"
  xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration ../AdServer/UserInfoSvcs/UserInfoManagerControllerConfig.xsd">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<!-- UserInfoExchanger config generate function -->
<xsl:template name="UserInfoExchangerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="pbe-config"/>
  <xsl:param name="user-info-exchanger-path"/>
  <xsl:param name="user-info-exchanger-config"/>
  <xsl:param name="secure-files-root"/>

  <cfg:UserInfoExchangerConfig>
    <xsl:variable name="colo-config" select="$pbe-config"/>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root[1]"/>
      <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
      <xsl:if test="count($env-config/@cache_root) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="user-info-exchanger-port">
      <xsl:value-of select="$user-info-exchanger-config/cfg:networkParams/@port"/>
      <xsl:if test="count($user-info-exchanger-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-user-info-exchanger-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="userInfoExchanger.port"
      method="text" omit-xml-declaration="yes"
      >  ['userInfoExchanger', <xsl:copy-of select="$user-info-exchanger-port"/>],</exsl:document>

    <xsl:variable name="external-network-params" select="$user-info-exchanger-config/cfg:externalNetworkParams"/>
    <xsl:variable name="global-secure-params" select="$pbe-config/cfg:secureParams"/>

    <xsl:variable name="chunks-root" select="concat($cache-root, '/UsersExchange')"/>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$user-info-exchanger-config/cfg:threadParams/@min"/>
        <xsl:if test="count($user-info-exchanger-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-user-info-exchanger-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$user-info-exchanger-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="UserInfoExchanger" name="UserInfoExchanger"/>
      </cfg:Endpoint>
      <xsl:if test="count($external-network-params) > 0">
        <xsl:variable name="external-host" select="$external-network-params/@host"/>

        <cfg:Endpoint>
          <xsl:attribute name="host"><xsl:value-of select="$external-host"/></xsl:attribute>
          <xsl:attribute name="port"><xsl:value-of select="$external-network-params/@port"/></xsl:attribute>

          <xsl:call-template name="ConvertSecureParams">
            <xsl:with-param name="secure-node" select="$external-network-params/@secure"/>
            <xsl:with-param name="global-secure-node" select="$global-secure-params"/>
            <xsl:with-param name="config-root" select="$config-root"/>
            <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
          </xsl:call-template>
          <cfg:Object servant="UserInfoExchanger" name="UserInfoExchanger"/>
        </cfg:Endpoint>
      </xsl:if>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$user-info-exchanger-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $user-info-exchanger-log-path)"/>
      <xsl:with-param name="default-log-level" select="$user-info-exchanger-log-level"/>
    </xsl:call-template>

    <cfg:RepositoryPlace>
      <xsl:attribute name="path"><xsl:value-of select="$chunks-root"/></xsl:attribute>
    </cfg:RepositoryPlace>
  </cfg:UserInfoExchangerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="user-info-exchanger-path"
    select="$xpath"/>

  <xsl:variable
    name="pbe-cluster-path"
    select="$xpath/.."/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($pbe-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($pbe-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find pbe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="pbe-config"
    select="$pbe-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="user-info-exchanger-config"
    select="$pbe-config/cfg:userInfoExchanger | $user-info-exchanger-path/configuration/cfg:userInfoExchanger"/>

  <xsl:variable
    name="env-config"
    select="$pbe-config/cfg:environment"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root[1]"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="secure-files-root" select="concat('/', $out-dir, '/cert/')"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($pbe-config) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find pbe config </xsl:message>
    </xsl:when>

    <xsl:when test="count($user-info-exchanger-config) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find userInfoExchanger config </xsl:message>
    </xsl:when>

    <xsl:when test="count($pbe-config) = 0">
       <xsl:message terminate="yes"> UserInfoExchanger: Can't find pbe config </xsl:message>
    </xsl:when>

  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/UserInfoSvcs/UserInfoExchangerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="UserInfoExchangerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="pbe-config" select="$pbe-config"/>
      <xsl:with-param name="user-info-exchanger-path" select="$user-info-exchanger-path"/>
      <xsl:with-param name="user-info-exchanger-config" select="$user-info-exchanger-config"/>
      <xsl:with-param name="secure-files-root" select="$secure-files-root"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

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

<!-- UserOperationGenerator config generate function -->
<xsl:template name="UserOperationGeneratorGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="user-operation-generator-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="be-cluster-path"/>

  <cfg:UserOperationGeneratorConfig
    work_threads="5"
    load_command="/usr/bin/rsync --partial -avz -t --timeout=55 --log-format=%f --delete-after --include='channel*' --exclude='*' ##IN_PATH## ##TEMP_PATH##">

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="in-snapshot-path" select="concat($workspace-root, '/log/UserOperationGenerator/In/Snapshot/')"/>
    <xsl:variable name="temp-snapshot-path" select="concat($workspace-root, '/log/UserOperationGenerator/Temp/')"/>
    <xsl:variable name="snapshot-path" select="concat($workspace-root, '/log/UserOperationGenerator/Snapshot/')"/>

    <xsl:attribute name="in_snapshot_path"><xsl:value-of select="$in-snapshot-path"/></xsl:attribute>
    <xsl:attribute name="temp_snapshot_path"><xsl:value-of select="$temp-snapshot-path"/></xsl:attribute>
    <xsl:attribute name="snapshot_path"><xsl:value-of select="$snapshot-path"/></xsl:attribute>
    <xsl:attribute name="load_period"><xsl:value-of select="$user-operation-generator-config/@load_period"/></xsl:attribute>
    <xsl:attribute name="refresh_period"><xsl:value-of select="$user-operation-generator-config/@refresh_period"/></xsl:attribute>

    <xsl:variable name="user-operation-generator-port">
      <xsl:value-of select="$user-operation-generator-config/cfg:networkParams/@port"/>
      <xsl:if test="count($user-operation-generator-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-user-operation-generator-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="user-operation-generator-logging" select="$user-operation-generator-config/cfg:logging"/>
    <xsl:variable name="user-operation-generator-log-level"><xsl:value-of select="$user-operation-generator-config/cfg:logging/@log_level"/>
      <xsl:if test="count($user-operation-generator-logging/@log_level) = 0">
        <xsl:value-of select="$default-log-level"/>
      </xsl:if>
    </xsl:variable>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$user-operation-generator-config/cfg:threadParams/@min"/>
        <xsl:if test="count($user-operation-generator-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="1"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$user-operation-generator-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$user-operation-generator-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $user-operation-generator-log-path)"/>
      <xsl:with-param name="default-log-level" select="$user-operation-generator-log-level"/>
    </xsl:call-template>

    <xsl:variable name="out-logs-dir" select="concat($workspace-root, '/log/UserOperationGenerator/Out/')"/>

    <xsl:variable name="common-chunks-number"><xsl:value-of select="$colo-config/cfg:userProfiling/@chunks_count"/>
      <xsl:if test="count($colo-config/cfg:userProfiling/@chunks_count) = 0">
        <xsl:value-of select="$user-info-manager-scale-chunks"/>
      </xsl:if>
    </xsl:variable>

    <cfg:OutLogs log_root="{$out-logs-dir}" common_chunks_number="{$common-chunks-number}">
      <cfg:UserOp period="30"/>
    </cfg:OutLogs>
  </cfg:UserOperationGeneratorConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="user-operation-generator-path"
    select="$xpath"/>

  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UserOperationGenerator: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> UserOperationGenerator: Can't find full cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="be-config"
    select="$be-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="fe-config"
    select="$fe-cluster-path/configuration/cfg:backendCluster"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="user-operation-generator-config"
    select="$user-operation-generator-path/configuration/cfg:userOperationGenerator"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> UserOperationGenerator: Can't find colo config config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/UserInfoSvcs/UserOperationGeneratorConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="UserOperationGeneratorGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="user-operation-generator-config" select="$user-operation-generator-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="be-cluster-path" select="$be-cluster-path"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

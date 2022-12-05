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
  xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration ../AdServer/UserInfoSvcs/UserBindServerConfig.xsd">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>
<xsl:include href="../UserIdBlackList.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="cluster-id" select="$CLUSTER_ID"/>

<!-- UserBindServer config generate function -->
<xsl:template name="UserBindServerConfigGenerator">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="user-bind-server-config"/>

  <cfg:UserBindServerConfig partition_index="0" partitions_number="1">
    <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root[1]"/>
      <xsl:if test="count($env-config) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="user-bind-server-port">
      <xsl:value-of select="$user-bind-server-config/cfg:networkParams/@port"/>
      <xsl:if test="count($user-bind-server-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-user-bind-server-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="userBindServer.port"
      method="text" omit-xml-declaration="yes"
      >  ['userBindServer', <xsl:copy-of select="$user-bind-server-port"/>],</exsl:document>

    <xsl:variable name="root-dir" select="concat($workspace-root, '/log/UserBindServer/Out/')"/>

    <xsl:variable name="update-config" select="$user-bind-server-config/cfg:updateParams"/>

    <xsl:attribute name="min_age">
      <xsl:if test="count($colo-config/cfg:coloParams/@min_RTB_user_age) = 0">14400</xsl:if><xsl:value-of
        select="$colo-config/cfg:coloParams/@min_RTB_user_age"/>
    </xsl:attribute>

    <xsl:attribute name="bind_on_min_age">
      <xsl:if test="count($colo-config/cfg:coloParams/@RTB_offline_UID_generation) = 0">false</xsl:if><xsl:value-of
        select="$colo-config/cfg:coloParams/@RTB_offline_UID_generation"/>
    </xsl:attribute>

    <xsl:attribute name="max_bad_event">
      <xsl:if test="count($colo-config/cfg:coloParams/@max_bad_SSPID_events) = 0">1</xsl:if><xsl:value-of
        select="$colo-config/cfg:coloParams/@max_bad_SSPID_events"/>
    </xsl:attribute>

    <!-- start config generation -->
    <!-- check that defined all needed parameters -->
    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$user-bind-server-config/cfg:threadParams/@min"/>
        <xsl:if test="count($user-bind-server-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-user-bind-server-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$user-bind-server-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="UserBindServer" name="{$current-user-bind-server-obj}"/>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$user-bind-server-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $user-bind-server-log-path)"/>
      <xsl:with-param name="default-log-level" select="$user-bind-server-log-level"/>
    </xsl:call-template>

    <xsl:variable name="chunks-root">
      <xsl:value-of select="concat($cache-root, '/UserBind', $cluster-id, '/')"/>
    </xsl:variable>

    <xsl:variable name="chunks-count">
      <xsl:value-of select="$colo-config/cfg:userProfiling/@bind_chunks_count"/>
      <xsl:if test="count($colo-config/cfg:userProfiling/@bind_chunks_count)
        = 0">
        <xsl:value-of select="$user-bind-server-scale-chunks"/>
      </xsl:if>
    </xsl:variable>

    <cfg:Storage portions="1024"
      chunks_root="{$chunks-root}"
      common_chunks_number="{$chunks-count}"
      prefix="UserSeen"
      bound_prefix="UserBind"
      dump_period="{$user-bind-server-config/@user_dump_period}"
      expire_time="{$user-bind-server-config/@user_seen_expire_time}"
      bound_expire_time="{$user-bind-server-config/@user_expire_time}">
      <xsl:attribute name="user_bind_keep_mode">
        <xsl:value-of select="$colo-config/cfg:userProfiling/@user_bind_keep_mode"/>
        <xsl:if test="count($colo-config/cfg:userProfiling/@user_bind_keep_mode)
          = 0">keep slave</xsl:if>
      </xsl:attribute>
    </cfg:Storage>

    <cfg:BindRequestStorage portions="1024"
      chunks_root="{$chunks-root}"
      common_chunks_number="{$chunks-count}"
      prefix="BindRequest"
      expire_time="{$user-bind-server-config/@bind_request_expire_time}"/>

    <xsl:if test="count($full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]) > 1">
      <cfg:OperationBackup file_prefix="UserBindOp" rotate_period="60">
        <xsl:attribute name="dir"><xsl:value-of
          select="concat($workspace-root, '/log/UserBindServer/Out/UserBindOp_', $cluster-id mod 2 + 1)"/></xsl:attribute>
      </cfg:OperationBackup>
    </xsl:if>

    <cfg:OperationLoad file_prefix="UserBindOp" check_period="60" threads="10">
      <xsl:attribute name="dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserBindServer/In/UserBindOp_', $cluster-id)"/></xsl:attribute>
      <xsl:attribute name="unprocessed_dir"><xsl:value-of
        select="concat($workspace-root, '/log/UserBindServer/Out/UserBindOp_', $cluster-id)"/></xsl:attribute>
    </cfg:OperationLoad>
    <xsl:call-template name="FillUserIdBlackList">
      <xsl:with-param name="desc" select="$full-cluster-path/../@description"/>
    </xsl:call-template>
  </cfg:UserBindServerConfig>

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="user-bind-server-path"
    select="$xpath"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UserBindServer: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> UserBindServer: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> UserBindServer: Can't find be-cluster group </xsl:message>
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
    name="user-bind-server-config"
    select="$user-bind-server-path/configuration/cfg:userBindServer"/>

  <xsl:variable
    name="env-config"
    select="$be-config/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="server-install-root"
    select="$env-config/@server_root[1]"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/UserInfoSvcs/UserBindServerConfig.xsd')}">
    <xsl:call-template name="UserBindServerConfigGenerator">
      <xsl:with-param name="full-cluster-path" select="$full-cluster-path"/>
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="user-bind-server-config" select="$user-bind-server-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

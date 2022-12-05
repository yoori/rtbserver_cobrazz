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

<!-- UserInfoManagerController config generate function -->
<xsl:template name="UserInfoManagerControllerConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="user-info-manager-controller-config"/>

  <cfg:UserInfoManagerControllerConfig>
    <xsl:variable name="colo-id" select="$colo-config/cfg:coloParams/@colo_id"/>

    <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
      <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
    </xsl:variable>

    <xsl:variable name="user-info-manager-controller-port">
      <xsl:value-of select="$user-info-manager-controller-config/cfg:networkParams/@port"/>
      <xsl:if test="count($user-info-manager-controller-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$def-user-info-manager-controller-port"/>
      </xsl:if>
    </xsl:variable>

    <exsl:document href="userInfoManagerController.port"
      method="text" omit-xml-declaration="yes"
      >  ['userInfoManagerController', <xsl:copy-of select="$user-info-manager-controller-port"/>],</exsl:document>

    <xsl:variable name="root-dir" select="concat($workspace-root, '/log/UserInfoManagerController/Out/')"/>

    <xsl:variable name="status-check-period"><xsl:value-of select="$user-info-manager-controller-config/cfg:controlParams/@status_check_period"/>
      <xsl:if test="count($user-info-manager-controller-config/cfg:controlParams/@status_check_period) = 0">
        <xsl:value-of select="$def-status-check-period"/>
      </xsl:if>
    </xsl:variable>

    <!-- start config generation -->
    <xsl:attribute name="colo_id"><xsl:value-of select="$colo-id"/></xsl:attribute>
    <xsl:attribute name="status_check_period"><xsl:value-of select="$status-check-period"/></xsl:attribute>

    <cfg:CorbaConfig>
      <xsl:attribute name="threading-pool"><xsl:value-of select="$user-info-manager-controller-config/cfg:threadParams/@min"/>
        <xsl:if test="count($user-info-manager-controller-config/cfg:threadParams/@min) = 0">
          <xsl:value-of select="$def-user-info-manager-controller-threads"/>
        </xsl:if>
      </xsl:attribute>

      <cfg:Endpoint host="*">
        <xsl:attribute name="port"><xsl:value-of select="$user-info-manager-controller-port"/></xsl:attribute>
        <cfg:Object servant="ProcessControl" name="ProcessControl"/>
        <cfg:Object servant="UserInfoManagerController" name="UserInfoManagerController"/>
        <cfg:Object servant="UserInfoClusterControl" name="UserInfoClusterControl"/>
        <cfg:Object servant="UserInfoClusterStats" name="UserInfoClusterStats"/>
        <cfg:Object servant="UserInfoManagerController">
          <xsl:attribute name="name"><xsl:value-of select="$current-user-info-manager-controller-obj"/></xsl:attribute>
        </cfg:Object>
      </cfg:Endpoint>
    </cfg:CorbaConfig>

    <xsl:call-template name="ConvertLogger">
      <xsl:with-param name="logger-node" select="$user-info-manager-controller-config/cfg:logging"/>
      <xsl:with-param name="log-file" select="concat($workspace-root, $user-info-manager-controller-log-path)"/>
      <xsl:with-param name="default-log-level" select="$user-info-manager-controller-log-level"/>
    </xsl:call-template>

    <xsl:for-each select="$fe-cluster-path/service[@descriptor = $user-info-manager-descriptor]">
      <xsl:variable name="user-info-manager-config" select="configuration/cfg:userInfoManager"/>

      <xsl:variable name="user-info-manager-port">
        <xsl:value-of select="$user-info-manager-config/cfg:networkParams/@port"/>
        <xsl:if test="count($user-info-manager-config/cfg:networkParams/@port) = 0">
          <xsl:value-of select="$def-user-info-manager-port"/>
        </xsl:if>
      </xsl:variable>

      <xsl:variable name="user-info-manager-hosts">
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="@host"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:for-each select="exsl:node-set($user-info-manager-hosts)//host">
        <xsl:variable
          name="corba_ref_prefix"
          select="concat('corbaloc:iiop:', ., ':', $user-info-manager-port)"/>
        <cfg:UserInfoManagerHost>
          <cfg:UserInfoManagerRef>
            <xsl:attribute name="name">
              <xsl:value-of select="."/>
            </xsl:attribute>
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba_ref_prefix, '/', $current-user-info-manager-obj)"/>
            </xsl:attribute>
          </cfg:UserInfoManagerRef>
          <cfg:UserInfoManagerControlRef name="UserInfoManagerControl">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba_ref_prefix, '/UserInfoManagerControl')"/>
            </xsl:attribute>
          </cfg:UserInfoManagerControlRef>
          <cfg:UserInfoManagerStatsRef name="UserInfoManagerStats">
            <xsl:attribute name="ref">
              <xsl:value-of select="concat($corba_ref_prefix, '/UserInfoManagerStats')"/>
            </xsl:attribute>
          </cfg:UserInfoManagerStatsRef>
        </cfg:UserInfoManagerHost>
      </xsl:for-each> <!-- hosts loop -->
    </xsl:for-each> <!-- user info managers loop -->
  </cfg:UserInfoManagerControllerConfig>

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
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> UserInfoManagerController: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoManagerController: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoManagerController: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> UserInfoManagerController: Can't find fe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="user-info-manager-controller-config"
    select="$xpath/configuration/cfg:userInfoManagerController"/>

  <xsl:variable name="server-install-root" select="$env-config/@server_root"/>

  <xsl:variable name="server-root"><xsl:value-of select="$server-install-root"/>
    <xsl:if test="count($server-install-root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration>
    <xsl:attribute name="xsi:schemaLocation"><xsl:value-of select="concat('http://www.adintelligence.net/xsd/AdServer/Configuration ', $server-root, '/xsd/UserInfoSvcs/UserInfoManagerControllerConfig.xsd')"/></xsl:attribute>
    <xsl:call-template name="UserInfoManagerControllerConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="colo-config" select="$colo-config"/>
      <xsl:with-param name="fe-cluster-path" select="$fe-cluster-path"/>
      <xsl:with-param name="user-info-manager-controller-config" select="$user-info-manager-controller-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="exsl">

<xsl:key name="descriptor" match="@descriptor" use="."/>
<xsl:key name="host" match="/host" use="."/>

<!--
  GetAttrDefTemplate
  GetAttr
  GetAutoRestartFolder
  GetAutoRestart
  GetUserName
  PrivateKeyFile
  GetZenOSSEnabled
  ZenossInstallRoot
  ZenossFolder
  ResolveHostName
  ConvertLogger
  ConvertSecureParams
  ConvertDBConnection
  NormalizeName
  GetHosts
  GetUniqueHosts
  GetVirtualServers
  GetLowestHost
  GetServiceRefs
  FillCorbaRef
  AddStatsDumper
  AddUserInfoManagerControllerGroups
  substring-before-last
  substring-after-last
-->

<xsl:include href="Variables.xsl"/>

<!-- Get attribute value consider "attribute value templates"
  (variables like Text${var_name}text ) in xsd default values.
  @param value XPath to attribute that value will be resolved
  @param app-env XPath to @var_name, its value will get as value of
    template variables ${var_name}
  @param xsd-app-env-type Reference to obtain defaults for $app-env from XSD
  -->
<xsl:template name="GetAttrDefTemplate">
  <xsl:param name="value" />
  <xsl:param name="app-env"/>
  <xsl:param name="xsd-app-env-type"/>
  <xsl:choose>
    <xsl:when test="contains($value, '${')">
      <xsl:value-of select="substring-before($value, '${')" />
      <xsl:variable name="name"
        select="substring-before(substring-after($value, '${'), '}')" />

      <xsl:variable name="app-env-var-value"
        select="$app-env/@*[name() = $name]"/>
      <xsl:variable name="app-env-var-default"
        select="$xsd-app-env-type//xsd:attribute[@name = $name]/@default"/>
      <xsl:variable name="app-env-var">
        <xsl:choose>
          <xsl:when test="count($app-env-var-value) != 0"><xsl:value-of
            select="$app-env-var-value"/></xsl:when>
          <xsl:when test="count($app-env-var-default) != 0"><xsl:value-of
            select="$app-env-var-default"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <!-- Evaluate "Attribute template value" by name -->
      <xsl:variable name="atv-value">
        <xsl:if test="count($app-env-var-value) != 0
          or count($app-env-var-default) != 0"><xsl:value-of
            select="$app-env-var"/>
        </xsl:if>
      </xsl:variable>
      <xsl:choose>
        <!-- Print resolved variable value -->
        <xsl:when test="count($app-env-var-value) != 0 or
          count($app-env-var-default) != 0">
          <xsl:call-template name="GetAttrDefTemplate">
            <xsl:with-param name="value" select="$atv-value" />
            <xsl:with-param name="app-env" select="$app-env"/>
          </xsl:call-template>
        </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('${', $name, '}')" />
        <!--xsl:message terminate="yes">Error: unknown variable '<xsl:value-of
            select="$name"/>'</xsl:message-->
        </xsl:otherwise>
      </xsl:choose>
      <!-- Parse rest of string after '}' -->
      <xsl:call-template name="GetAttrDefTemplate">
        <xsl:with-param  name="value"
          select="substring-after(substring-after($value, '${'), '}')" />
        <xsl:with-param name="app-env" select="$app-env"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$value" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="GetAttr">
  <xsl:param name="node"/>
  <xsl:param name="name"/>
  <xsl:param name="type"/>
  <xsl:variable name="attr" select="$type//xsd:attribute[@name = $name]"/>
  <xsl:if test="count($attr) != 1">
    <xsl:message terminate="yes">Error: xsd:attribute '<xsl:value-of select="concat($type/@name,'/@',$name)"/>' not found</xsl:message>
  </xsl:if>
  <xsl:if test="count($attr/@default) = 0">
    <xsl:message terminate="yes">Error: default value for xsd:attribute '<xsl:value-of select="concat($type/@name,'/@',$name)"/>' not found</xsl:message>
  </xsl:if>
  <xsl:variable name="value" select="$node/@*[name() = $name]"/>
  <xsl:choose>
    <xsl:when test="count($value) != 0"><xsl:value-of select="$value"/></xsl:when>
    <xsl:otherwise><xsl:value-of select="$attr/@default"/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="GetAutoRestart">
  <xsl:param name="app-env"/>
  <xsl:variable name="status">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$app-env"/>
      <xsl:with-param name="name" select="'autorestart'"/>
      <xsl:with-param name="type" select="$xsd-app-config-type"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="string($status)"/>
</xsl:template>

<xsl:template name="GetAutoRestartFolder">
  <xsl:param name="app-env"/>
  <xsl:variable name="status">
    <xsl:call-template name="GetAutoRestart">
      <xsl:with-param name="app-env" select="$app-env"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:if test="$status = 'true'">
    <xsl:value-of select="$app-env/cfg:develParams/@autorestart_state_root"/>
    <xsl:if test="string-length($app-env/cfg:develParams/@autorestart_state_root)
      = 0">/opt/foros/manager/var/state/<xsl:value-of
          select="concat('server-', $colo-name)"/></xsl:if>
  </xsl:if>
</xsl:template>

<xsl:template name="GetUserName">
  <xsl:param name="app-env"/>
  <xsl:call-template name="GetAttr">
    <xsl:with-param name="node" select="$app-env"/>
    <xsl:with-param name="name" select="'user_name'"/>
    <xsl:with-param name="type" select="$xsd-zone-management-type"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="PrivateKeyFile">
  <xsl:param name="product-identifier"/>
  <xsl:param name="app-env"/>

  <xsl:variable name="ssh-key">
  <xsl:call-template name="GetAttrDefTemplate">
    <xsl:with-param name="value" select="$app-env/@ssh_key"/>
    <xsl:with-param name="app-env" select="$app-env"/>
    <xsl:with-param name="xsd-app-env-type" select="$xsd-zone-management-type"/>
  </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="def-ssh-key">
  <xsl:call-template name="GetAttrDefTemplate">
    <xsl:with-param name="value"
      select="$xsd-zone-management-type//xsd:attribute[@name = 'ssh_key']/@default"/>
    <xsl:with-param name="app-env" select="$app-env"/>
    <xsl:with-param name="xsd-app-env-type" select="$xsd-zone-management-type"/>
  </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when
      test="count($app-env/../cfg:forosZoneManagement/cfg:private_key[ . != '' ]) != 0 or
            count($app-env/cfg:private_key[ . != '' ]) != 0"><xsl:value-of
        select="$def-ssh-key"/>.<xsl:value-of select="$product-identifier"/>
    </xsl:when>
    <xsl:when
      test="count($app-env) = 0">
      <xsl:value-of select="$def-ssh-key"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$ssh-key"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="GetZenOSSEnabled">
  <xsl:param name="app-xpath"/>
  <xsl:call-template name="GetAttr">
    <xsl:with-param name="node" select="$app-xpath/configuration/cfg:environment/cfg:ZenOSS"/>
    <xsl:with-param name="name" select="'enabled'"/>
    <xsl:with-param name="type" select="$xsd-zenoss-type"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="ZenossInstallRoot">
  <xsl:param name="app-xpath"/>
  <xsl:variable name="zenoss-enabled">
    <xsl:call-template name="GetZenOSSEnabled">
      <xsl:with-param name="app-xpath" select="$app-xpath"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
    <!-- Take default values from XSD, if <ZenOSS> omitted -->
    <xsl:variable name="install-root" select="
      $app-xpath/configuration/cfg:environment/cfg:ZenOSS/@install_root |
        $xsd-zenoss-type/xsd:attribute[@name='install_root']/@default"/>
    <xsl:value-of select="$install-root"/>
  </xsl:if>
</xsl:template>

<xsl:template name="ZenossFolder">
  <xsl:param name="app-xpath"/>
  <xsl:variable name="zenoss-enabled">
    <xsl:call-template name="GetZenOSSEnabled">
      <xsl:with-param name="app-xpath" select="$app-xpath"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
    <xsl:variable name="zenoss_install_root"><xsl:call-template name="ZenossInstallRoot">
        <xsl:with-param name="app-xpath" select="$app-xpath"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat($zenoss_install_root,'/lib/',$colocation-name)"/>
  </xsl:if>
</xsl:template>

<xsl:template name="ResolveHostName">
  <xsl:param name="base-host"/>
  <xsl:param name="error-prefix"/>
  <xsl:if test="count(/colo:colocation/host[@name = $base-host]) != 1">
    <xsl:message terminate="yes"><xsl:value-of select="$error-prefix"/>: Functions::ResolveHostName:
    <xsl:value-of select="count(/colo:colocation/host[@name = $base-host])"/> hosts instead one for name <xsl:value-of select="$base-host"/>.</xsl:message>
  </xsl:if>
  <xsl:choose>
    <xsl:when test="count(/colo:colocation/host[@name = $base-host][1]/@hostName) > 0">
      <xsl:value-of select="/colo:colocation/host[@name = $base-host][1]/@hostName"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$base-host"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="ConvertLogger">
  <xsl:param name="logger-node"/>
  <xsl:param name="log-file"/>
  <xsl:param name="default-log-level"/>
  <xsl:param name="no-trace"/>

  <xsl:variable name="log-level"><xsl:value-of select="$logger-node/@log_level"/>
    <xsl:if test="count($logger-node/@log_level) = 0">
      <xsl:value-of select="$default-log-level"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="sys-log-level">
    <xsl:choose>
      <xsl:when test="$default-syslog-level &lt; $log-level">
        <xsl:value-of select="$default-syslog-level"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$log-level"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="sys-log"><xsl:value-of select="$logger-node/@sys_log"/>
    <xsl:if test="count($logger-node/@sys_log) = 0">
      <xsl:value-of select="$def-sys-log"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="rotate-size">
    <xsl:value-of select="$logger-node/@rotate_size"/>
    <xsl:if test="count($logger-node/@rotate_size) = 0">
      <xsl:value-of select="$def-rotate-size"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="rotate-time">
    <xsl:value-of select="$logger-node/@rotate_time"/>
    <xsl:if test="count($logger-node/@rotate_time) = 0">
      <xsl:value-of select="$def-rotate-time"/>
    </xsl:if>
  </xsl:variable>

  <cfg:Logger>
    <xsl:attribute name="filename">
      <xsl:value-of select="$log-file"/>
    </xsl:attribute>
    <xsl:attribute name="log_level"><xsl:value-of select="$log-level"/></xsl:attribute>
    <xsl:if test="$sys-log = 'true'">
      <cfg:SysLog>
        <xsl:attribute name="log_level"><xsl:value-of select="$sys-log-level"/></xsl:attribute>
      </cfg:SysLog>
    </xsl:if>
    <cfg:Suffix>
      <xsl:attribute name="time_span"><xsl:value-of select="$rotate-time * 60"/></xsl:attribute>
      <xsl:attribute name="size_span"><xsl:value-of select="$rotate-size * 1024 * 1024"/></xsl:attribute>
      <xsl:choose>
        <xsl:when test="$no-trace = 'true'">
          <xsl:attribute name="max_log_level"><xsl:value-of select="$log-level"/></xsl:attribute>
          <xsl:attribute name="name"><xsl:value-of select="'.log'"/></xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="max_log_level"><xsl:value-of select="$default-error-log-level"/></xsl:attribute>
          <xsl:attribute name="name"><xsl:value-of select="'.error'"/></xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
    </cfg:Suffix>
    <xsl:if test="$no-trace != 'true'">
      <cfg:Suffix>
        <xsl:attribute name="min_log_level"><xsl:value-of select="$default-error-log-level + 1"/></xsl:attribute>
        <xsl:attribute name="max_log_level"><xsl:value-of select="'8'"/></xsl:attribute>
        <xsl:attribute name="time_span"><xsl:value-of select="$rotate-time * 60"/></xsl:attribute>
        <xsl:attribute name="size_span"><xsl:value-of select="$rotate-size * 1024 * 1024"/></xsl:attribute>
        <xsl:attribute name="name"><xsl:value-of select="'.trace'"/></xsl:attribute>
      </cfg:Suffix>
    </xsl:if>
  </cfg:Logger>
</xsl:template>

<xsl:template name="ConvertSecureParams">

  <xsl:param name="secure-node"/>
  <xsl:param name="global-secure-node"/>
  <xsl:param name="config-root"/>
  <xsl:param name="secure-files-root"/>

  <xsl:if test="$secure-node = 'true'">
    <xsl:variable name="ca-value">
      <xsl:value-of select="$global-secure-node/@ca"/>
      <xsl:if test="count($global-secure-node/@ca) = 0">
        <xsl:value-of select="$default-secure-params-ca"/>
      </xsl:if>
    </xsl:variable>
    <cfg:Secure>
      <xsl:attribute name="key"><xsl:value-of
         select="concat($config-root, $secure-files-root, $default-secure-params-key)"/></xsl:attribute>
      <xsl:attribute name="certificate"><xsl:value-of
         select="concat($config-root, $secure-files-root, $default-secure-params-certificate)"/></xsl:attribute>
      <xsl:attribute name="certificate_authority"><xsl:value-of
         select="concat($config-root, $secure-files-root, $ca-value)"/></xsl:attribute>
      <xsl:attribute name="key-word"><xsl:value-of
         select="$default-secure-params-password"/></xsl:attribute>
    </cfg:Secure>
  </xsl:if>
</xsl:template>

<xsl:template name="ConvertDBConnection">
  <xsl:param name="db-connection"/>
  <xsl:param name="db-link"/>
  <xsl:param name="default-timeout"/>

  <cfg:DBConnection>
    <xsl:attribute name="db"><xsl:value-of select="$db-link"/></xsl:attribute>
    <xsl:attribute name="user"><xsl:value-of select="$db-connection/@user"/></xsl:attribute>
    <xsl:if test="count($db-connection/@schema) > 0">
      <xsl:attribute name="schema"><xsl:value-of select="$db-connection/@schema"/></xsl:attribute>
    </xsl:if>
    <xsl:attribute name="password"><xsl:value-of select="$db-connection/@password"/></xsl:attribute>

    <xsl:choose>
      <xsl:when test="count($db-connection/@statement_timeout) > 0">
        <xsl:attribute name="statement_timeout"><xsl:value-of select="$db-connection/@statement_timeout"/></xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="string-length($default-timeout) > 0">
          <xsl:attribute name="statement_timeout"><xsl:value-of select="$default-timeout"/></xsl:attribute>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </cfg:DBConnection>
</xsl:template>

<xsl:template name="NormalizeName">
  <xsl:param name="name"/>

  <xsl:variable name="norm-name" select="translate($name,
                                                  'ABCDEFGHIJKLMNOPQRSTUVWXYZ ',
                                                  'abcdefghijklmnopqrstuvwxyz_')"/>

  <xsl:value-of select="$norm-name"/>
</xsl:template>

<xsl:template name="substring-before-last">
  <xsl:param name="arg"/>
  <xsl:param name="delim"/>

  <xsl:variable name="head" select="substring-before($arg, $delim)" />
  <xsl:variable name="tail" select="substring-after($arg, $delim)" />
  <xsl:value-of select="$head" />
  <xsl:if test="contains($tail, $delim)">
    <xsl:value-of select="$delim"/>
    <xsl:call-template name="substring-before-last">
      <xsl:with-param name="arg" select="$tail"/>
      <xsl:with-param name="delim" select="$delim" />
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="substring-after-last">
  <xsl:param name="arg"/>
  <xsl:param name="delim"/>

  <xsl:variable name="tail" select="substring-after($arg, $delim)"/>
  <xsl:choose>
    <xsl:when test="contains($tail, $delim)">
      <xsl:call-template name="substring-after-last">
        <xsl:with-param name="arg" select="$tail"/>
        <xsl:with-param name="delim" select="$delim" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$tail"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="GetHosts">
  <xsl:param name="hosts"/>
  <xsl:param name="error-prefix"/>

  <xsl:variable name="normalized-hosts" select="normalize-space($hosts)"/>

  <xsl:if test="string-length($normalized-hosts) > 0">
    <xsl:variable name="head" select="substring-before($normalized-hosts, ' ')" />
    <xsl:variable name="tail" select="substring-after($normalized-hosts, ' ')" />

    <xsl:choose>
      <!-- normalized hosts don't contains space -->
      <xsl:when test="string-length($head) = 0">
        <host>
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="$normalized-hosts"/>
            <xsl:with-param name="error-prefix" select="$error-prefix"/>
          </xsl:call-template>
        </host>
      </xsl:when>
      <xsl:otherwise>
        <host>
          <xsl:call-template name="ResolveHostName">
            <xsl:with-param name="base-host" select="$head"/>
            <xsl:with-param name="error-prefix" select="$error-prefix"/>
          </xsl:call-template>
        </host>
        <xsl:call-template name="GetHosts">
          <xsl:with-param name="hosts" select="normalize-space($tail)"/>
          <xsl:with-param name="error-prefix" select="$error-prefix" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>

<xsl:template name="GetUniqueHosts">
  <xsl:param name="services-xpath"/>
  <xsl:param name="error-prefix"/>

  <xsl:variable name="hosts">
    <xsl:for-each select="$services-xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="$error-prefix"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:for-each select="exsl:node-set($hosts)//host[not(. = preceding-sibling::host/.)]"><xsl:value-of
    select="concat(' ', .)"/></xsl:for-each>
</xsl:template>

<xsl:template name="GetVirtualServers">
  <xsl:param name="xpath"/>
  <xsl:variable name="servers" select="$xpath[
     (count(@enable) = 0 or @enable='true')]"/>
  <xsl:for-each select="$servers">
     <xsl:copy-of select="."/>
  </xsl:for-each>
</xsl:template>

<!-- return lowest(lexicaly) resolved hostname for services -->
<xsl:template name="GetLowestHost">
  <xsl:param name="services-xpath"/>
  <xsl:param name="error-prefix"/>

  <xsl:variable name="hosts">
    <xsl:for-each select="$services-xpath">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="$error-prefix"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="sorted-hosts">
    <xsl:for-each select="exsl:node-set($hosts)//host">
      <xsl:sort select="."/>
      <host><xsl:value-of select="."/></host>
    </xsl:for-each>
  </xsl:variable>

  <xsl:value-of select="exsl:node-set($sorted-hosts)//host[1]"/>
</xsl:template>

<xsl:template name="GetServiceRefs">
  <xsl:param name="services"/>
  <xsl:param name="def-port"/>
  <xsl:param name="error-prefix"/>

  <Refs>
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
      <serviceRef host="{.}" port="{$port}"/>
    </xsl:for-each>

  </xsl:for-each>
 </Refs>
</xsl:template>

<xsl:template name="FillCorbaRef">
  <xsl:param name="service-path"/>
  <xsl:param name="services-config"/>
  <xsl:param name="default-port"/>
  <xsl:param name="obj-name"/>

  <xsl:variable name="hosts">
    <xsl:call-template name="GetHosts">
      <xsl:with-param name="hosts" select="$service-path/@host"/>
      <xsl:with-param name="error-prefix" select="concat('Resolving of ', $obj-name)"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="port"><xsl:value-of select="$service-path/*/cfg:networkParams/@port"/>
    <xsl:if test="count($service-path/*/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$services-config/cfg:networkParams/@port"/>
      <xsl:if test="count($services-config/cfg:networkParams/@port) = 0">
        <xsl:value-of select="$default-port"/>
      </xsl:if>
    </xsl:if>
  </xsl:variable>

  <xsl:attribute name="ref">
    <xsl:value-of select="concat('corbaloc:iiop:', exsl:node-set($hosts)//host[1],
      ':', $port, $obj-name)"/>
  </xsl:attribute>
</xsl:template>

<xsl:template name="AddStatsDumper">
  <xsl:param name="stats-collector-path"/>
  <xsl:param name="stats-collector-config"/>
  <xsl:param name="provide-channel-counters"/>

  <xsl:if test="count($stats-collector-path)">
    <xsl:choose>
      <xsl:when test="string-length($provide-channel-counters) > 0">
        <cfg:StatsDumper>
          <xsl:attribute name="period">
            <xsl:value-of select="$stats-collector-config/@period"/>
            <xsl:if test="count($stats-collector-config/@period) = 0">
              <xsl:value-of select="$def-stats-dumping-period"/>
            </xsl:if>
          </xsl:attribute>
          <xsl:attribute name="provide_channel_counters">
            <xsl:value-of select="$provide-channel-counters"/>
          </xsl:attribute>
          <cfg:StatsDumperRef name="StatsDumperRef">
            <xsl:call-template name="FillCorbaRef">
              <xsl:with-param name="service-path" select="$stats-collector-path"/>
              <xsl:with-param name="services-config" select="$stats-collector-config"/>
              <xsl:with-param name="default-port" select="$def-stats-collector-port"/>
              <xsl:with-param name="obj-name" select="'/StatsCollector'"/>
            </xsl:call-template>
          </cfg:StatsDumperRef>
        </cfg:StatsDumper>
      </xsl:when>
      <xsl:otherwise>
        <cfg:StatsDumper>
          <xsl:attribute name="period">
            <xsl:value-of select="$stats-collector-config/@period"/>
            <xsl:if test="count($stats-collector-config/@period) = 0">
              <xsl:value-of select="$def-stats-dumping-period"/>
            </xsl:if>
          </xsl:attribute>
          <cfg:StatsDumperRef name="StatsDumperRef">
            <xsl:call-template name="FillCorbaRef">
              <xsl:with-param name="service-path" select="$stats-collector-path"/>
              <xsl:with-param name="services-config" select="$stats-collector-config"/>
              <xsl:with-param name="default-port" select="$def-stats-collector-port"/>
              <xsl:with-param name="obj-name" select="'/StatsCollector'"/>
            </xsl:call-template>
          </cfg:StatsDumperRef>
        </cfg:StatsDumper>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>

<xsl:template name="AddUserBindControllerGroups">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="error-prefix"/>

  <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
    <xsl:if test="count(./service[@descriptor = $user-bind-controller-descriptor]) > 0">
      <cfg:UserBindControllerGroup name="UserBindControllers">
        <xsl:for-each select="./service[@descriptor = $user-bind-controller-descriptor]">
          <xsl:variable name="hosts">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
              <xsl:with-param name="error-prefix"
                select="concat($error-prefix,
                  ': AddUserBindControllerGroups: UserBindController hosts resolving: ')"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:variable
            name="user-bind-controller-config"
            select="./configuration/cfg:userBindController"/>

          <xsl:variable name="user-bind-controller-port">
            <xsl:value-of select="$user-bind-controller-config/cfg:networkParams/@port"/>
            <xsl:if test="count($user-bind-controller-config/cfg:networkParams/@port) = 0">
              <xsl:value-of select="$def-user-bind-controller-port"/>
            </xsl:if>
          </xsl:variable>

          <xsl:for-each select="exsl:node-set($hosts)//host">
            <cfg:Ref>
              <xsl:attribute name="ref">
                <xsl:value-of select="concat('corbaloc:iiop:', ., ':',
                  $user-bind-controller-port, $current-user-bind-controller-obj)"/>
              </xsl:attribute>
            </cfg:Ref>
          </xsl:for-each>
        </xsl:for-each>
      </cfg:UserBindControllerGroup>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<xsl:template name="AddUserInfoManagerControllerGroups">
  <xsl:param name="full-cluster-path"/>
  <xsl:param name="error-prefix"/>

  <xsl:for-each select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]">
    <cfg:UserInfoManagerControllerGroup name="UserInfoManagerControllers">
      <xsl:for-each select="./service[@descriptor = $user-info-manager-controller-descriptor]">
        <xsl:variable name="hosts">
          <xsl:call-template name="GetHosts">
            <xsl:with-param name="hosts" select="@host"/>
            <xsl:with-param name="error-prefix"
              select="concat($error-prefix,
                ': AddUserInfoManagerControllerGroups: UserInfoManagerController hosts resolving: ')"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:variable
          name="user-info-manager-controller-config"
          select="./configuration/cfg:userInfoManagerController"/>

        <xsl:variable name="user-info-manager-controller-port">
          <xsl:value-of select="$user-info-manager-controller-config/cfg:networkParams/@port"/>
          <xsl:if test="count($user-info-manager-controller-config/cfg:networkParams/@port) = 0">
            <xsl:value-of select="$def-user-info-manager-controller-port"/>
          </xsl:if>
        </xsl:variable>

          <xsl:for-each select="exsl:node-set($hosts)//host">
            <cfg:Ref>
              <xsl:attribute name="ref">
                <xsl:value-of select="concat('corbaloc:iiop:', ., ':',
                  $user-info-manager-controller-port, '/', $current-user-info-manager-controller-obj)"/>
              </xsl:attribute>
            </cfg:Ref>
          </xsl:for-each>
        </xsl:for-each>
      </cfg:UserInfoManagerControllerGroup>
    </xsl:for-each>
</xsl:template>

<xsl:template name="GetReferrerLoggingValue">
  <xsl:param name="referrer-logging"/>

  <xsl:choose>
    <xsl:when test="count($referrer-logging) = 0 or
    $referrer-logging = 'disabled'">empty</xsl:when>
    <xsl:when test="$referrer-logging = 'domain'">domain</xsl:when>
    <xsl:when test="$referrer-logging = 'domain and path'">path</xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">Unexpected referrer_logging value</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Collect configuration from different virtual servers of frontend
  Uses in genetaion for ad and content cluster 
  @param virtual-servers list of virtual servers
  @param port-field-name name of port attribute in virtual server
  @param def-port default port
  @param target-host target host for monitoring
  @param generate-certificates '1' add generated certificates in configuration
  @param config-root root of configuration
  @param out-dir of certificates
  -->
<xsl:template name="PackedVirtualServersGenerator">
  <xsl:param name="virtual-servers"/>
  <xsl:param name="port-field-name"/>
  <xsl:param name="def-port"/>
  <xsl:param name="target-host"/>
  <xsl:param name="generate-certificates"/>
  <xsl:param name="config-root"/>
  <xsl:param name="out-dir"/>

  <xsl:variable name="non-packed-virtual-servers">
    <xsl:for-each select="$virtual-servers">
      <xsl:variable name="colo-virtual-server" select="."/>
      <xsl:variable name="port"><xsl:value-of select="@*[local-name() = $port-field-name]"/><xsl:if
        test="count(@*[local-name() = $port-field-name]) = 0"><xsl:value-of
          select="$def-port"/></xsl:if></xsl:variable>

      <xsl:for-each select="$colo-virtual-server/cfg:adservingDomain |
        $colo-virtual-server/cfg:thirdPartyContentDomain |
        $colo-virtual-server/cfg:biddingDomain |
        $colo-virtual-server/cfg:videoDomain |
        $colo-virtual-server/cfg:clickDomain |
        $colo-virtual-server/cfg:profilingDomain">
        <virtual-server>
          <xsl:attribute name="port"><xsl:value-of select="$port"/></xsl:attribute>
          <xsl:choose>
            <xsl:when test="count($colo-virtual-server/@keep_alive) = 0 or $colo-virtual-server/@keep_alive = '1'
              or $colo-virtual-server/@keep_alive = 'true'">
              <xsl:variable name="keep-alive-timeout"><xsl:value-of select="$colo-virtual-server/@keep_alive_timeout"/><xsl:if
                 test="count($colo-virtual-server/@keep_alive_timeout) = 0">5</xsl:if></xsl:variable>
              <xsl:variable name="max-keep-alive-requests"><xsl:value-of select="$colo-virtual-server/@max_keep_alive_requests"/><xsl:if
                 test="count($colo-virtual-server/@max_keep_alive_requests) = 0">100</xsl:if></xsl:variable>
              <xsl:variable name="id"><xsl:value-of select="$port"/>/1/<xsl:value-of
                select="$keep-alive-timeout"/>/<xsl:value-of
                select="$max-keep-alive-requests"/></xsl:variable>

              <xsl:attribute name="keep_alive">1</xsl:attribute>
              <xsl:attribute name="keep_alive_timeout"><xsl:value-of select="$keep-alive-timeout"/></xsl:attribute>
              <xsl:attribute name="max_keep_alive_requests"><xsl:value-of select="$max-keep-alive-requests"/></xsl:attribute>
              <xsl:attribute name="id"><xsl:value-of select="$id"/></xsl:attribute>
              <xsl:attribute name="full_id"><xsl:value-of select="$id"/>/<xsl:value-of select="@name"/></xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="keep_alive">0</xsl:attribute>
              <xsl:attribute name="keep_alive_timeout">0</xsl:attribute>
              <xsl:attribute name="max_keep_alive_requests">0</xsl:attribute>
              <xsl:attribute name="id"><xsl:value-of select="$port"/>/0/0/0</xsl:attribute>
              <xsl:attribute name="full_id"><xsl:value-of select="$port"/>/0/0/0/<xsl:value-of select="@name"/></xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:attribute name="domain"><xsl:value-of select="@name"/></xsl:attribute>

          <xsl:choose>
            <xsl:when test="local-name() = 'adservingDomain'"><enable module="request"/></xsl:when>
            <xsl:when test="local-name() = 'thirdPartyContentDomain'"><enable module="creative"/></xsl:when>
            <xsl:when test="local-name() = 'biddingDomain'"><enable module="bidding"/></xsl:when>
            <xsl:when test="local-name() = 'videoDomain'"><enable module="video"/></xsl:when>
            <xsl:when test="local-name() = 'clickDomain'"><enable module="click"/></xsl:when>
            <xsl:when test="local-name() = 'profilingDomain'"><enable module="profiling"/></xsl:when>
          </xsl:choose>
        </virtual-server>
      </xsl:for-each>

      <!-- add adserving module for install target host for testing
        and monitoring -->
      <xsl:if test="count($colo-virtual-server/cfg:adservingDomain[
        @name = $target-host]) = 0">
        <virtual-server port="{$port}" domain="{$target-host}">
          <xsl:attribute name="keep_alive">0</xsl:attribute>
          <xsl:attribute name="keep_alive_timeout">0</xsl:attribute>
          <xsl:attribute name="max_keep_alive_requests">0</xsl:attribute>
          <xsl:attribute name="id"><xsl:value-of select="$port"/>/0/0/0</xsl:attribute>
          <xsl:attribute name="full_id"><xsl:value-of select="$port"/>/0/0/0/<xsl:value-of select="$target-host"/></xsl:attribute>
          <enable module="request"/>
        </virtual-server>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <!-- combine virtual servers by full_id (port/keep alives/domain)
    and sort enable elements by module name -->
  <xsl:variable name="full-id-packed-virtual-servers">
    <xsl:for-each select="exsl:node-set($non-packed-virtual-servers)/virtual-server[
      not(@full_id = preceding-sibling::virtual-server/@full_id)]">
      <xsl:variable name="full-id"><xsl:value-of select="@full_id"/></xsl:variable>
      <virtual-server
        port="{@port}"
        keep_alive="{@keep_alive}"
        keep_alive_timeout="{@keep_alive_timeout}"
        max_keep_alive_requests="{@max_keep_alive_requests}"
        domain="{@domain}"
        id="{@id}"
        full_id="{@full_id}">
        <xsl:attribute name="modules_id"><xsl:value-of select="@id"/>/<xsl:for-each
          select="exsl:node-set($non-packed-virtual-servers)/virtual-server[
            @full_id = $full-id]/enable[not(@module = preceding-sibling::enable/@module)]">
            <xsl:sort select="@module"/>
            <xsl:value-of select="@module"/>;</xsl:for-each>
        </xsl:attribute>
        <xsl:for-each select="exsl:node-set($non-packed-virtual-servers)/virtual-server[
          @full_id = $full-id]/enable[not(@module = preceding-sibling::enable/@module)]">
          <xsl:sort select="@module"/>
          <xsl:copy-of select="."/>
        </xsl:for-each>
      </virtual-server>
    </xsl:for-each>
  </xsl:variable>

  <!-- group by all modules-id(=id/module;..) -->
  <xsl:for-each select="exsl:node-set($full-id-packed-virtual-servers)/virtual-server[
    not(@modules_id = preceding-sibling::virtual-server/@modules_id)]">
    <xsl:variable name="modules-id"><xsl:value-of select="@modules_id"/></xsl:variable>
    <virtual-server
      port="{@port}"
      id="{@id}"
      modules_id="{@modules_id}"
      full_id="{@full_id}">
      <xsl:if test="$generate-certificates = '1'">
        <xsl:attribute name="certificate"><xsl:value-of
          select="$config-root"/>/<xsl:value-of
          select="$out-dir"/>/cert/apcert-<xsl:value-of select="@port"/>.pem</xsl:attribute>
        <xsl:attribute name="certificate_key"><xsl:value-of
          select="$config-root"/>/<xsl:value-of
          select="$out-dir"/>/cert/apkey-<xsl:value-of select="@port"/>.pem</xsl:attribute>
        <xsl:attribute name="ca_certificate"><xsl:value-of
          select="$config-root"/>/<xsl:value-of
          select="$out-dir"/>/cert/apca-<xsl:value-of select="@port"/>.pem</xsl:attribute>
        <!-- required for nginx -->
        <xsl:attribute name="certificate_and_ca"><xsl:value-of
          select="$config-root"/>/<xsl:value-of
          select="$out-dir"/>/cert/apcertca-<xsl:value-of select="@port"/>.pem</xsl:attribute>
      </xsl:if>
      <keep-alive-directives>
        <xsl:choose>
          <xsl:when test="count(@keep_alive) = 0 or @keep_alive = '1' or @keep_alive = 'true'">
          KeepAlive On
            <xsl:if test="count(@keep_alive_timeout) > 0">
            KeepAliveTimeout <xsl:value-of select="@keep_alive_timeout"/>
            </xsl:if>
            <xsl:if test="count(@max_keep_alive_requests) > 0">
            MaxKeepAliveRequests <xsl:value-of select="@max_keep_alive_requests"/>
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
          KeepAlive Off
          </xsl:otherwise>
        </xsl:choose>
      </keep-alive-directives>
      <xsl:for-each select="exsl:node-set($full-id-packed-virtual-servers)/virtual-server[
        @modules_id = $modules-id]">
        <domain name="{@domain}"/>
      </xsl:for-each>

      <!-- modules must be equal in each grouped element -->
      <!-- revert to old structure (TODO : change) -->
      <xsl:for-each select="exsl:node-set($full-id-packed-virtual-servers)/virtual-server[
        @modules_id = $modules-id]">
        <xsl:variable name="domain"><xsl:value-of select="@domain"/></xsl:variable>
        <xsl:for-each select="enable">
          <xsl:choose>
            <xsl:when test="@module = 'request'"><adserving-domain name="{$domain}"/></xsl:when>
            <xsl:when test="@module = 'creative'"><content-domain name="{$domain}"/></xsl:when>
            <xsl:when test="@module = 'bidding'"><bidding-domain name="{$domain}"/></xsl:when>
            <xsl:when test="@module = 'video'"><video-domain name="{$domain}"/></xsl:when>
            <xsl:when test="@module = 'click'"><click-domain name="{$domain}"/></xsl:when>
            <xsl:when test="@module = 'profiling'"><profiling-domain name="{$domain}"/></xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </xsl:for-each>

      <xsl:copy-of select="enable"/>
    </virtual-server>
  </xsl:for-each>

  <!-- push redirect domain's without packing -->
  <xsl:for-each select="$virtual-servers">
    <xsl:variable name="colo-virtual-server" select="."/>
    <xsl:variable name="port"><xsl:value-of select="@*[local-name() = $port-field-name]"/><xsl:if
      test="count(@*[local-name() = $port-field-name]) = 0"><xsl:value-of
        select="$def-port"/></xsl:if></xsl:variable>

    <xsl:for-each select="$colo-virtual-server/cfg:redirectDomain[
        not(@name = preceding-sibling::cfg:redirectDomain/@name)]">
      <virtual-server port="{$port}">
        <xsl:if test="$generate-certificates = '1'">
          <xsl:attribute name="certificate"><xsl:value-of
            select="$config-root"/>/<xsl:value-of
            select="$out-dir"/>/cert/apcert-<xsl:value-of select="$port"/>.pem</xsl:attribute>
          <xsl:attribute name="certificate_key"><xsl:value-of
            select="$config-root"/>/<xsl:value-of
            select="$out-dir"/>/cert/apkey-<xsl:value-of select="$port"/>.pem</xsl:attribute>
          <xsl:attribute name="ca_certificate"><xsl:value-of
            select="$config-root"/>/<xsl:value-of
            select="$out-dir"/>/cert/apca-<xsl:value-of select="$port"/>.pem</xsl:attribute>
        </xsl:if>
        <redirect-domain name="{@name}" target_domain="{@target_domain}" expiration="{@expiration}"/>
      </virtual-server>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template> <!-- end PackedVirtualServersGenerator -->

<xsl:template name="ZmqSocketGenerator">
  <xsl:param name="zmq-socket-config"/>

  <xsl:if test="count($zmq-socket-config/@hwm) > 0">
    <xsl:attribute name="hwm">
      <xsl:value-of select="$zmq-socket-config/@hwm"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="count($zmq-socket-config/@linger) > 0">
    <xsl:attribute name="linger">
      <xsl:value-of select="$zmq-socket-config/@linger"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="count($zmq-socket-config/@reconnect_interval) > 0">
    <xsl:attribute name="reconnect_interval">
      <xsl:value-of select="$zmq-socket-config/@reconnect_interval"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="count($zmq-socket-config/@non_block) > 0">
    <xsl:attribute name="non_block">
      <xsl:value-of select="$zmq-socket-config/@non_block"/>
    </xsl:attribute>
  </xsl:if>

  <xsl:if test="count($zmq-socket-config/cfg:serverSecurity) > 0">
    <cfg:ServerSecurity server_secret="{$zmq-socket-config/cfg:serverSecurity/@server_secret}"/>
  </xsl:if>

  <xsl:if test="count($zmq-socket-config/cfg:clientSecurity) > 0">
    <cfg:ClientSecurity 
      server_public="{$zmq-socket-config/cfg:clientSecurity/@server_public}"
      client_secret="{$zmq-socket-config/cfg:clientSecurity/@client_secret}"
      client_public="{$zmq-socket-config/cfg:clientSecurity/@client_public}"
    />
  </xsl:if>

  <xsl:for-each select="$zmq-socket-config/cfg:address">

    <cfg:Address domain="{@domain}" port="{@port}"/>

  </xsl:for-each>
</xsl:template>

<xsl:template name="ZmqSocketGeneratorWithDefault">
  <xsl:param name="socket-config"/>
  <xsl:param name="default-socket-config"/>

  <xsl:choose>
    <xsl:when test="count($socket-config/@hwm) > 0">
      <xsl:attribute name="hwm">
        <xsl:value-of select="$socket-config/@hwm"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="count($default-socket-config/@hwm) > 0">
      <xsl:attribute name="hwm">
        <xsl:value-of select="$default-socket-config/@hwm"/>
      </xsl:attribute>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/@non_block) > 0">
      <xsl:attribute name="non_block">
        <xsl:value-of select="$socket-config/@non_block"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="count($default-socket-config/@non_block) > 0">
      <xsl:attribute name="non_block">
        <xsl:value-of select="$default-socket-config/@non_block"/>
      </xsl:attribute>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/@linger) > 0">
      <xsl:attribute name="linger">
        <xsl:value-of select="$socket-config/@linger"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="count($default-socket-config/@linger) > 0">
      <xsl:attribute name="linger">
        <xsl:value-of select="$default-socket-config/@linger"/>
      </xsl:attribute>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/@reconnect_interval) > 0">
      <xsl:attribute name="reconnect_interval">
        <xsl:value-of select="$socket-config/@reconnect_interval"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="count($default-socket-config/@reconnect_interval) > 0">
      <xsl:attribute name="reconnect_interval">
        <xsl:value-of select="$default-socket-config/@reconnect_interval"/>
      </xsl:attribute>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/cfg:serverSecurity) > 0">
      <cfg:ServerSecurity server_secret="{$socket-config/cfg:serverSecurity/@server_secret}"/>
    </xsl:when>
    <xsl:when test="count($default-socket-config/serverSecurity) > 0">
      <cfg:ServerSecurity server_secret="{$default-socket-config/serverSecurity/@server_secret}"/>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/cfg:clientSecurity) > 0">
      <cfg:ClientSecurity 
        server_public="{$socket-config/cfg:clientSecurity/@server_public}"
        client_secret="{$socket-config/cfg:clientSecurity/@client_secret}"
        client_public="{$socket-config/cfg:clientSecurity/@client_public}"
      />
    </xsl:when>
    <xsl:when test="count($default-socket-config/clientSecurity) > 0">
      <cfg:ClientSecurity 
        server_public="{$default-socket-config/cfg:clientSecurity/@server_public}"
        client_secret="{$default-socket-config/cfg:clientSecurity/@client_secret}"
        client_public="{$default-socket-config/cfg:clientSecurity/@client_public}"
      />
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="count($socket-config/cfg:address) > 0">
      <xsl:for-each select="$socket-config/cfg:address">
        <cfg:Address domain="{@domain}" port="{@port}"/>
      </xsl:for-each>
    </xsl:when>
    <xsl:when test="count($default-socket-config/address) > 0">
      <xsl:for-each select="$default-socket-config/address">
        <cfg:Address domain="{@domain}" port="{@port}"/>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>

</xsl:template>

<xsl:template name="SaveKafkaTopic">
  <xsl:param name="topic-config" />
  <xsl:param name="default-topic-name"/>
  <xsl:param name="kafka-config"/>
  <xsl:variable name="broker_str">
    <xsl:for-each select="$kafka-config/cfg:broker">
      <xsl:value-of select="@host"/>:<xsl:value-of 
        select="@port"/><xsl:if test="count(@port) = 0">9092</xsl:if><xsl:if 
        test="position()!=last()">,</xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:attribute name="brokers">
    <xsl:value-of select="$broker_str"/>
  </xsl:attribute>
  <xsl:attribute name="topic"><xsl:value-of 
    select="$topic-config/@topic"/><xsl:if  
    test="count($topic-config/@topic) = 0"><xsl:value-of 
    select="$default-topic-name"/></xsl:if>
  </xsl:attribute>
  <xsl:attribute name="threads"><xsl:value-of 
    select="$topic-config/@threads"/><xsl:if 
    test="count($topic-config/@threads) = 0"><xsl:value-of 
    select="$default-kafka-threads"/></xsl:if>
  </xsl:attribute>
  <xsl:attribute name="message_queue_size"><xsl:value-of 
    select="$topic-config/@message_queue_size"/><xsl:if 
    test="count($topic-config/@message_queue_size) = 0"><xsl:value-of 
    select="$default-kafka-message-queue-size"/></xsl:if>
  </xsl:attribute>
</xsl:template>

<xsl:template name="GetConnectSocketToProfilingServersConfig">
  <xsl:param name="fe-cluster-path"/>
  <xsl:param name="default-port"/>

  <xsl:variable
    name="profiling-server-config"
    select="$fe-cluster-path/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']/configuration/cfg:profilingServer"/>

  <xsl:variable name="dmp-profiling-info-port">
    <xsl:value-of select="$profiling-server-config/cfg:networkParams/@dmp_profiling_info_port"/>
    <xsl:if test="count($profiling-server-config/cfg:networkParams/@dmp_profiling_info_port) = 0">
      <xsl:value-of select="$def-zmq-profiling-server-dmp-profiling-info-port"/>
    </xsl:if>
  </xsl:variable>

  <socket hwm="1" non_block="false">
  <xsl:for-each select="$fe-cluster-path/service[@descriptor = 'AdCluster/FrontendSubCluster/ProfilingServer']">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'ZmqProfilingBalancer:ProfilingServer'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($hosts)/host">
      <address domain="{.}" port="{$dmp-profiling-info-port}"/>
    </xsl:for-each>
  </xsl:for-each>
  </socket>
</xsl:template>

</xsl:stylesheet>

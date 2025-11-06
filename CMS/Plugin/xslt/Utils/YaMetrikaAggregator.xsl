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
<xsl:include href="../Variables.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template name="YaMetrikaAggregatorConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="ya-metrika-aggregator-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-host">
    <xsl:value-of select="$ya-metrika-aggregator-config/cfg:clickhouse/@host"/>
    <xsl:if test="count($ya-metrika-aggregator-config/cfg:clickhouse/@host) = 0">
      <xsl:value-of select="$def-ya-metrika-aggregator-clickhouse-host"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-port">
    <xsl:value-of select="$ya-metrika-aggregator-config/cfg:clickhouse/@port"/>
    <xsl:if test="count($ya-metrika-aggregator-config/cfg:clickhouse/@port) = 0">
      <xsl:value-of select="$def-ya-metrika-aggregator-clickhouse-port"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-db">
    <xsl:value-of select="$ya-metrika-aggregator-config/cfg:clickhouse/@db"/>
    <xsl:if test="count($ya-metrika-aggregator-config/cfg:clickhouse/@db) = 0">
      <xsl:value-of select="$def-ya-metrika-aggregator-clickhouse-db"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-user">
    <xsl:value-of select="$ya-metrika-aggregator-config/cfg:clickhouse/@user"/>
    <xsl:if test="count($ya-metrika-aggregator-config/cfg:clickhouse/@user) = 0">
      <xsl:value-of select="$def-ya-metrika-aggregator-clickhouse-user"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="clickhouse-password">
    <xsl:value-of select="$ya-metrika-aggregator-config/cfg:clickhouse/@password"/>
    <xsl:if test="count($ya-metrika-aggregator-config/cfg:clickhouse/@password) = 0">
      <xsl:value-of select="$def-ya-metrika-aggregator-clickhouse-password"/>
    </xsl:if>
  </xsl:variable>

  <cfg:YaMetrikaAggregatorConfig>
    <cfg:ClickhouseConfig
      host="{$clickhouse-host}"
      port="{$clickhouse-port}"
      db="{$clickhouse-db}"
      user="{$clickhouse-user}"
      password="{$clickhouse-password}"/>

    <xsl:variable name="postgres-host">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:postgres/@host"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:postgres/@host) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-postgres-host"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="postgres-port">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:postgres/@port"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:postgres/@port) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-postgres-port"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="postgres-db">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:postgres/@db"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:postgres/@db) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-postgres-db"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="postgres-user">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:postgres/@user"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:postgres/@user) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-postgres-user"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="postgres-password">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:postgres/@password"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:postgres/@password) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-postgres-password"/>
      </xsl:if>
    </xsl:variable>

    <cfg:PostgresConfig
      host="{$postgres-host}"
      port="{$postgres-port}"
      db="{$postgres-db}"
      user="{$postgres-user}"
      password="{$postgres-password}"/>

    <xsl:variable name="http-connect-timeout">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:http/@connect_timeout"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:http/@connect_timeout) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-http-connect-timeout"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="http-limit">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:http/@limit"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:http/@limit) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-http-limit"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="http-max-attempts">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:http/@max_attempts"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:http/@max_attempts) = 0">
        <xsl:value-of select="$def-ya-metrika-aggregator-max-attempts"/>
      </xsl:if>
    </xsl:variable>

    <cfg:HttpConfig
      connect_timeout="{$http-connect-timeout}"
      limit="{$http-limit}"
      max_attempts="{$http-max-attempts}"/>

    <xsl:variable name="logging-level">
      <xsl:value-of select="$ya-metrika-aggregator-config/cfg:logging/@log_level"/>
      <xsl:if test="count($ya-metrika-aggregator-config/cfg:logging/@log_level) = 0">
        <xsl:value-of select="1"/>
      </xsl:if>
    </xsl:variable>

    <xsl:variable name="logging-out-dir" select="concat($workspace-root, '/log/YaMetrikaAggregator/')"/>

    <cfg:LoggingConfig
      log_level="{$logging-level}"
      out_log_dir="{$logging-out-dir}"/>

    <xsl:variable name="pid-path" select="concat($workspace-root, '/run/ya_metrika_aggregator.pid')"/>
    <cfg:ParamsConfig
      metrics_window_days="{$def-ya-metrika-aggregator-metrics-window-days}"
      update_period_hours="{$def-ya-metrika-aggregator-update-period-hours}"
      postgres_timeout="{$def-ya-metrika-aggregator-postgres-timeout}"
      pid_path="{$pid-path}"/>
  </cfg:YaMetrikaAggregatorConfig>

</xsl:template>

<xsl:template match="/">
  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="be-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $be-cluster-descriptor]"/>

  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor]"/>

  <xsl:variable
    name="ya-metrika-aggregator-path"
    select="$xpath"/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
      <xsl:message terminate="yes"> ExpressionMatcher: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
      <xsl:message terminate="yes"> ExpressionMatcher: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($be-cluster-path) = 0">
      <xsl:message terminate="yes"> ExpressionMatcher: Can't find be-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($ya-metrika-aggregator-path) = 0">
      <xsl:message terminate="yes"> ExpressionMatcher: Can't find log expression matcher node </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="env-config"
    select="$be-cluster-path/configuration/cfg:backendCluster/cfg:environment | $colo-config/cfg:environment"/>

  <xsl:variable
    name="ya-metrika-aggregator-config"
    select="$ya-metrika-aggregator-path/configuration/cfg:yaMetrikaAggregator"/>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>

  <cfg:AdConfiguration
    xsi:schemaLocation="{concat('http://www.adintelligence.net/xsd/AdServer/Configuration ',
      $server-root, '/xsd/Utils/YaMetrikaAggregatorConfig.xsd')}">
    <xsl:call-template name="YaMetrikaAggregatorConfigGenerator">
      <xsl:with-param name="env-config" select="$env-config"/>
      <xsl:with-param name="ya-metrika-aggregator-config" select="$ya-metrika-aggregator-config"/>
    </xsl:call-template>
  </cfg:AdConfiguration>

</xsl:template>

</xsl:stylesheet>
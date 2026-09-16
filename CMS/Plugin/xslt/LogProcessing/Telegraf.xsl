<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn cfg colo">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="../Variables.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath"/>
  <xsl:variable name="env-config"
    select="$cluster-path/configuration/cfg:cluster/cfg:environment"/>
  <xsl:variable name="out-dir" select="$OUT_DIR"/>
  <xsl:variable name="colo-name" select="substring-before($out-dir, '/')"/>
  <xsl:variable name="cluster-name"
    select="substring-before(substring-after($out-dir, '/'), '/')"/>
  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of
      select="$def-config-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of
      select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-dir" select="concat($config-root, '/', $out-dir)"/>
  <xsl:variable name="influxdb-url"><xsl:value-of
    select="$env-config/@monitoring_influxdb_url"/>
    <xsl:if test="count($env-config/@monitoring_influxdb_url) = 0">
      <xsl:text>http://192.168.10.144:8086</xsl:text>
    </xsl:if>
  </xsl:variable>
  <xsl:variable name="database"><xsl:value-of
    select="$env-config/@monitoring_influxdb_database"/>
    <xsl:if test="count($env-config/@monitoring_influxdb_database) = 0">
      <xsl:text>telegraf</xsl:text>
    </xsl:if>
  </xsl:variable>

[global_tags]
  colo = "<xsl:value-of select="$colo-name"/>"
  cluster = "<xsl:value-of select="$cluster-name"/>"

[agent]
  hostname = "<xsl:value-of select="$HOST"/>"

[[inputs.execd]]
  command = [
    "/usr/bin/python3",
    "<xsl:value-of select="concat($server-root, '/bin/StatCollector.py')"/>",
    "--config",
    "<xsl:value-of select="concat($config-dir, '/StatCollectorConfig.json')"/>",
    "--loop"
  ]
  signal = "none"
  restart_delay = "10s"
  data_format = "influx"

[[outputs.influxdb]]
  urls = ["<xsl:value-of select="$influxdb-url"/>"]
  database = "<xsl:value-of select="$database"/>"
  skip_database_creation = true
</xsl:template>

</xsl:stylesheet>

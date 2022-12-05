<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<xsl:template name="EnvironmentConfigGenerator">
  <xsl:param name="app-path"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="env-config"/>

  <xsl:variable name="app-config" select="$app-path/configuration/cfg:environment"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="cache-root"><xsl:value-of select="$env-config/@cache_root"/>
    <xsl:if test="count($env-config/@cache_root) = 0"><xsl:value-of select="$def-cache-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-bin-root"><xsl:value-of select="$env-config/@server_bin_root"/>
    <xsl:if test="count($env-config/@server_bin_root) = 0"><xsl:value-of select="$server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="mib-root"><xsl:value-of select="$env-config/@mib_root"/>
    <xsl:if test="count($env-config/@mib_root) = 0"><xsl:value-of select="$server-root"/>/mibs</xsl:if>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count($env-config/@unixcommons_root) > 0">
  unix_commons_root=<xsl:value-of select="$env-config/@unixcommons_root"/>
    </xsl:when>
    <xsl:otherwise>
  unix_commons_root=<xsl:value-of select="$def-unixcommons-root"/>
    </xsl:otherwise>
  </xsl:choose>
  export unix_commons_root
  <xsl:variable name="devel-params" select="$app-config/cfg:develParams"/>
  <xsl:variable name="secure-params" select="$colo-config/cfg:secureParams"/>

  server_root=<xsl:value-of select="$server-root"/>
  server_bin_root=<xsl:value-of select="$server-bin-root"/>
  mib_root=<xsl:value-of select="$mib-root"/>
  config_root=<xsl:value-of select="$config-root"/>/<xsl:value-of select="$out-dir"/>
  cache_root=<xsl:value-of select="$cache-root"/>
  current_config_dir=$config_root/CurrentEnv
  workspace_root=<xsl:value-of select="$workspace-root"/>
  data_root=<xsl:value-of select="$data-root"/>
  log_root=$workspace_root/log
  <xsl:variable name="zenoss-enabled">
    <xsl:call-template name="GetZenOSSEnabled">
      <xsl:with-param name="app-xpath" select="$app-path"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:if test="$zenoss-enabled = 'true' or $zenoss-enabled = '1'">
  ZENOSS_DIR=<xsl:call-template name="ZenossFolder">
    <xsl:with-param name="app-xpath" select="$app-path"/>
    </xsl:call-template>
  export ZENOSS_DIR
  </xsl:if>

  export server_root
  export server_bin_root
  export config_root
  export cache_root
  export current_config_dir
  export workspace_root
  export data_root
  export log_root
  export TNS_ADMIN

  <xsl:if test="$devel-params/@valgrind">
    VALGRIND_PREFIX="valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes<xsl:if test="$devel-params/@suppression-file"> --suppressions=<xsl:value-of select="$devel-params/@suppression-file"/></xsl:if>"
    export VALGRIND_PREFIX
  </xsl:if>

  ps_data_root=<xsl:value-of select="$config-root"/>/www/PageSense
  export ps_data_root

  auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
  auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
  export auto_test_path

  PATH=$PATH:$server_root/bin
  PATH=$PATH:$server_root/build/bin
  PATH=$PATH:$server_root/ConfigSys:$auto_test_path
  PATH=$PATH:$server_root/Predictor/Scripts
  PATH=$PATH:$server_root/Predictor
  PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root
  export PATH

  PERL5LIB=$PERL5LIB:$server_root/bin
  PERL5LIB=$PERL5LIB:$server_root/DACS
  PERL5LIB=$PERL5LIB:$server_root/ConfigSys:$auto_test_path
  PERL5LIB=$PERL5LIB:$server_root/Predictor/Scripts
  PERL5LIB=$PERL5LIB:$server_root/Predictor
  export PERL5LIB

  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/pgsql-9.3/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/foros/vanga/lib

  export LD_LIBRARY_PATH

  NLS_LANG=.AL32UTF8
  export NLS_LANG
  NLS_NCHAR=AL32UTF8
  export NLS_NCHAR

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/.."/>
  <xsl:variable name="full-cluster-path" select="$xpath"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> Environment: Can't find full-cluster group (XPATH) </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
       <xsl:message terminate="yes"> Environment: Can't find colo config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="string-length($out-dir) = 0">
       <xsl:message terminate="yes"> Environment: Unexpected empty OUT_DIR_SUFFIX </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="EnvironmentConfigGenerator">
    <xsl:with-param name="app-path" select="$app-path"/>
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="colo-config" select="$colo-config"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>

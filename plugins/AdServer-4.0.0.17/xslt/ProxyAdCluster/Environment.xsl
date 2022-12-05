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

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<xsl:template name="EnvironmentConfigGenerator">
  <xsl:param name="app-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="devel-params"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-bin-root"><xsl:value-of select="$env-config/@server_bin_root"/>
    <xsl:if test="count($env-config/@server_bin_root) = 0"><xsl:value-of select="$server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="unix-commons-root"><xsl:value-of select="$env-config/@unixcommons_root"/>
    <xsl:if test="count($env-config/@unixcommons_root) = 0"><xsl:value-of select="$def-unixcommons-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="mib-root"><xsl:value-of select="$env-config/@mib_root"/>
    <xsl:if test="count($env-config/@mib_root) = 0"><xsl:value-of select="$server-root"/>/mibs</xsl:if>
  </xsl:variable>
  <xsl:variable name="secure-params" select="$colo-config/cfg:secureParams"/>

  export unix_commons_root=<xsl:value-of select="$unix-commons-root"/>

  export server_root=<xsl:value-of select="$server-root"/>
  export server_bin_root=<xsl:value-of select="$server-bin-root"/>
  export mib_root=<xsl:value-of select="$mib-root"/>
  export config_root=<xsl:value-of select="$config-root"/>/<xsl:value-of select="$out-dir"/>
  export data_root=<xsl:value-of select="$data-root"/>
  export current_config_dir=$config_root/CurrentEnv
  export workspace_root=<xsl:value-of select="$workspace-root"/>
  export log_root=$workspace_root/log

  <xsl:if test="$devel-params/@valgrind">
    export VALGRIND_PREFIX="valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes<xsl:if test="$devel-params/@suppression-file"> --suppressions=<xsl:value-of select="$devel-params/@suppression-file"/></xsl:if>"
  </xsl:if>

  export ps_data_root=$config_root/www/PageSense
  export dacs_root=$config_root/DACS

  export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
  export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts

  export PATH=$PATH:$server_bin_root
  export PATH=$PATH:$server_root/bin
  export PATH=$PATH:$server_root/build/bin
  export PATH=$PATH:$server_root/ConfigSys:$auto_test_path
  export PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root

  export PERL5LIB=$PERL5LIB:$server_root/bin
  export PERL5LIB=$PERL5LIB:$server_root/DACS
  export PERL5LIB=$PERL5LIB:$server_root/ConfigSys:$auto_test_path

  export PERL5LIB=$PERL5LIB:$config_root/<xsl:value-of select="$out-dir"/>/DACS
  export PERL5LIB=$PERL5LIB:$dacs_root

  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/perl5/5.8.8/x86_64-linux-thread-multi/CORE
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/foros/vanga/lib

  export NLS_LANG=.AL32UTF8
  export NLS_NCHAR=AL32UTF8

</xsl:template>

<!-- -->
<xsl:template match="/">

  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/.."/>
  <xsl:variable name="full-cluster-path" select="$xpath"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($full-cluster-path) = 0">
      <xsl:message terminate="yes"> Proxy Environment: Can't find full-cluster group (XPATH) </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="devel-params" select="$colo-config/cfg:develParams"/>


  <xsl:call-template name="EnvironmentConfigGenerator">
    <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="colo-config" select="$colo-config"/>
    <xsl:with-param name="devel-params" select="$devel-params"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!-- Test EnvParams config generate function -->
<xsl:template name="TestEnvParamsConfigGenerator">
  <xsl:param name="app-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="cluster-type"/>
  <xsl:param name="pg-connection"/>
  <xsl:param name="result-processing"/>
  <xsl:param name="auto-test-config"/>
  <xsl:param name="auto-test-path"/>
  <xsl:param name="root-path"/>
  <xsl:param name="taskbot"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count($env-config/@unixcommons_root) > 0">
export unix_commons_root=<xsl:value-of select="$env-config/@unixcommons_root"/>
    </xsl:when>
    <xsl:otherwise>
export unix_commons_root=<xsl:value-of select="$def-unixcommons-root"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:variable name="config-root"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="request-domain" select="$colo-config/cfg:coloParams/cfg:domain[1]/@name"/>
  <xsl:variable name="ssh-key">
    <xsl:call-template name="PrivateKeyFile">
      <xsl:with-param name="product-identifier" select="$PRODUCT_IDENTIFIER"/>
      <xsl:with-param name="app-env" select="$app-config/cfg:forosZoneManagement"/>
    </xsl:call-template>
  </xsl:variable>

<xsl:variable name="tested-port">
  <xsl:choose>
    <xsl:when test="count($colo-config/cfg:coloParams/@external_http_port) > 0">
      <xsl:value-of select="$colo-config/cfg:coloParams/@external_http_port"/>
    </xsl:when>
    <xsl:otherwise>80</xsl:otherwise>
  </xsl:choose>
</xsl:variable>

export tested_port=<xsl:value-of select="$tested-port"/>

export tested_host=<xsl:value-of select="$request-domain"/>
export server_root=<xsl:value-of select="$server-root"/>
export workspace_root=<xsl:value-of select="$workspace-root"/>
export config_root=<xsl:value-of select="$config-root"/>
export test_config_root=$workspace_root/run/Tests
export test_log_path=$workspace_root/log/Tests

export cluster_type=<xsl:value-of select="$cluster-type"/>
export adserver_ssh_identity=<xsl:value-of select="$ssh-key"/>

<xsl:if test="count($colo-config/cfg:develParams) > 0">
<xsl:if test="$colo-config/cfg:develParams = 'true'">
export vg_report_suffix = "valgrind"
</xsl:if>
</xsl:if>

export PATH=$PATH:$server_root/bin
export PATH=$PATH:$server_root/build/bin
export PATH=$PATH:$server_root:$server_root/tests/AutoTests/PerlUtils
export PATH=$PATH:$server_root/lib/utils/tests/AutoTests/PerlUtils
export PATH=$PATH:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
export PATH=$PATH:$server_root/lib/utils/tests/AutoTests/PerlUtils/NightlyScripts
export PATH=$PATH:$server_root/tests/PerformanceTests/PerlUtils
export PATH=$PATH:$server_root/lib/utils/tests/PerformanceTests/PerlUtils
export PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/perl5/5.8.8/x86_64-linux-thread-multi/CORE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/foros/vanga/lib

export NLS_LANG=.AL32UTF8
export NLS_NCHAR=AL32UTF8


export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
export auto_test_path=$auto_test_path:$server_root/lib/utils/tests/AutoTests/PerlUtils
export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
export auto_test_path=$auto_test_path:$server_root/lib/utils/tests/AutoTests/PerlUtils/NightlyScripts

export test_commons=$server_root/tests/Commons/Perl
export test_commons=$test_commons:$server_root/lib/utils/tests/Commons/Perl

export performance_test_path=$server_root/tests/PerformanceTests/PerlUtils
export performance_test_path=$performance_test_path:$server_root/lib/utils/tests/PerformanceTests/PerlUtils

export PERL5LIB=$PERL5LIB:$test_commons:$auto_test_path:$performance_test_path:$server_root/DACS

export PYTHONPATH=$PYTHONPATH:$server_root/lib/utils/PyCommons
export pg_connection_string="<xsl:value-of select="$pg-connection/@connection_string"/>"

<xsl:if test="count($result-processing) > 0">

export url=<xsl:value-of select="$result-processing/@url"/>
export http_root=<xsl:value-of select="$result-processing/@http-root"/>
export http_test_path=<xsl:value-of select="$result-processing/@http-test-path"/>
  <xsl:if test="count($result-processing/cfg:mailList) > 0">

export mail_list=<xsl:value-of select="$result-processing/cfg:mailList"/>
  </xsl:if>

  <xsl:if test="count($result-processing/@history-path) > 0">
export history_path=<xsl:value-of select="$result-processing/@history-path"/>
  </xsl:if>

  <xsl:if test="count($result-processing/@dst-sub-path) > 0">
export dst_sub_path=<xsl:value-of select="$result-processing/@dst-sub-path"/>
  </xsl:if>

</xsl:if>

export adserver_host=<xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="$auto-test-path/@host"/>
    </xsl:call-template>
export server_start_prefix="<xsl:value-of select="$auto-test-config/@server-start-prefix"/>"

export colocation_name=<xsl:value-of select="$colo-name"/>

<xsl:variable name="hosts">
<xsl:for-each select="$root-path/host">
  <xsl:variable name="current-host">
    <xsl:call-template name="ResolveHostName">
      <xsl:with-param name="base-host" select="@name"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="concat($current-host, ' ')"/>
</xsl:for-each>
</xsl:variable>

export nb_hosts="<xsl:value-of select="normalize-space($hosts)"/>"

<xsl:if test="count($taskbot/@root-path) > 0">
export taskbot_root=<xsl:value-of select="$taskbot/@root-path"/>
</xsl:if>

</xsl:template>

<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/../../.."/>
  <xsl:variable name="auto-test-path" select="$xpath"/>

  <xsl:variable
    name="full-cluster-path"
    select="$xpath/../.."/>

  <xsl:variable
    name="root-path"
    select="$xpath/../../../.."/>


  <xsl:variable
    name="fe-cluster-path"
    select="$full-cluster-path/serviceGroup[@descriptor = $fe-cluster-descriptor][1]"/>

  <!-- check pathes -->
  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
       <xsl:message terminate="yes"> Tests::EnvParams: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
       <xsl:message terminate="yes"> Tests::EnvParams: Can't find full-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
       <xsl:message terminate="yes"> Tests::EnvParams: Can't find fe-cluster group </xsl:message>
    </xsl:when>
  </xsl:choose>

  <!-- find config sections -->
  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>

  <xsl:variable
    name="central-colo-config"
    select="$full-cluster-path/configuration/cfg:cluster/cfg:central"/>

  <xsl:variable
    name="remote-colo-config"
    select="$full-cluster-path/configuration/cfg:cluster/cfg:remote"/>

  <xsl:variable name="cluster-type">
    <xsl:choose>
      <xsl:when test="count($remote-colo-config) > 0">remote</xsl:when>
      <xsl:when test="count($central-colo-config) > 0">central</xsl:when>
      <xsl:otherwise>unknown</xsl:otherwise>
    </xsl:choose>
 </xsl:variable>

  <xsl:variable
    name="auto-test-config"
    select="$auto-test-path/configuration/cfg:autoTest"/>

  <xsl:variable
    name="test-global-config"
    select="$auto-test-path/../configuration/cfg:testsCommon"/>

  <xsl:variable name="pg-connection"
    select="$auto-test-config/cfg:pgConnection | $test-global-config/cfg:pgConnection"/>

  <xsl:variable name="result-processing" select="$test-global-config/cfg:testResultProcessing"/>

  <xsl:variable name="taskbot" select="$test-global-config/cfg:taskbot"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
      <xsl:message terminate="yes"> Tests::EnvParams: Can't find colo config </xsl:message>
    </xsl:when>

    <xsl:when test="count($pg-connection) = 0">
      <xsl:message terminate="yes"> Tests::EnvParams: Can't find pg connection </xsl:message>
    </xsl:when>

  </xsl:choose>

  <xsl:call-template name="TestEnvParamsConfigGenerator">
    <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="colo-config" select="$colo-config"/>
    <xsl:with-param name="cluster-type" select="$cluster-type"/>
    <xsl:with-param name="pg-connection" select="$pg-connection"/>
    <xsl:with-param name="result-processing" select="$result-processing"/>
    <xsl:with-param name="auto-test-config" select="$auto-test-config"/>
    <xsl:with-param name="auto-test-path" select="$auto-test-path"/>
    <xsl:with-param name="root-path" select="$root-path"/>
    <xsl:with-param name="taskbot" select="$taskbot"/>
  </xsl:call-template>

</xsl:template>

</xsl:stylesheet>

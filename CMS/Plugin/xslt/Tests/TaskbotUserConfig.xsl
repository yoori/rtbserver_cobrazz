<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl">


<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
# -*- cperl -*-
<xsl:variable name="colo-config" select="//serviceGroup[@descriptor = $ad-cluster-descriptor][1]/configuration/cfg:cluster"/>
<xsl:variable name="app-config" select="//configuration/cfg:environment"/>
<xsl:variable name="user-name"><xsl:value-of select="$app-config/cfg:forosZoneManagement/@user_name"/>
  <xsl:if test="count($app-config/cfg:forosZoneManagement/@user_name) = 0"><xsl:value-of select="$def-user-name"/></xsl:if>
</xsl:variable>
<xsl:variable
    name="taskbot-config"
    select="$xpath/configuration/cfg:testsCommon/cfg:taskbot"/>
<xsl:variable name="taskbot-host"><xsl:value-of select="$taskbot-config/@db-host"/>
  <xsl:if test="count($taskbot-config/@db-host) = 0">
    <xsl:value-of select="$default-taskbot-db-host"/>
  </xsl:if>
</xsl:variable>


my $taskbot_share = "$ENV{server_root}/tests/taskbot";

my %config = (
   taskbot_dbi => ["DBI:mysql:"
                   . ";mysql_read_default_file=$ENV{config_root}/TestConfig/AutoTests/mysql.cnf"
                   . ";mysql_read_default_group=taskbot"],
   sh => '/bin/sh',
   gzip_threshold => 128,
   gzip_ratio => 0.9,
   max_output_size => 3 * 1024 * 1024,
   environment => { TASKBOT_USER => '<xsl:value-of select="$user-name"/>',
                    TASKBOT_HOST => '<xsl:value-of select="$taskbot-host"/>',
                  },
);

\%config;

</xsl:template>

</xsl:stylesheet>

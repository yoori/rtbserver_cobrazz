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

<xsl:output method="text" indent="no" encoding="utf-8"/>
<xsl:include href="../Functions.xsl"/>
<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:template match="/">
<xsl:variable
    name="taskbot-config"
    select="$xpath/configuration/cfg:testsCommon/cfg:taskbot"/>
<xsl:variable name="db-host"><xsl:value-of select="$taskbot-config/@db-host"/>
  <xsl:if test="count($taskbot-config/@db-host) = 0">
    <xsl:value-of select="$default-taskbot-db-host"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="db-name"><xsl:value-of select="$taskbot-config/@db-name"/>
  <xsl:if test="count($taskbot-config/@db-name) = 0">
    <xsl:value-of select="$default-taskbot-db-name"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="db-user"><xsl:value-of select="$taskbot-config/@db-user"/>
  <xsl:if test="count($taskbot-config/@db-user) = 0">
    <xsl:value-of select="$default-taskbot-db-user"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="db-password"><xsl:value-of select="$taskbot-config/@db-password"/>
  <xsl:if test="count($taskbot-config/@db-password) = 0">
    <xsl:value-of select="$default-taskbot-db-password"/>
  </xsl:if>
</xsl:variable>
[taskbot]
host=<xsl:value-of select="$db-host"/>
database=<xsl:value-of select="$db-name"/>
user=<xsl:value-of select="$db-user"/>
password=<xsl:value-of select="$db-password"/>
</xsl:template>
</xsl:stylesheet>

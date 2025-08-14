<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
        version="1.0"
        xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
        xmlns:dyn="http://exslt.org/dynamic"
        exclude-result-prefixes="dyn"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
        xmlns:colo="http://www.foros.com/cms/colocation">

  <xsl:output method="text" indent="no" encoding="utf-8"/>

  <xsl:include href="../Variables.xsl"/>

  <xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="out-dir" select="$OUT_DIR"/>

  <xsl:template match="/">

    <xsl:variable name="segment-loader-path" select="$xpath"/>
    <xsl:variable name="service-config" select="$segment-loader-path/configuration/cfg:SegmentLoader"/>

    <xsl:variable name="pid-file-path">
      <xsl:value-of select="$service-config/cfg:pidFilePath"/>
      <xsl:if test="count($service-config/cfg:pidFilePath) = 0">SegmentLoader.pid</xsl:if>
    </xsl:variable>

    <xsl:variable name="loglevel">
      <xsl:value-of select="$service-config/cfg:logging/cfg:loglevel"/>
      <xsl:if test="count($service-config/cfg:logging/cfg:loglevel) = 0">Info</xsl:if>
    </xsl:variable>

    <xsl:variable name="yandex-account-id">
      <xsl:value-of select="$service-config/cfg:yandex/cfg:account_id"/>
      <xsl:if test="count($service-config/cfg:yandex/cfg:account_id) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="yandex-gpt-api-key">
      <xsl:value-of select="$service-config/cfg:yandex/cfg:gpt_api_key"/>
      <xsl:if test="count($service-config/cfg:yandex/cfg:gpt_api_key) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="db-name">
      <xsl:value-of select="$service-config/cfg:database/cfg:name"/>
      <xsl:if test="count($service-config/cfg:database/cfg:name) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="db-user">
      <xsl:value-of select="$service-config/cfg:database/cfg:user"/>
      <xsl:if test="count($service-config/cfg:database/cfg:user) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="db-password">
      <xsl:value-of select="$service-config/cfg:database/cfg:password"/>
      <xsl:if test="count($service-config/cfg:database/cfg:password) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="db-host">
      <xsl:value-of select="$service-config/cfg:database/cfg:host"/>
      <xsl:if test="count($service-config/cfg:database/cfg:host) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="db-port">
      <xsl:value-of select="$service-config/cfg:database/cfg:port"/>
      <xsl:if test="count($service-config/cfg:database/cfg:port) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="account-id">
      <xsl:value-of select="$service-config/cfg:account_id"/>
      <xsl:if test="count($service-config/cfg:account_id) = 0">...</xsl:if>
    </xsl:variable>

    <xsl:variable name="prefix">
      <xsl:value-of select="$service-config/cfg:gpt/cfg:prefix"/>
      <xsl:if test="count($service-config/cfg:gpt/cfg:prefix) = 0">Taxonomy.ChatGPT.</xsl:if>
    </xsl:variable>

    <xsl:variable name="interval">
      <xsl:value-of select="$service-config/cfg:interval"/>
      <xsl:if test="count($service-config/cfg:interval) = 0">1d</xsl:if>
    </xsl:variable>

    <xsl:variable name="statement-timeout">
      <xsl:value-of select="$service-config/cfg:statement_timeout"/>
      <xsl:if test="count($service-config/cfg:statement_timeout) = 0">5000</xsl:if>
    </xsl:variable>

    <xsl:variable name="check-days">
      <xsl:value-of select="$service-config/cfg:checkDays"/>
      <xsl:if test="count($service-config/cfg:checkDays) = 0">3</xsl:if>
    </xsl:variable>

    <xsl:variable name="chunk-size">
      <xsl:value-of select="$service-config/cfg:chunkSize"/>
      <xsl:if test="count($service-config/cfg:chunkSize) = 0">1000</xsl:if>
    </xsl:variable>

    <xsl:variable name="gpt-dir">
      <xsl:value-of select="$service-config/cfg:gpt/cfg:gptdir"/>
      <xsl:if test="count($service-config/cfg:gpt/cfg:gptdir) = 0">GPTresults</xsl:if>
    </xsl:variable>

    <xsl:variable name="gpt-file">
      <xsl:value-of select="$service-config/cfg:gpt/cfg:gptFile"/>
      <xsl:if test="count($service-config/cfg:gpt/cfg:gptFile) = 0">GPTresults.json</xsl:if>
    </xsl:variable>

    <xsl:variable name="websites-dir">
      <xsl:value-of select="$service-config/cfg:websitesdir"/>
      <xsl:if test="count($service-config/cfg:websitesdir) = 0">websites</xsl:if>
    </xsl:variable>

    <xsl:variable name="store-sites">
      <xsl:value-of select="$service-config/cfg:storeSites"/>
      <xsl:if test="count($service-config/cfg:storeSites) = 0">true</xsl:if>
    </xsl:variable>

    <xsl:variable name="store-gpt">
      <xsl:value-of select="$service-config/cfg:storeGpt"/>
      <xsl:if test="count($service-config/cfg:storeGpt) = 0">true</xsl:if>
    </xsl:variable>

    <xsl:variable name="check-timeout">
      <xsl:value-of select="$service-config/cfg:checkTimeout"/>
      <xsl:if test="count($service-config/cfg:checkTimeout) = 0">300</xsl:if>
    </xsl:variable>

    <xsl:variable name="exp-time-date">
      <xsl:value-of select="$service-config/cfg:expTimeDate"/>
      <xsl:if test="count($service-config/cfg:expTimeDate) = 0">60</xsl:if>
    </xsl:variable>

    <xsl:variable name="path-gpt">
      <xsl:value-of select="$service-config/cfg:gpt/cfg:pathGPT"/>
      <xsl:if test="count($service-config/cfg:gpt/cfg:pathGPT) = 0">Utils/GPT/getSiteCategories.py</xsl:if>
    </xsl:variable>

    <xsl:variable name="attempts">
      <xsl:value-of select="$service-config/cfg:attempts"/>
      <xsl:if test="count($service-config/cfg:attempts) = 0">3</xsl:if>
    </xsl:variable>

    <xsl:variable name="message-size">
      <xsl:value-of select="$service-config/cfg:messagesize"/>
      <xsl:if test="count($service-config/cfg:messagesize) = 0">300</xsl:if>
    </xsl:variable>
    {
      "pidFilePath": "<xsl:value-of select="$pid-file-path"/>",
      "loglevel": "<xsl:value-of select="$loglevel"/>",

      "yandex_account_id": "<xsl:value-of select="$yandex-account-id"/>",
      "yandex_gpt_api_key": "<xsl:value-of select="$yandex-gpt-api-key"/>",

      "db_name": "<xsl:value-of select="$db-name"/>",
      "db_user": "<xsl:value-of select="$db-user"/>",
      "db_password": "<xsl:value-of select="$db-password"/>",
      "db_host": "<xsl:value-of select="$db-host"/>",
      "db_port": "<xsl:value-of select="$db-port"/>",

      "account_id": "<xsl:value-of select="$account-id"/>",

      "prefix": "<xsl:value-of select="$prefix"/>",
      "interval": "<xsl:value-of select="$interval"/>",
      "statement_timeout": <xsl:value-of select="$statement-timeout"/>,
      "checkDays": <xsl:value-of select="$check-days"/>,
      "chunkSize": <xsl:value-of select="$chunk-size"/>,
      "gptdir": "<xsl:value-of select="$gpt-dir"/>",
      "gptFile": "<xsl:value-of select="$gpt-file"/>",
      "websitesdir": "<xsl:value-of select="$websites-dir"/>",
      "storeSites": <xsl:value-of select="$store-sites"/>,
      "storeGpt": <xsl:value-of select="$store-gpt"/>,
      "checkTimeout": <xsl:value-of select="$check-timeout"/>,
      "expTimeDate": <xsl:value-of select="$exp-time-date"/>,
      "pathGPT": "<xsl:value-of select="$path-gpt"/>",
      "attempts": <xsl:value-of select="$attempts"/>,
      "messagesize": <xsl:value-of select="$message-size"/>
    }
  </xsl:template>
</xsl:stylesheet>
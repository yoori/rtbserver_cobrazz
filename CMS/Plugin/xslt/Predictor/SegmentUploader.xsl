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

<xsl:output method="text" indent="yes" omit-xml-declaration="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<xsl:template name="SegmentUploaderConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="segment-uploader-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="ulimit_files" select="$segment-uploader-ulimit-files"/>
  <xsl:variable name="period" select="$segment-uploader-period"/>
  <xsl:variable name="upload_wait_time" select="$segment-uploader-upload-wait-time"/>
  <xsl:variable name="upload_threads" select="$segment-uploader-upload-threads"/>
  <xsl:variable name="pg_host" select="$segment-uploader-pg-connection-host"/>
  <xsl:variable name="pg_dbname" select="$segment-uploader-pg-connection-dbname"/>
  <xsl:variable name="pg_user" select="$segment-uploader-pg-connection-user"/>
  <xsl:variable name="pg_password" select="$segment-uploader-pg-connection-password"/>  
  <xsl:variable name="http_host" select="$segment-uploader-http-host"/>
  <xsl:variable name="http_port" select="$segment-uploader-http-port"/>
  <xsl:variable name="account_id" select="$segment-uploader-account-id"/>
  <xsl:variable name="pid_file" select="$segment-uploader-pid-file"/>
  <xsl:variable name="private_key_file" select="$segment-uploader-private-key-file"/>
  <xsl:variable name="log_dir" select="concat($workspace-root, $segment-uploader-log-directory)"/>
  <xsl:variable name="workspace_dir" select="concat($workspace-root, $segment-uploader-workspace-directory)"/>
  <xsl:variable name="url_segments_dir" select="concat($workspace-root, $segment-uploader-url-segments-directory)"/>
  <xsl:variable name="upload_url" select="$segment-uploader-upload-url"/>
  <xsl:variable name="in_dir" select="concat($workspace-root, $segment-uploader-input-directory)"/>
  <xsl:variable name="format" select="$segment-uploader-format"/>
  <xsl:variable name="name" select="$segment-uploader-name"/>
  <xsl:variable name="channel_prefix" select="$segment-uploader-channel-prefix"/>

  <xsl:text>{</xsl:text>

  <xsl:text>
  "ulimit_files" : </xsl:text>
  <xsl:value-of select="$ulimit_files"/>
  <xsl:text>,</xsl:text>
  
  <xsl:text>
  "period" : </xsl:text>
  <xsl:value-of select="$period"/>
  <xsl:text>,</xsl:text>

  <xsl:text>
  "upload_wait_time" : </xsl:text>
  <xsl:value-of select="$upload_wait_time"/>
  <xsl:text>,</xsl:text>

  <xsl:text>
  "upload_threads" : </xsl:text>
  <xsl:value-of select="$upload_threads"/>
  <xsl:text>,</xsl:text>

  <xsl:text>
  "pg_host" : "</xsl:text>
  <xsl:value-of select="$pg_host"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "pg_db" : "</xsl:text>
  <xsl:value-of select="$pg_dbname"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "pg_user" : "</xsl:text>
  <xsl:value-of select="$pg_user"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "pg_pass" : "</xsl:text>
  <xsl:value-of select="$pg_password"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "http_host" : "</xsl:text>
  <xsl:value-of select="$http_host"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "http_port" : "</xsl:text>
  <xsl:value-of select="$http_port"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "account_id" : "</xsl:text>
  <xsl:value-of select="$account_id"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "pid_file" : "</xsl:text>
  <xsl:value-of select="$pid_file"/>
  <xsl:text>",</xsl:text>
  
  <xsl:text>
  "private_key_file" : "</xsl:text>
  <xsl:value-of select="$private_key_file"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "log_dir" : "</xsl:text>
  <xsl:value-of select="$log_dir"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "workspace_dir" : "</xsl:text>
  <xsl:value-of select="$workspace_dir"/>
  <xsl:text>",</xsl:text>
  
  <xsl:text>
  "url_segments_dir" : "</xsl:text>
  <xsl:value-of select="$url_segments_dir"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "upload_url" : ["</xsl:text>
  <xsl:value-of select="$upload_url"/>
  <xsl:text>"],</xsl:text>

  <xsl:text>
  "in_dirs" : [</xsl:text>

    <xsl:text>
    {</xsl:text>

      <xsl:text>
      "in_dir" : "</xsl:text>
      <xsl:value-of select="$in_dir"/>
      <xsl:text>",</xsl:text>

      <xsl:text>
      "format" : "</xsl:text>
      <xsl:value-of select="$format"/>
      <xsl:text>",</xsl:text>

      <xsl:text>
      "name" : "</xsl:text>
      <xsl:value-of select="$name"/>
      <xsl:text>",</xsl:text>

      <xsl:text>
      "channel_prefix" : "</xsl:text>
      <xsl:value-of select="$channel_prefix"/>
      <xsl:text>"</xsl:text>

  <xsl:text>
    }</xsl:text>
      
  <xsl:text>
  ]</xsl:text>

  <xsl:text>&#xa;}</xsl:text>

</xsl:template>


<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="segment-uploader-path" select="$xpath"/>
  <xsl:variable name="colo-config" select="$cluster-path/configuration/cfg:cluster"/>
  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="segment-uploader-config" select="$segment-uploader-path/configuration/cfg:segmentuploader"/>

  <xsl:call-template name="SegmentUploaderConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="segment-uploader-config" select="$segment-uploader-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

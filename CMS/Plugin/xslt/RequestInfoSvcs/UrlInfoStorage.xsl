<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  extension-element-prefixes="exsl" exclude-result-prefixes="dyn xsd">
  <xsl:include href="../Functions.xsl"/>
  <xsl:output method="xml" indent="yes" encoding="utf-8"/>
  <xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

  <xsl:template match="/">
    <xsl:variable name="services"
      select="$xpath/../..//service[@descriptor = $url-info-storage-descriptor]"/>
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="$xpath/@host"/>
        <xsl:with-param name="error-prefix" select="'UrlInfoStorage'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="count($xpath) != 1 or count($services) != 1 or
      count(exsl:node-set($hosts)/host) != 1">
      <xsl:message terminate="yes">UrlInfoStorage requires one service on one host.</xsl:message>
    </xsl:if>

    <xsl:variable name="service" select="$xpath/configuration/cfg:urlInfoStorage"/>
    <xsl:variable name="env" select=
      "$xpath/../configuration/cfg:backendCluster/cfg:environment |
       $xpath/../../configuration/cfg:cluster/cfg:environment"/>
    <xsl:variable name="workspace-root">
      <xsl:choose>
        <xsl:when test="$env/@workspace_root">
          <xsl:value-of select="$env/@workspace_root"/>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="$def-workspace-root"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="cache-root">
      <xsl:choose>
        <xsl:when test="$env/@cache_root"><xsl:value-of select="$env/@cache_root"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$def-cache-root"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="server-root">
      <xsl:choose>
        <xsl:when test="$env/@server_root"><xsl:value-of select="$env/@server_root"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="$def-server-root"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="grpc-port">
      <xsl:choose>
        <xsl:when test="$service/cfg:networkParams/@grpc_port">
          <xsl:value-of select="$service/cfg:networkParams/@grpc_port"/>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="$def-url-info-storage-grpc-port"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="http-port">
      <xsl:choose>
        <xsl:when test="$service/cfg:networkParams/@http_port">
          <xsl:value-of select="$service/cfg:networkParams/@http_port"/>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="$def-url-info-storage-http-port"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="defaults" select=
      "document('../../xsd/RequestInfoSvcs/UrlInfoStorageApp.xsd')/xsd:schema/
       xsd:complexType[@name = 'UrlInfoStorageConfigType']/xsd:attribute"/>

    <exsl:document href="urlInfoStorage.port" method="text" omit-xml-declaration="yes">
      <xsl:text>  ['urlInfoStorageGrpc', </xsl:text><xsl:value-of select="$grpc-port"/>
      <xsl:text>],&#10;  ['urlInfoStorageHttp', </xsl:text><xsl:value-of select="$http-port"/>
      <xsl:text>],&#10;</xsl:text>
    </exsl:document>
    <cfg:AdConfiguration
      xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration
      {$server-root}/xsd/RequestInfoSvcs/UrlInfoStorageConfig.xsd">
      <cfg:UrlInfoStorageConfig pid_file="{$workspace-root}/run/UrlInfoStorage.pid"
        storage_path="{$cache-root}/UrlInfoStorage">
        <xsl:for-each select="$defaults[starts-with(@name, 'rocksdb_')]">
          <xsl:call-template name="StorageAttribute">
            <xsl:with-param name="service" select="$service"/>
          </xsl:call-template>
        </xsl:for-each>
        <cfg:GrpcConfig>
          <xsl:for-each select="$defaults[starts-with(@name, 'grpc_')]">
            <xsl:call-template name="StorageAttribute">
              <xsl:with-param name="service" select="$service"/>
              <xsl:with-param name="name" select="substring-after(@name, 'grpc_')"/>
            </xsl:call-template>
          </xsl:for-each>
          <cfg:Endpoint host="0.0.0.0" port="{$grpc-port}"/>
        </cfg:GrpcConfig>
        <cfg:HttpConfig>
          <xsl:for-each select="$defaults[@name = 'http_threads']">
            <xsl:call-template name="StorageAttribute">
              <xsl:with-param name="service" select="$service"/>
              <xsl:with-param name="name" select="'process_threads'"/>
            </xsl:call-template>
          </xsl:for-each>
          <cfg:Endpoint host="0.0.0.0" port="{$http-port}"/>
        </cfg:HttpConfig>
        <xsl:call-template name="ConvertLogger">
          <xsl:with-param name="logger-node" select="$service/cfg:logging"/>
          <xsl:with-param name="log-file"
            select="concat($workspace-root, '/log/UrlInfoStorage/UrlInfoStorage')"/>
          <xsl:with-param name="default-log-level" select="$default-log-level"/>
        </xsl:call-template>
      </cfg:UrlInfoStorageConfig>
    </cfg:AdConfiguration>
  </xsl:template>

  <xsl:template name="StorageAttribute">
    <xsl:param name="service"/>
    <xsl:param name="name" select="@name"/>
    <xsl:variable name="source-name" select="@name"/>
    <xsl:attribute name="{$name}">
      <xsl:choose>
        <xsl:when test="$service/@*[name() = $source-name]">
          <xsl:value-of select="$service/@*[name() = $source-name]"/>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="@default"/></xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
  </xsl:template>
</xsl:stylesheet>

<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:param name="HOST"/>
  <xsl:variable name="service" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="config" select="$service/configuration/cfg:aiAgent"/>

  <xsl:template match="/">
AI_MODEL=<xsl:choose><xsl:when test="$config/@model"><xsl:value-of select="$config/@model"/></xsl:when><xsl:otherwise>qwen3</xsl:otherwise></xsl:choose>
AI_MODEL_DIR=<xsl:choose><xsl:when test="$config/@model_dir"><xsl:value-of select="$config/@model_dir"/></xsl:when><xsl:otherwise>/u01/foros/server-ai/models/ollama/qwen3</xsl:otherwise></xsl:choose>
AI_OLLAMA_BIN=<xsl:choose><xsl:when test="$config/@ollama_binary"><xsl:value-of select="$config/@ollama_binary"/></xsl:when><xsl:otherwise>/opt/foros/ollama/bin/ollama</xsl:otherwise></xsl:choose>
AI_LISTEN_HOST=<xsl:choose><xsl:when test="$config/@listen_host"><xsl:value-of select="$config/@listen_host"/></xsl:when><xsl:otherwise>0.0.0.0</xsl:otherwise></xsl:choose>
AI_PORT=<xsl:choose><xsl:when test="$config/@port"><xsl:value-of select="$config/@port"/></xsl:when><xsl:otherwise>11434</xsl:otherwise></xsl:choose>
export AI_MODEL AI_MODEL_DIR AI_OLLAMA_BIN AI_LISTEN_HOST AI_PORT
  </xsl:template>
</xsl:stylesheet>

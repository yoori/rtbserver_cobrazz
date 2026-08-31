<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:param name="COLOCATION_NAME"/>
  <xsl:variable name="cluster" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="environment" select="$cluster/configuration/cfg:cluster/cfg:environment"/>

  <xsl:template match="/">
    <xsl:variable name="workspace-root"><xsl:choose>
      <xsl:when test="$environment/@workspace_root"><xsl:value-of select="$environment/@workspace_root"/></xsl:when>
      <xsl:otherwise>/u01/foros/server-ai/var</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="config-root"><xsl:choose>
      <xsl:when test="$environment/@config_root"><xsl:value-of select="$environment/@config_root"/></xsl:when>
      <xsl:otherwise>/opt/foros/server-ai/etc</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="server-root"><xsl:choose>
      <xsl:when test="$environment/@server_root"><xsl:value-of select="$environment/@server_root"/></xsl:when>
      <xsl:otherwise>/opt/foros/server-ai</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="server-bin-root"><xsl:choose>
      <xsl:when test="$environment/@server_bin_root"><xsl:value-of select="$environment/@server_bin_root"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$server-root"/>/bin</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="dacs-root"><xsl:choose>
      <xsl:when test="$environment/@dacs_root"><xsl:value-of select="$environment/@dacs_root"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$server-root"/>/DACS</xsl:otherwise>
    </xsl:choose></xsl:variable>
server_root=<xsl:value-of select="$server-root"/>
server_bin_root=<xsl:value-of select="$server-bin-root"/>
config_root=<xsl:value-of select="$config-root"/>/<xsl:value-of select="$COLOCATION_NAME"/>/aicluster
workspace_root=<xsl:value-of select="$workspace-root"/>
log_root=$workspace_root/log
PATH=$PATH:$server_bin_root
PERL5LIB=$PERL5LIB:<xsl:value-of select="$dacs-root"/>
PYTHONPATH=/u01/foros/server-ai/python3.12-torch/site-packages${PYTHONPATH:+:$PYTHONPATH}
export server_root server_bin_root config_root workspace_root log_root PATH PERL5LIB PYTHONPATH
<xsl:if test="$environment/@cuda_visible_devices">CUDA_VISIBLE_DEVICES=<xsl:value-of select="$environment/@cuda_visible_devices"/>
export CUDA_VISIBLE_DEVICES
</xsl:if>
  </xsl:template>
</xsl:stylesheet>

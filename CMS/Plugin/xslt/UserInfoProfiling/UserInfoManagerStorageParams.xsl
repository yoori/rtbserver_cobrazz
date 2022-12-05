<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration">

<!-- Defines configuration of UserInfoManager container -->
<xsl:template name="UserInfoManagerStorageParams">
  <xsl:param name="cache-root"/>
  <xsl:param name="cluster-id"/>
  <xsl:param name="colo-config"/>

  <xsl:element name="chunks-root">
    <xsl:value-of select="concat($cache-root, '/Users', $cluster-id, '/')"/>
  </xsl:element>

  <xsl:element name="chunks-count">
    <xsl:value-of select="$colo-config/cfg:userProfiling/@chunks_count"/>
    <xsl:if test="count($colo-config/cfg:userProfiling/@chunks_count) = 0">
      <xsl:value-of select="$user-info-manager-scale-chunks"/>
    </xsl:if>
  </xsl:element>

  <rw-buffer-size>10485760</rw-buffer-size>
  <rwlevel-max-size>104857600</rwlevel-max-size>
  <max-undumped-size>262144000</max-undumped-size>
  <max-levels0>20</max-levels0>

</xsl:template>

</xsl:stylesheet>
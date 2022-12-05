<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
>

<xsl:template name="ExpressionMatcherServices">
  <xsl:param name="tag"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="cache-root"/>
  <xsl:param name="all-hosts"/>
  <xsl:param name="ssh-key"/>
  <xsl:param name="config-root"/>
  <xsl:param name="colo-name"/>
  <xsl:param name="sync-logs-hosts"/>
  <xsl:param name="workspace-root"/>
  <xsl:param name="chunk-configurator"/>
  
  <xsl:variable name="distrib-count"><xsl:value-of select="$colo-config/cfg:inventoryStats/@distrib_count"/>
    <xsl:if test="count($colo-config/cfg:inventoryStats/@distrib_count) = 0">
      <xsl:value-of select="$default-distrib-count"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="em-configurator-args"> --chunks-count=<xsl:value-of
    select="$distrib-count"/> --target-hosts='<xsl:for-each 
    select="exsl:node-set($all-hosts)//host"> <xsl:value-of
    select="."/>,</xsl:for-each>' --modifier 'ExpressionMatcher::Modifier' --chunks-root='<xsl:value-of
    select="concat($cache-root, '/ExpressionMatcher')"/>' --transport='ssh:<xsl:value-of
    select="$ssh-key"/>' --environment='<xsl:value-of select="$config-root"/>/<xsl:value-of
    select="$colo-name"/>/adcluster/environment.sh' </xsl:variable>

  <xsl:variable name="em-distribution-file"
    select="'/tmp/ExpressionMatcherDistribution.xml.%%SMS_UNIQUE%%'"/>

  <!-- propagate distribution to all unique sync logs service hosts -->
  <xsl:variable name="em-distribution-file-propagate-command">
    <xsl:for-each select="exsl:node-set($sync-logs-hosts)//host[not(. = preceding-sibling::host/.)]"
      > &amp;&amp; ssh -i '<xsl:value-of select="$ssh-key"/>' '<xsl:value-of select="."/>' "mkdir -p <xsl:value-of
      select="$workspace-root"/>/run/" &amp;&amp; rsync -e 'ssh -i <xsl:value-of
      select="$ssh-key"/>' <xsl:value-of
      select="$em-distribution-file"/> <xsl:value-of select="concat(' ', .)"/>:<xsl:value-of
      select="$workspace-root"/>/run/ExpressionMatcherDistribution.xml</xsl:for-each>
  </xsl:variable>

  <Service type="AdServer::LogProcessing::ExpressionMatcherChecker"
    host="localhost">
    <xsl:attribute name="name"><xsl:value-of select="'lp-ExpressionMatcherChecker'"/></xsl:attribute>
    <xsl:attribute name="start_cmd"><xsl:value-of select="$chunk-configurator"/> check <xsl:value-of
    select="$em-configurator-args"/> --xml-out=<xsl:value-of select="$em-distribution-file"/>
    <xsl:value-of select="concat(' ', $em-distribution-file-propagate-command)"
    /> || ( EX=$? ; if [ $EX -eq 1 ] ; then eval 'echo -e %%SMS_COLOR_ERROR%%redistribution required%%SMS_COLOR_PLAIN%%' &amp;&amp; exit 1 ; else exit -1 ; fi ; )</xsl:attribute>
  </Service>
  <Service type="AdServer::LogProcessing::ExpressionMatcherDistributor"
    host="localhost">
    <xsl:attribute name="name"><xsl:value-of select="'lp-ExpressionMatcherDistributor'"/></xsl:attribute>
    <xsl:attribute name="start_cmd"><xsl:value-of select="$chunk-configurator"/> reconf <xsl:value-of
    select="$em-configurator-args"/> --force=%%force%% --check-hosts='<xsl:for-each
    select="exsl:node-set($all-hosts)//host">
    <xsl:value-of select="."/>,</xsl:for-each>%%check-hosts%%'</xsl:attribute>
  </Service>

</xsl:template>

</xsl:stylesheet>
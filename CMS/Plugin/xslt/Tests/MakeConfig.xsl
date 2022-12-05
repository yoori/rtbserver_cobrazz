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

<xsl:variable name="colo-config" select="//serviceGroup[@descriptor = $ad-cluster-descriptor][1]/configuration/cfg:cluster"/>
<xsl:variable name="env-config" select="$colo-config/cfg:environment"/>

<xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
  <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
</xsl:variable>


<xsl:variable name="ptest-conf" 
   select="$xpath/.."/>      

<xsl:variable name="frontends"
    select="//serviceGroup[@descriptor = $fe-cluster-descriptor]/service[@descriptor = $frontend-descriptor]"/>

<xsl:variable name="request-domain" select="$colo-config/cfg:coloParams/cfg:domain[1]/@name"/>

  <xsl:choose>
    <xsl:when test="count($xpath) = 0">
     <xsl:message terminate="yes">PerformanceTest:MakeConfig: Can't fixnd performance test config.</xsl:message>
    </xsl:when>
    <xsl:when test="count($colo-config) = 0">
     <xsl:message terminate="yes">PerformanceTest:MakeConfig: Colo config not found.</xsl:message>
    </xsl:when>
    <xsl:when test="count($env-config) = 0">
     <xsl:message terminate="yes">PerformanceTest:MakeConfig: Environment config not found.</xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:variable name="tested-frontend-port">
    <xsl:choose>
      <xsl:when test="count($colo-config/cfg:coloParams/@external_http_port) > 0">
        <xsl:value-of select="$colo-config/cfg:coloParams/@external_http_port"/>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$def-frontend-port"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

<xsl:variable name="frontend-hosts">
  <xsl:choose>
    <xsl:when test="count($frontends) > 0">
       <xsl:for-each select="$frontends">
         <xsl:variable name="frontend-port">
           <xsl:choose>        
             <xsl:when test="count(configuration/cfg:frontend/cfg:networkParams/@port) > 0">
               <xsl:value-of select="configuration/cfg:frontend/cfg:networkParams/@port"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$tested-frontend-port"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:variable name="hosts">
            <xsl:call-template name="GetHosts">
              <xsl:with-param name="hosts" select="@host"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:for-each select="exsl:node-set($hosts)//host">     
            <frontend><xsl:value-of select="."/>:<xsl:value-of select="$frontend-port"/></frontend>
          </xsl:for-each>
        </xsl:for-each>
    </xsl:when>
    <xsl:when test="count($request-domain) > 0">
    <frontend><xsl:value-of select="$request-domain"/>:<xsl:value-of select="$tested-frontend-port"/></frontend>
    </xsl:when>
    <xsl:otherwise>
    <frontend>localhost:<xsl:value-of select="$tested-frontend-port"/></frontend>
    </xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:for-each select="$xpath">
<xsl:variable name="channels-config">
  <xsl:choose>
    <xsl:when test="count(cfg:channels/cfg:channel) > 0">--channel_config_file ${config_root}/TestConfig/PerformanceTest-<xsl:value-of select="@name"/>/channels.cfg</xsl:when>
    <xsl:when test="count(cfg:channelsCount) > 0">--channels_count <xsl:value-of select="$xpath/cfg:channelsCount/@count"/></xsl:when>
    <xsl:otherwise>--channels_count 10</xsl:otherwise>
  </xsl:choose>
</xsl:variable>
<xsl:variable name="campaigns-config">
  <xsl:choose>
    <xsl:when test="count(cfg:campaigns/cfg:campaign) > 0">--campaign_config_file ${config_root}/TestConfig/PerformanceTest-<xsl:value-of select="@name"/>/campaigns.cfg</xsl:when>
    <xsl:when test="count(cfg:campaignsCount) > 0">--campaigns_count <xsl:value-of select="$xpath/cfg:campaignsCount/@count"/></xsl:when>
    <xsl:otherwise>--campaigns_count 10</xsl:otherwise>
  </xsl:choose>
</xsl:variable>

function db_clean_<xsl:value-of select="@name"/> ()
{
echo "Cleaning db for <xsl:value-of select="@name"/>..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test '<xsl:value-of select="@name"/>'
check_error $? "db_clean.pl"
echo "Cleaning db for <xsl:value-of select="@name"/>...ok"

}

function make_config_<xsl:value-of select="@name"/> ()
{

rm -rf ${test_config_root}/PerformanceTest-<xsl:value-of select="@name"/>/*
mkdir -p ${test_config_root}/PerformanceTest-<xsl:value-of select="@name"/>

  echo "Configuring performance <xsl:value-of select="@name"/>..."
  make_config.pl \
  --connection-string="$pg_connection_string" \
  --prefix <xsl:value-of select="@name"/> \
  <xsl:for-each select="exsl:node-set($frontend-hosts)//frontend"><xsl:value-of select="concat('--server', ' ', .)"/><xsl:text> \&#xa;  </xsl:text></xsl:for-each>--xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  <xsl:value-of select="$campaigns-config"/> \
  <xsl:value-of select="$channels-config"/> \
  <xsl:if test="count($ptest-conf/@add_confluence_report) > 0 and translate($ptest-conf/@add_confluence_report, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',  'abcdefghijklmnopqrstuvwxyz') = 'true'">--confluence_report_path  <xsl:value-of select="$workspace-root"/>/log/PerformanceTest/Confluence.report<xsl:text> \&#xa;  </xsl:text></xsl:if><xsl:if test="count(@ad_all_optouts) > 0 and translate(@ad_all_optouts, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',  'abcdefghijklmnopqrstuvwxyz') = 'true'">--ad_all_optouts<xsl:text> \&#xa;  </xsl:text></xsl:if><xsl:if test="count(@have_ron) > 0 and translate(@have_ron, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',  'abcdefghijklmnopqrstuvwxyz') = 'true'">--have_ron<xsl:text> \&#xa;  </xsl:text></xsl:if><xsl:if test="count(@create_free_tags) > 0 and translate(@create_free_tags, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',  'abcdefghijklmnopqrstuvwxyz') = 'true'">--create_free_tags<xsl:text> \&#xa;  </xsl:text></xsl:if><xsl:for-each select="./@*[name() != 'name' and name() != 'have_ron' and name() != 'ad_all_optouts' and name() != 'create_free_tags']"><xsl:value-of select="concat('--', name(), ' ', .)"/><xsl:text> \&#xa;  </xsl:text></xsl:for-each>${test_config_root}/PerformanceTest-<xsl:value-of select="@name"/>/Config.xml
  [ $? -ne 0 ] &amp;&amp; echo "command '$2' return error status code $1" &gt;&amp;2 &amp;&amp;  exit 3
  echo "Configuring performance <xsl:value-of select="@name"/>...ok"
}

</xsl:for-each>

</xsl:template>

</xsl:stylesheet>

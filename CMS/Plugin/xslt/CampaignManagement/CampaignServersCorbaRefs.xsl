<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="exsl">

<xsl:output method="xml" indent="yes" encoding="utf-8"/>

<xsl:variable name="xsd-campaign-server-network-params-type"
  select="document('../../xsd/CampaignManagement/CampaignServerAppType.xsd')/xsd:schema/xsd:complexType[
    @name='CampaignServerNetworkParamsType']"/>

<xsl:variable name="xsd-billing-server-network-params-type"
  select="document('../../xsd/CampaignManagement/BillingServerAppType.xsd')/xsd:schema/xsd:complexType[
    @name='BillingServerNetworkParamsType']"/>

<xsl:template name="CampaignServerCorbaRefs">
  <xsl:param name="campaign-servers"/>
  <xsl:param name="service-name"/>

  <cfg:CampaignServerCorbaRef name="CampaignServer">
    <xsl:for-each select="$campaign-servers">

      <xsl:variable
        name="campaign-server-config"
        select="configuration/cfg:campaignServer"/>

      <xsl:choose>
        <xsl:when test="count(@host) = 0">
          <xsl:message terminate="yes"> <xsl:value-of select="$service-name"/>: campaign server host undefined. </xsl:message>
        </xsl:when>
      </xsl:choose>

      <xsl:variable name="hosts">
       <xsl:call-template name="GetHosts">
         <xsl:with-param name="hosts" select="@host"/>
         <xsl:with-param name="error-prefix"  select="'CampaignManager'"/>
         </xsl:call-template>
      </xsl:variable>

      <xsl:for-each select="exsl:node-set($hosts)/host">
        <xsl:variable name="campaign-server-port" select="$campaign-server-config/cfg:networkParams/@port |
          $xsd-campaign-server-network-params-type/xsd:attribute[@name='port']/@default"/>

        <cfg:Ref ref="{concat('corbaloc:iiop:', ., ':',
          $campaign-server-port, $current-campaign-server-obj)}"/>
      </xsl:for-each>
    </xsl:for-each>
  </cfg:CampaignServerCorbaRef>

</xsl:template>

<xsl:template name="BillingServerCorbaRefs">
  <xsl:param name="billing-servers"/>
  <xsl:param name="error-prefix"/>

  <xsl:if test="count($billing-servers) = 0">
    <xsl:message terminate="yes"><xsl:value-of
      select="$error-prefix"/>: BillingServerCorbaRefs: xpath is empty</xsl:message>
  </xsl:if>

  <xsl:for-each select="$billing-servers">
    <xsl:variable name="billing-server-config"
       select="configuration/cfg:billingServer"/>

    <xsl:if test="count(@host) = 0">
      <xsl:message terminate="yes"><xsl:value-of
        select="$error-prefix"/>: BillingServerCorbaRefs: xpath is incorrect (
          element don't have host attribute)</xsl:message>
    </xsl:if>

    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix"
          select="concat($error-prefix, ': BillingServerCorbaRefs')"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:for-each select="exsl:node-set($hosts)/host">
      <xsl:variable name="billing-server-port" select="$billing-server-config/cfg:networkParams/@port |
        $xsd-billing-server-network-params-type/xsd:attribute[@name='port']/@default"/>

      <cfg:Ref ref="{concat('corbaloc:iiop:', ., ':',
        $billing-server-port, '/', $current-billing-server-obj)}"/>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

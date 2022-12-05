<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  exclude-result-prefixes="dyn">

<xsl:variable name="user-action-dir-name" select="'UserActions'"/>
<xsl:variable name="user-action-prefix" select="'UserAction_'"/>

<xsl:variable name="user-campaign-reach-dir-name" select="'UserCampaignReach'"/>
<xsl:variable name="user-campaign-reach-prefix" select="'UserCampaignReach_'"/>

<xsl:variable name="passback-dir-name" select="'Passback'"/>
<xsl:variable name="passback-prefix" select="'Passback_'"/>

<xsl:variable name="request-dir-name" select="'Requests'"/>
<xsl:variable name="request-prefix" select="'Request_'"/>
<xsl:variable name="def-request-expire-time" select="'604800'"/>
<xsl:variable name="def-passback-request-expire-time" select="'43200'"/>

<xsl:variable name="user-fraud-protection-dir-name" select="'UserFraudProtection'"/>
<xsl:variable name="user-fraud-protection-prefix" select="'UserFraudProtection_'"/>

<xsl:variable name="user-site-reach-chunks-root" select="'UserSiteReach'"/>
<xsl:variable name="user-site-reach-prefix" select="'UserSiteReach_'"/>

<xsl:variable name="user-tag-request-group-chunks-root" select="'UserTagRequestGroup'"/>
<xsl:variable name="user-tag-request-group-prefix" select="'UserTagRequestGroup_'"/>

</xsl:stylesheet>

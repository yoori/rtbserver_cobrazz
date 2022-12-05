<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="exsl">

  <xsl:template name="FillUserIdBlackList">
    <xsl:param name="desc"/>
    <cfg:UserIdBlackList>
      A3kph0f4RPu5A9tmlArhSg..
      Aecu7YZcTraYlGPieC7JUAC-
      Aecu7YZcTraYlGPieC7JUA..
      <xsl:if test="$desc = 'Managed by svn'">
      vP2ZB64ZQq69elUBxH43DQ..
      cHKUDcsOSTSnGMhwaDsKZQ
      b9VF6K8wTsuPstbYXCI0LA..
      GC0X19pxSUy-IqNxPVyMOA..
      </xsl:if>
    </cfg:UserIdBlackList>
  </xsl:template>
</xsl:stylesheet>

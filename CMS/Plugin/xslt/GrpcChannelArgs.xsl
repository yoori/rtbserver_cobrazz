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

  <xsl:template name="GrpcServerChannelArgList">
    <cfg:ChannelArgs>
    <!--<cfg:ChannelArg key="key1" value="value1"/>-->
    </cfg:ChannelArgs>
  </xsl:template>

  <xsl:template name="GrpcClientChannelArgList">
    <cfg:ChannelArgs>
      <!--<cfg:ChannelArg key="key1" value="value1"/>-->
    </cfg:ChannelArgs>
  </xsl:template>

</xsl:stylesheet>
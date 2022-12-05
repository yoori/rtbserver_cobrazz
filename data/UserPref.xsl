<?xml version="1.0" encoding="windows-1251"?>
<!DOCTYPE xsl:stylesheet>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" indent="no" encoding="Windows-1251"/>

<xsl:template match="/">
  <html>
  <table>
  <style type="text/css">
    a.button{
      float:left;
      text-decoration: none;
      margin-right:5px;
      background-color: #666666;
      background-position:right;
      background-repeat:no-repeat;
      cursor:pointer;
      cursor:hand;
    }
    a.button div{
      color:#FFF;
      padding:0 7px;
      line-height:20px;
      background-repeat:no-repeat;
      background-position:left;
      font-size:13px;
      white-space:nowrap;
    }
  </style>

  <xsl:apply-templates/>
  </table>
  </html>
</xsl:template> 

<xsl:template name="response" match="response">
  <html>
    <table border="0" bordercolor="black">
      <xsl:for-each select="category">
        <xsl:call-template name="category"/>
      </xsl:for-each>
    </table>
  </html>
</xsl:template>

<xsl:template name="category">
  <tr>
    <td><xsl:value-of select="@name"/></td>
    <td>
      <xsl:choose>
        <xsl:when test="@status = 'default' or @status = 'disabled'">
          <a class="button">
            <xsl:attribute name="href"><xsl:value-of select="@add"/></xsl:attribute>
            <div>Add</div>
          </a>
        </xsl:when>
      </xsl:choose>
    </td>
    <td>
      <xsl:choose>
        <xsl:when test="@status = 'default' or @status = 'enabled' or @status = 'detected'">
          <a class="button">
            <xsl:attribute name="href"><xsl:value-of select="@remove"/></xsl:attribute>
            <div>Remove</div>
          </a>
        </xsl:when>
      </xsl:choose>
    </td>
  </tr>
</xsl:template> 

</xsl:stylesheet>

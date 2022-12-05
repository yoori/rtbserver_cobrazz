<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="/">
    <html>
      <head>
        <style type="text/css">
          .activeLink {
          background-color: #ffff00;
          }
          div.references > a:target {
          background-color: #ffff00;
          }
        </style>
        <title>
          <xsl:value-of select="//struct/@id"/>
        </title>
      </head>
      <body>
        <h1 style="text-align: center">
          <xsl:value-of select="//struct/@id"/>
        </h1>
        <table width="100%" border="0" >
          <tbody>
            <tr>
              <td valign="top" style="background-color: #ddffcc;">
                <ul>
                  <xsl:for-each select="struct/table">
                    <li>
                      <a href="#{@id}forward" name="{@id}back" class="gotolink">
                        <xsl:value-of select="@id"/>
                      </a>
                    </li>
                  </xsl:for-each>
                </ul>
              </td>
              <td valign="top">
                <xsl:for-each select="struct/table">
                  <table width="100%" border="0" >
                    <caption style="background-color:#aabbee">
                      <div class="references">
                        <a href="#{@id}back" name="{@id}forward">
                          <xsl:value-of select="@id"/>
                        </a>
                      </div>
                    </caption>
                    <thead>
                      <xsl:for-each select="row">
                        <xsl:if test="position()=last()">
                          <xsl:call-template name="HeaderRow"/>
                        </xsl:if>
                      </xsl:for-each>
                    </thead>
                    <tbody>
                      <xsl:for-each select="row">
                        <tr class="references">
                          <td style="width:24px">
                            <div class="references">
                              <a name="{anchor}">
                                <xsl:value-of select="position()"/>
                              </a>
                            </div>
                          </td>
                          <xsl:call-template name="BodyRow"/>
                        </tr>
                      </xsl:for-each>
                    </tbody>
                  </table>
                </xsl:for-each>
              </td>
            </tr>
          </tbody>
        </table>
      </body>
      <script type="text/javascript" src="http://yui.yahooapis.com/2.5.1/build/yahoo-dom-event/yahoo-dom-event.js"></script>
      <script type="text/javascript" src="highlight.js"/>
    </html>

  </xsl:template>
  <xsl:template name="HeaderRow">
    <tr>
      <td style="width:24px">
        <a></a>
      </td>
      <!--
      <td style="background-color:#dddddd" align="center" valign="middle">
        id
      </td>-->
      <xsl:for-each select="field">
        <td style="background-color:#dddddd" align="center" valign="middle">
          <xsl:value-of select="@id"/>
        </td>
      </xsl:for-each>
      <xsl:for-each select="link">
        <td style="background-color:#dddddd" align="center" valign="middle">
          <xsl:value-of select="@id"/>
        </td>
      </xsl:for-each>
    </tr>
  </xsl:template>
  <xsl:template name="BodyRow">
    <!--
    <td style="background-color:#ffdddd" align="center" valign="middle">
      <xsl:value-of select="@id"/>
    </td>-->
    <xsl:for-each select="field">
      <td align="center" valign="middle" style="background-color:#ddffcc">
        <pre><xsl:value-of select="."/></pre>
      </td>
    </xsl:for-each>
    <xsl:for-each select="link">
      <td align="center" valign="middle" style="background-color:#d0f0c0">
        <xsl:choose>
          <xsl:when test="@href">
            <a href="#{@href}" class="gotolink">
              <pre><xsl:value-of select="."/></pre>
            </a>
          </xsl:when>
          <xsl:otherwise>
            <a>
              <pre><xsl:value-of select="."/></pre>
            </a>
          </xsl:otherwise>
        </xsl:choose>
      </td>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>

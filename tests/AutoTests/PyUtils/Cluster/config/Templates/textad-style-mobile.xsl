<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:template name="mtop">
    body {background-color:#ccffff;}
    .impression {width:100%; height:100%;}
    .creative td {vertical-align:middle; text-align:left; padding:0 0.5em; line-height:1;}
    .DISPLAY_URL {font-size:inherit;}
    &#35;ibutton {right:3px; top:3px; background-color:#ccffff;}
    &#35;logo {color:gray; position:fixed; right:1px; bottom:1px; padding:0.1em; font:normal 0.8em Arial,sans-serif; background-color:#ccffff;}
  </xsl:template>
  <xsl:template name="script">
    <!-- Avoid single quotes in script -->
    function getWinWidth() {
      return window.innerWidth || (document.compatMode=="CSS1Compat" ? document.documentElement.clientWidth : document.body.clientWidth);
    }
    var ruleNum = 5,
    styleSheet = document.styleSheets[0],
    rule = styleSheet.cssRules ? styleSheet.cssRules[ruleNum] : styleSheet.rules[ruleNum],
    fontSizeInit = rule.style.fontSize.match(new RegExp("^\\\\d*"))[0];
    window.onresize = function () {
      var koef = getWinWidth()/screen.width;
      rule.style.fontSize = parseInt(fontSizeInit*koef) + "px";
    };
    onresize();
  </xsl:template>
  <xsl:template name="logo">
    <div id="logo">Powered by OIX</div>
  </xsl:template>
</xsl:stylesheet>

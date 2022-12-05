<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:template name="image-style">
    img {width:100%; height:100%; border:0 none;}
    td.himage,td.one-image {padding-top:<xsl:value-of select="$image-top-padding"/>px; padding-right:0; padding-bottom:0;}
    td.text {width:100%; text-align:left !important;}
    td.one-image {text-align:right !important;}
    td.one-text {text-align:left !important; vertical-align:middle !important;}
    .vimage-container {width:100%; text-align:center;}
    div.himage {overflow:hidden;}
    div.vimage {overflow:hidden; margin-left:auto; margin-right:auto; padding-bottom:0.3em;}
    div.one-image {overflow:hidden; margin-left:auto; margin-right:0;}
    div.i234x60x1 {
      width:69px; height:50px;
    }
    div.i234x90x1 {
      width:110px; height:80px;
    }
    div.i728x90x1,div.i728x90x2,div.i728x90x3,div.i728x90x4 {
      width:110px; height:80px;
    }
    div.i468x60x1,div.i468x60x2 {
      width:69px; height:50px;
    }
    div.i300x250x1,div.i300x250x2,div.i336x280x1,div.i336x280x2 {
      width:110px; height:80px;
    }
    div.i300x250x3,div.i300x250x4,div.i336x280x3,div.i336x280x4 {
      width:69px; height:50px;
    }
    div.i120x600x1,div.i120x600x2,div.i120x600x3,div.i120x600x4 {
      width:110px; height:80px;
    }
    div.i160x600x1,div.i160x600x2,div.i160x600x3,div.i160x600x4,div.i160x600x5 {
      width:110px; height:80px;
    }
    div.i580x150x1 {
      width:110px; height:80px;
    }
    div.i580x150x2,div.i580x150x3 {
      width:69px; height:50px;
    }
    div.i250x250x1 {
      width:110px; height:80px;
    }
    div.i250x250x2,div.i250x250x3 {
      width:69px; height:50px;
    }
    div.i560x200x1,div.i560x200x2 {
      width:110px; height:80px;
    }
    div.i560x200x3 {
      width:69px; height:50px;
    }
    div.i545x150x1 {
      width:110px; height:80px;
    }
    div.i545x150x2,div.i545x150x3 {
      width:69px; height:50px;
    }
  </xsl:template>
</xsl:stylesheet>

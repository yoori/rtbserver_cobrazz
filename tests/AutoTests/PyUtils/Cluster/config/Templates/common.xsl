<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:variable name="js-format" select="'js'"/>
  <xsl:variable name="html-format" select="'html'"/>
  <xsl:variable name="appformat" select="//token[@name='APP_FORMAT']"/>
  <xsl:variable name="host" select="//token[@name='ADIMAGE-SERVER']"/>
  <xsl:variable name="ibutton-enabled" select="//token[@name='AD_FOOTER_ENABLED']='Yes'"/>
  <xsl:variable name="onclick">
    <![CDATA[if (window.ActiveXObject) {event.preventDefault && event.preventDefault(); this.parentNode.click();}]]>
  </xsl:variable>
  <xsl:variable name="random" select="//token[@name='RANDOM']"/>
  <xsl:variable name="iframe-id" select="concat('c', $random)"/>
  <xsl:variable name="tag-id" select="//token[@name='TAGID']"/>
  <xsl:variable name="opacity">
    <xsl:choose>
      <xsl:when test="number(//token[@name='OPACITY'])>=0">
        <xsl:value-of select="number(//token[@name='OPACITY'])"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="pixels" select="//token[contains(@name, 'TRACKPIXEL') or contains(@name, 'CRADVTRACKPIXEL') or contains(@name, 'PUBL_TAG_TRACK_PIXEL')]"/>
  <xsl:template match="impression">
    <xsl:call-template name="js"/>
  </xsl:template>
  <xsl:template name="text">
    <xsl:apply-templates select="token[@name='HEADLINE']"/>
    <xsl:apply-templates select="token[@name='DESCRIPTION1' and string-length()>0]"/>
    <xsl:apply-templates select="token[@name='DESCRIPTION2' and string-length()>0]"/>
    <xsl:apply-templates select="token[@name='DISPLAY_URL']"/>
  </xsl:template>
  <xsl:template match="token">
    <xsl:param name="class"/>
    <xsl:param name="no-break"/>
    <xsl:choose>
      <xsl:when test="contains(@name, 'PIXEL')">
        <xsl:if test="string-length()>0">
          <xsl:value-of select="concat(., '\n')" disable-output-escaping="yes"/>
        </xsl:if>
      </xsl:when>
      <xsl:when test="string-length($class)>0">
        <div class="{$class}">
          <xsl:call-template name="encode">
            <xsl:with-param name="value" select="."/>
          </xsl:call-template>
        </div>
      </xsl:when>
      <xsl:when test="@name='HEADLINE' or @name='DISPLAY_URL'">
        <div class="{@name}">
          <xsl:call-template name="encode">
            <xsl:with-param name="value" select="."/>
          </xsl:call-template>
        </div>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="encode">
          <xsl:with-param name="value" select="."/>
        </xsl:call-template>
        <xsl:choose>
          <xsl:when test="string-length($no-break)=0">
            <br/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="' '"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="image">
    <xsl:param name="url"/>
    <xsl:param name="class" select="'image'"/>
    <div class="{$class}">
      <img src="{$url}"/>
    </div>
  </xsl:template>
  <xsl:template name="closebutton">
    <xsl:param name="class"/>
    <xsl:variable name="func-call" select="concat('n=parent.document.getElementById(\x27', $iframe-id, '\x27); n.parentNode.removeChild(n);')"/>
    <a id="closebutton" class="{$class}" href="javascript:void(0)" onclick="{$func-call}">\x26nbsp;</a>
  </xsl:template>
  <xsl:template name="encode">
    <xsl:param name="value"/>
    <xsl:variable name="single-quote">'</xsl:variable>
    <xsl:choose>
      <xsl:when test="$appformat=$js-format">
        <xsl:choose>
          <!-- If extension function available, apply it... -->
          <xsl:when test="function-available('foros:escape-js')">
            <xsl:value-of disable-output-escaping="no" select="foros:escape-js(normalize-space($value))"/>
          </xsl:when>
          <!-- ...otherwise at least replace single quotes -->
          <xsl:otherwise>
            <xsl:value-of disable-output-escaping="no" select="translate(normalize-space($value), $single-quote, '`')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="ibutton-html">
    <xsl:param name="class"/>
    <xsl:if test="$ibutton-enabled">
      <div id="ibutton" class="{$class}"><a target="_blank" href="{//token[@name='AD_FOOTER_URL']}">i</a></div>
    </xsl:if>
  </xsl:template>
  <xsl:template name="refresh-routine">
    <xsl:text disable-output-escaping="yes"><![CDATA[
(function(){try{function c8(){var X=function(W){try{W=W||event;if(AL(W.target||W.srcElement)){NV();G.zQ('click',X,document);}}catch(s){}};G.hO('click',X,document);}function NU(b){var uK=document.getElementById(b),X=function(W){try{W=W||event;if(W.keyCode==13){NV();G.zQ('keydown',X,uK);}}catch(s){}};uK&&G.hO('keydown',X,uK);}function NV(){var N=']]></xsl:text><xsl:value-of select="$random"/><xsl:text disable-output-escaping="yes"><![CDATA[',j3=']]></xsl:text><xsl:value-of select="$iframe-id"/><xsl:text disable-output-escaping="yes"><![CDATA[';G.del_(j3);G.resEnv_(N);PSenv[N].dbl_=null;PSenv[N].ns_.run_();}function AL(Jr){return Jr.getAttribute('onclick')&&(Jr.getAttribute('onclick').indexOf('mainmenuclick')>-1||Jr.getAttribute('onclick').indexOf('submit')>-1)||Jr.parentNode.getAttribute('onclick')&&(Jr.parentNode.getAttribute('onclick').indexOf('mainmenuclick')>-1||Jr.parentNode.getAttribute('onclick').indexOf('submit')>-1)||Jr.parentNode.id&&Jr.parentNode.id=='controls';}c8();NU('searchinput');}catch(s){}})();
      ]]>
    </xsl:text>
  </xsl:template>
  <xsl:template name="display-url">
    <xsl:variable name="url" select="token[@name='DISPLAY_URL']"/>
    <xsl:choose>
      <xsl:when test="string-length($url)=0">
        <xsl:value-of select="token[@name='OIXCLICK']"/>
      </xsl:when>
      <xsl:when test="starts-with($url, 'http')">
        <xsl:call-template name="encode">
          <xsl:with-param name="value" select="$url"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="encode">
          <xsl:with-param name="value" select="concat('http://', $url)"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="font-adjust-script">
    <xsl:param name="id"/>
    <xsl:param name="max-height"/>
    <xsl:param name="min-font-size"/>
    <xsl:param name="max-font-size"/><![CDATA[
      function adjust_() {
        if (node_.offsetHeight) {
          var maxHeight_ = ]]><xsl:value-of select="$max-height"/><![CDATA[,
          minFontSize_ = ]]><xsl:value-of select="$min-font-size"/><![CDATA[,
          maxFontSize_ = ]]><xsl:value-of select="$max-font-size"/><![CDATA[,
          fontSize_ = maxFontSize_;
          while (node_.offsetHeight>maxHeight_ && fontSize_>minFontSize_)
            node_.style.fontSize = --fontSize_+"px";
          clearInterval(int_);
        }
      }
      var node_ = document.getElementById("]]><xsl:value-of select="$id"/><![CDATA["),
      int_ = setInterval(adjust_, 5);]]>
  </xsl:template>
  <xsl:template name="fingerprint">
    <xsl:text disable-output-escaping="yes"><![CDATA[
void(0);var G=typeof(G)=='object'?G:{};G.i=function(){};G.i.prototype={L:function(h){with(h){width=1;height=1;frameBorder=0;scrolling='no';style.position='absolute';}return h;},e:function(){return this.Y5().appendChild(this.L(document.createElement('IFRAME')));},Y5:function(){var b='svc'+parseInt(new Date().getTime()/1000000),g=document.getElementById(b);if(!g){var Hv=document.body||document.documentElement;g=Hv.insertBefore(document.createElement('DIV'),Hv.firstChild);g.id=b;}return g;},zC:function(k){return parseInt(Math.random()*k)==0;},Vr:function(a8,k){with(this){k=window.PSprp_rate==null?k:PSprp_rate;if(!k||zC(k)){this.h=e();h.src=a8;}return this;}}};(function(){try{var Iq=']]></xsl:text><xsl:value-of select="//token[@name=&apos;ADSERVER&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[';PSenv=window.PSenv||{};PSenv.fp_=PSenv.fp_||(new G.i().Vr(Iq+'/services/ftype?op=frame&v=html')&&1);}catch(s){}})();
      ]]>
    </xsl:text>
  </xsl:template>
  <xsl:template name="retargeting">
    <xsl:text disable-output-escaping="yes"><![CDATA[
void(0);var G=typeof(G)=='object'?G:{};G.i=function(){};G.i.prototype={L:function(h){with(h){width=1;height=1;frameBorder=0;scrolling='no';style.position='absolute';}return h;},e:function(){return this.Y5().appendChild(this.L(document.createElement('IFRAME')));},Y5:function(){var b='svc'+parseInt(new Date().getTime()/1000000),g=document.getElementById(b);if(!g){var Hv=document.body||document.documentElement;g=Hv.insertBefore(document.createElement('DIV'),Hv.firstChild);g.id=b;}return g;},zC:function(k){return parseInt(Math.random()*k)==0;},Vr:function(a8,k){with(this){k=window.PSprp_rate==null?k:PSprp_rate;if(!k||zC(k)){this.h=e();h.src=a8;}return this;}}};(function(){try{var Qf=']]></xsl:text><xsl:value-of select="//token[@name=&apos;USERSTATUS&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',dz=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELSOPTIN&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',VH=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELS&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',e3=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELSOPTOUT&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[';if(Qf==1){if(dz)new G.i().Vr(dz,100);}else if(VH==1&&e3)new G.i().Vr(e3);}catch(s){}})();
      ]]>
    </xsl:text>
  </xsl:template>
</xsl:stylesheet>

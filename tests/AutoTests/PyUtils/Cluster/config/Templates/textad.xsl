<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:foros="http://foros.com/oix/xslt-template">
  <xsl:import href="common.xsl"/>
  <xsl:output omit-xml-declaration="yes" media-type="text/html" encoding="UTF-8"/>
  <xsl:include href="textad-style.xsl"/>
  <xsl:variable name="size" select="//token[@name='TAGSIZE']"/>
  <xsl:variable name="tag-id" select="//token[@name='TAGID']"/>
  <xsl:variable name="creative-count" select="count(//creative)"/>
  <xsl:variable name="width">
    <xsl:choose>
      <xsl:when test="string-length(//token[@name='TAGWIDTH'])>0">
        <xsl:value-of select="//token[@name='TAGWIDTH']"/>
      </xsl:when>
      <!-- 234x90 OIX Preview -->
      <xsl:otherwise>234</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="height">
    <xsl:choose>
      <xsl:when test="string-length(//token[@name='TAGHEIGHT'])>0">
        <xsl:value-of select="//token[@name='TAGHEIGHT']"/>
      </xsl:when>
      <!-- 234x90 OIX Preview -->
      <xsl:otherwise>90</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="ibutton-enabled" select="//token[@name='AD_FOOTER_ENABLED']='Yes'"/>
  <xsl:variable name="image-class" select="concat('i', $width, 'x', $height, 'x', $creative-count)"/>
  <xsl:variable name="onclick">
    <![CDATA[if (window.ActiveXObject) {event.preventDefault && event.preventDefault(); this.parentNode.click();}]]>
  </xsl:variable>
  <xsl:variable name="mobile-size" select="'mtop'"/>
  <xsl:template match="impression">
    <xsl:choose>
      <xsl:when test="$appformat=$js-format">
        <xsl:call-template name="js"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="html"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="js">
    <xsl:call-template name="engine"/>
    <xsl:text disable-output-escaping="yes"><![CDATA[
(function(){try{new G.Nm({inframe:1,width:']]></xsl:text><xsl:value-of select="$width"/><xsl:text disable-output-escaping="yes"><![CDATA[',height:']]></xsl:text><xsl:value-of select="$height"/><xsl:text disable-output-escaping="yes"><![CDATA[',size:']]></xsl:text><xsl:value-of select="$size"/><xsl:text disable-output-escaping="yes"><![CDATA[',src:']]></xsl:text><xsl:call-template name="html"/><xsl:text disable-output-escaping="yes"><![CDATA['}).rJ();}catch(s){G.ZD(s.message||s);}})();void(0);var G=typeof(G)=='object'?G:{};G.i=function(){};G.i.prototype={L:function(h){with(h){width=1;height=1;frameBorder=0;scrolling='no';style.position='absolute';}return h;},e:function(){return this.Y5().appendChild(this.L(document.createElement('IFRAME')));},Y5:function(){var b='svc'+parseInt(new Date().getTime()/1000000),g=document.getElementById(b);if(!g){var Hv=document.body||document.documentElement;g=Hv.insertBefore(document.createElement('DIV'),Hv.firstChild);g.id=b;}return g;},zC:function(k){return parseInt(Math.random()*k)==0;},Vr:function(a8,k){with(this){k=window.PSprp_rate==null?k:PSprp_rate;if(!k||zC(k)){this.h=e();h.src=a8;}return this;}}};(function(){try{var Qf=']]></xsl:text><xsl:value-of select="//token[@name=&apos;USERSTATUS&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',dz=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELSOPTIN&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',VH=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELS&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',e3=']]></xsl:text><xsl:value-of select="//token[@name=&apos;PUBPIXELSOPTOUT&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[';if(Qf==1){if(dz)new G.i().Vr(dz,100);}else if(VH==1&&e3)new G.i().Vr(e3);}catch(s){}})();
    ]]></xsl:text>
  </xsl:template>
  <xsl:template name="engine">
    <xsl:choose>
      <xsl:when test="$size=$mobile-size">
        <xsl:text disable-output-escaping="yes"><![CDATA[
    void('3.1.0.15');
var G=typeof(G)=='object'?G:{};G.F=function(mo,S4){for(var C in S4)mo[C]=S4[C];};G.F(G,{v:'3.1.0.15',RY:function(gY){this.ZZ=this.ZZ||[];var f;for(var Y=0;Y<gY.length;Y++)if(gY[Y]){f=new Image();f.src=gY[Y];this.ZZ.push(f);}},ZD:function(zp,w9){with(this){var Iq=']]></xsl:text><xsl:value-of select="//token[@name=&apos;ADSERVER&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',K=']]></xsl:text><xsl:value-of select="//token[@name=&apos;TAGID&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',j=']]></xsl:text><xsl:value-of select="//token[@name=&apos;CCID&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA['||0,nD=K+','+j;RY([Iq+'/services/sl.gif'+'?app=adclient&op=view&res=F'+'&ct='+nD+'&tid='+K+'&ccid='+j+'&src='+'creative'+'&rnd='+M()]);}},M:function(){return parseInt(Math.random()*10000000);}});G.F(G,{O3:function(A){if(A.tagName=='IFRAME'){A.frameBorder=0;A.marginWidth=0;A.marginHeight=0;A.scrolling='no';}with(A.style){padding=0;margin=0;border='0 none';}return A;},Wb:function(h,l,z){try{with(h.contentWindow){document.write(l);setTimeout('document.close()',1000);}return h;}catch(s){if(z&&!h.src)h.src='javascript:"<script>try{document.domain=\''+document.domain+'\';}catch(e){}</'+'script>"';z=z||0;z++;if(z<6){l='<script>document.domain=\''+document.domain+'\';</'+'script>'+l;var R8=this;setTimeout(function(){R8.Wb(h,l,z);},200);}else h.parentNode.removeChild(h);return h;}},pP:function(Ij){return!/^https?\:\/\/\w+/.test(Ij);},getRoot_:function(){var E=document;if(!E.documentElement.clientWidth)return E.body;if(E.compatMode=='BackCompat')return E.body;else return E.documentElement;},TU:function(){return document._readyState=='loaded'||!(document.readyState=='loading'||window.ActiveXObject&&document.readyState=='interactive');},resEnv_:function(b){var S=window;if(S.PSenv){var q=PSenv[b];if(q){S.PStid=q.PStid;S.PSsize=q.PSsize;S.PSies=q.PSies;S.PStr=q.PStr;S.PSpb=q.PSpb;S.PSpt=q.PSpt;S.PSpf=q.PSpf;S.PSopacity=q.PSopacity;S.PSrmposition=q.PSrmposition;S.PSrmoffset=q.PSrmoffset;S.PSbackground=q.PSbackground;return 1;}}},zQ:function(W,X,A){try{A=A||window;if(A.removeEventListener){A.removeEventListener(W,X,false);return 1;}else return A.detachEvent('on'+W,X);}catch(s){}},hO:function(W,X,A){try{A=A||window;if(A.addEventListener){A.addEventListener(W,X,false);return 1;}else return A.attachEvent('on'+W,X);}catch(s){}}});G.J1=function(J){this.J=J;this.o=J.o||J.width;this.u=J.u||J.height;this.b=G.M();};G.J1.prototype={NJ:function(){with(this){var E=document;try{this.A=E.body.insertBefore(E.createElement('DIV'),E.body.firstChild);A.id=b;with(A.style){display='none';position='absolute';top=0;left=0;}}catch(s){E.write(_());}}return this;},_:function(){var r='<div id="'+this.b+'" style="display:none; position:absolute; top:0; left:0;"></div>';if(!document.body)r='<body>'+r+'</body>';return r;},L:function(h){with(this){hK(h);Br();BG(h);h.parentNode.style.display='';return h;}},hK:function(h){with(this){h=h||g4;h.width=Qm();h.height=Dj();}},Qm:function(){return window.innerWidth||(document.compatMode=='CSS1Compat'?document.documentElement.clientWidth:document.body.clientWidth);},Dj:function(){return parseInt(40*this.Qm()/screen.width);},Br:function(u){document.body.style.paddingTop=this.Dj()+this.Oi()+'px';},Oi:function(){try{if(this.jh==null){var Q2=window.ActiveXObject?document.body.currentStyle.paddingTop:getComputedStyle(document.body,'').paddingTop;this.jh=Number(Q2.replace(/\D/g,''));}return this.jh;}catch(s){return 0;}},BG:function(A){A=A||this.g4;A.parentNode.style.left=(window.scrollX||document.body.scrollLeft||document.documentElement.scrollLeft)+'px';},sp:function(){try{var R8=this,__=function(W){R8.hK();R8.Br();W.type=='load'?setTimeout(function(){R8.BG()},500):R8.BG();},Xy=['resize','scroll','load'];for(var Y=0;Y<Xy.length;Y++)G.hO(Xy[Y],__);}catch(s){}}};G.F(G.J1.prototype,{jb:function(){return this.A||(this.A=document.getElementById(this.b));},e:function(){with(this)return this.h||(this.h=L(jb().appendChild(G.O3(document.createElement('IFRAME')))));}});G.Nm=function(H){this.H=H;};G.Nm.prototype={as:'host',Xv:'pixel',rJ:function(){with(this)kJ().p8();H[Xv]&&G.RY(H[Xv].split('\n'));}};G.F(G.Nm.prototype,{kJ:function(){this.l=new G.J4(this.H).d7();return this;},p8:function(){with(this){this.B2=this.B2||new G.J1(H).NJ();G.Wb(B2.e(),l);B2.sp();}}});G.J4=function(H){with(this){this.H=H;this.I5=H[J9].toLowerCase();this.p=H[k6]?H[k6].toLowerCase():'';this.c1=H[mV]||'';}};G.J4.prototype={J9:'size',k6:'type',d:'src',as:'host',mV:'ibutton',d7:function(){with(this)return H[d]+c1;}};
        ]]></xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text disable-output-escaping="yes"><![CDATA[
    void('3.1.0.15');
var G=typeof(G)=='object'?G:{};G.F=function(mo,S4){for(var C in S4)mo[C]=S4[C];};G.F(G,{v:'3.1.0.15',RY:function(gY){this.ZZ=this.ZZ||[];var f;for(var Y=0;Y<gY.length;Y++)if(gY[Y]){f=new Image();f.src=gY[Y];this.ZZ.push(f);}},ZD:function(zp,w9){with(this){var Iq=']]></xsl:text><xsl:value-of select="//token[@name=&apos;ADSERVER&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',K=']]></xsl:text><xsl:value-of select="//token[@name=&apos;TAGID&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA[',j=']]></xsl:text><xsl:value-of select="//token[@name=&apos;CCID&apos;]"/><xsl:text disable-output-escaping="yes"><![CDATA['||0,nD=K+','+j;RY([Iq+'/services/sl.gif'+'?app=adclient&op=view&res=F'+'&ct='+nD+'&tid='+K+'&ccid='+j+'&src='+'creative'+'&rnd='+M()]);}},M:function(){return parseInt(Math.random()*10000000);}});G.F(G,{O3:function(A){if(A.tagName=='IFRAME'){A.frameBorder=0;A.marginWidth=0;A.marginHeight=0;A.scrolling='no';}with(A.style){padding=0;margin=0;border='0 none';}return A;},Wb:function(h,l,z){try{with(h.contentWindow){document.write(l);setTimeout('document.close()',1000);}return h;}catch(s){if(z&&!h.src)h.src='javascript:"<script>try{document.domain=\''+document.domain+'\';}catch(e){}</'+'script>"';z=z||0;z++;if(z<6){l='<script>document.domain=\''+document.domain+'\';</'+'script>'+l;var R8=this;setTimeout(function(){R8.Wb(h,l,z);},200);}else h.parentNode.removeChild(h);return h;}},pP:function(Ij){return!/^https?\:\/\/\w+/.test(Ij);},getRoot_:function(){var E=document;if(!E.documentElement.clientWidth)return E.body;if(E.compatMode=='BackCompat')return E.body;else return E.documentElement;},TU:function(){return document._readyState=='loaded'||!(document.readyState=='loading'||window.ActiveXObject&&document.readyState=='interactive');},resEnv_:function(b){var S=window;if(S.PSenv){var q=PSenv[b];if(q){S.PStid=q.PStid;S.PSsize=q.PSsize;S.PSies=q.PSies;S.PStr=q.PStr;S.PSpb=q.PSpb;S.PSpt=q.PSpt;S.PSpf=q.PSpf;S.PSopacity=q.PSopacity;S.PSrmposition=q.PSrmposition;S.PSrmoffset=q.PSrmoffset;S.PSbackground=q.PSbackground;return 1;}}},zQ:function(W,X,A){try{A=A||window;if(A.removeEventListener){A.removeEventListener(W,X,false);return 1;}else return A.detachEvent('on'+W,X);}catch(s){}},hO:function(W,X,A){try{A=A||window;if(A.addEventListener){A.addEventListener(W,X,false);return 1;}else return A.attachEvent('on'+W,X);}catch(s){}}});G.J1=function(J){this.J=J;this.o=J.o||J.width;this.u=J.u||J.height;this.b=G.M();};G.J1.prototype={NJ:function(){with(this){var E=document,R=J.A;if(R&&R.parentNode.tagName!='HEAD'){this.A=R.parentNode.insertBefore(E.createElement('DIV'),R);A.id=b;A.style.display='none';}else if(!G.TU())E.write(_());}return this;},_:function(){var r='<div id="'+this.b+'" style="display:none;"></div>';if(!document.body)r='<body>'+r+'</body>';return r;},hF:function(a8){this.e().src=a8;this.wV();},wV:function(){var B2=this.jb(),X=function(){try{B2.parentNode.removeChild(B2);}catch(s){}};G.hO('beforeunload',X);},L:function(h){with(this){h.width=o+'px';h.height=u+'px';with(h.parentNode.style){width=o+'px';height=u+'px';display='';}return h;}}};G.F(G.J1.prototype,{jb:function(){return this.A||(this.A=document.getElementById(this.b));},e:function(){with(this)return this.h||(this.h=L(jb().appendChild(G.O3(document.createElement('IFRAME')))));}});G.Nm=function(H){this.H=H;};G.Nm.prototype={J9:'size',b6:'inframe',as:'host',Xv:'pixel',rJ:function(nW){with(this){kJ();if(H[b6])p8();else{if(G.TU())throw{message:'async_write'};document.write(l);}H[Xv]&&G.RY(H[Xv].split('\n'));}}};G.F(G.Nm.prototype,{kJ:function(){this.l=new G.J4(this.H).d7();return this;},p8:function(){with(this){this.B2=this.B2||new G.J1(H).NJ();G.Wb(B2.e(),l);}}});G.J4=function(H){with(this){this.H=H;this.I5=H[J9].toLowerCase();this.p=H[k6]?H[k6].toLowerCase():'';this.c1=H[mV]||'';this.o=H[AD]||H[V];this.u=H[NP]||H[U];}};G.J4.prototype={J9:'size',k6:'type',d:'src',UO:'href',LX:'alt',_W:'onclick',a4:'swf',BS:'wmode',jd:'flashvars',V:'width',U:'height',AD:'maxwidth',NP:'maxheight',Bx:'ver',I_:'id',as:'host',mV:'ibutton',b6:'inframe',d7:function(){with(this){var r;switch(p){case 'img':case 'image':r=Qk();break;case 'flash':r=qs();break;default:r=_();}return r||'';}},Qk:function(){with(this){var Dr=H[_W]?' onclick="'+H[_W]+'"':'';return '<a href="'+H[UO]+'" target="_blank"><img src="'+H[d]+'" style="border:0 none; margin:0; padding:0; width:'+o+'px; height:'+u+'px;"'+(H[LX]?' alt="'+H[LX]+'" title="'+H[LX]+'"':'')+Dr+'/></a>';}},qs:function(){with(this){if(H[a4]&&vN()){var b=H[I_],l8='width="'+o+'" height="'+u+'">'+'<param name="quality" value="high"/>'+'<param name="wmode" value="'+(H[BS]||'transparent')+'"/>'+'<param name="AllowScriptAccess" value="always"/>'+(H[jd]?'<param name="flashvars" value="'+H[jd]+'"/>':''),r=window.ActiveXObject?'<object '+(b?'id="'+b+'"':'')+' classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000" codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab" '+l8+'<param name="movie" value="'+H[a4]+'"/>'+'</object>':'<object '+(b?'name="'+b+'"':'')+' type="application/x-shockwave-flash" data="'+H[a4]+'" '+l8+'</object>';}else var r=Qk();return r;}},_:function(){with(this)if(G.pP(H[d]))return H[d];else if(p=='jscript')return '<script src="'+H[d]+'"></'+'script>';else return '<iframe src="'+H[d]+'" width="'+o+'" height="'+u+'" frameborder="0" marginheight="0" marginwidth="0" scrolling="no"></iframe>';},vN:function(){with(this){var mr=_4();return mr&&(!H[Bx]||q9(mr)>=q9(Oy(H[Bx])));}},q9:function(rg){var GV=rg.toString().split(/[^\d]+/);return(GV[0]||0)*100000+(GV[1]||0)*1000+Number(GV[2]||0);},_4:function(){with(this){var w,Sx=0,BJ=navigator.plugins;if(BJ&&BJ.length){w=BJ['Shockwave Flash'];Sx=w?Oy(w.description):0;}if(!w&&window.ActiveXObject)try{w=new ActiveXObject('ShockwaveFlash.ShockwaveFlash');Sx=w?Oy(w.GetVariable('$version')):0;}catch(s){}return Sx;}},Oy:function(Lg){var UK=Lg.toString().match(/\d+/g);return UK?UK.join('.'):'';}};
        ]]></xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="html">
    <html>
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
        <style type="text/css">
          <xsl:call-template name="encode">
            <xsl:with-param name="value">
              <xsl:call-template name="style"/>
            </xsl:with-param>
          </xsl:call-template>
        </style>
        <xsl:if test="$size=$mobile-size">
            <script type="text/javascript">
              <xsl:call-template name="encode">
                <xsl:with-param name="value">
                  <xsl:call-template name="script"/>
                </xsl:with-param>
              </xsl:call-template>
            </script>
        </xsl:if>
      </head>
      <body>
        <div class="impression">
          <xsl:apply-templates select="creative"/>
        </div>
        <xsl:call-template name="ibutton-html"/>
        <xsl:apply-templates select="//token[contains(@name, 'TRACKPIXEL') or contains(@name, 'CRADVTRACKPIXEL') or contains(@name, 'PUBL_TAG_TRACK_PIXEL')]"/>
        <xsl:if test="$appformat=$html-format">
          <script type="text/javascript">
            <xsl:call-template name="fingerprint"/>
            <xsl:call-template name="retargeting"/>
          </script>
        </xsl:if>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="creative">
    <xsl:variable name="image-url" select="token[@name='IMAGE_FILE']"/>
    <xsl:variable name="with-image" select="string-length($image-url)>0 and $image-url!=concat($host, '/creatives')"/>
    <xsl:variable name="creative-class">
        <xsl:if test="position()=last() and $creative-count>1">
            <xsl:value-of select="'last'"/>
        </xsl:if>
    </xsl:variable>
    <xsl:variable name="display-url">
      <xsl:call-template name="display-url"/>
    </xsl:variable>
    <xsl:variable name="crclick">
      <xsl:call-template name="encode">
        <xsl:with-param name="value" select="token[@name='CRCLICK']"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="quote">
      <xsl:choose>
        <xsl:when test="$appformat=$js-format">\x26quot;</xsl:when>
        <xsl:otherwise>&quot;</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="cr-onclick1" select="concat('this.href=', $quote, $crclick, $quote)"/>
    <xsl:variable name="cr-onclick2" select="concat('new Image().src=', $quote, token[@name='OIXPRECLICK'], $quote)"/>
    <a href="{$display-url}" onclick="{$cr-onclick1}; {$cr-onclick2}" target="_blank">
      <div class="creative {$creative-class}">
        <table onclick="{normalize-space($onclick)}">
          <xsl:choose>
            <!-- Vertical image/text -->
            <xsl:when test="$size='120x600' or $size='160x600' or $image-class='i250x250x1'">
              <tr><td><div class="wrap">
                <xsl:if test="$with-image">
                  <div class="vimage-container">
                    <xsl:call-template name="image">
                      <xsl:with-param name="url" select="$image-url"/>
                      <xsl:with-param name="class" select="concat($image-class, ' vimage')"/>
                    </xsl:call-template>
                  </div>
                </xsl:if>
                <xsl:call-template name="text"/>
              </div></td></tr>
            </xsl:when>
            <!-- Mobile -->
            <xsl:when test="$size=$mobile-size">
              <tr><td>
                <xsl:call-template name="text"/>
              </td></tr>
              <tr><td>
                <xsl:call-template name="logo"/>
              </td></tr>
            </xsl:when>
            <!-- Horizontal image/text -->
            <xsl:when test="$creative-count=1">
              <tr>
                <xsl:choose>
                  <xsl:when test="$with-image">
                    <td class="one-image">
                      <xsl:call-template name="image">
                        <xsl:with-param name="url" select="$image-url"/>
                        <xsl:with-param name="class" select="concat($image-class, ' one-image')"/>
                      </xsl:call-template>
                    </td>
                    <td class="one-text">
                      <xsl:call-template name="text"/>
                    </td>
                  </xsl:when>
                  <xsl:otherwise>
                    <td class="one">
                      <xsl:call-template name="text"/>
                    </td>
                  </xsl:otherwise>
                </xsl:choose>
              </tr>
            </xsl:when>
            <xsl:otherwise>
              <tr>
                <xsl:choose>
                  <xsl:when test="$with-image">
                    <td class="himage {$image-class}">
                      <xsl:call-template name="image">
                        <xsl:with-param name="url" select="$image-url"/>
                        <xsl:with-param name="class" select="concat($image-class, ' himage')"/>
                      </xsl:call-template>
                    </td><td class="text">
                      <xsl:call-template name="text"/>
                    </td>
                  </xsl:when>
                  <xsl:otherwise>
                    <td>
                      <xsl:call-template name="text"/>
                    </td>
                  </xsl:otherwise>
                </xsl:choose>
              </tr>
            </xsl:otherwise>
          </xsl:choose>
        </table>
      </div>
    </a>
  </xsl:template>
  <xsl:template name="text">
    <xsl:apply-templates select="token[@name='HEADLINE']"/>
    <xsl:if test="$size!=$mobile-size">
      <!-- Don't use with mobile -->
      <xsl:apply-templates select="token[@name='DESCRIPTION1' and string-length()>0]"/>
      <xsl:apply-templates select="token[@name='DESCRIPTION2' and string-length()>0]"/>
    </xsl:if>
    <xsl:apply-templates select="token[@name='DISPLAY_URL']"/>
  </xsl:template>
  <xsl:template match="token">
    <xsl:choose>
      <xsl:when test="contains(@name, 'PIXEL')">
        <xsl:if test="string-length()>0">
          <img src="{.}" width="0" height="0" style="visibility:hidden; position:absolute;"/>
        </xsl:if>
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
        <xsl:value-of select="' '"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>

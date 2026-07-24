#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>

#include <CampaignSvcs/CampaignManager/CreativeTextGenerator.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManagerCore.hpp>
#include <CampaignSvcs/CampaignManager/CreativeInstantiator.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs;
  using namespace AdServer::CampaignSvcs::AdInstances;

  const char TEMPLATE_BODY[] =
    R"bsfm(<html><head><meta http-equiv=Content-Type content="text/html; charset=UT)bsfm"
    R"bsfm(F-8"><meta http-equiv=X-UA-Compatible content="IE=Edge"><link rel=icon h)bsfm"
    R"bsfm(ref="data:;base64,iVBORw0KGgo="></head><body style="margin:0; padding:0;)bsfm"
    R"bsfm( overflow:hidden;"><script type=text/javascript>!function(b){function c()bsfm"
    R"bsfm(d){return'<html><head><meta http-equiv="Content-Type" content="text/html)bsfm"
    R"bsfm(;charset=UTF-8"/><base href="##ADIMAGE-SERVER##/templates/video/"/><link)bsfm"
    R"bsfm( rel="icon" href="data:;base64,iVBORw0KGgo="/><link rel="stylesheet" typ)bsfm"
    R"bsfm(e="text/css" href="'+d.lc+'" crossorigin="use-credentials"/><script type)bsfm"
    R"bsfm(="text/javascript">d ={params:{host: "##ADSERVER##", tid: "##TAGID=##", )bsfm"
    R"bsfm(ccid: "##CCID=##"}, swf: "##ADIMAGE-SERVER##/templates/video/video-js/vi)bsfm"
    R"bsfm(deo-js2.swf", videowidth: "##VIDEO_WIDTH=##", random: "##mime-url:RANDOM)bsfm"
    R"bsfm(##", autoplay: ##VIDEO_AUTO_START=1##, muted: ##VIDEO_MUTED_START=1##, l)bsfm"
    R"bsfm(oop: ##VIDEO_LOOP=1##, pixel: ["", "##js:PUBL_TAG_TRACK_PIXEL=##", ""], )bsfm"
    R"bsfm(trackhtml: ["##js:TRACKHTMLURL=##"]};'+d.mc+'</'+'script><script type="t)bsfm"
    R"bsfm(ext/javascript" src="'+d.nc+'"></'+'script></head><body onload="start();)bsfm"
    R"bsfm(">'+d.oc+'</body></html>'}try{b.g=function(b,c){this.h=b||'',this.i=c||{)bsfm"
    R"bsfm(}},b.g.prototype={j:1,k:2,m:window.ActiveXObject?2048:4096,o:function(b,)bsfm"
    R"bsfm(c,e,g){var h=this;return''!==c&&null!=c||h.k&e?(c=g?c.substr(0,g):c,h.i[)bsfm"
    R"bsfm(b]=h.j&e?h.p(c,g):c,h):h},q:function(b,c){var e=this;b=b||'';var g=e.s(b)bsfm"
    R"bsfm(),h=g.length-e.m;return h>0&&c&&(e.u(c,h),g=e.s(b)),g},s:function(b){var)bsfm"
    R"bsfm( c=this.i,e=this.h+'?';for(var g in c)c.hasOwnProperty(g)&&(e+=this.C(g))bsfm"
    R"bsfm();return e=e.slice(0,-1),b&&(e+=('&'==b.charAt(0)?'':'&')+b),e},C:functi)bsfm"
    R"bsfm(on(b){return b+'='+this.i[b]+'&'},u:function(b,c){var e=this.i[b];this.i)bsfm"
    R"bsfm([b]=escape_(decodeURIComponent(e),e.length-c)},p:function(b,c){do var e=)bsfm"
    R"bsfm(encodeURIComponent(b);while(c&&e.length>c&&(b=b.slice(0,-1)));return e}})bsfm"
    R"bsfm(,b.D=function(){},b.D.prototype={F:function(b,c){var e=this;return c=nul)bsfm"
    R"bsfm(l==window.PSprp_rate?c:PSprp_rate,!b||c&&!e.G(c)||(e.H=e.J(),e.H.src=b),)bsfm"
    R"bsfm(e},J:function(){var b=this;return b.H||b.K().appendChild(b.L(document.cr)bsfm"
    R"bsfm(eateElement('IFRAME')))},K:function(){var b='svc'+parseInt((new Date).ge)bsfm"
    R"bsfm(tTime()/1e6),c=document.getElementById(b);if(!c){var e=document.body||do)bsfm"
    R"bsfm(cument.documentElement;c=e.insertBefore(document.createElement('DIV'),e.)bsfm"
    R"bsfm(firstChild),c.id=b}return c},L:function(b){return b.width=1,b.height=1,b)bsfm"
    R"bsfm(.frameBorder=0,b.scrolling='no',b.style.position='absolute',b},G:functio)bsfm"
    R"bsfm(n(b){return 0==parseInt(Math.random()*b)}},b.M=function(b,c){for(var e i)bsfm"
    R"bsfm(n c)b[e]=c[e];return b},b.M(b,{N:function(c,e,g){var h=b.O(),i=c+': '+(e)bsfm"
    R"bsfm(.message||e);h!=top?window.console&&console.log(i):(h.PSerr||(h.PSerr=[])bsfm"
    R"bsfm()).push(i),g=g||{},'object'==typeof e&&b.R([b.T({app:'adclient',op:'view)bsfm"
    R"bsfm(',res:'F',tid:g.tid||'',ccid:g.ccid||''},g.host)])},R:function(c,e){if(!)bsfm"
    R"bsfm(e)var g=b.V(),e=g.W=g.W||[];for(var h,i=0;c&&i<c.length;i++)c[i]&&(h=new)bsfm"
    R"bsfm( Image,h.src=c[i],e.push(h))},X:function(c){for(var e=0;c&&e<c.length;e+)bsfm"
    R"bsfm(+)c[e]&&(new b.D).F(c[e])},T:function(c,e){e=e||'##ADSERVER=##';var g=c.)bsfm"
    R"bsfm(tid||'##TAGID=##',h=c.ccid||'##CCID=##';return new b.g(e+"/sl.gif",c).o()bsfm"
    R"bsfm('app',c.app||"adtempl").o('src',c.src||"creative").o('tid',g).o('ccid',h)bsfm"
    R"bsfm().o('rnd',this.Y()).q()},Y:function(){return parseInt(1e7*Math.random()))bsfm"
    R"bsfm(},V:function(c){var e=b.O();return PSenv=e.PSenv||(e.PSenv={}),c?PSenv[c)bsfm"
    R"bsfm(]||(PSenv[c]={}):PSenv},Pa:function(c,e){var g=b.V(c),h=g.t&&g.t.ret()||)bsfm"
    R"bsfm({};return e?h[e]:h},O:function(){try{return top.document&&top||window}ca)bsfm"
    R"bsfm(tch(b){try{return parent.document&&parent||window}catch(b){}}return wind)bsfm"
    R"bsfm(ow},Z:function(b){return b=b||document,'BackCompat'==b.compatMode&&b.bod)bsfm"
    R"bsfm(y?b.body:b.documentElement}}),b.Ia=function(b,c,e){return e=e||window,e.)bsfm"
    R"bsfm(removeEventListener?e.removeEventListener(b,c,!1)||1:e.detachEvent?e.det)bsfm"
    R"bsfm(achEvent('on'+b,c):void 0},b.Ja=function(b,c,e){return e=e||window,e.add)bsfm"
    R"bsfm(EventListener?e.addEventListener(b,c,!1)||1:e.attachEvent?e.attachEvent()bsfm"
    R"bsfm('on'+b,c):void 0},b.Ka=function(c){var e=c.ia,g=c.ja,h=c.Qa,i=function(b)bsfm"
    R"bsfm(){return b.id=h,b.width=e,b.height=g,b.frameBorder=0,b.marginWidth=0,b.m)bsfm"
    R"bsfm(arginHeight=0,b.scrolling='no',b.allowTransparency=!0,b},j=function(){re)bsfm"
    R"bsfm(turn'<iframe id="'+h+'" width="'+e+'" height="'+g+'" frameborder="0" mar)bsfm"
    R"bsfm(ginheight="0" marginwidth="0" scrolling="no"></iframe>'},k=function(c){b)bsfm"
    R"bsfm(.Ja('beforeunload',function(){try{c.parentNode.removeChild(c)}catch(b){})bsfm"
    R"bsfm(})};return{ca:function(){var e=this,g=document,h=b.Pa(c.Ra,'node');retur)bsfm"
    R"bsfm(n h?(e.La=i(g.createElement('IFRAME')),'HEAD'==h.parentNode.tagName?(g.b)bsfm"
    R"bsfm(ody||g.documentElement).appendChild(e.La):h.parentNode.insertBefore(e.La)bsfm"
    R"bsfm(,h)):(e.Ma=1,g.write(j())),this},Na:function(b){var c=this.Oa();return c)bsfm"
    R"bsfm(.src=b,k(c),c},Oa:function(){return this.La||(this.La=document.getElemen)bsfm"
    R"bsfm(tById(h))}}},b.Sa=function(c){var e=this;e.i=c,e.ia=c.Ta||c.ia,e.ja=c.Ua)bsfm"
    R"bsfm(||c.ja,e.Va=c.Wa||c.Xa;var g='##mime-url:RANDOM##',h='##ADIMAGE-SERVER##)bsfm"
    R"bsfm('||'//'+"media.targetrtb.com";b.M(c,{Ya:h+"/templates/",Ra:g,Za:'c'+g,Qa)bsfm"
    R"bsfm(:'p'+g})},b.Sa.prototype={$a:function(b){var c=this,e=c.i;return b=b||c.)bsfm"
    R"bsfm(_a(),b=b&&e.ab?c.bb(b):b,Number(e.eb)&&c.Va&&((new Image).src=c.Va,setTi)bsfm"
    R"bsfm(meout(function(){c.fb()},1e3*e.eb)),b&&c.gb(b)},fa:function(c,e,g){try{v)bsfm"
    R"bsfm(ar h=this,i=h.$a(),j=c.contentWindow;j.document.write(i),j.setTimeout('d)bsfm"
    R"bsfm(ocument.close()',2e3),e&&e()}catch(k){if(c.src)if(c.parentNode.removeChi)bsfm"
    R"bsfm(ld(c),!g||3>g)try{g=g+1||1,h.fa(b.Ka(this.i).ca().Oa(),e,g)}catch(k){b.N)bsfm"
    R"bsfm((204,k)}else b.N(205,k);else c.src='javascript:"<script>try{document.dom)bsfm"
    R"bsfm(ain=\''+document.domain+'\';}catch(e){}</'+'script>"',i='<script>documen)bsfm"
    R"bsfm(t.domain=\''+document.domain+'\';</'+'script>'+i,setTimeout(function(){h)bsfm"
    R"bsfm(.fa(c,e,g)},100)}},bb:function(b){return/<\/html>\s*$/i.test(b)?b:'<html)bsfm"
    R"bsfm(><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8)bsfm"
    R"bsfm("/><link rel="icon" href="data:;base64,iVBORw0KGgo="/></head><body>'+b+')bsfm"
    R"bsfm(</body></html>'},fb:function(){try{var c=this.hb(),e=this.ib(this.Va);'B)bsfm"
    R"bsfm(ODY'==c.tagName?c.innerHTML=e:c.outerHTML=e}catch(g){b.N(222,g.message)})bsfm"
    R"bsfm(},gb:function(b){var c=this.i,e='##js:AD_FOOTER_PRESENT=##'.toLowerCase()bsfm"
    R"bsfm(),g=('a'==e||'yes'!=e&&c.jb&&'yes'==c.jb.toLowerCase())&&c.kb?this.lb():)bsfm"
    R"bsfm('';return this.mb(b,g)||'<div id="'+c.Qa+'" style="position:relative; wi)bsfm"
    R"bsfm(dth:'+this.ia+'px; height:'+this.ja+'px;">'+b+g+'</div>'},lb:function(){)bsfm"
    R"bsfm(var b=this.i,c=b.ia*b.ja>11360?'cm.png':'cmi.png',e=b.pc?this.qc:0;retur)bsfm"
    R"bsfm(n'<a href="'+b.kb+'" target="_blank" style="display:block; position:abso)bsfm"
    R"bsfm(lute; right:0; top:'+e+'px; z-index:100000000; outline:0;"><img src="'+b)bsfm"
    R"bsfm(.Ya+'img/'+c+'" alt="AdChoices" style="border:0 none; padding:0; margin:)bsfm"
    R"bsfm(0;"/></a>'},mb:function(b,c){var e=/<\/body>|<\/html>/i;return e.test(b))bsfm"
    R"bsfm(?b.replace(e,c+'$&'):''},hb:function(){var b=this.i,c=document,e=c.getEl)bsfm"
    R"bsfm(ementById(b.Za);if(e)return e;var g=c.getElementById(b.Qa),h='IFRAME'==g)bsfm"
    R"bsfm(.tagName?g:g.getElementsByTagName('IFRAME')[0];return h.contentWindow.do)bsfm"
    R"bsfm(cument.getElementById(b.Za)},nb:function(b){return!/^(https?\:)?\/\/\w+/)bsfm"
    R"bsfm(.test(b)},ib:function(b){var c=this.i,e=c.ob?' alt="'+c.ob+'" title="'+c)bsfm"
    R"bsfm(.ob+'"':'',g=c.Vb?' onclick="'+c.Vb+'"':'',h=c.Wb?' onmouseover="'+c.Wb+)bsfm"
    R"bsfm('"':'',i=c.Xb?' onfocus="'+c.Xb+'"':'';return(c.Yb||'')+'<a id="'+c.Za+')bsfm"
    R"bsfm(" href="'+c.qb+'" oncontextmenu="return false;" target="_blank" style="d)bsfm"
    R"bsfm(isplay:block; width:'+this.ia+'px; height:'+this.ja+'px;"><img src="'+(b)bsfm"
    R"bsfm(||c.Zb||c.Xa)+'" style="border:0 none; margin:0; padding:0; width:100%; )bsfm"
    R"bsfm(height:100%;"'+e+g+h+i+'/></a>'}},b.rb=function(c){var e=function(){};re)bsfm"
    R"bsfm(turn e.prototype=new b.Sa(c),b.M(e.prototype,{_a:function(){var b=this,e)bsfm"
    R"bsfm(=c.aa;return b.nb(e)?e:'<iframe id="'+c.Za+'" src="'+e+'" width="'+b.ia+)bsfm"
    R"bsfm('" height="'+b.ja+'" frameborder="0" marginheight="0" marginwidth="0" sc)bsfm"
    R"bsfm(rolling="no"></iframe>'}}),new e},b.Gb=function(b){this.Hb=b},b.$=functi)bsfm"
    R"bsfm(on(){return'loading'==document.readyState||document.documentMode<10&&'in)bsfm"
    R"bsfm(teractive'==document.readyState},b.Ib=function(c){var e=c.i,g=function())bsfm"
    R"bsfm({b.R(e.Jb)},h=function(){};return h.prototype=new b.Gb(c),b.M(h.prototyp)bsfm"
    R"bsfm(e,{Kb:function(){var h=b.V(e.Ra),i=document.getElementById('s'+e.Ra);(h.)bsfm"
    R"bsfm(a||i&&'HEAD'==i.parentNode.tagName||!b.$())&&(e.ab=1),e.ab?this.Lb():(do)bsfm"
    R"bsfm(cument.write(c.$a()),g())},Lb:function(){var h=b.Ka(e).ca().Oa();h.Ma?se)bsfm"
    R"bsfm(tTimeout(function(){c.fa(h,g)},100):c.fa(h,g)}}),new h},b.Qb=function(b,)bsfm"
    R"bsfm(c){return('return (window.PSenv || parent.PSenv)["##mime-url:RANDOM##"].)bsfm"
    R"bsfm(cb.run('+(c&&'"'+c+'"'||'event')+', '+(b||'{}')+');').replace(/"/g,'&quo)bsfm"
    R"bsfm(t;')},b.Rb=function(c){var e=b.V(c);return e.cb?e.cb:(e.cb=this,this.ba=)bsfm"
    R"bsfm(c,void(this.Sb={}))},b.Rb.prototype={run:function(b,c){var e=this.Sb[b.t)bsfm"
    R"bsfm(ype||b],g=!0;if(e){c=c||{},c.Tb=b;for(var h=0;h<e.length;h++)'function'=)bsfm"
    R"bsfm(=typeof e[h]&&(g=e[h].call(this,c,h)&&g)}return g},Ub:function(b,c){retu)bsfm"
    R"bsfm(rn(this.Sb[b]||(this.Sb[b]=[])).push(c),this}},function(){var c='click';)bsfm"
    R"bsfm(new b.Rb('##mime-url:RANDOM##').Ub(c,function(b){var c=b.clk,e=/\*amp\*m)bsfm"
    R"bsfm(\*eql\*f\d?|\&m\=f\d?/; function pix677_land(){var sfmb654=new Image().s)bsfm"
    R"bsfm(rc='##ADVCLICKTRACKING=##';var sfm787=new Image().src='##ADVCLICKTRACKIN)bsfm"
    R"bsfm(G2=##';var sfm789=new Image().src='##ADVCLICKTRACKING3=##'}; pix677_land)bsfm"
    R"bsfm((); return c=1==this._b?c.replace(e,''):c,b.win=open(c,'_blank'),!0}).Ub)bsfm"
    R"bsfm((c,function(c){return b.R(c.px),!0}).Ub(c,function(c){try{var e=c.Tb;if()bsfm"
    R"bsfm(e.offsetX>0&&e.offsetY>0||e.pageX>0&&e.pageY>0){var g=function(){for(var)bsfm"
    R"bsfm( b={IMG:1,A:1,VIDEO:1,BODY:1},c=e.target||e.srcElement;c;c=c.parentNode))bsfm"
    R"bsfm(if(b[c.tagName]||c.className&&(c.className.indexOf('text')>-1||c.classNa)bsfm"
    R"bsfm(me.indexOf('creative')>-1))return c}(),h=function(){return document.crea)bsfm"
    R"bsfm(teTouch?Math.min(window.innerHeight/document.documentElement.clientHeigh)bsfm"
    R"bsfm(t,window.innerWidth/document.documentElement.clientWidth):1}(),i=functio)bsfm"
    R"bsfm(n(){return'IMG'==(e.target||e.srcElement).tagName?'cli':'cl'},j=function)bsfm"
    R"bsfm((){if('LI'==g.parentNode.tagName)throw'NOT SUP';return e.offsetX?Math.ce)bsfm"
    R"bsfm(il(e.offsetX/h/g.offsetWidth*5):Math.ceil((e.pageX/h-m('offsetLeft'))/g.)bsfm"
    R"bsfm(offsetWidth*5)},k=function(){return e.offsetY?Math.ceil(e.offsetY/h/g.of)bsfm"
    R"bsfm(fsetHeight*5):Math.ceil((e.pageY/h-m('offsetTop'))/g.offsetHeight*5)},m=)bsfm"
    R"bsfm(function(b){for(var c=0,e=g;e;e=e.offsetParent)c+=e[b]||0;return c};b.R()bsfm"
    R"bsfm([b.T({op:i()+j()+'-'+k(),res:'S',ccid:c.ccid})])}}catch(n){b.N(213,n.mes)bsfm"
    R"bsfm(sage)}return!0}).Ub(c,function(c){try{var e=c.win;if(!e)return!0;for(var)bsfm"
    R"bsfm( g=[3,5,10,20,30,60,120],h=function(g){setTimeout(function(){e&&!e.close)bsfm"
    R"bsfm(d&&b.R([b.T({op:'click-'+g,res:'S',ccid:c.ccid})])},1e3*g)};g.length;)h()bsfm"
    R"bsfm(g.shift());return!1}catch(i){return b.N(214,i.message),!0}})}();var e=fu)bsfm"
    R"bsfm(nction(){var b='';return b+='d.click="##js:CLICK##";'},g=function(){retu)bsfm"
    R"bsfm(rn Number('##VIDEO_WIDTH=##')?'<img src="##xml:IMAGE_FILE=##" alt="##AD_)bsfm"
    R"bsfm(TITLE=##" title="##AD_TITLE=##"'+' onclick="'+b.Qb('{clk:"##js:CLICK##"})bsfm"
    R"bsfm(')+'"'+' style="width:##WIDTH##px; height:##HEIGHT##px; position:relativ)bsfm"
    R"bsfm(e;"/>'+'<div style="position:absolute; ##xml:VIDEO_CSS=##">'+h('##VIDEO_)bsfm"
    R"bsfm(WIDTH=##','##VIDEO_HEIGHT=##')+'</div>':h('##WIDTH##','##HEIGHT##')},h=f)bsfm"
    R"bsfm(unction(b,c){return'<video id="video_ad" oncanplay="onCanPlay();" class=)bsfm"
    R"bsfm("video-js vjs-default-skin vjs-big-play-centered" width="'+b+'" height=")bsfm"
    R"bsfm('+c+'">'+i()+'</video>'},i=function(){var b=['##js:MP4_LOW169=##','##js:)bsfm"
    R"bsfm(MP4_ST169=##','##js:OGG_LOW169=##','##js:OGG_ST169=##','##js:WEBM_LOW169)bsfm"
    R"bsfm(=##','##js:WEBM_ST169=##'],c=['##js:MP4_LOW43=##','##js:MP4_ST43=##','##)bsfm"
    R"bsfm(js:OGG_LOW43=##','##js:OGG_ST43=##','##js:WEBM_LOW43=##','##js:WEBM_ST43)bsfm"
    R"bsfm(=##'],e=['video/mp4','video/mp4','video/ogg','video/ogg','video/webm','v)bsfm"
    R"bsfm(ideo/webm','video/mp4','video/mp4','video/ogg','video/ogg','video/webm',)bsfm"
    R"bsfm('video/webm'],g=function(b){src_='';for(var c=0;c<b.length;c++)b[c]&&(sr)bsfm"
    R"bsfm(c_+='<source src="'+b[c]+'" type="'+e[c]+'" onerror="onError(event)"/>'))bsfm"
    R"bsfm(;return src_};return g(Number('##WIDTH##')/Number('##HEIGHT##')>1.5?b.co)bsfm"
    R"bsfm(ncat(c):c.concat(b))},j={lc:'video.css',nc:'video.js',mc:e(),oc:g()};b.I)bsfm"
    R"bsfm(b(b.rb({ab:1,aa:c(j),ia:'##WIDTH##',ja:'##HEIGHT##',jb:'##js:AD_FOOTER_E)bsfm"
    R"bsfm(NABLED=##',kb:'##js:AD_FOOTER_URL=##'})).Kb(),function(){try{var c='##US)bsfm"
    R"bsfm(ERSTATUS=##',e='##PUBPIXELSOPTIN=##',g='##PUBPIXELS=##',h='##PUBPIXELSOP)bsfm"
    R"bsfm(TOUT=##';1==c?(new b.D).F(e):1==g&&(new b.D).F(h)}catch(i){b.N(232,i.mes)bsfm"
    R"bsfm(sage)}}(),function(){try{b.Ob=function(){return[]};var c='##ADSERVER##',)bsfm"
    R"bsfm(e=new RegExp('(https?:)?\\/\\/'+"media.targetrtb.com".replace(/\W/g,'\\$)bsfm"
    R"bsfm(&'),'i');if(e.test(c)){var g=b.Ob();g&&g.length||(g.length=1);for(var h=)bsfm"
    R"bsfm(0;h<g.length;h++)(new b.D).F(c+"/tag/"+'container.html')}}catch(i){b.N(2)bsfm"
    R"bsfm(27,i.message)}}(),b.Fa=function(c){var e=c.getBoundingClientRect(),g=e.w)bsfm"
    R"bsfm(idth||c.offsetWidth,h=e.height||c.offsetHeight,i=b.Z(),j=Math.max(0,Math)bsfm"
    R"bsfm(.min(i.clientWidth,Math.min(g,Math.min(e.left+g-i.scrollLeft,i.scrollLef)bsfm"
    R"bsfm(t+i.clientWidth-e.left)))),k=Math.max(0,Math.min(i.clientHeight,Math.min)bsfm"
    R"bsfm((h,Math.min(e.top+h-i.scrollTop,i.scrollTop+i.clientHeight-e.top))));ret)bsfm"
    R"bsfm(urn Math.round(j*k/(g*h)*100)||0},function(){try{var c=b.O(),e=c.documen)bsfm"
    R"bsfm(t.getElementById('p##mime-url:RANDOM##'),g='##js:TRACKHTMLURL=##';if(e&&)bsfm"
    R"bsfm(top==c){var h=function(b,c){return b?b+(b.indexOf('?')<0?'?v='+c:'&v='+c)bsfm"
    R"bsfm():''},i=function(){try{var c=Number(e.getAttribute('data-vis')),j=b.Fa(e)bsfm"
    R"bsfm();c>=50&&j>=50?b.X([h(g,1)]):(e.setAttribute('data-vis',j),setTimeout(i,)bsfm"
    R"bsfm(2e3))}catch(k){b.N(115,k.message)}};b.X([h(g,0)]),setTimeout(i,500)}else)bsfm"
    R"bsfm( b.X([g])}catch(j){b.N(116,j.message)}}()}catch(k){b.N(200,k)}}({});func)bsfm"
    R"bsfm(tion pix2_track(opew1){new Image().src=opew1;} var timesfm1=##MP4_DURATI)bsfm"
    R"bsfm(ON=5##*250;timesfm2=##MP4_DURATION=5##*500;timesfm3=##MP4_DURATION=5##*7)bsfm"
    R"bsfm(50;timesfm4=##MP4_DURATION=5##*1000;pix2_track('##TRACKPIXEL=##&t=c&nm=v)bsfm"
    R"bsfm(start');pix2_track('##CRADVTRACKPIXELSTART=##');pix2_track('##CRADVTRACK)bsfm"
    R"bsfm(PIXEL3=##');setTimeout(function(){pix2_track('##CRADVTRACKPIXELFQ=##');})bsfm"
    R"bsfm(,timesfm1);setTimeout(function(){pix2_track('##TRACKPIXEL##&t=c&nm=vq1'))bsfm"
    R"bsfm(;},timesfm1);setTimeout(function(){pix2_track('##CRADVTRACKPIXELMP=##');)bsfm"
    R"bsfm(},timesfm2);setTimeout(function(){pix2_track('##TRACKPIXEL##&t=c&nm=vmid)bsfm"
    R"bsfm(');},timesfm2);setTimeout(function(){pix2_track('##CRADVTRACKPIXELTQ=##')bsfm"
    R"bsfm();},timesfm3);setTimeout(function(){pix2_track('##TRACKPIXEL##&t=c&nm=vq)bsfm"
    R"bsfm(3');},timesfm3);setTimeout(function(){pix2_track('##CRADVTRACKPIXELCOMPL)bsfm"
    R"bsfm(ETE=##');},timesfm4);setTimeout(function(){pix2_track('##TRACKPIXEL##&t=)bsfm"
    R"bsfm(c&nm=vcomplete');},timesfm4);pix2_track('##CRADVTRACKPIXELSTART2=##');pi)bsfm"
    R"bsfm(x2_track('##CRADVTRACKPIXEL4=##');setTimeout(function(){pix2_track('##CR)bsfm"
    R"bsfm(ADVTRACKPIXELFQ2=##');},timesfm1);setTimeout(function(){pix2_track('##CR)bsfm"
    R"bsfm(ADVTRACKPIXELMP2=##');},timesfm2);setTimeout(function(){pix2_track('##CR)bsfm"
    R"bsfm(ADVTRACKPIXELTQ2=##');},timesfm3);setTimeout(function(){pix2_track('##CR)bsfm"
    R"bsfm(ADVTRACKPIXELCOMPLETE2=##');},timesfm4);pix2_track('##CRADVTRACKPIXELSTA)bsfm"
    R"bsfm(RT3=##');pix2_track('##CRADVTRACKPIXEL5=##');setTimeout(function(){pix2_)bsfm"
    R"bsfm(track('##CRADVTRACKPIXELFQ3=##');},timesfm1);setTimeout(function(){pix2_)bsfm"
    R"bsfm(track('##CRADVTRACKPIXELMP3=##');},timesfm2);setTimeout(function(){pix2_)bsfm"
    R"bsfm(track('##CRADVTRACKPIXELTQ3=##');},timesfm3);pix2_track('##CRADVTRACKPIX)bsfm"
    R"bsfm(EL=##');pix2_track('##CRADVTRACKPIXEL2=##');setTimeout(function(){pix2_t)bsfm"
    R"bsfm(rack('##CRADVTRACKPIXELCOMPLETE3=##');},timesfm4);pix2_track('##CRADVTRA)bsfm"
    R"bsfm(CKPIXELSTART4=##');pix2_track('##CRADVTRACKPIXEL6=##');setTimeout(functi)bsfm"
    R"bsfm(on(){pix2_track('##CRADVTRACKPIXELFQ4=##');},timesfm1);setTimeout(functi)bsfm"
    R"bsfm(on(){pix2_track('##CRADVTRACKPIXELMP4=##');},timesfm2);setTimeout(functi)bsfm"
    R"bsfm(on(){pix2_track('##CRADVTRACKPIXELTQ4=##');},timesfm3);setTimeout(functi)bsfm"
    R"bsfm(on(){pix2_track('##CRADVTRACKPIXELCOMPLETE4=##');},timesfm4);</script></)bsfm"
    R"bsfm(body></html>)bsfm";

  struct Options
  {
    unsigned long count = 0;
    unsigned long threads = 1;
    std::string rid_private_key =
      "/home/jurij_kuznecov/projects/run/etc-ssv400/adserver/"
      "adcluster/build00/rid_private_key.der";
    std::string template_root = "/tmp/CampaignManagerInstantiateCreativeTest";
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct Fixture
  {
    ReferenceCounting::SmartPtr<CampaignConfig> campaign_config;
    Colocation_var colocation;
    Campaign_var campaign;
    Creative_var creative;
    Tag_var tag;
    Tag::Size_var tag_size;
    CampaignManagerCore::CommonAdRequest request_info;
    CampaignManagerCore::InstantiateParams instantiate_params;
    CampaignManagerCore::AdSlotContext ad_slot_context;
    std::shared_ptr<Generics::MonoAllocatorArena> arena;
    Generics::MonoVector<unsigned long> exclude_pubpixel_accounts;

    Fixture()
      : instantiate_params(AdServer::Commons::Optional<unsigned long>(12345)),
        arena(std::make_shared<Generics::MonoAllocatorArena>()),
        exclude_pubpixel_accounts(arena.get())
    {}
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: CampaignManagerInstantiateCreativeTest --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>                    instantiate_creative calls count\n"
      << "  --threads <N>                  worker threads (default: 1)\n"
      << "  --rid-private-key <p>          RID private key path\n"
      << "  --template-root <p>            temp template directory\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    StringOption opt_rid_private_key(
      "/home/jurij_kuznecov/projects/run/etc-ssv400/adserver/"
      "adcluster/build00/rid_private_key.der");
    StringOption opt_template_root("/tmp/CampaignManagerInstantiateCreativeTest");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("rid-private-key"), opt_rid_private_key);
    args.add(equal_name("template-root"), opt_template_root);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.threads = *opt_threads;
    options.rid_private_key = *opt_rid_private_key;
    options.template_root = *opt_template_root;

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if(options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    return options;
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if(getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  CreativeInstantiator::CreativeInstantiate
  make_creative_instantiate()
  {
    CreativeInstantiator::CreativeInstantiate creative_instantiate;

    CreativeInstantiateRule rule;
    rule.url_prefix = "https://ad.example/";
    rule.image_url = "https://img.example/";
    rule.publ_url = "https://pub.example/";
    rule.click_url = "https://click.example/click";
    rule.ad_server = "https://ad.example";
    rule.ad_image_server = "https://img.example";
    rule.track_pixel_url = "https://track.example/pixel";
    rule.notice_url = "https://notice.example";
    rule.action_pixel_url = "https://action.example/action";
    rule.local_passback_prefix = "https://passback.example/";
    rule.dynamic_creative_prefix = "https://dynamic.example/";
    rule.passback_template_path_prefix = "/tmp/";
    rule.passback_pixel_url = "https://passback.example/pixel";
    rule.user_bind_url = "https://bind.example";
    rule.pub_pixels_optin = "https://pub.example/optin";
    rule.pub_pixels_optout = "https://pub.example/optout";
    rule.script_instantiate_url = "https://script.example";
    rule.iframe_instantiate_url = "https://iframe.example";
    rule.direct_instantiate_url = "https://direct.example";
    rule.nonsecure_direct_instantiate_url = "http://direct.example";
    rule.video_instantiate_url = "https://video.example";
    rule.nonsecure_video_instantiate_url = "http://video.example";

    creative_instantiate.creative_rules["secure"] = rule;

    return creative_instantiate;
  }

  void
  write_template_file(const std::filesystem::path& path)
  {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(path, std::ios::binary);
    if(!out)
    {
      throw std::runtime_error("can't create template file: " + path.string());
    }

    out.write(TEMPLATE_BODY, sizeof(TEMPLATE_BODY) - 1);
    if(!out)
    {
      throw std::runtime_error("can't write template file: " + path.string());
    }
  }

  RevenueDecimal
  revenue(unsigned long value)
  {
    return RevenueDecimal(false, value, 0);
  }

  Account_var
  make_account(unsigned long account_id, const Currency_var& currency)
  {
    Account_var account(new AccountDef());
    account->account_id = account_id;
    account->internal_account_id = account_id;
    account->flags = 0;
    account->at_flags = AccountTypeFlags::USE_SELF_BUDGET;
    account->text_adserving = 'A';
    account->currency = currency;
    account->country = "RU";
    account->commision = RevenueDecimal::ZERO;
    account->media_handling_fee = RevenueDecimal::ZERO;
    account->budget = revenue(1000000);
    account->paid_amount = RevenueDecimal::ZERO;
    account->status = 'A';
    account->eval_status = 'A';
    account->auction_rate = AR_GROSS;
    account->use_pub_pixels = false;
    account->self_service_commission = RevenueDecimal::ZERO;
    account->set_available(true);
    return account;
  }

  Fixture
  make_fixture(const std::filesystem::path& template_path)
  {
    Fixture fixture;

    fixture.campaign_config = new CampaignConfig();
    fixture.campaign_config->default_click_token_processor =
      BaseTokenProcessor::default_token_processor(
        CreativeTokens::ADV_CLICK_URL.c_str());

    Currency_var currency(new Currency());
    currency->currency_id = 1;
    currency->currency_exchange_id = 1;
    currency->effective_date = 0;
    currency->fraction = 1;
    currency->currency_code = "RUB";
    currency->rate = RevenueDecimal::div(revenue(1), revenue(1));
    fixture.campaign_config->currencies[currency->currency_id] = currency;
    fixture.campaign_config->currency_codes.emplace(
      Generics::StringHashAdapter(currency->currency_code),
      currency);

    Account_var publisher = make_account(9803, currency);
    Account_var account = make_account(11081, currency);
    Account_var advertiser = make_account(12028, currency);
    fixture.campaign_config->accounts[publisher->account_id] = publisher;
    fixture.campaign_config->accounts[account->account_id] = account;
    fixture.campaign_config->accounts[advertiser->account_id] = advertiser;

    Site_var site(new Site());
    site->site_id = 11;
    site->freq_cap_id = 0;
    site->noads_timeout = 0;
    site->flags = 0;
    site->account = publisher;
    site->status = 'A';
    fixture.campaign_config->sites[site->site_id] = site;

    Size_var size(new Size());
    size->size_id = 9;
    size->protocol_name = "300x250";
    size->size_type_id = 1;
    size->width = 300;
    size->height = 250;
    fixture.campaign_config->sizes[size->size_id] = size;

    fixture.tag = new Tag();
    fixture.tag->tag_id = 15037;
    fixture.tag->site = site;
    fixture.tag->flags = 0;
    fixture.tag->marketplace = 'O';
    fixture.tag->adjustment = RevenueDecimal::ZERO;
    fixture.tag->allow_expandable = false;
    fixture.tag->min_visibility = 0;
    fixture.tag->auction_max_ecpm_share = RevenueDecimal::ZERO;
    fixture.tag->auction_prop_probability_share = RevenueDecimal::ZERO;
    fixture.tag->auction_random_share = RevenueDecimal::ZERO;
    fixture.tag->pub_max_random_cpm = RevenueDecimal::ZERO;
    fixture.tag->max_random_cpm = RevenueDecimal::ZERO;
    fixture.tag->cost_coef = revenue(1);
    fixture.tag->skip_min_ecpm = true;

    fixture.tag_size = new Tag::Size();
    fixture.tag_size->size = size;
    fixture.tag_size->max_text_creatives = 0;
    fixture.tag->sizes[size->size_id] = fixture.tag_size;
    fixture.campaign_config->tags[fixture.tag->tag_id] = fixture.tag;

    fixture.colocation = new Colocation();
    fixture.colocation->colo_id = 693;
    fixture.colocation->colo_rate_id = 1;
    fixture.colocation->at_flags = 0;
    fixture.colocation->account = account;
    fixture.colocation->revenue_share = RevenueDecimal::ZERO;
    fixture.colocation->ad_serving = CS_ALL;
    fixture.colocation->hid_profile = false;
    fixture.campaign_config->colocations[fixture.colocation->colo_id] = fixture.colocation;

    fixture.campaign = new Campaign();
    fixture.campaign->campaign_id = 236995;
    fixture.campaign->campaign_group_id = 236995;
    fixture.campaign->account = account;
    fixture.campaign->advertiser = advertiser;
    fixture.campaign->fc_id = 0;
    fixture.campaign->group_fc_id = 0;
    fixture.campaign->ccg_rate_id = 1;
    fixture.campaign->ccg_rate_type = CR_CPM;
    fixture.campaign->flags = 0;
    fixture.campaign->marketplace = 'O';
    fixture.campaign->status = 'A';
    fixture.campaign->eval_status = 'A';
    fixture.campaign->country = "RU";
    fixture.campaign->imp_revenue = revenue(1);
    fixture.campaign->click_revenue = RevenueDecimal::ZERO;
    fixture.campaign->click_sys_revenue = RevenueDecimal::ZERO;
    fixture.campaign->action_revenue = RevenueDecimal::ZERO;
    fixture.campaign->commision = RevenueDecimal::ZERO;
    fixture.campaign->ccg_type = CT_DISPLAY;
    fixture.campaign->targeting_type = 'C';
    fixture.campaign->start_user_group_id = 0;
    fixture.campaign->end_user_group_id = 99;
    fixture.campaign->ctr_reset_id = 0;
    fixture.campaign->mode = CM_NON_RANDOM;
    fixture.campaign->min_uid_age = Generics::Time::ZERO;
    fixture.campaign->seq_set_rotate_imps = 0;
    fixture.campaign->delivery_coef = TAG_DELIVERY_MAX;
    fixture.campaign->max_pub_share = RevenueDecimal::ZERO;
    fixture.campaign->bid_strategy = BS_MAX_REACH;

    Creative::CategorySet categories;
    fixture.creative = new Creative(
      fixture.campaign,
      2527264,
      857412,
      0,
      1,
      "html",
      "1",
      OptionValue(1, "https://advertiser.example/landing"),
      "advertiser.example",
      "advertiser.example",
      categories);
    fixture.creative->https_safe_flag = true;

    Creative::Size creative_size;
    creative_size.size = size;
    creative_size.up_expand_space = 0;
    creative_size.right_expand_space = 0;
    creative_size.down_expand_space = 0;
    creative_size.left_expand_space = 0;
    creative_size.expandable = false;
    creative_size.tokens[CreativeTokens::WIDTH] = OptionValue(0, "300");
    creative_size.tokens[CreativeTokens::HEIGHT] = OptionValue(0, "250");
    creative_size.available_appformats.insert("html");
    fixture.creative->sizes[size->size_id] = creative_size;

    fixture.campaign->add_creative(fixture.creative);
    fixture.campaign_config->campaigns[fixture.campaign->campaign_id] =
      fixture.campaign;
    fixture.campaign_config->creatives[fixture.creative->creative_id] =
      fixture.creative;
    fixture.campaign_config->campaign_creatives.emplace(
      Generics::NumericHashAdapter<unsigned long>(fixture.creative->ccid),
      fixture.creative);

    ReferenceCounting::SmartPtr<RCOptionTokenValueMap> template_tokens(
      new RCOptionTokenValueMap());
    ReferenceCounting::SmartPtr<RCOptionTokenValueMap> template_hidden_tokens(
      new RCOptionTokenValueMap());

    CreativeTemplate creative_template(
      template_path.c_str(),
      CreativeTemplateFactory::Handler::CTT_TEXT,
      "text/html;charset=utf-8",
      false,
      template_tokens,
      template_hidden_tokens,
      Generics::Time::get_time_of_day());
    fixture.campaign_config->creative_templates.insert(
      CreativeTemplateKey("html", "300x250", "html"),
      creative_template);

    fixture.request_info.time = Generics::Time::get_time_of_day();
    fixture.request_info.request_id =
      AdServer::Commons::RequestId::create_random_based();
    fixture.request_info.creative_instantiate_type = "secure";
    fixture.request_info.request_type = AR_OPENRTB_WITH_CLICKURL;
    fixture.request_info.random = 3311422;
    fixture.request_info.colo_id = 693;
    fixture.request_info.external_user_id = "msc/test5";
    fixture.request_info.source_id = "msc";
    fixture.request_info.referer = "http://uralsib.ru";
    fixture.request_info.peer_ip = "213.33.171.240";
    fixture.request_info.user_agent =
      "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) "
      "Gecko/20100101 Firefox/73.0";
    fixture.request_info.user_status = US_OPTIN;
    fixture.request_info.tokens.push_back(
      CampaignManagerCore::TokenInfo{CreativeTokens::TAGID, "bad-tag"});

    fixture.instantiate_params.generate_pubpixel_accounts = true;
    fixture.instantiate_params.publisher_account_id = 9803;
    fixture.instantiate_params.init_source_macroses = true;

    fixture.ad_slot_context.test_request = false;
    fixture.ad_slot_context.request_blacklisted = false;
    fixture.ad_slot_context.publisher_account_id = 9803;
    fixture.ad_slot_context.tag_size = "300x250";

    return fixture;
  }

  void
  instantiate_once(
    CreativeInstantiator& creative_instantiator,
    Fixture& fixture,
    std::atomic<unsigned long>& checksum)
  {
    AdSelectionResult ad_selection_result;
    ad_selection_result.tag = fixture.tag;
    ad_selection_result.tag_size = fixture.tag_size;
    ad_selection_result.auction_type = AT_MAX_ECPM;

    CampaignSelectionData selection;
    selection.request_id = fixture.request_info.request_id;
    selection.campaign = fixture.campaign;
    selection.creative = fixture.creative;
    selection.ecpm = revenue(1);
    selection.ecpm_bid = revenue(1);
    selection.actual_cpc = RevenueDecimal::ZERO;
    selection.ctr = RevenueDecimal::ZERO;
    selection.conv_rate = RevenueDecimal::ZERO;
    selection.campaign_imps = 0;
    selection.count_impression = true;
    selection.track_impr = true;
    selection.selection_done = true;
    ad_selection_result.selected_campaigns.push_back(selection);

    CampaignManagerCore::RequestResultParams request_result_params;
    CampaignManagerCore::CreativeParamsList creative_params_list;
    std::string creative_body;

    creative_instantiator.instantiate_creative(
      fixture.request_info,
      fixture.campaign_config,
      fixture.colocation,
      fixture.instantiate_params,
      "html",
      ad_selection_result,
      request_result_params,
      creative_params_list,
      creative_body,
      fixture.ad_slot_context,
      &fixture.exclude_pubpixel_accounts);

    const std::string expected_tag_token =
      std::string("tid: \"") + String::StringManip::IntToStr(
        fixture.tag->tag_id).str().str() + "\"";
    if(creative_body.find(expected_tag_token) == std::string::npos)
    {
      throw std::runtime_error(
        "system TAGID token was not instantiated");
    }

    checksum.fetch_add(
      creative_body.size() + request_result_params.mime_format.size(),
      std::memory_order_relaxed);
  }

  CreativeInstantiator::Config
  make_creative_instantiator_config(const std::filesystem::path& template_path)
  {
    CreativeInstantiator::Config result;
    result.service_index = "1_1";
    result.post_instantiate_script_mime_format = "text/html";
    result.post_instantiate_iframe_mime_format = "text/html";
    result.post_instantiate_script_template_file = template_path.string();
    result.post_instantiate_iframe_template_file = template_path.string();
    result.instantiate_track_html_file = template_path.string();
    return result;
  }
}

namespace AdServer::CampaignSvcs::CTR
{
  CTRProviderImpl::CTRProviderImpl(
    const String::SubString&,
    const Generics::Time& config_timestamp,
    Generics::TaskRunner*)
    : config_timestamp_(config_timestamp),
      remove_config_files_at_destroy_(false)
  {}

  CTRProviderImpl::~CTRProviderImpl() noexcept = default;

  CTRProvider::Calculation_var
  CTRProviderImpl::create_calculation(const CampaignSelectParams*) const noexcept
  {
    return nullptr;
  }

  Generics::Time
  CTRProviderImpl::check_config_appearance(
    std::string&,
    const String::SubString&)
  {
    return Generics::Time::ZERO;
  }

  void
  CTRProviderImpl::remove_config_files_at_destroy(bool val) const noexcept
  {
    remove_config_files_at_destroy_ = val;
  }

  CTRProviderImpl::Calculation::~Calculation() noexcept = default;

  CTRProvider::CalculationContext_var
  CTRProviderImpl::Calculation::create_context(const Tag::Size*) const noexcept
  {
    return nullptr;
  }

  std::string
  CTRProviderImpl::Calculation::algorithm_id(const Creative*) const noexcept
  {
    return std::string();
  }

  CTRProviderImpl::CalculationContext::~CalculationContext() noexcept = default;

  RevenueDecimal
  CTRProviderImpl::CalculationContext::get_ctr(const Creative*) const
  {
    return RevenueDecimal::ZERO;
  }

  bool
  CTRProviderImpl::CalculationContext::check_rate(
    const Creative*,
    RevenueDecimal* rate,
    bool* creative_dependent) const
  {
    if(rate)
    {
      *rate = RevenueDecimal::ZERO;
    }

    if(creative_dependent)
    {
      *creative_dependent = false;
    }

    return true;
  }

  void
  CTRProviderImpl::CalculationContext::get_ctr_details(
    CTRList& ctrs,
    const Creative*) const
  {
    ctrs.clear();
  }

}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const std::filesystem::path template_path =
      std::filesystem::path(options.template_root) / "instantiate-template.txt";
    write_template_file(template_path);

    CreativeInstantiator::CreativeInstantiate creative_instantiate =
      make_creative_instantiate();

    Logging::Logger_var logger(new Logging::Null::Logger);
    CampaignManagerCore::CountryList country_whitelist;
    PassbackTemplateMap passback_templates;
    CampaignManagerCore::TokenToParamMap token_to_parameters;
    AdServer::Commons::IPCrypter_var ip_crypter;
    Generics::SignedUuidGenerator rid_signer(options.rid_private_key.c_str());
    CreativeInstantiator creative_instantiator(
      make_creative_instantiator_config(template_path),
      creative_instantiate,
      passback_templates,
      token_to_parameters,
      ip_crypter,
      rid_signer,
      logger,
      country_whitelist);

    Fixture fixture = make_fixture(template_path);
    std::atomic<unsigned long> checksum{0};

    const CpuTimes start_cpu = current_cpu_times();

    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    const unsigned long per_thread = options.count / options.threads;
    const unsigned long remainder = options.count % options.threads;

    for(unsigned long thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      const unsigned long items =
        per_thread + (thread_index < remainder ? 1 : 0);

      workers.emplace_back(
        [
          &creative_instantiator,
          &fixture,
          items,
          &checksum
        ]()
        {
          for(unsigned long i = 0; i < items; ++i)
          {
            instantiate_once(creative_instantiator, fixture, checksum);
          }
        });
    }

    for(auto& worker : workers)
    {
      worker.join();
    }

    const CpuTimes finish_cpu = current_cpu_times();
    const double user_cpu = finish_cpu.user - start_cpu.user;
    const double sys_cpu = finish_cpu.sys - start_cpu.sys;

    std::cout
      << "completed: " << options.count
      << ", threads: " << options.threads
      << ", checksum: " << checksum.load()
      << ", cpu_time: " << format_float(user_cpu + sys_cpu) << "s"
      << ", user_cpu_time: " << format_float(user_cpu) << "s"
      << ", sys_cpu_time: " << format_float(sys_cpu) << "s"
      << std::endl;

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }

  return 1;
}

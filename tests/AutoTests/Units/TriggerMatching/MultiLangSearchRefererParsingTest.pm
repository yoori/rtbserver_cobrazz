
package MultiLangSearchRefererParsingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use encoding 'utf8';

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account =>
                        { name => 1,
                          role_id => DB::Defaults::instance()->advertiser_role });

  $ns->output("Tags/Default", DB::Defaults::instance()->tag, "tid");


  $ns->output("Channels", 3, "List of Channels");

  # 'scarlett johansson' on korean
  my $korean =
    qq[\xec\x8a\xa4\xec\xb9\xbc\xeb\xa0\x9b\x20] .
    qq[\xec\x9a\x94\xed\x95\x9c\xec\x8a\xa8\x20\xed\x99\x94\xeb\xb3\xb4];
  Encode::_utf8_on($korean);
  my $ch1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $acc,
    keyword_list => $korean,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S") ]));
  $ns->output("Channel/01", $ch1->search_key(), $korean);

  # 'scarlett johansson' on english
  my $english = qq[scarlett johansson];
  Encode::_utf8_on($english);
  my $ch2 = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    account_id => $acc,
    keyword_list => $english,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S") ]));
  $ns->output("Channel/02", $ch2->search_key(), $english);

  # 'johansson scarlett' on russian
  my $russian =
    qq[\xd0\xb9\xd0\xbe\xd1\x85\xd0\xb0\xd0\xbd\xd1\x81\xd1\x81\xd0\xbe\xd0\xbd\x20] .
    qq[\xd1\x81\xd0\xba\xd0\xb0\xd1\x80\xd0\xbb\xd0\xb5\xd1\x82\xd1\x82];
  Encode::_utf8_on($russian);
  my $ch3 = $ns->create(DB::BehavioralChannel->blank(
    name => 3,
    account_id => $acc,
    keyword_list => $russian,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S") ]));
  $ns->output("Channel/03", $ch3->search_key(), $russian);

  $ns->output("ChannelsEnd", 3, "List of Channels");

  # ADSC-4447 custom SearchEngine
  my $search_engine_1 = $ns->create(SearchEngine =>
                                    { name => "mysearch",
                                      host => "mysearch.com",
                                      regexp => 'search.*[?&]phrase=([^&]+).*'});

  my $search_engine_2 = $ns->create(SearchEngine =>
                                    { name => "webpoisk",
                                      host => "webpoisk.ru",
                                      regexp => 'find.*[?&]query=([^&]+).*',
                                      encoding => 'koi8-r'});

  my $search_engine_3 = $ns->create(SearchEngine =>
                                    { name => "koreasearch",
                                      host => "koreasearch.co.kr",
                                      regexp => 'cgi-bin/results.*[?&]keywords=([^&]+).*',
                                      encoding => 'euc-kr'});

  my $search_engine_4 = $ns->create(SearchEngine =>
                                    { name => "brasilseacrh",
                                      host => "brasilseacrh.br",
                                      regexp => 'pergunta.*[?&]p=([^&]+).*',
                                      encoding => 'iso-8859-1'});

  $ns->output("Search Engines", 3, "List of search engines");
  # Cases for ADSC-4447
  $ns->output($search_engine_1->{host}, "http://www.mysearch.com/search?param1=value1&phrase=%%QUERY%%&param2=value2", "utf-8");
  $ns->output($search_engine_2->{host}, "http://www.webpoisk.ru/find?query=%%QUERY%%&param1=value1&param2=value2", "koi8-r");
  $ns->output($search_engine_3->{host}, "http://www.koreasearch.co.kr/cgi-bin/results.cgi?param1=value1&param2=value2&keywords=", "euc-kr");
  $ns->output($search_engine_4->{host}, "http://www.brasilseacrh.br/pergunta.php?param1=value1&param2=value2&p=", "iso-8859-1");
  # Cases for ADSC-4447 end.
  $ns->output("www.google.com", "http://www.google.com/search?hl=ru&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA+%D0%B2+Google&lr=&aq=f&q=", "utf-8");
  $ns->output("www.altavista.com",
"http://us.yhs4.search.yahoo.com/yhs/search?fr=altavista&itag=ody&q=%%QUERY%%&kgs=1&kls=0", "utf-8");
  $ns->output("www.overture.com", "http://search.yahoo.com/search?fr=&p=", "utf-8");
  $ns->output("www.ebay.com", "http://search.ebay.com/search/search.dll?from=R40&_trksid=m37&category0=&satitle=", "utf-8");
  $ns->output("www.rambler.ru", "http://www.rambler.ru/srch?set=www&btnG=%CD%E0%E9%F2%E8%21&words=", "utf-8");
  $ns->output("www.yandex.ru", "http://www.yandex.ru/yandsearch?text=", "utf-8");
  $ns->output("www.aport.ru", "http://www.aport.ru/search/?q=", "utf-8");
  $ns->output("www.lycos.com", "http://search.lycos.com/web/?q=", "utf-8");
  $ns->output("alltheweb.com", "http://search.yahoo.com/search;_ylt=A0oG7p8yAbNMwq4Alayl87UF;_ylc=X1MDMjE0MjQ3ODk0OARfcgMyBGZyA3NmcARuX2dwcwMxMARvcmlnaW4Dc3ljBHF1ZXJ5A3h4eARzYW8DMQ--?p=%%QUERY%%&fr=sfp&fr2=&iscqry=", "utf-8");
  $ns->output("www.dmoz.org", "http://search.dmoz.org/cgi-bin/search?search=", "utf-8");
  $ns->output("www.hotbot.com", "http://hotbot.com/search/web?q=", "utf-8");
  $ns->output("www.msn.com", "http://search.msn.com/results.aspx?FORM=MSNH&q=", "utf-8");
  $ns->output("www.live.com", "http://search.live.com/results.aspx?go=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&mkt=ru-ru&scope=&FORM=LIVSOP&q=", "utf-8");
  $ns->output("www.searchhippo.com", "http://www.searchhippo.com/q.php?q=", "utf-8");
  $ns->output("www.pricegrabber.com", "http://www.pricegrabber.com/search.php?submit=Find+It%21&form_keyword=", "utf-8");
  $ns->output("www.a9.com", "http://a9.com/search/?search_text=", "utf-8");
  $ns->output("www.kanoodle.com", "http://kanoodle.com/?s=", "utf-8");
  $ns->output("www.aol.com", "http://search.aol.com/aol/search?invocationType=comsearch30&do=Search&q=", "utf-8");
  $ns->output("www.scrubtheweb.com", "http://www.scrubtheweb.com/cgi-bin/search.cgi?x=0&y=0&q=", "utf-8");
  $ns->output("www.excite.com", "http://msxml.excite.com/search/web?q=", "utf-8");
  $ns->output("www.naver.com", "http://search.naver.com/search.naver?where=nexearch&x=0&y=0&frm=t1&sm=top_hty&query=", "euc-kr");
  $ns->output("www.empas.com", "http://search.nate.com/search/all.html?s=&x=0&y=0&q=", "euc-kr");
  $ns->output("kr.yahoo.com", "http://kr.search.yahoo.com/search?fr=kr-front_sb&p=", "euc-kr");
  $ns->output("www.google.co.kr", "http://www.google.co.kr/search?hl=ru&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA+%D0%B2+Google&lr=&aq=f&q=", "utf-8");
  $ns->output("www.daumn.net", "http://search.daum.net/search?w=tot&t__nil_searchbox=suggest&sug=&q=", "utf-8");
  $ns->output("www.lycos.co.kr", "http://search.lycos.kr/web/?q=%%QUERY%%&keyvol=0097208e7e799affd9de", "euc-kr");
  $ns->output("kr.msn.com", "http://livesearch.msn.co.kr/results.aspx?FORM=MSNH&x=0&y=0&AspxAutoDetectCookieSupport=1&q=", "utf-8");
  $ns->output("www.korea.com", "http://search.korea.com/search.codi?chk_xkeys=btn&start=1&where=0&userid=&query=%%QUERY%%&module=contents", "euc-kr");
  $ns->output("kr.altavista.com", "http://kr.altavista.com/web/results?itag=ody&kgs=0&kls=0&q=", "utf-8");
  $ns->output("www.mannam.com", "http://www.mannam.com/cgi-bin/search.cgi?query=", "euc-kr");
  $ns->output("www.chol.com", "http://search.chol.com/cgi-bin/search.cgi?h=chol&q=", "euc-kr");
  $ns->output("www.metacrawler.com", "http://www.metacrawler.com/search/web?fcoid=417&fcop=topnav&fpid=27&q=", "utf-8");
  $ns->output("www.dogpile.com", "http://www.dogpile.com/info.dogpl.t1.a/search/web?fcoid=417&fcop=topnav&fpid=27&q=", "utf-8");
  # ADSC-5034
  $ns->output("www.baidu.com", "http://www.baidu.com/s?wd=", "gb2312");
  $ns->output("www.google.com.hk", "http://www.google.com.hk/search?hl=zh-TW&source=hp&biw=1280&bih=866&q=%%QUERY%%&aq=f&aqi=g9g-s1&aql=&gs_sm=e&gs_upl=4401l5279l0l5l5l0l0l0l0l237l784l2.1.2", "utf-8");
  $ns->output("sg.yahoo.com", "http://sg.search.yahoo.com/search;_ylt=Aps3oE9PtFukVnPXYiUfyAmCG7J_;_ylc=X1MDMjE0MjM3ODg4MgRfcgMyBGZyA3lmcC10LTcxMgRuX2dwcwMwBG9yaWdpbgNzZy55YWhvby5jb20EcXVlcnkDc3BvcnQEc2FvAzA-?vc=&p=%%QUERY%%&toggle=1&cop=mss&ei=UTF-8&fr=yfp-t-712", "utf-8");
  $ns->output("asia.ezilon.com", "http://find.ezilon.com/search.php?q=%%QUERY%%&v=asia", "utf-8");
  $ns->output("www.bing.com/?cc=sg", "http://www.bing.com/search?q=%%QUERY%%&go=&form=QBLH&filt=rf", "utf-8");
  $ns->output("www.google.com.sg", "http://www.google.com.sg/search?sclient=psy-ab&hl=ru&site=&source=hp&q=%%QUERY%%&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&gbv=1&ei=5rvMTo-fL7D74QT27aUq", "utf-8");
  $ns->output("ro.yahoo.com", "http://ro.search.yahoo.com/search?p=%%QUERY%%&toggle=1&cop=mss&ei=UTF-8&fr=yfp-t-724", "utf-8");
  $ns->output("www.google.ro", "http://www.google.ro/search?sclient=psy-ab&hl=ru&site=&source=hp&q=%%QUERY%%&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&gbv=1&ei=773MTqrCN6b-4QTG8cwy", "utf-8");
  $ns->output("www.google.com.au", "http://www.google.com.au/search?sclient=psy-ab&hl=ru&site=&source=hp&q=%%QUERY%%&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&gbv=1&ei=WMHMTrycOqSk4gTN_810", "utf-8");
  $ns->output("www.bing.com/?cc=au", "http://www.bing.com/search?q=%%QUERY%%&go=&form=QBLH&filt=rf&qs=n&sk=", "utf-8");
  $ns->output("au.yahoo.com", "http://au.search.yahoo.com/search?p=%%QUERY%%&rd=r2&fr=yfp-t-501&ei=UTF-8", "utf-8");
  $ns->output("www.sensis.com.au", "http://www.sensis.com.au/search.do;jsessionid=4D349B826E20E948B44F6346802FCF8F.worker2-1?partnerId=&find=%%QUERY%%&profile=au_web_only", "utf-8");
  $ns->output("www.zoo.com", "http://www.zoo.com/search?q=%%QUERY%%&target=au", "utf-8");
  $ns->output("www.google.co.nz", "http://www.google.co.nz/search?sclient=psy-ab&hl=ru&site=&source=hp&q=%%QUERY%%&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&gbv=1&ei=FsLMTt6dGMb04QSx5OVk", "utf-8");
  $ns->output("nz.bing.com", "http://nz.bing.com/search?q=%%QUERY%%&go=&form=QBLH&filt=rf&qs=AS&sk=&pq=new+zealand+poli&sp=1&sc=8-16", "utf-8");
  $ns->output("nz.yahoo.com", "http://nz.search.yahoo.com/search?p=%%QUERY%%&rd=r2&fr=yfp-t-501&ei=UTF-8", "utf-8");
  $ns->output("www.google.com.tr", "http://www.google.com.tr/search?sclient=psy-ab&hl=ru&site=&source=hp&q=%%QUERY%%&btnG=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA&gbv=1&ei=lMPMTsb1A6rc4QTYo9SkAw", "utf-8");
  $ns->output("Search EnginesEnd", 3, "List of search engines");
}

1;

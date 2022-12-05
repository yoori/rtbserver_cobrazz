
package MultiLangChannelMatching;

use strict;
use warnings;
use encoding 'utf8';
use DB::Defaults;
use DB::Util;

sub create_page_channel
{
  my ($self, $ns, $index, $account, $keyword) = @_;
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'KWD' . $index,
    account_id => $account,
    keyword_list => $keyword,
    url_kwd_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "R",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60) ]));
  $ns->output('PAGECHANNEL' . $index, $channel->page_key());
  $ns->output('SEARCHCHANNEL' . $index, $channel->search_key());
  $ns->output('URLKWDCHANNEL' . $index, $channel->url_kwd_key());
}

sub create_url_channel
{
  my ($self, $ns, $index, $account, $referer) = @_;
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'URL' . $index,
    account_id => $account,
    url_list => $referer,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 )]));
  $ns->output('URLCHANNEL' . $index, $channel->url_key());
}

sub init
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my @keywords = (
    # 1
    # korean 'rabbit'
    qq[\xed\x86\xa0\xeb\x81\xbc],
    # 2
    # korean 'my dog'
    qq[\xeb\x82\xb4\x20\xea\xb0\x9c],
    # 3
    # Keywords for CDML2 Test Plan Environment. Test 1.9.
    qq[Q\xc4\x90\xce\x98\xd1\xbe\xd4\x86\xd4\xb9\xe1\x82\xab\xe1\xba\x84\xe1\xbd\xae\xe2\x84\xb2\xe2\x93\x81\xe2\xb1\xb5\xef\xbc\xba],
    # 4
    qq[\xc3\x9f\xe1\xba\x97\xd6\x87\xce\x90\xc5\x89\xe1\xba\x9e\xe1\xbd\x96\xef\xac\x96],
    # 5
    qq[sssss\xca\xbcn],
    # 6
    qq[\xd5\xb4\xd5\xadfffi],
    # 7
    qq[\xe1\xba\x9esss\xca\xbcn],
    # 8
    qq[\xd5\xb4\xd5\xad\xef\xac\x80\xef\xac\x81],
    # 9
    qq[ss\xc3\x9fs\xc5\x89],
    # 10
    qq[\xef\xac\x97f\xef\xac\x83],
    # 11
    qq[\xe1\xbe\x8c\xe1\xbf\xbc],
    # 12
    qq[I\xc4\xb0],

    # 13
    qq[\xe2\xb1\xa8\xe2\xb1\xaa\xe2\xb1\xac\xe2\xb1\xb1\xe2\xb1\xb3],
    # 14
    qq[\xe3\x8d\xbf\xe3\x8d\xb1],
    # 15
    qq[\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5\xc3\xa7],
    # 16
    qq[aaccee~kktt\x29-\x27\x60\x21\x40\x23\x24\x25\x5e\x26\x28\x29\x21\xe2\x84\x96\x3b\x25\x3a\x3c\x3e.bbddee],
    # 17
    qq[\xc4\xb1s\xc4\xb1k],
    # 18
    qq[\xc3\x83\xc3\x87\xc3\x86\xc3\x98\xc3\x9e],
    # 19
    qq[\xc5\x9e\xc3\x87\xc4\x9e-45aa\x20I\xc5\x9f\xc4\xb1k101a\x20\xc5\x9f1e~kdkdaa],
    # 20
    qq[\x22kdkdaa~45aa\x20\xc5\x9f1e-\xc5\x9e\xc3\x87\xc4\x9e\x20I\xc5\x9f\xc4\xb1k101a\x22],
    # 21
    qq[\xef\xbd\x81\xef\xbd\x81\xef\xbd\x82\xef\xbd\x82\xef\xbc\x90\xef\xbc\x90\xef\xbc\x91] . 
    qq[\xef\xbc\x91\xef\xbc\xb8\xef\xbc\xb9\xef\xbc\xba],
    # 22
    qq[\xe3\x85\xa4\xe3\x84\xb1\xe3\x85\x8e],
    # 23
    qq[\xe3\x83\xb2\xe3\x82\xa1\xe3\x83\x83],
    # 24
    qq[\xef\xbd\xa5\xef\xbd\xa6\xef\xbd\xa7\xef\xbd\xaf],
    # 25
    qq[\xef\xbe\xa0\xef\xbe\xa1\xef\xbe\xbe\xef\xbe\xbf],
    # 26
    qq[\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\x97\xe3\x83\xb3\xe3\x82\xa2\xe3\x82\xaf\xe3\x82\xbb\xe3\x82\xb9],
    # 27
    qq[\xec\xa0\x9c\xec\xa3\xbc\xed\x98\xb8\xed\x85\x94\xeb\x93\xb1\xea\xb8\x89],
    # 28
    qq[\xe5\x9f\x8e\xe5\xb8\x82\xe4\xba\xba\xe7\x9a\x84] .
    qq[\xe5\xbf\x83\xe7\x88\xb1\xe5\xae\xa0\xe7\x89\xa9],
    # 29
    qq[\xec\x98\xa4\xed\x9c\x98\x20\xe5\x9f\x8e\xe5\xb8\x82\xe4\xba\xba\xe7\x9a\x84\xe5\xbf\x83\xe7\x88\xb1\xe5\xae\xa0\xe7\x89\xa9\x20\xec\x95\xbc\xea\xb1\xb0],
    # 30
    qq[\xe5\x8f\xaf\xe4\xbb\xa5\xe4\xb8\xba\xe6\x95\x88\xe5\x8a\x9b]
);

  # URL for CDML2 Test Plan Environment. Test 1.10.
  my @urls = (
    qq[Sch\xc3\xb6ne-Stra\xc3\x9fe.de]);


  my @kwd_triggers = (
    # 1
    $keywords[0],
    # 2
    $keywords[1],
    # 3
    $keywords[2], 
    # 4
    $keywords[3], 
    # 5
    $keywords[4] . "\n" .  $keywords[5],
    # 6
    $keywords[6] . "\n" .  $keywords[7],
    # 7
    $keywords[8] . "\n" .  $keywords[9],
    # 8
    $keywords[10],
    # 9
    $keywords[11],
    # 10
    $keywords[12],
    # 11
    $keywords[13],
    # 12
    $keywords[14],
    # 13
    $keywords[15],
    # 14
    $keywords[16],
    # 15
    $keywords[17],
    # 16
    $keywords[18] . "\n" .  $keywords[19],
    # 17
    $keywords[20] . "\n" .  $keywords[21]  . "\n" .  $keywords[22],
    # 18
    $keywords[23] . "\n" .  $keywords[24],
    # 19
    $keywords[25] . "\n" .  $keywords[26] . "\n" .
    "[" . $keywords[27] . "]"  . "\n" .
    "\"" . $keywords[29] . "\""
);


  # Keywords
  my $idx = 0;
  foreach my $keyword (@keywords) {
    $ns->output('KEYWORD' . ++$idx, $keyword);
  }
  $ns->output('KEYWORD2_1', "\"" . $keywords[1] . "\"");
  $ns->output('FT3', $keywords[2]);
  $ns->output(
    'KEYWORD4_1', 
    qq[ss\xe1\xba\x97\xd5\xa5\xd6\x82\xce\x90\xca\xbcnss\xe1\xbd\x96\xd5\xbe\xd5\xb6]);
  $ns->output('KEYWORD5_1', qq[\xef\xac\x97f\xef\xac\x80i]);
  $ns->output('KEYWORD5_2', qq[\xc3\x9fs\xe1\xba\x9e\xc5\x89]);
  $ns->output('KEYWORD6_1', qq[sssss\xca\xbcn]);
  $ns->output('KEYWORD6_2', qq[\xd5\xb4\xd5\xadfffi]);
  $ns->output('KEYWORD11_1', qq[\xe1\xbc\x84\xcf\x89]);
  $ns->output('KEYWORD11_2', qq[\xe1\xbe\x8c\xe1\xbf\xbc]);
  $ns->output('KEYWORD14_1', 
    qq[\xe6\xa0\xaa\xe5\xbc\x8f\xe4\xbc\x9a\xe7\xa4\xbehpa]);
  $ns->output('KEYWORD15_1', 
    qq[aaaaaac\x20isik\x20ac\xc3\xa6\xc3\xb8\xc3\xbe]);
  $ns->output('KEYWORD15_2', 
    qq[\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5] . 
    qq[\xc3\xa7\x20\xc4\xb1s\xc4\xb1k\x20\xc3\x83\xc3\x87] . 
    qq[\xc3\x86\xc3\x98\xc3\x9e]);
  $ns->output('KEYWORD16_1',
              'aaccee kktt no bbddee');
  $ns->output('FT16', 
    qq[aaccee~kktt\x29\x5e\xe2\x84\x96-\x28bbddee]);
  $ns->output('KEYWORD17_1',
    'kdkdaa 45aa isik101a s1e scg');
  $ns->output('KEYWORD17_2', 
    qq[kdkdaa~45aa\x20\xc5\x9f1e-\xc5\x9e\xc3\x87\xc4\x9e\x20i\xc5\x9f\xc4\xb1k101a]);
  $ns->output('KEYWORD18_1',
   'isik101a scg 45aa kdkdaa s1e');
  $ns->output('KEYWORD18_2', 
    qq[\xc5\x9e\xc3\x87\xc4\x9e-45aa\x20\xc5\x9f1e~kdkdaa\x20I\xc5\x9f\xc4\xb1k101a]);
  $ns->output('KEYWORD19_1', 
    'isik101a scg 45aa s1e kdkdaa');
  $ns->output('KEYWORD19_2', 
    qq[45aa-\xc5\x9e\xc3\x87\xc4\x9e\x20I\xc5\x9f\xc4\xb1k101a\x20\xc5\x9f1e~kdkdaa]);
  $ns->output('KEYWORD21_1', 'aabb0011xyz');
  $ns->output('FT26', $keywords[25]);
  $ns->output('KEYWORD26_1', 
    qq[\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\x97\xe3\x83\xb3\x20] . 
    qq[\xe3\x82\xa2\xe3\x82\xaf\xe3\x82\xbb\xe3\x82\xb9]);
  $ns->output('KEYWORD30_1', 
    qq[\xE5\x8F\xAF\xE4\xBB\xA5\x20\xE4\xB8\xBA\x20] . 
    qq[\xE6\x95\x88\xE5\x8A\x9B]);

  # Keyword channels      
  $idx = 0;
  foreach my $trigger (@kwd_triggers) {
    $self->create_page_channel($ns, ++$idx, $account, $trigger);
  }

  # URL channels      
  $idx = 0;
  foreach my $url (@urls) {
    $self->create_url_channel($ns, ++$idx, $account, $url);
  }
  # REFERERS channels      
  $ns->output('URL1', 'xn--schne-strasse-kmb.de');
  $ns->output('URL2', 'xn--Schne-Strae-46a50a.de');

  # Lingjoin

  my $l_channel_1 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Lingjoin1',
    account_id => $account,
    keyword_list =>
     qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1] . "\n" .
     qq["\xe6\x96\xb9\xe9\x9d\xa2\x20\xe5\xb7\xb2\x20\xe7\xbb\x8f\x20\xe7\xa1\xae\xe8\xae\xa4"],
    search_list =>
      qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1] . "\n" .
      qq["\xe6\x96\xb9\xe9\x9d\xa2\x20\xe5\xb7\xb2\x20\xe7\xbb\x8f\x20\xe7\xa1\xae\xe8\xae\xa4"] . "\n" .
      "[" . qq[\xe4\xba\x86\xe6\x98\x8e\xe6\x98\xbe\xe5\x8f\x98\xe5\x8c\x96] . "]",
    url_kwd_list =>
      qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1] . "\n" .
      qq["\xe6\x96\xb9\xe9\x9d\xa2\x20\xe5\xb7\xb2\x20\xe7\xbb\x8f\x20\xe7\xa1\xae\xe8\xae\xa4"],
    country_code => 'CN',
    language => 'en',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "R",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ) ]));

  $ns->output('CHANNELLJ1' , $l_channel_1);
  $ns->output('PAGELJ1' , $l_channel_1->page_key());
  $ns->output('SEARCHLJ1', $l_channel_1->search_key());
  $ns->output('URLKWDLJ1', $l_channel_1->url_kwd_key());

  my $l_channel_2 = $ns->create(DB::BehavioralChannel->blank(
    name => 'Lingjoin2',
    account_id => $account,
    keyword_list =>
     qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1],
    search_list =>
     qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1],
    url_kwd_list =>
     qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1],
    country_code => 'GN',
    language => 'zh',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "R",
        minimum_visits => 1,
        time_from => 0,
        time_to => 60 ) ]));

  $ns->output('CHANNELLJ2' , $l_channel_2);
  $ns->output('PAGELJ2' , $l_channel_2->page_key());
  $ns->output('SEARCHLJ2', $l_channel_2->search_key());
  $ns->output('URLKWDLJ2', $l_channel_2->url_kwd_key());

  $ns->output('KEYWORDLJ1_1', qq[\xe8\x9e\x8d\xe8\xb5\x84 \xe4\xb8\x9a\xe5\x8a\xa1]);
  $ns->output('KEYWORDLJ1_2', qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1]);

  $ns->output('KEYWORDLJ2', qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a \xe5\x8a\xa1]);

  $ns->output('KEYWORDLJ3', qq[\xe6\x96\xb9\xe9\x9d\xa2\xe5\xb7\xb2\xe7\xbb\x8f\xe7\xa1\xae\xe8\xae\xa4]);
  $ns->output('FTLJ3', qq[\xe6\x96\xb9\xe9\x9d\xa2\xe5\xb7\xb2\xe7\xbb\x8f\xe7\xa1\xae\xe8\xae\xa4]);

  $ns->output('KEYWORDLJ4', qq[\xe4\xba\x86\x20\xe6\x98\x8e\xe6\x98\xbe\x20\xe5\x8f\x98\xe5\x8c\x96]);

  $ns->output('KEYWORDLJ5', qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1]);
  $ns->output('FTLJ5', qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1]);

  $ns->output('PHRASELG1_1', qq[\xe8\x9e\x8d\xe8\xb5\x84\x20\xe4\xb8\x9a\xe5\x8a\xa1]);
  $ns->output('PHRASELG1_3', qq[\xe6\x96\xb9\xe9\x9d\xa2 \xe5\xb7\xb2 \xe7\xbb\x8f \xe7\xa1\xae\xe8\xae\xa4]);
  $ns->output('PHRASELG1_4', qq[\xe4\xba\x86 \xe6\x98\x8e\xe6\x98\xbe \xe5\x8f\x98\xe5\x8c\x96]);
  $ns->output('PHRASELG1_5', qq[\xe8\x9e\x8d\xe8\xb5\x84 \xe4\xb8\x9a\xe5\x8a\xa1]);

  $ns->output('PHRASELG2_1', qq[\xE8\x9E\x8D\xE8\xB5\x84\xE4\xB8\x9A \xE5\x8A\xA1]);
  $ns->output('PHRASELG2_3', qq[\xe6\x96\xb9 \xe9\x9d\xa2\xe5\xb7\xb2\xe7\xbb\x8f \xe7\xa1\xae \xe8\xae\xa4]);

  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});

}

1;

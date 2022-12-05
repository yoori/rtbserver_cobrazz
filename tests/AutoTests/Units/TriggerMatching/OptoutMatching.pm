
package OptoutMatching;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "KWD");
  # To use search request for URL matching
  my $search_engine = make_autotest_name($ns, "URL");
  my $url = "$search_engine.ru";

  $ns->create(SearchEngine =>
    { name => $search_engine,
      host => lc($url),
      regexp => 'search.*[?&]phrase=([^&]+).*'});

  # Advertisier
  my $advertiser = $ns->create(Account => {
    name => "Advertiser",
    role_id => DB::Defaults::instance()->advertiser_role });

  # Channels
  my $channelPage = $ns->create(DB::BehavioralChannel->blank(
    name => "Page",
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 86400, trigger_type => 'P')]));

  my $channelSearch = $ns->create(DB::BehavioralChannel->blank(
    name => "Search",
    account_id => $advertiser,
    search_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 86400, trigger_type => 'S')]));

  my $channelURL = $ns->create(DB::BehavioralChannel->blank(
    name => "URL",
    account_id => $advertiser,
    url_list => $url,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 86400, trigger_type => 'U')]));


  $ns->output("KEYWORD", $keyword);
  $ns->output("SEARCH", "http://www.$url/search?phrase=$keyword");

  $ns->output("PAGECHANNEL", $channelPage);
  $ns->output("PAGEBP", $channelPage->page_key());
  $ns->output("PAGETRIGGER", $channelPage->keyword_channel_triggers_->[0]->channel_trigger_id());
  $ns->output("SEARCHCHANNEL", $channelSearch);
  $ns->output("SEARCHBP", $channelSearch->search_key());
  $ns->output("SEARCHTRIGGER", $channelSearch->search_channel_triggers_->[0]->channel_trigger_id());
  $ns->output("URLCHANNEL", $channelURL);
  $ns->output("URLBP", $channelURL->url_key());
  $ns->output("URLTRIGGER", $channelURL->url_channel_triggers_->[0]->channel_trigger_id());

  $ns->output("OPTINCOLO", 
    DB::Defaults::instance()->optin_only_isp->{colo_id});
  $ns->output("ALLCOLO", 
    DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("NONOPTOUTCOLO", 
    DB::Defaults::instance()->non_optout_isp->{colo_id});
  $ns->output("NOADSCOLO", 
    DB::Defaults::instance()->no_ads_isp->{colo_id});
  $ns->output("DELETEDCOLO", 
    DB::Defaults::instance()->deleted_colo_isp->{colo_id}); 
}

1;

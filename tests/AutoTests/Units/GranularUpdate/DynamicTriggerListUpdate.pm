package DynamicTriggerListUpdate;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub normalize_url 
{
  my ($url) = @_;
  my $normalized_url = DB::BehavioralChannelBase::normalize_url_($url);
  $normalized_url =  
    index($url, 'http://www.') == 0 && index($normalized_url, 'www.') == -1?
    'www.' . $normalized_url: $normalized_url;
  return $normalized_url;
}

sub prepare_triggers
{
  my ($trigger, $normalize) = @_;
  return $normalize->($trigger);
}

sub create_trigger
{
  my ($self, $ns, $channel, $trigger, $type, $status) = @_;

  my $group = undef;

  if($type eq 'U')
  {
    if($trigger !~ m|^(?:http://)?([^/?]*).*$|)
    {
      die "Incorrect trigger word '$trigger'";  
    }
    $group = $1;
  }

  my $db_trigger =
    $ns->create(DB::BehavioralChannel::Trigger->blank(
      trigger_type => $type eq 'U'? $type: 'K',
      normalized_trigger => lc $trigger,
      channel_type =>  
        ($channel->channel_type() eq 'S'? 'S': 'A'),
      qa_status => 'A',
      country_code => defined $channel->{country_code}? $channel->{country_code}: ''));

  my $channel_trigger = $ns->create(
    DB::BehavioralChannel::ChannelTrigger->blank(
      channel_id => $channel->channel_id(),
      trigger_id =>  $db_trigger->trigger_id(),
      trigger_type => $type,
      qa_status => $status,
      trigger_group => $group,
      channel_type =>
        ($channel->channel_type() eq 'S'? 'S': 'A'),
      masked => ($type eq 'U' ? 'N': undef),
      original_trigger => $trigger));

  return ($db_trigger, $channel_trigger);
}

sub change_trigger
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("CHANGETRIGGER");

  my $keyword_orig = make_autotest_name($ns, "original");
  my $keyword_changed = make_autotest_name($ns, "changed");

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel",
    account_id => $advertiser,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P')]));

  my $i = 0;
  foreach my $kwd ($keyword_orig, $keyword_changed)
  {
    my ($trigger, $channel_trigger) =
      $self->create_trigger(
        $ns, $channel, $kwd, 'P', 
        $kwd eq $keyword_orig? 'A': 'D');

    $ns->output("ChannelTrigger/" . ++$i, $channel_trigger);
    $ns->output("Trigger/" . $i, $trigger);
  }

  $ns->output("KeywordOrig", $keyword_orig);
  $ns->output("KeywordChanged", $keyword_changed);
  $ns->output("Channel", $channel);
  $ns->output("BP", $channel->page_key());
}

sub no_adv
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("NOADV");

  my $new_no_adv_words = "robbery";
  my $new_no_adv_url_words = "bannedAdvURLKeywordNew";
  my $new_no_adv_urls = "http://alcoholism.about.com";

  $ns->output("ChannelTriggerKwd", 
     DB::Defaults::instance()->no_adv_channel->keyword_channel_triggers()->[0]);
  $ns->output("ChannelTriggerUrlKwd", 
     DB::Defaults::instance()->no_adv_channel->url_kwd_channel_triggers()->[0]);
  $ns->output("ChannelTriggerUrl", 
     DB::Defaults::instance()->no_adv_channel->url_channel_triggers()->[0]);
  $ns->output("Channel", 
     DB::Defaults::instance()->no_adv_channel->channel_id());

  my ($trigger, $channel_trigger) =
    $self->create_trigger(
      $ns, DB::Defaults::instance()->no_adv_channel, 
      $new_no_adv_urls, 'U', 'D');

  $ns->output("ChannelTriggerUrlNew", $channel_trigger);

  $ns->output("NewUrls", $new_no_adv_urls);
  $ns->output("NewWords", $new_no_adv_words);
  $ns->output("NewUrlWords", $new_no_adv_url_words);

  $ns->output("ExpectedUrls", "alcoholism.about.com/");
  $ns->output("ExpectedWords", $new_no_adv_words);
  $ns->output("ExpectedUrlWords", lc($new_no_adv_url_words));

  $ns->output("Urls", 
     prepare_triggers(
       DB::Defaults::instance()->no_adv_channel->
         url_channel_triggers()->[0]->{original_trigger},
       \&DynamicTriggerListUpdate::normalize_url));

  $ns->output("Words", 
     prepare_triggers(
       DB::Defaults::instance()->no_adv_channel->
         keyword_channel_triggers()->[0]->{original_trigger},
       \&DB::BehavioralChannelBase::normalize_keyword_));

  $ns->output("UrlWords", 
     prepare_triggers(
       DB::Defaults::instance()->no_adv_channel->
         url_kwd_channel_triggers()->[0]->{original_trigger},
       \&DB::BehavioralChannelBase::normalize_keyword_));

  $ns->output("KWD", $new_no_adv_words);
  $ns->output("URLKWD", $new_no_adv_url_words);
  $ns->output("REF", "http://alcoholnews.org");
}

sub no_track
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("NOTRACK");

  my $new_no_track_words = "crime";
  my $new_no_track_url_words = "bannedTrackURLKeywordNew";

  $ns->output("ChannelTriggerKwd", 
     DB::Defaults::instance()->no_track_channel->keyword_channel_triggers()->[0]);
  $ns->output("ChannelTriggerUrlKwd", 
     DB::Defaults::instance()->no_track_channel->url_kwd_channel_triggers()->[0]);
  $ns->output("ChannelTriggerUrl", 
     DB::Defaults::instance()->no_track_channel->url_channel_triggers()->[0]);
  $ns->output("Channel", 
    DB::Defaults::instance()->no_track_channel->channel_id());

  my ($trigger, $channel_trigger) =
    $self->create_trigger(
      $ns, DB::Defaults::instance()->no_track_channel, 
      $new_no_track_words, 'P', 'D');
  $ns->output("ChannelTriggerKwdNew", $channel_trigger);

  $ns->output("NewWords", $new_no_track_words);
  $ns->output("NewUrlWords", $new_no_track_url_words);

  $ns->output("ExpectedWords", $new_no_track_words);
  $ns->output("ExpectedUrlWords", lc($new_no_track_url_words));
  $ns->output("ExpectedUrls", "");

  $ns->output("Urls", 
    prepare_triggers(
      DB::Defaults::instance()->no_track_channel->
        url_channel_triggers()->[0]->{original_trigger},
      \&DynamicTriggerListUpdate::normalize_url));

  $ns->output("Words", 
    prepare_triggers(
      DB::Defaults::instance()->no_track_channel->
        keyword_channel_triggers()->[0]->{original_trigger},
     \&DB::BehavioralChannelBase::normalize_keyword_));

  $ns->output("UrlWords", 
    prepare_triggers(
      DB::Defaults::instance()->no_track_channel->
        url_kwd_channel_triggers()->[0]->{original_trigger},
     \&DB::BehavioralChannelBase::normalize_keyword_));

  $ns->output("KWD1", "murder");
  $ns->output("KWD2", $new_no_track_words);
  $ns->output("SEARCH", $new_no_track_url_words);
  $ns->output("REF", "google.com/search?hl=en&q=crime&btnG=Search");  
}

sub init
{
  my ($self, $ns) = @_;

  # Common
  my $publisher = $ns->create(
    Publisher => { name => "Publisher" });

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser'});

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $advertiser,
    keyword_list => 
      "robbery\n" . 
      "crime\n" . 
      "murder\n" . 
      "unbanned_2",
    url_list => 
      "alcoholnews.org\n" .
      "alcoholism.about.com\n" .
      "www.tobacco.org/ads\n" .
      "www.smokin4free.com/parliament.html",
    url_kwd_list =>
      "bannedAdvURLKeyword\n" .
      "bannedTrackURLKeyword\n" .
      "bannedAdvURLKeywordNew\n" .
      "bannedTrackURLKeywordNew",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3 * 60 * 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "R",
        time_to => 3 * 60 * 60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U",
        time_to => 3 * 60 * 60) ]));

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    channel_id => $channel->{channel_id},
    campaigncreativegroup_cpm => 1000,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("CC", $campaign->{cc_id});

  # Test cases
  $self->change_trigger($ns);
  $self->no_adv($ns);
  $self->no_track($ns);
}

1;


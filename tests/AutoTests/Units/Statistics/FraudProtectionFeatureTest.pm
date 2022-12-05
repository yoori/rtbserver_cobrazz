
package FraudProtectionFeatureTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_text_campaigns
{
  my ($self, $ns, $name, $args) = @_;

  my $max_creative_size = @{$args->{cpc_bids}};

  my $size = $ns->create(CreativeSize => {
    name => "Size-" .  $name,
    max_text_creatives => $max_creative_size });

  my $publisher = 
   $ns->create(Publisher => {
     name => "Publisher-" . $name,
     size_id => $size });

  my $keyword = make_autotest_name($ns, $name);

  my $account = 
    $ns->create(Account => {
      name => 'Advertiser-' . $name . '-Channel',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $channel = 
      $ns->create(DB::BehavioralChannel->blank(
        name => 'Channel-' . $name,
        account_id => $account,
        keyword_list => $keyword,
        channel_type => 'K',
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P') ]));

  my $idx = 0;
  foreach my $cpc_bid (@{$args->{cpc_bids}}) 
  {
    my $campaign = 
      $ns->create(TextAdvertisingCampaign => { 
        name => 'Campaign-' . $name . "-" . ++$idx,
        size_id => $size,
        template_id =>  DB::Defaults::instance()->text_template,
        original_keyword => $channel->{keyword_list_},
        ccgkeyword_channel_id => $channel->{channel_id},
        ccgkeyword_ctr => 0.01,
        max_cpc_bid => $cpc_bid,
        site_links => [
          {site_id => $publisher->{site_id} }] });

    $ns->output($name . "/CC" . $idx, $campaign->{cc_id});
    $ns->output($name . "/CCG" . $idx, $campaign->{ccg_id});
    $ns->output($name . "/CCCPC"  . $idx, $cpc_bid);
  }

  $ns->output($name . "/KWD", $keyword);
  $ns->output($name . "/CHANNEL", $channel);
  $ns->output($name . "/TID", $publisher->{tag_id});
}

sub create_channel_campaigns
{
  my ($self, $ns, $name, $args) = @_;

  my $max_creative_size = @{$args->{revenues}};

  my $size = $ns->create(CreativeSize => {
    name => "Size-" .  $name,
    max_text_creatives => $max_creative_size });

  my $publisher = 
   $ns->create(Publisher => {
     name => "Publisher-" . $name,
     size_id => $size });

  my $channel = undef;

  if (defined $args->{channel_rate})
  {
    my $keyword = make_autotest_name($ns, $name);

    my $account = 
        $ns->create(Account => {
          name => 'Advertiser-' . $name . '-Channel',
          role_id => DB::Defaults::instance()->cmp_role });

    $channel = 
      $ns->create(DB::BehavioralChannel->blank(
        name => 'Channel-' . $name,
        account_id => $account,
        keyword_list => $keyword,
        cpm => $args->{channel_rate},
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P') ]));

    $ns->output($name . "/KWD", $keyword);
    $ns->output($name . "/CHANNEL", $channel);
    $ns->output($name . "/CHANNELCPM", $args->{channel_rate} / 1000);
  }

  my $idx = 0;
  foreach my $revenues (@{$args->{revenues}}) 
  {
    my $account = 
        $ns->create(Account => {
          name => 'Advertiser-' . $name  . "-" . ++$idx ,
          role_id => DB::Defaults::instance()->advertiser_role });

    my $cmp_channel = $channel;
    if (not defined $channel)
    {

      my $keyword = make_autotest_name($ns, $name . "-" . $idx );
      
      my $cmp = 
         $ns->create(Account => {
            name => 'CMP-' . $name  . "-" . $idx ,
            role_id => DB::Defaults::instance()->cmp_role });

      $cmp_channel = 
        $ns->create(DB::BehavioralChannel->blank(
          name => 'Channel-' . $name . "-" . $idx ,
          account_id => $cmp,
          keyword_list => $keyword,
          cpm => $revenues->{channel_rate},
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P',
              time_to => 6*60*60) ]));

      $ns->output($name . "/KWD" . $idx, $keyword);
      $ns->output($name . "/CHANNEL" . $idx, $cmp_channel);
      $ns->output($name . "/CHANNELCPM"  . $idx,   $revenues->{channel_rate} / 1000);
    }

    my $campaign =  $ns->create(ChannelTargetedTACampaign => {
      name => 'Campaign-' . $name . "-" . $idx ,
      account_id => $account,
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $cmp_channel,
      targeting_channel_id => undef,
      campaigncreativegroup_cpm =>  $revenues->{cpm},
      campaigncreativegroup_cpc =>  $revenues->{cpc},
      campaigncreativegroup_ctr => 0.01,
      site_links => [
        {site_id => $publisher->{site_id} }] });

    $ns->output($name . "/CC" . $idx, $campaign->{cc_id});

    if (defined $revenues->{cpm})
    {
      $ns->output($name . "/CCCPM"  . $idx, $revenues->{cpm} / 1000);
    }
    if (defined $revenues->{cpc})
    {
      $ns->output($name . "/CCCPC"  . $idx, $revenues->{cpc});
    }
  }

  $ns->output($name . "/TID", $publisher->{tag_id});
  
}

sub create_display_campaign
{
  my ($self, $ns, $name, $args) = @_;

  my $keyword = make_autotest_name($ns, $name);

  my $publisher = 
   $ns->create(Publisher => {
     name => "Publisher-" . $name });

  my $account = 
    $ns->create(Account => {
      name => 'Advertiser-' . $name,
      role_id => DB::Defaults::instance()->advertiser_role });

  my $channel = undef;
  if (not (defined $args->{flags} && 
           ($args->{flags} & DB::Campaign::RON)))
  {
    my $account = 
      $ns->create(Account => {
         name => 'CMP-' . $name,
         role_id => DB::Defaults::instance()->cmp_role });

    $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel-' . $name,
      account_id => $account,
      keyword_list => $keyword,
      cpm => $args->{channel_rate},
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));
    $ns->output($name . "/CHANNEL", $channel);
  }

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Display-Campaign-' . $name,
    account_id => $account,
    channel_id => $channel,
    targeting_channel_id => undef,
    campaigncreativegroup_cpm => $args->{cpm},
    campaigncreativegroup_cpc => $args->{cpc},
    campaigncreativegroup_cpa => $args->{cpa},
    campaigncreativegroup_flags => 
      defined $args->{flags}? $args->{flags}:
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{ site_id => $publisher->{site_id} }]});

  $ns->output($name . "/TID", $publisher->{tag_id});
  $ns->output($name . "/KWD", $keyword);
  $ns->output($name . "/CC", $campaign->{cc_id});
  $ns->output($name . "/CCG", $campaign->{ccg_id});
  if (defined  $args->{channel_rate}) 
  {
    $ns->output($name . "/CHANNELCPM", $args->{channel_rate} / 1000);
  }
  if (defined $args->{cpm}) 
  {
    $ns->output($name . "/CCCPM", $args->{cpm} / 1000);
  }
  if (defined $args->{cpc}) 
  {
    $ns->output($name . "/CCCPC", $args->{cpc});
  }
  if (defined $args->{cpa}) 
  {
    $ns->output($name . "/CCCPA", $args->{cpa});
  }
}

sub init
{
  my ($self, $ns) = @_;

  my $inactive_period = 
    get_config($ns->pq_dbh, 'USER_INACTIVITY_TIMEOUT');


  # Impression fraud condition
  $ns->create(FraudCondition => {
    type => "IMP",
    period => 1_000_000,
    limit => 86400 } ); # Need for prevent fraud rollback

  $ns->create(FraudCondition => {
    type => "IMP",
    period => 60,
    limit => 15 } );

  # Click fraud condition
  $ns->create(FraudCondition => {
    type => "CLK",
    period => 1,
    limit =>  10} );

  $ns->create(FraudCondition => {
    type => "CLK",
    period => $inactive_period * 3,
    limit =>  40} );

  # Campaigns
  $self->create_display_campaign(
    $ns, 'CLICKFRAUD', 
    {cpm => 20, channel_rate => 20 });

  $self->create_display_campaign(
    $ns, 'GENUINE', 
    {cpm => 20, channel_rate => 20 });

  $self->create_display_campaign(
    $ns, 'CPA', 
    {cpa => 1, channel_rate => 20 });

  $self->create_display_campaign(
   $ns, 'CPM', 
   {cpm => 2, channel_rate => 20 });

  $self->create_channel_campaigns(
    $ns, 'CHANNELTEXT', 
    {revenues => [ { cpm => 31, channel_rate => 25 }, 
                   { cpm => 29, channel_rate => 30 } ] });

  $self->create_display_campaign(
   $ns, 'UNCONFIRMEDIMPS', 
   {cpm => 2, channel_rate => 20 });

  $self->create_display_campaign(
    $ns, 'NOFRAUD', 
    {cpa => 15, channel_rate => 20 });

  $self->create_channel_campaigns(
    $ns, 'TANOFRAUD', 
    {revenues => [ { cpm => 31, channel_rate => 25 }, 
                   { cpm => 29, channel_rate => 30 } ] });

  $self->create_channel_campaigns(
    $ns, 'TAFRAUD', 
    {revenues => [ {cpc => 4 },
                   {cpc => 3 },
                   {cpc => 2 },
                   {cpc => 1 } ], 
     channel_rate => 15  });

  $self->create_display_campaign(
    $ns, 'IMPFRAUD', 
    {cpm => 20, channel_rate => 20 });

  $self->create_display_campaign(
    $ns, 'IMPWAITFRAUD', 
    {cpm => 20, channel_rate => 20 });

  $self->create_display_campaign(
    $ns, 'FRAUDOVERRIDE', 
    {cpm => 10, channel_rate => 10 });

  $self->create_text_campaigns(
    $ns, 'FRAUDTEXT', 
    { cpc_bids => [40, 30] });

  $self->create_display_campaign(
    $ns, 'REVERSEDORDER',
    {cpm => 10, channel_rate => 10 });

  $self->create_display_campaign(
    $ns, 'DELAYEDCLICKS',
    {cpc => 20, channel_rate => 10 });

  my $client_version = lc(make_autotest_name($ns, 'version'));
  $ns->output("ClientVersion", $client_version);

  $ns->output("UserInactivityPeriod", $inactive_period);
}

1;

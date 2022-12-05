package MaxPublisherShareTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub get_site_links_
{
  my ($self) = @_;
  my @links = map 
    { site_id => $_->{site_id} }, 
      values %{$self->{publishers_}};
  return \@links;
}

sub create_publishers_
{
  my ($self, $args) = @_;
  
  foreach my $p (@$args)
  {
    my %pub_args;
    $pub_args{name} = $p->{name};
    $pub_args{pricedtag_country_code} = $p->{country};
    $pub_args{pricedtag_cpm} = $p->{cpm};

    if (defined $p->{site})
    {
      $pub_args{account_id} = 
          $self->{publishers_}{$p->{site}}->{account_id};
      $pub_args{site_id} = 
          $self->{publishers_}{$p->{site}}->{site_id};
    }
    elsif (defined $p->{account})
    {
      $pub_args{account_id} = 
        $self->{publishers_}{$p->{account}}->{account_id};
    }
    $pub_args{size_id} = $self->{size_} if defined $self->{size_};

    my $publisher = $self->{ns_}->create(Publisher => \%pub_args);    
    $self->{publishers_}->{$p->{name}} = $publisher;
    $self->{ns_}->output("CPM/" . $p->{name}, $p->{cpm});
    $self->{ns_}->output("PUBLISHER/" . $p->{name}, $publisher->{account_id});
    $self->{ns_}->output("TID/" . $p->{name}, $publisher->{tag_id});
  }
}

sub create_display_campaign_
{
  my ($self, $args) = @_;

  my $keyword = make_autotest_name($self->{ns_}, "Keyword");

  my $advertiser = $self->{ns_}->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type});

  my $campaign = $self->{ns_}->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    channel_id => 
      DB::BehavioralChannel->blank(
        account_id => $advertiser,
        name => 'Channel',
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]),
    campaigncreativegroup_cpm => $args->{cpm},
    campaigncreativegroup_budget => $args->{budget},
    campaigncreativegroup_delivery_pacing =>
      defined $args->{daily_budget}? 'F': 'U',
    campaigncreativegroup_daily_budget => $args->{daily_budget},
    campaign_max_pub_share => $args->{share},
    site_links => $self->get_site_links_() });

  $self->{ns_}->output("CC", $campaign->{cc_id});
  $self->{ns_}->output("CCG", $campaign->{ccg_id});
  $self->{ns_}->output("CAMPAIGN", $campaign->{campaign_id});
  $self->{ns_}->output("KEYWORD", $keyword);
}

sub create_text_
{
  my ($self, $args) = @_;
  my %campaigns;
  foreach my $c (@$args)
  {
    die  $self->{ns_}->namespace . ". Campaign '$c->{campaign}' is undefined!" 
      if defined $c->{campaign} and not exists $campaigns{$c->{campaign}};

    my $head_campaign;
    $head_campaign = $campaigns{$c->{campaign}} if defined $c->{campaign};
    my $advertiser = defined $head_campaign? 
      $head_campaign->{account_id}: 
         $self->{ns_}->create(Account => {
           name => 'Advertiser',
           role_id => DB::Defaults::instance()->advertiser_role,
           account_type_id => DB::Defaults::instance()->advertiser_type,
           text_adserving => 'M'});

    my $keyword = make_autotest_name($self->{ns_}, $c->{name});

    my $channel = $self->{ns_}->create(DB::BehavioralChannel->blank(
      name => 'Channel-' . $c->{name},
      account_id => $advertiser,
      keyword_list => $keyword,
      channel_type => 
        defined $c->{type} && $c->{type} eq 'K'? 'K': 'B',
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P', 
          time_to => 0)]));

    my $campaign;
    my %cmp_args;
    $cmp_args{name} = $c->{name};
    $cmp_args{size_id} = $self->{size_};
    $cmp_args{account_id} = $advertiser;
    $cmp_args{campaign_id} = $head_campaign->{campaign_id} if defined $head_campaign;
    $cmp_args{template_id} = DB::Defaults::instance()->text_template;
    $cmp_args{site_links} = $self->get_site_links_();
    $cmp_args{campaigncreativegroup_budget} = $c->{budget};
    $cmp_args{campaigncreativegroup_daily_budget}= $c->{daily_budget} 
      if defined $c->{daily_budget};
    $cmp_args{campaign_max_pub_share} = $c->{share}
      if not defined $head_campaign;
    if (defined $c->{type} and $c->{type} eq 'K')
    {
       $cmp_args{original_keyword} = $keyword;
       $cmp_args{max_cpc_bid} = $c->{cpc};
       $cmp_args{ccgkeyword_channel_id} = $channel->{channel_id};
       $cmp_args{ccgkeyword_ctr} = 0.1;
       $campaign = $self->{ns_}->create(
         TextAdvertisingCampaign => \%cmp_args);
     }
    else
    {
      $cmp_args{campaigncreativegroup_cpm} = $c->{cpm};
      $cmp_args{campaigncreativegroup_cpc} = $c->{cpc};
      $cmp_args{campaigncreativegroup_ctr} = 0.1;
      $cmp_args{channel_id} = $channel->{channel_id};
      $campaign = $self->{ns_}->create(
       ChannelTargetedTACampaign => \%cmp_args);
    }

    $self->{ns_}->output("CAMPAIGN/" . $c->{name}, $campaign->{campaign_id});
    $self->{ns_}->output("CCG/" . $c->{name}, $campaign->{ccg_id});
    $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
    $self->{ns_}->output("KEYWORD/" . $c->{name}, $keyword);
    $campaigns{$c->{name}} = $campaign;
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name, $args) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{case_name_} = $case_name;
  $self->{ns_} = $ns->sub_namespace($case_name);
  $self->{publishers_} = {};
  $self->{size_} = $ns->create(CreativeSize => {
    name => "Text",
    max_text_creatives => $args->{max_text_creatives}}) 
      if defined $args->{max_text_creatives};

  $self->create_publishers_($args->{publishers});
  $self->create_display_campaign_($args->{display}) 
     if (defined $args->{display});
  $self->create_text_($args->{text})
     if (defined $args->{text});

  return $self;
}

1;

package MaxPublisherShareTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub share_expiring
{
  my ($self, $ns) = @_;
  my $case = new MaxPublisherShareTest::TestCase(
    $ns, "EXPIRE",
    { publishers => [{name => '1', cpm => 0}],
      display => 
        { cpm => 1000, budget => 10, share => 0.1}});
}

sub three_sites_daily_budget
{
  my ($self, $ns) = @_;
  my $case = new MaxPublisherShareTest::TestCase(
    $ns, "3SITES",
    { publishers => 
        [ { name => '1', cpm => 0 },
          { name => '2', cpm => 0 },
          { name => '3', cpm => 0, account => '1' } ],
      display => 
        { cpm => 1000, budget => 10000,
          daily_budget => 10, share => 0.1}});  
}

sub text
{
  my ($self, $ns) = @_;

  my $case = new MaxPublisherShareTest::TestCase(
    $ns, "TEXT",
    { max_text_creatives => 3,
      publishers => 
        [ { name => '1', cpm => 0 } ],
      text => 
        [ { name => '1', budget => 0.22, share => 0.3, cpc => 0.06, type => 'K' },
          { name => '2', budget => 0.19, share => 0.3, cpc => 0.059, campaign => '1' },
          { name => '3', budget => 0.10, share => 0.6, cpc => 0.058, type => 'K' } ]});
}

sub text_daily
{
  my ($self, $ns) = @_;

  my $case = new MaxPublisherShareTest::TestCase(
    $ns, "TEXTDAILY",
    { max_text_creatives => 1,
      publishers => 
        [ { name => '1', cpm => 0 }, 
          { name => '2', cpm => 0 } ],
      text => 
        [ { name => '1', budget => undef, daily_budget => 1.0,
            share => 0.1, cpm => 110 } ]});
}

sub init
{
  my ($self, $ns) = @_;
  
  $self->share_expiring($ns);
  $self->three_sites_daily_budget($ns);
  $self->text($ns);
  $self->text_daily($ns);
}

1;

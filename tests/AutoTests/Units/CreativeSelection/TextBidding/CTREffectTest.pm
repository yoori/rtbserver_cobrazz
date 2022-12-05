package CTREffectTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_behavioral_channels
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: channel '$arg->{name}' is redefined!"
      if exists $self->{channels}->{$arg->{name}};

    my (@keywords, @urls);
    if (defined $arg->{keyword_list})
    {
      foreach my $keyword (split(/\W+/, $arg->{keyword_list}))
      {
        $self->{keywords}->{$keyword} = make_autotest_name($self->{ns}, $keyword)
          unless exists $self->{keywords}->{$keyword};
        push @keywords, $self->{keywords}->{$keyword};
        $self->{ns}->output("$arg->{name}/$keyword", $self->{keywords}->{$keyword});
      }
    }
    if (defined $arg->{url_list})
    {
      foreach my $url (split(/\W+/, $arg->{url_list}))
      {
        $self->{urls}->{$url} = "www" . make_autotest_name($self->{ns}, $url) . "com"
          unless exists $self->{urls}->{$url};
        push @urls, $self->{urls}->{$url};
        $self->{ns}->output("$arg->{name}/URL#", $self->{urls}->{$url});
      }
    }
    $arg->{keyword_list} = join("\n", @keywords);
    $arg->{url_list} = join("\n", @urls);

    die "$self->{case_name}: must be defined keyword or url for '$arg->{name}' channel!"
      unless @keywords or @urls;

    foreach my $bp (@{ $arg->{behavioral_parameters} })
    {
      $bp = DB::BehavioralChannel::BehavioralParameter->blank(%$bp);
    }

    $arg->{account_id} = $self->{ns}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role }) unless defined $arg->{account_id};

    $self->{channels}->{$arg->{name}} = $self->{ns}->create(DB::BehavioralChannel->blank(%$arg));

    $self->{ns}->output("$arg->{name}/ID", $self->{channels}->{$arg->{name}}->channel_id());
  }
}

sub create_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns}->{$arg->{name}};

    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    $arg->{size_id} = DB::Defaults::instance()->size unless defined $arg->{size_id};
    $arg->{ccg_keyword_id} = undef;
    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};

    if (defined $arg->{campaign})
    {
      die "$self->{case_name}: campaign '$arg->{campaign}' is undefined!"
        unless defined $self->{campaigns}->{$arg->{campaign}};
      $arg->{campaign_id} = $self->{campaigns}->{$arg->{campaign}}->{campaign_id};
      delete $arg->{campaign};
    }

    if (defined $arg->{creatives})
    {
      $arg->{creative_id} = undef;
      $arg->{cc_id} = undef;
    }

    if (defined $arg->{specific_sites})
    {
      $arg->{campaigncreativegroup_flags} |= DB::Campaign::INCLUDE_SPECIFIC_SITES;
      $arg->{site_links} = [];
      foreach my $site (split(/\W+/, $arg->{specific_sites}))
      {
        die "$self->{case_name}: publisher '$site' is undefined!"
          unless defined $self->{publishers}->{$site};

        push @{$arg->{site_links}},
             { site_id => $self->{publishers}->{$site}->{site_id} };
      }
    }

    my %args_copy = %$arg;
    delete $args_copy{keywords};
    $self->{campaigns}->{$arg->{name}} =
      $self->{ns}->create(TextAdvertisingCampaign => \%args_copy);

    my $cc_idx = 1;
    foreach my $creative (@{$arg->{creatives}})
    {
      $creative->{name} = "$arg->{name}CC" . $cc_idx
        unless defined $creative->{name};
      $creative->{account_id} = $arg->{account_id}
        unless defined $creative->{account_id};
      $creative->{template_id} = DB::Defaults::instance()->text_template
        unless defined $creative->{template_id};

      my $creative_id = $self->{ns}->create(Creative => $creative);
      my $cc_id = $self->{ns}->create(CampaignCreative => {
        creative_id => $creative_id,
        ccg_id => $self->{campaigns}->{$arg->{name}}->{ccg_id} });

      $self->{ns}->output("$arg->{name}/CCID#" . $cc_idx++, $cc_id);
    }

    $self->{ns}->output("$arg->{name}/CCID", $self->{campaigns}->{$arg->{name}}->{cc_id})
      if defined $self->{campaigns}->{$arg->{name}}->{cc_id};

    $self->{ns}->output("$arg->{name}/CCGID", $self->{campaigns}->{$arg->{name}}->{ccg_id});

    my $kwd_idx = 1;
    foreach my $keyword (@{$arg->{keywords}})
    {
      my %args = %$keyword;
      my $keyword_name = $keyword->{original_keyword};
      $args{ccg_id} = $self->{campaigns}->{$arg->{name}}->{ccg_id};
      my ($negative, $kwd) = $keyword->{original_keyword} =~ m/^(-?)(.+)$/;
      $args{original_keyword} = $negative . $self->{keywords}->{$kwd};
      $args{channel_id} = $self->{channels}->{$keyword->{channel}}->channel_id()
        if defined $keyword->{channel};
      delete $args{channel}; 
      my $ccg_keyword_id = $self->{ns}->create(CCGKeyword => \%args);
      $keyword->{max_cpc_bid} = 0 unless defined $keyword->{max_cpc_bid};
      $self->{ns}->output("$arg->{name}/$keyword_name/CPC", $keyword->{max_cpc_bid});
      $self->{ns}->output("$arg->{name}/$keyword_name/ID", $ccg_keyword_id);
      $self->{ns}->output("$arg->{name}/$keyword_name/ECPM",
        $keyword->{max_cpc_bid} * 100 * (defined $keyword->{ctr} ? $keyword->{ctr} : GLOBAL_CTR) * 1000);
      $kwd_idx++;
    }
  }
}

sub create_channel_targeted_campaigns_
{
  my ($self, $type, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns}->{$arg->{name}};

    $arg->{account_id} = $self->{ns}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role }) unless defined $arg->{account_id};
    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};

    if (defined $arg->{channel})
    {
      die "$self->{case_name}: channel '$arg->{channel}' is undefined!"
        unless defined $self->{channels}->{$arg->{channel}};
      $arg->{channel_id} = $self->{channels}->{$arg->{channel}};
      delete $arg->{channel};
    }
    if (defined $arg->{campaign})
    {
      die "$self->{case_name}: campaign '$arg->{campaign}' is undefined!"
        unless defined $self->{campaigns}->{$arg->{campaign}};
      $arg->{campaign_id} = $self->{campaigns}->{$arg->{campaign}}->{campaign_id};
      delete $arg->{campaign};
    }
    if (defined $arg->{specific_sites})
    {
      $arg->{campaigncreativegroup_flags} |= DB::Campaign::INCLUDE_SPECIFIC_SITES;
      $arg->{site_links} = [];
      foreach my $site (split(/\W+/, $arg->{specific_sites}))
      {
        die "$self->{case_name}: publisher '$site' is undefined!"
          unless defined $self->{publishers}->{$site};

        push @{$arg->{site_links}},
             { site_id => $self->{publishers}->{$site}->{site_id} };
      }
      delete $arg->{specific_sites};
    }

    $arg->{campaigncreativegroup_cpm} = 0
      unless defined $arg->{campaigncreativegroup_cpm} or
             defined $arg->{cpm};

    $arg->{campaigncreativegroup_ar} = 0.01
      if defined $arg->{campaigncreativegroup_cpa};

    $self->{campaigns}->{$arg->{name}} = $self->{ns}->create($type => $arg);

    my $cpm = $arg->{campaigncreativegroup_cpm} || $arg->{cpm} || undef;
    my $cpc = $arg->{campaigncreativegroup_cpc} || $arg->{cpc} || undef;
    my $cpa = $arg->{campaigncreativegroup_cpa} || $arg->{cpa} || undef;
    my $ctr = $arg->{campaigncreativegroup_ctr} || $arg->{ctr} || GLOBAL_CTR;

    $self->{ns}->output("$arg->{name}/CCID", $self->{campaigns}->{$arg->{name}}->{cc_id});
    $self->{ns}->output("$arg->{name}/CPM", $cpm) if defined $cpm;
    $self->{ns}->output("$arg->{name}/CPC", $cpc) if defined $cpc;
    $self->{ns}->output("$arg->{name}/CPA", $cpa) if defined $cpa;

    if ($arg->{campaigncreativegroup_rate_type} eq "CPM" and defined $cpm)
    {
      $self->{ns}->output("$arg->{name}/ECPM", $cpm * 100);
    }
    elsif ($arg->{campaigncreativegroup_rate_type} eq "CPC" and defined $cpc)
    {
      $self->{ns}->output("$arg->{name}/ECPM", $cpc * 100 * $ctr * 1000);
    }
    elsif ($arg->{campaigncreativegroup_rate_type} eq "CPA" and defined $cpa)
    {
      $self->{ns}->output("$arg->{name}/ECPM", $cpa * 100 * GLOBAL_CTR * 1000);
    }
  }
}

sub create_display_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    if (defined $arg->{action})
    {
      if (defined $self->{actions}->{$arg->{action}})
      { $arg->{action_id} = $self->{actions}->{$arg->{action}} }
      else
      {
        $arg->{action_id} = $self->{ns}->create(Action => {
          name => $arg->{name},
          url => "http:\\www.foros.com",
          account_id => $arg->{account_id}});
        $self->{actions}->{$arg->{action}} = $arg->{action_id};
      }
      $self->{ns}->output("$arg->{name}/ACTION", $arg->{action_id});
    }
  }
  $self->create_channel_targeted_campaigns_("DisplayCampaign", $args);
}

sub create_channel_targeted_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    $arg->{size_id} = DB::Defaults::instance()->size unless defined $arg->{size_id};
  }

  $self->create_channel_targeted_campaigns_("ChannelTargetedTACampaign", $args);
}

sub create_publishers
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: publisher '$arg->{name}' is redefined!"
      if exists $self->{publishers}->{$arg->{name}};

    $arg->{site_id} = $self->{publishers}->{$arg->{site}}->{site_id}
      if defined $arg->{site};

    $arg->{tag_id} = undef if defined $arg->{tags};
    my %args_copy = %$arg;
    delete $args_copy{site};
    delete $args_copy{tags};
    $self->{publishers}->{$arg->{name}} = $self->{ns}->create(Publisher => \%args_copy );
  
    $self->{ns}->output("$arg->{name}/TAG_ID",
                        $self->{publishers}->{$arg->{name}}->{tag_id})
      unless defined $arg->{tags};

    foreach my $tag (@{$arg->{tags}})
    {
      my $tag_id = $self->{ns}->create(PricedTag => {
        name => $arg->{name}."-".$tag->{name},
        site_id => $self->{publishers}->{$arg->{name}}->{site_id},
        cpm => $tag->{cpm},
        adjustment => $tag->{adjustment}
      });
      $self->{publishers}->{$arg->{name}}->{$tag->{name}} = $tag_id;
      $self->{ns}->output("$arg->{name}/$tag->{name}", $tag_id);
      $self->{ns}->output("$arg->{name}/$tag->{name}/Adjustment", $tag->{adjustment})
        if defined $tag->{adjustment};
    }
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless {}, $self; }

  $self->{ns} = $ns->sub_namespace($case_name);
  $self->{case_name} = $case_name;

  return $self;
}

sub create
{
  my $self = shift;
  my ($class, $args) = @_;
  return $self->{ns}->create($class, $args);  
}

sub output
{
  my $self = shift;
  my ($key, $value, $description) = @_;
  $self->{ns}->output($key, $value, $description);
}

1;


package CTREffectTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant MINUTE => 60;
use constant HOUR => 60 * MINUTE;
use constant DAY => 24 * HOUR;

sub tag_adjustment_
{
  my ($self, $ns) = @_;

  my $test_case = new CTREffectTest::TestCase($ns, "TagAdjustment");

  $test_case->output("REFERER", "http://www.ctrcalculationtest.com");

  my $tag_4_cpm = 70;

  $test_case->create_publishers([
    { name => "Site1",
      account_id => DB::Defaults::instance()->publisher_account,
      tags => [
        { name => "Tag1",
          cpm => $self->{default_cpm},
          adjustment => 1 },
        { name => "Tag2",
          cpm => 20,
          adjustment => 2 },
        { name => "Tag3",
          cpm => 10,
          adjustment => 0.5 },
        { name => "Tag4",
          cpm => $tag_4_cpm,
          adjustment => 16 },
        { name => "Tag5",
          cpm => 0.5,
          adjustment => 0.00000001 },
        { name => "Tag6",
          cpm => 10,
          adjustment => 2 },
        { name => "Tag7",
          cpm => 5,
          adjustment => 0.5 },
        { name => "Tag8",
          cpm => 0,
          adjustment => -1 },
        { name => "Tag9",
          cpm => 0,
          adjustment => 1 }]},

    { name => "Site2",
      account_id => DB::Defaults::instance()->publisher_account,
      tags => [
        { name => "Tag1",
          cpm => 10,
          adjustment => 1 },
        { name => "Tag2",
          cpm => 20,
          adjustment => 2 },
        { name => "Tag3",
          cpm => 10,
          adjustment => 0.5 }]},

    { name => "Site3",
      account_id => DB::Defaults::instance()->publisher_account,
      tags => [
        { name => "Tag1",
          cpm => 10,
          adjustment => 1 },
        { name => "Tag2",
          cpm => 20,
          adjustment => 2 },
        { name => "Tag3",
          cpm => 10,
          adjustment => 0.5 }]},
  ]);

  $test_case->create_behavioral_channels([
    { name => "DChannel-CPM",
      keyword_list => "DKeywordCPM",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel-CPC",
      keyword_list => "DKeywordCPC",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel-CPC-Negative",
      keyword_list => "DKeywordCPCNegative",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel-CPA",
      keyword_list => "DKeywordCPA",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel-CPA-Negative",
      keyword_list => "DKeywordCPANegative",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel1",
      keyword_list => "DKeyword1",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel2",
      keyword_list => "DKeyword2",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "DChannel3",
      keyword_list => "DKeyword3",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "CTChannel-CPM",
      keyword_list => "CTKeywordCPM",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "CTChannel-CPM#2",
      keyword_list => "CTKeywordCPM2",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "CTChannel-CPC",
      keyword_list => "CTKeywordCPC",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "CTChannel-CPA",
      keyword_list => "CTKeywordCPA",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 60 } ] },

    { name => "KChannel1-1",
      channel_type => 'K',
      keyword_list => "KKeyword11",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => DAY },
        { trigger_type => 'S', time_to => DAY } ] },

    { name => "KChannel1-2",
      channel_type => 'K',
      keyword_list => "KKeyword12",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel2",
      channel_type => 'K',
      keyword_list => "KKeyword2",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannelNegative",
      channel_type => 'K',
      keyword_list => "KKeywordNegative",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 5*MINUTE },
        { trigger_type => 'S', time_to => MINUTE } ] }
  ]);

  $test_case->create_display_campaigns([
    { name => "DisplayCPM",
      account_id => $self->{default_account},
      campaign_name => "Display",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 13,
      channel => "DChannel-CPM",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => 0.1 },

    { name => "DisplayCPC",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.13,
      channel => "DChannel-CPC",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => 0.1 },

    { name => "DisplayCPCNegative",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.13,
      channel => "DChannel-CPC-Negative",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => -0.1 },

    { name => "DisplayCPCRON",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.12,
      campaigncreativegroup_ctr => 0.1,
      specific_sites => "Site2",
      campaigncreativegroup_flags => DB::Campaign::RON,
      channel_id => undef },

    { name => "DisplayCPA",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPA',
      campaigncreativegroup_cpa => 1.3,
      channel => "DChannel-CPA",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => 0.1 },

    { name => "DisplayCPANegative",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPA',
      campaigncreativegroup_cpa => 1.3,
      channel => "DChannel-CPA-Negative",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => -0.1 },

    { name => "DisplayCPM#1",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 30,
      specific_sites => "Site1",
      channel => "DChannel1" },

    { name => "DisplayCPM#2",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 23,
      specific_sites => "Site1",
      channel => "DChannel2" },

    { name => "DisplayCPM#3",
      account_id => $self->{default_account},
      campaign => "DisplayCPM",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 11,
      specific_sites => "Site1",
      channel => "DChannel3" },
  ]);

  $test_case->create_channel_targeted_text_campaigns([
    { name => "ChannelTargetedCPM",
      account_id => $self->{default_account},
      campaign_name => "ChannelTargetedText",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 13,
      channel => "CTChannel-CPM",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => 0.1 },

    { name => "ChannelTargetedCPM#2",
      account_id => $self->{default_account},
      campaign => "ChannelTargetedCPM",
      campaigncreativegroup_rate_type => 'CPM',
      campaigncreativegroup_cpm => 30,
      specific_sites => "Site1",
      channel => "CTChannel-CPM#2" },

    { name => "ChannelTargetedCPC",
      account_id => $self->{default_account},
      campaign => "ChannelTargetedCPM",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.13,
      specific_sites => "Site1",
      channel => "CTChannel-CPC",
      campaigncreativegroup_ctr => 0.1 },

    { name => "ChannelTargetedCPCRON",
      account_id => $self->{default_account},
      campaign => "ChannelTargetedCPM",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.12,
      campaigncreativegroup_ctr => 0.1,
      specific_sites => "Site3",
      campaigncreativegroup_flags => DB::Campaign::RON,
      channel_id => undef },

    { name => "ChannelTargetedCPA",
      account_id => $self->{default_account},
      campaign => "ChannelTargetedCPM",
      campaigncreativegroup_rate_type => 'CPA',
      campaigncreativegroup_cpa => 1.3,
      channel => "CTChannel-CPA",
      specific_sites => "Site1",
      campaigncreativegroup_ctr => 0.1 },
  ]);

  my $text_cmp_2_cpc = 0.01;
  my $text_cmp_2_ctr = 1;
  
  $test_case->create_text_campaigns([
    { name => "KeywordTargeted",
      campaign => "ChannelTargetedCPM",
      account_id => $self->{default_account},
      keywords => [
        { original_keyword => "KKeyword11", channel => "KChannel1-1", max_cpc_bid => 0.12, ctr => 0.1 },
        { original_keyword => "KKeyword12", channel => "KChannel1-2", max_cpc_bid => 0.22, ctr => 0.1 },
        { original_keyword => "KKeywordNegative", channel => "KChannelNegative", max_cpc_bid => 0.12, ctr => -0.1 } ] },

    { name => "KeywordTargeted#2",
      campaign => "ChannelTargetedCPM",
      account_id => $self->{default_account},
      keywords => [
        { original_keyword => "KKeyword2", channel => "KChannel2", max_cpc_bid => $text_cmp_2_cpc, ctr => $text_cmp_2_ctr } ] }
  ]);

  my $tag_cpm_with_margin = $tag_4_cpm;
  my $text_cmp_2_ecpm = $text_cmp_2_cpc * $text_cmp_2_ctr * 1000;

  $test_case->output("KeywordTargeted/CLICK_REVENUE",
    money_upscale(($tag_cpm_with_margin - $text_cmp_2_ecpm) / $text_cmp_2_ctr / 1000));
}

sub time_of_week_
{
  my ($self, $ns) = @_;

  my $test_case = new CTREffectTest::TestCase($ns, "TOWCoefficient");

  $test_case->create_behavioral_channels([
    { name => "DChannel",
      keyword_list => "DKeyword",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => MINUTE } ] },

    { name => "KChannel1-1",
      channel_type => 'K',
      keyword_list => "Keyword11",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel1-2",
      channel_type => 'K',
      keyword_list => "Keyword12",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel1-3",
      channel_type => 'K',
      keyword_list => "Keyword13",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel1-4",
      channel_type => 'K',
      keyword_list => "Keyword14",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel2-1",
      channel_type => 'K',
      keyword_list => "Keyword21",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] },

    { name => "KChannel2-2",
      channel_type => 'K',
      keyword_list => "Keyword22",
      account_id => $self->{default_account},
      behavioral_parameters => [
        { trigger_type => 'P', time_to => 7*DAY },
        { trigger_type => 'S', time_to => 7*DAY } ] }
  ]);

  $test_case->create_display_campaigns([
    { name => "DisplayCPC",
      campaigncreativegroup_rate_type => 'CPC',
      campaigncreativegroup_cpc => 0.3,
      campaigncreativegroup_ctr => 0.1,
#      specific_sites => "Site1",
      channel => "DChannel" }
  ]);

  my $text_cmp_2_kw_2_cpc = 0.8;
  my $text_cmp_2_kw_2_ctr = 0.1;

  $test_case->create_text_campaigns([
    { name => "KeywordTargeted#1",
      account_id => $self->{default_account},
      keywords => [
        { original_keyword => "Keyword11", channel => "KChannel1-1", max_cpc_bid => 0.12, ctr => 0.1, tow => 2 },
        { original_keyword => "Keyword12", channel => "KChannel1-2", max_cpc_bid => 0.13, ctr => 0.1, tow => 1 },
        { original_keyword => "Keyword13", channel => "KChannel1-3", max_cpc_bid => 0.12, ctr => 0.1, tow => 16 },
        { original_keyword => "Keyword14", channel => "KChannel1-4", max_cpc_bid => 0.12, ctr => 0.01, tow => 0.00001 } ]},

    { name => "KeywordTargeted#2",
      account_id => $self->{default_account},
      campaign => "KeywordTargeted#1",
      keywords => [
        { original_keyword => "Keyword21", channel => "KChannel2-1", max_cpc_bid => 0.13, ctr => 0.1, tow => 1 },
        { original_keyword => "Keyword22", channel => "KChannel2-2", max_cpc_bid => $text_cmp_2_kw_2_cpc, ctr => $text_cmp_2_kw_2_ctr, tow => 1 } ]}
  ]);

  # Click revenue for campaign above the KeywordTargeted#2 campaign
  # KeywordTargeted#1/Keyword13 CTR = 100% because of TOW = 16
  $test_case->output("KeywordTargeted#1/Keyword13/CLICK_REVENUE",
    money_upscale(($text_cmp_2_kw_2_cpc * 100 * $text_cmp_2_kw_2_ctr * 1000 + 1) / 100 / 1000));

  # Click revenue for campaign above the KeywordTargeted#1 campaign
  # KeywordTargeted#1/Keyword14 CTR = 0% because of TOW = 0.00001
  $test_case->output("KeywordTargeted#2/Keyword22/CLICK_REVENUE",
    money_upscale($self->{default_cpm} / $text_cmp_2_kw_2_ctr / 1000));
}

sub init
{
  my ($self, $ns) = @_;

  $self->{default_account} = $ns->create(Account => {
    name => "default",
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type,
    text_adserving => 'M' });

  $self->{default_cpm} = 10;

  $self->tag_adjustment_($ns);
  $self->time_of_week_($ns);
}

1;

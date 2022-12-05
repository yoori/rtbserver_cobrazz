package ChannelTriggerPerformanceTest::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub new
{
  my ($self, $ns, $case_name) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  $self->{ns_} = $ns->sub_namespace($case_name);
  $self->{case_name_} = $case_name;
  $self->{channels_} = {};
  $self->{campaigns_} = {};
  $self->{advertiser_} =
    $self->{ns_}->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });
  return $self;
}

sub namespace
{
  my $self = shift;
  return $self->{ns_};
}

sub create_channels
{
  my ($self, $args) = @_;

  foreach my $channel (@$args)
  {
    $channel->{name} = "Channel#" . (scalar(keys %{$self->{channels_}}) + 1)
      unless defined $channel->{name};
    die "$self->{case_name_}: channel '$channel->{name}' is redefined!"
      if exists $self->{channels_}->{$channel->{name}};

    my (%page_keywords, %search_keywords, %urls);
    if (defined $channel->{keyword_list})
    {
      %page_keywords = ref $channel->{keyword_list}
        ? %{$channel->{keyword_list}}
        : map { $_ => 'A' } split(/\W+/, $channel->{keyword_list});
    }
    if (defined $channel->{search_list})
    {
      %search_keywords = ref $channel->{search_list}
        ? %{$channel->{keyword_list}}
        : map { $_ => 'A' } split(/\W+/, $channel->{search_list});
    }
    else
    {
      %search_keywords = %page_keywords;
    }
    if (defined $channel->{url_list})
    {
      %urls = ref $channel->{url_list}
        ? %{$channel->{url_list}}
        : map { $_ => 'A' } split(/\W+/, $channel->{url_list});
    }
    delete $channel->{keyword_list};
    delete $channel->{search_list};
    delete $channel->{url_list};

    $channel->{channel_type} = 'B' unless defined $channel->{channel_type};
    $channel->{account_id} = $self->{advertiser_}
      unless defined $channel->{advertiser};
    $channel->{behavioral_parameters} = [
      {trigger_type => "P"},
      {trigger_type => "S"},
      {trigger_type => "U"} ]
      unless  defined $channel->{behavioral_parameters} or
              defined $channel->{expression};

    if (defined $channel->{expression})
    {
      $channel->{channel_type} = 'E';
      for my $e (grep {m/.+/} split(/\W+/, $channel->{expression}))
      {
        die "$self->{case_name_}: Channel '$e' " .
          "(used in expression $channel->{name}) is undefined!"
          unless exists $self->{channels_}->{$e};
        my $channel_id = $self->{channels_}->{$e}->channel_id();
        $channel->{expression} =~ s/$e/$channel_id/g;
      }
    }

    foreach my $bp (@{ $channel->{behavioral_parameters} })
    {
      $bp = DB::BehavioralChannel::BehavioralParameter->blank(%$bp);
    }

    if ($channel->{channel_type} eq 'E')
    {
      delete $channel->{channel_type};
      $self->{channels_}->{$channel->{name}} =
        $self->{ns_}->create(DB::ExpressionChannel->blank(%$channel));
      $channel->{channel_type} = 'E';
    }
    else
    {
      $self->{channels_}->{$channel->{name}} =
        $self->{ns_}->create(DB::BehavioralChannel->blank(%$channel));
    }

    foreach my $trigger_list ({ list => \%page_keywords,
                                trigger_type => 'P' },
                              { list => \%search_keywords,
                                trigger_type => 'S' } )
    {
      foreach my $full_keyword (keys %{$trigger_list->{list}})
      {
        my ($negative, $keyword) = $full_keyword =~ m/^(-?)(.+)$/;
        $self->{keywords_}->{$keyword} =
          make_autotest_name($self->{ns_}, $keyword)
          unless defined $self->{keywords_}->{$keyword};

        $self->{ns_}->output("$channel->{name}/TRIGGERS/$keyword",
          $self->{channels_}->{$channel->{name}}->add_trigger(
            $self->{ns_},
            $negative . $self->{keywords_}->{$keyword},
            $trigger_list->{trigger_type},
            $trigger_list->{list}->{$keyword})->channel_trigger_id()); 

        $self->{ns_}->output("$channel->{name}/KEYWORDS/$keyword",
          $self->{keywords_}->{$keyword});
      }
    }

    foreach my $full_url (keys %urls)
    {
      my ($negative, $url) = $full_url =~ m/^(-?)(.+)$/;
      $self->{urls_}->{$url} = make_autotest_name($self->{ns_}, $url)
        unless defined $self->{urls_}->{$url};

      $self->{ns_}->output("$channel->{name}/TRIGGERS/$url",
        $self->{channels_}->{$channel->{name}}->add_trigger(
          $self->{ns_},
          $negative . $self->{urls_}->{$url},
          'U',
          $urls{$url})->channel_trigger_id());

      $self->{ns_}->output("$channel->{name}/URLS/$url",
        $self->{urls_}->{$url});
    }

    $self->{ns_}->output("$channel->{name}/CHANNEL_ID",
      $self->{channels_}->{$channel->{name}}->channel_id());
    foreach my $bp (@{ $channel->{behavioral_parameters} })
    {
      if (uc($bp->{trigger_type}) eq "P")
      { $self->{ns_}->output("$channel->{name}/PAGE_KEY",
          $self->{channels_}->{$channel->{name}}->page_key()); }
      elsif (uc($bp->{trigger_type}) eq "S")
      { $self->{ns_}->output("$channel->{name}/SEARCH_KEY",
          $self->{channels_}->{$channel->{name}}->search_key()); }
      elsif (uc($bp->{trigger_type}) eq "U")
      { $self->{ns_}->output("$channel->{name}/URL_KEY",
          $self->{channels_}->{$channel->{name}}->url_key()); }
    }
  }
}

sub create_channel_targeted_campaigns_
{
  my ($self, $type, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name_}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns_}->{$arg->{name}};

    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};

    if (defined $arg->{channel})
    {
      die "$self->{case_name_}: channel '$arg->{channel}' is undefined!"
        unless defined $self->{channels_}->{$arg->{channel}};
      $arg->{channel_id} = $self->{channels_}->{$arg->{channel}};
      delete $arg->{channel};
    }

    if (defined $arg->{campaign})
    {
      die "$self->{case_name_}: campaign $arg->{campaign} is undefined!"
        unless defined $self->{campaigns_}->{$arg->{campaign}};
      $arg->{campaign_id} =
        $self->{campaigns_}->{$arg->{campaign}}->{campaign_id};
      $arg->{account_id} =
        $self->{campaigns_}->{$arg->{campaign}}->{account_id};
      delete $arg->{campaign};
    }

    $self->{campaigns_}->{$arg->{name}} = $self->{ns_}->create($type => $arg);

    $self->{ns_}->output("$arg->{name}/CCID",
      $self->{campaigns_}->{$arg->{name}}->{cc_id});
  }
}

sub create_display_campaigns
{
  my ($self, $args) = @_;
  $self->create_channel_targeted_campaigns_("DisplayCampaign", $args);
}

sub create_channel_targeted_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    unless (defined $arg->{size_id})
    {
      $arg->{size_id} = DB::Defaults::instance()->text_size;
      $arg->{creative_size_type_id} =
        DB::Defaults::instance()->other_size_type;
    }
  }
  $self->create_channel_targeted_campaigns_("ChannelTargetedTACampaign", $args);
}


sub create_text_campaigns
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name_}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns_}->{$arg->{name}};

    $arg->{template_id} = DB::Defaults::instance()->text_template
      unless defined $arg->{template_id};
    unless (defined $arg->{size_id})
    {
      $arg->{size_id} = DB::Defaults::instance()->text_size;
      $arg->{creative_size_type_id} =
        DB::Defaults::instance()->other_size_type;
    }
    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};
    $arg->{campaign_flags} = 0
      unless defined $arg->{campaign_flags};

    $arg->{ccg_keyword_id} = undef if defined $arg->{keywords};
    my $keywords = $arg->{keywords};
    delete $arg->{keywords};
    $self->{campaigns_}->{$arg->{name}} =
      $self->{ns_}->create(TextAdvertisingCampaign => $arg);

    $self->{ns_}->output("$arg->{name}/CCID",
      $self->{campaigns_}->{$arg->{name}}->{cc_id});

    foreach my $keyword (@$keywords)
    {
      die "$self->{case_name_}: Channel '$keyword->{channel}' is undefined!"
        unless exists $self->{channels_}->{$keyword->{channel}};

      my $k_channel = $self->{channels_}->{$keyword->{channel}};

      die "$self->{case_name_}: Channel '$keyword->{channel}' " .
        "must be with 'K' type (beacuse it's used in TA campaign)"
        if $k_channel->channel_type() ne 'K';

      my ($channel_trigger) = ( @{$k_channel->{keyword_channel_triggers_}},
                                @{$k_channel->{search_channel_triggers_}} );

      $self->{ns_}->create(CCGKeyword => {
        ccg_id => $self->{campaigns_}->{$arg->{name}}->{ccg_id},
        original_keyword => $channel_trigger->{original_trigger},
        max_cpc_bid => $keyword->{max_cpc_bid},
        trigger_type => $channel_trigger->{trigger_type},
        channel_id => $k_channel->channel_id()});
    }
  }
}

1;

package ChannelTriggerPerformanceTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant MINUTE => 60;
use constant HOUR => 60 * MINUTE;
use constant DAY => 24 * HOUR;

sub reason_of_impression_
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.1');

  $testcase->create_channels([
    { name => "Channel1",
      keyword_list => join(',', map { "Keyword1_$_" } 1 .. 10),
      search_list => '',
      url_list => join(',', map { "URL1_$_" } 1 .. 10),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 } ] },
    { name => "Channel2",
      keyword_list => join(',', map { "Keyword2_$_" } 1 .. 20),
      url_list => "URL2_1",
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE, 
          minimum_visits => 1 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 10 * MINUTE, 
          minimum_visits => 1 } ] },
    { name => "Channel3",
      keyword_list => join(',', map { "Keyword3_$_" } 1 .. 3),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE, 
          minimum_visits => 1 } ] },
    { name => "ChannelCMP",
      account_id => $self->{cmp_account},
      visibility => 'PUB',
      keyword_list => join(',', map { "KeywordCMP_$_" } 1 .. 20 ),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => DAY, 
          minimum_visits => 1 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => DAY, 
          minimum_visits => 1 } ] },
    { name => "ChannelWithoutCCG",
      keyword_list => join(',', map { "Keyword4_$_" } 1 .. 5) .
        ",CommonKeyword",
      search_list => '',
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE, 
          minimum_visits => 1 } ] },
    { name => "KeywordChannel1",
      channel_type => 'K',
      search_list => "KKeyword1",
      behavioral_parameters => [
        { trigger_type => 'S',
          time_from => 0,
          time_to => MINUTE, 
          minimum_visits => 1 } ] },
    { name => "KeywordChannel2",
      channel_type => 'K',
      search_list => "KKeyword2",
      behavioral_parameters => [
        { trigger_type => 'S',
          time_from => 0,
          time_to => MINUTE,    
          minimum_visits => 1 } ] },
    { name => "KeywordChannel3",
      channel_type => 'K',
      search_list => "KKeyword3",
      behavioral_parameters => [
        { trigger_type => 'S',
          time_from => 0,
          time_to => MINUTE,    
          minimum_visits => 1 } ] },
    { name => "EChannel",
      expression => "Channel1|Channel2" },
    { name => "Expression",
      expression => "Channel1|Channel2|Channel3|EChannel|ChannelCMP" }
  ]);

  $testcase->create_display_campaigns([{
    name => "DisplayCampaign",
    cpm => 10,
    channel => "Expression" }]);

  $testcase->create_text_campaigns([{
    name => "TextCampaign",
    keywords => [
      { channel => "KeywordChannel1", max_cpc_bid => 1 },
      { channel => "KeywordChannel2", max_cpc_bid => 1 },
      { channel => "KeywordChannel3", max_cpc_bid => 1 } ] }]);
}

sub behavioral_params_restrictions_ 
{     
  my ($self, $ns) = @_;
          
  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.2');

  $testcase->create_channels([
    { name => "Channel1",
      keyword_list => join(',', map { "Keyword1_$_" } 1 .. 5),
      search_list => '',
      url_list => join(',', map { "URL1_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 5 * MINUTE,
          minimum_visits => 2 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 5 * MINUTE, 
          minimum_visits => 1 } ] },
    { name => "Channel2",
      keyword_list => join(',', map { "Keyword2_$_" } 1 .. 5),
      url_list => join(',', map { "URL2_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 5 * MINUTE,
          minimum_visits => 2 },
        { trigger_type => 'S',
          time_from => 5 * MINUTE,
          time_to => 10 * MINUTE, 
          minimum_visits => 2 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 5 * MINUTE,
          minimum_visits => 1 } ] },
    { name => "Channel3",
      keyword_list => join(',', map { "Keyword3_$_" } 1 .. 5),
      url_list => join(',', map { "URL3_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => DAY, 
          minimum_visits => 2 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => 3 * DAY,        
          minimum_visits => 2 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 2 * DAY,
          minimum_visits => 1 } ] },
  ]);

  $testcase->create_display_campaigns([
    { name => "Campaign1",
      cpm => 10,
      channel => "Channel1" },
    { name => "Campaign2", 
      cpm => 11, 
      channel => "Channel2" },
    { name => "Campaign3", 
      cpm => 12,
      size_id => DB::Defaults::instance()->size_300x250,
      channel => "Channel3" },
  ]);
}

sub trigger_status_restrictions_ 
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.3');

  $testcase->create_channels([
    { name => "Channel",
      keyword_list => {
        Keyword1 => 'A',
        Keyword2 => 'A',
        Keyword3 => 'A',
        Keyword4 => 'H',
        Keyword5 => 'D',
        "-NegativeKeyword" => 'A'},
      url_list => {
        URL1 => 'A',
        URL2 => 'A',
        URL3 => 'A',
        URL4 => 'H',
        URL5 => 'D',
        "-NegativeURL" => 'A' },
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => 20 * MINUTE,
          minimum_visits => 1 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => DAY,
          minimum_visits => 1 } ] } ] );

  $testcase->create_display_campaigns([{
    name => 'Campaign',
    cpm => 10,
    channel => "Channel" }]);
}

sub channels_linked_with_text_campaign_ 
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.4');

  $testcase->create_channels([
    { name => "Channel1",
      keyword_list => "Keyword1_1,CommonKeyword",
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => 20 * MINUTE,
          minimum_visits => 1 } ] },
    { name => "Channel2",
      keyword_list =>
        join(',', map { "Keyword2_$_" } 1 .. 4) . ",CommonKeyword",
      search_list => '',
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 } ] }
  ]);

$testcase->create_channel_targeted_text_campaigns([
  { name => "CTTCampaign1",
    account_text_adserving => 'M',
    cpm => 6,
    channel => "Channel1" },
  { name => "CTTCampaign2",
    campaign => "CTTCampaign1",
    cpm => 5, 
    channel => "Channel1" } ] );
}

sub last_visits_for_history_channels_ 
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.5');

  $testcase->create_channels([{
    name => "Channel",
    keyword_list => join(',', map { "Keyword$_" } 1 .. 5),
    url_list => join(',', map { "URL$_" } 1 .. 5),
    behavioral_parameters => [
      { trigger_type => 'P',
        time_from => 0,
        time_to => DAY,
        minimum_visits => 1 },
      { trigger_type => 'S',
        time_from => DAY,
        time_to => 3 * DAY,
        minimum_visits => 2 },
      { trigger_type => 'U',
        time_from => 0,
        time_to => 2 * DAY,
        minimum_visits => 1 } ] } ]);

  $testcase->create_display_campaigns([{
    name => "Campaign",
    cpm => 10,
    channel => "Channel" }]);

}

sub trigger_performance_for_temp_users_
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.6');

  $testcase->create_channels([
    { name => "Channel1",
      keyword_list => join(',', map { "Keyword1_$_" } 1 .. 10),
      search_list => '',
      url_list => join(',', map { "URL1_$_" } 1 .. 10),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 10 * MINUTE,
          minimum_visits => 1 } ] },
    { name => "Channel2",
      keyword_list => join(',', map { "Keyword2_$_" } 1 .. 5),
      url_list => join(',', map { "URL2_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => DAY,
          minimum_visits => 1 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 2 * DAY,
          minimum_visits => 1 } ] },
    { name => "Expression",
      expression => "Channel1|Channel2" }]);

  $testcase->create_display_campaigns([{
    name => "Campaign",
    cpm => 10,
    channel => "Expression" }]);
}

sub asynchronous_logging_
{
  my ($self, $ns) = @_;

  my $testcase = new ChannelTriggerPerformanceTest::Case($ns, 'Test#2.7');

  $testcase->create_channels([
    { name => "Channel1",
      keyword_list => join(',', map { "Keyword1_$_" } 1 .. 5),
      url_list => join(',', map { "URL1_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => DAY,
          minimum_visits => 2 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => 3 * DAY,
          minimum_visits => 2 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 2 * DAY,
          minimum_visits => 1 } ] },
    { name => "Channel2",
      keyword_list => join(',', map { "Keyword2_$_" } 1 .. 5),
      url_list => join(',', map { "URL2_$_" } 1 .. 5),
      behavioral_parameters => [
        { trigger_type => 'P',
          time_from => 0,
          time_to => DAY,
          minimum_visits => 2 },
        { trigger_type => 'S',
          time_from => 0,
          time_to => 3 * DAY,
          minimum_visits => 2 },
        { trigger_type => 'U',
          time_from => 0,
          time_to => 2 * DAY,
          minimum_visits => 1 } ] },
    { name => "Expression",
      expression => "Channel1|Channel2" }]);

  $testcase->create_display_campaigns([{
    name => "Campaign",
    cpm => 12,
    channel => "Expression" }]);
}

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    role_id => DB::Defaults::instance()->advertiser_role(),
    account_type_id => DB::Defaults::instance()->advertiser_type()});

  $self->{cmp_account} = $ns->create(Account =>
    { name => 'CMP',
      role_id => DB::Defaults::instance()->cmp_role,
      account_type_id => DB::Defaults::instance()->cmp_type });

  my $publisher_468x60 = $ns->create(Publisher => {
    name => 'tag_468x60',
    pricedtag_adjustment => 0.9 });

  my $publisher_300x250 = $ns->create(Publisher => {
    name => 'tag_300x250',
    pricedtag_size_id => DB::Defaults::instance()->size_300x250 });

  $ns->output("TAG/468x60", $publisher_468x60->{tag_id});
  $ns->output("TAG/300x250", $publisher_300x250->{tag_id});
  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("NonDefaultColo", DB::Defaults::instance()->ads_isp->{colo_id});

  $self->reason_of_impression_($ns);
  $self->behavioral_params_restrictions_($ns);
  $self->trigger_status_restrictions_($ns);
  $self->channels_linked_with_text_campaign_($ns);
  $self->last_visits_for_history_channels_($ns);
  $self->trigger_performance_for_temp_users_($ns);
  $self->asynchronous_logging_($ns);

}

1;

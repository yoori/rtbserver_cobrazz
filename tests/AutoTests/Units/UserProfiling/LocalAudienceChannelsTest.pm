
package LocalAudienceChannelsTest::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub new
{
  my $self = shift;
  my ($ns, $casename, $entities) = @_;

  $self = bless {}, $self unless ref $self;

  $self->{ns_} = $ns->sub_namespace($casename);
  $self->{casename} = $casename;

  # Defines creators and creation order
  my @creators = (
    [ 'publishers',\&create_publishers ],
    [ 'behavioral_channels',\&create_behavioral_channels ],
    [ 'audience_channels', \&create_audience_channels ],
    [ 'expression_channels', \&create_expression_channels ],
    [ 'display_campaigns', \&create_display_campaigns ],
    [ 'text_campaigns', \&create_channel_targeted_text_campaigns ]
  );

  foreach (@creators)
  {
    my ($entity, $func) = @$_;
    $func->($self, $entities->{$entity}) if exists $entities->{$entity};
  }

  return $self;
}

sub create_behavioral_channels
{
  my $self = shift;
  my ($args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{caename}: Channel '$arg->{name}' already defined!"
      if exists $self->{channels_}->{$arg->{name}};

    $arg->{keyword_list} = make_autotest_name($self->{ns_}, $arg->{name})
      unless defined $arg->{keyword_list} or
             defined $arg->{url_list} or
             defined $arg->{search_list} or
             defined $arg->{url_kwd_list};

    unless (exists $arg->{behavioral_parameters})
    {
      my $bp = $arg->{behavioral_parameters};
      push @$bp, {trigger_type => "P"} if defined $arg->{keyword_list};
      push @$bp, {trigger_type => "S"} if defined $arg->{search_list};
      push @$bp, {trigger_type => "U"} if defined $arg->{url_list};
      push @$bp, {trigger_type => "R"} if defined $arg->{url_kwd_list};
    }

    foreach my $bp (@{ $arg->{behavioral_parameters} })
    {
      $bp = DB::BehavioralChannel::BehavioralParameter->blank(%$bp);
    }

    $arg->{account_id} = $self->{ns_}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role })
      unless defined $arg->{account_id};

    $self->{channels_}->{$arg->{name}} =
      $self->{ns_}->create(DB::BehavioralChannel->blank(%$arg));
    $self->{ns_}->output("$arg->{name}/ID",
      $self->{channels_}->{$arg->{name}}->channel_id());
  }
}

sub create_audience_channels
{
  my $self = shift;
  my ($args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{caename}: Channel '$arg->{name}' already defined!"
      if exists $self->{channels_}->{$arg->{name}};

    $arg->{account_id} = $self->{ns_}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role })
      unless defined $arg->{account_id};

    $self->{channels_}->{$arg->{name}} =
      $self->{ns_}->create(DB::AudienceChannel->blank(%$arg));

    $self->{ns_}->output("$arg->{name}/ID",
      $self->{channels_}->{$arg->{name}}->channel_id());
  }
}

sub create_expression_channels
{
  my $self = shift;
  my ($args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{caename}: Channel '$arg->{name}' already defined!"
      if exists $self->{channels_}->{$arg->{name}};

    my $expression = $arg->{expression}
      or die "$self->{caename}: expression must " .
             "be defined for '$arg->{name}' channel";

    foreach my $ch_name (split(/\s+/, $arg->{expression}))
    {
      die "$self->{caename}: Channel '$ch_name' " .
          "used in expression isn't defined"
        unless defined $self->{channels_}->{$ch_name};
      $arg->{expression} =~
        s/$ch_name/$self->{channels_}->{$ch_name}->{channel_id}/;
    }

    $arg->{account_id} = $self->{ns_}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role })
      unless defined $arg->{account_id};

    $self->{channels_}->{$arg->{name}} = $self->{ns_}->create(
      DB::ExpressionChannel->blank(%$arg));

    $self->{ns_}->output("$arg->{name}/ID",
      $self->{channels_}->{$arg->{name}}->channel_id());
  }
}

sub create_channel_targeted_campaigns_
{
  my ($self, $type, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{casename}: campaign '$arg->{name}' is redefined!"
      if exists $self->{campaigns_}->{$arg->{name}};

    $arg->{campaigncreativegroup_flags} = 0
      unless defined $arg->{campaigncreativegroup_flags};
    $arg->{campaign_flags} = 0
      unless defined $arg->{campaign_flags};

    if (defined $arg->{channel})
    {
      die "$self->{casename}: Channel '$arg->{channel}' isn't defined!"
        unless defined $self->{channels_}->{$arg->{channel}};
      $arg->{channel_id} = $self->{channels_}->{$arg->{channel}};
      delete $arg->{channel};
    }
    else
    {
      $arg->{campaigncreativegroup_flags} |= DB::Campaign::RON
    }
    if (defined $arg->{campaign})
    {
      die "$self->{casename}: campaign '$arg->{campaign}' isn't defined!"
        unless defined $self->{campaigns_}->{$arg->{campaign}};
      $arg->{campaign_id} = $self->{campaigns_}->{$arg->{campaign}}->{campaign_id};
      delete $arg->{campaign};
    }
    if (defined $arg->{specific_sites})
    {
      $arg->{site_links} = [];
      foreach my $site (split(/\W+/, $arg->{specific_sites}))
      {
        die "$self->{casename}: publisher '$site' is undefined!"
          unless defined $self->{publishers_}->{$site};

        push @{$arg->{site_links}},
             { site_id => $self->{publishers_}->{$site}->{site_id} };
      }
      delete $arg->{specific_sites};
      $arg->{campaigncreativegroup_flags} |=
        DB::Campaign::INCLUDE_SPECIFIC_SITES;
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
      $arg->{creative_size_type_id} =
        DB::Defaults::instance()->other_size_type;
      $arg->{size_id} = DB::Defaults::instance()->text_size;
    }
  }
  $self->create_channel_targeted_campaigns_("ChannelTargetedTACampaign", $args);
}

sub create_publishers
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{casename}: Publisher '$arg->{name}' already defined!"
      if defined $self->{publishers_}->{$arg->{name}};

    if (defined $arg->{site})
    {
      die "$self->{casename}: Site '$arg->{site}' isn't defined!"
        unless exists $self->{publishers_}->{$arg->{site}};
      $arg->{site_id} = $self->{publishers_}->{$arg->{site}}->{site_id};
      delete $arg->{site};
    }

    $self->{publishers_}->{$arg->{name}} =
      $self->{ns_}->create(Publisher => $arg);

    $self->{ns_}->output("$arg->{name}/TAG_ID",
      $self->{publishers_}->{$arg->{name}}->{tag_id});
  }
}

1;

package LocalAudienceChannelsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub channels_matching
{
  my $self = shift;
  my ($ns) = @_;

  my $size = $ns->create(CreativeSize => {
    name => "120x600",
    max_text_creatives => 3 });

  new LocalAudienceChannelsTest::Case($ns, "ChannelMatching",{
    behavioral_channels => [{ name => "Behavioral" }],
    audience_channels => [{ name => "Audience1" },
                          { name => "Audience2" },
                          { name => "Audience3" },
                          { name => "Audience4" },
                          { name => "Audience5" },
                          { name => "Audience6" },
                          { name => "Audience7" }],
    expression_channels => [{ name => "Expression1",
                              expression => "Behavioral|Audience6" },
                            { name => "Expression2",
                              expression => "Audience6&Behavioral" },
                            { name => "Expression3",
                              expression => "Audience5^Audience6" }],
    display_campaigns => [{ name => "DCampaign",
                            campaigncreativegroup_cpm => 10,
                            channel => "Audience1",
                            size_id => $size },
                          { name => "DCampaignRTB",
                            channel => "Audience7",
                            size_id => $size }],
    text_campaigns => [{ name => "TCampaign1",
                         campaigncreativegroup_cpm => 3,
                         channel => "Audience2" },
                       { name => "TCampaign2",
                         campaigncreativegroup_cpm => 2,
                         campaign => "TCampaign1",
                         channel => "Audience3" },
                       { name => "TCampaign3",
                         campaigncreativegroup_cpm => 1,
                         campaign => "TCampaign1",
                         channel => "Audience4" },
                       { name => "TCampaign4",
                         channel => "Expression1" },
                       { name => "TCampaign5",
                         channel => "Expression2" },
                       { name => "TCampaign6",
                         channel => "Expression3" }],
    publishers => [{ name => "DefaultPublisher",
                     pricedtag_size_id => $size },
                   { name => "RTBPublisher",
                     pricedtag_size_id => $size,
                     account_id => DB::Defaults::instance()->openrtb_account }]
  });
}

sub uids_list_update
{
  my $self = shift;
  my ($ns) = @_;

  new LocalAudienceChannelsTest::Case($ns, "UidsListUpdate", {
    audience_channels => [{ name => "Audience1" },
                          { name => "Audience2" },
                          { name => "Audience3" }],
    display_campaigns => [{ name => "DCampaign1",
                            campaigncreativegroup_cpm => 3,
                            channel => "Audience1" },
                          { name => "DCampaign2",
                            campaigncreativegroup_cpm => 2,
                            channel => "Audience2" },
                          { name => "DCampaign3", # untargeted
                            campaigncreativegroup_cpm => 1 }]
  });
}


sub init
{
  my ($self, $ns) = @_;

}

1;

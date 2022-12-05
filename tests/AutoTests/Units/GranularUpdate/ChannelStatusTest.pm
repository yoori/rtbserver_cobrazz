package ChannelStatusTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant ACTIVE => "A";
use constant DELETED => "D";
use constant INACTIVE => "I";
use constant PENDING_INACTIVATION => "E";

our %expression_channels;
our %behavioral_channels;

sub channel_status
{
  my ($display_status) = @_;
  if ($display_status eq DB::BehavioralChannel::DISPLAY_STATUS_PENDING_INACTIVATION)
  {
    return PENDING_INACTIVATION;
  }
  elsif ($display_status eq DB::Defaults::instance()->DISPLAY_STATUS_DELETED)
  {
    return DELETED;
  }
  elsif ($display_status eq DB::Defaults::instance()->DISPLAY_STATUS_INACTIVE)
  {
    return INACTIVE;
  }
  return ACTIVE;
}

sub make_expression_channel_name
{
  my ($original) = @_;
  $original =~ s/[\(\)]/-/g;
  $original = "E-$original";
  return $original;
}

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $cmp = $ns->create(Account => {
    name => 'CMP',
    role_id => DB::Defaults::instance()->cmp_role });

  foreach my $display_status (DB::Defaults::instance()->DISPLAY_STATUS_LIVE,
    DB::Defaults::instance()->DISPLAY_STATUS_DELETED,
    DB::Defaults::instance()->DISPLAY_STATUS_INACTIVE,
    DB::BehavioralChannel::DISPLAY_STATUS_PENDING_INACTIVATION)
  {
    for my $is_const ("var", "const")
    {
      my $adserver_status = channel_status($display_status);
      my $name = "B-$adserver_status-$is_const";
      my $keyword = make_autotest_name($ns, $name);
      $behavioral_channels{$name} = $ns->create(DB::BehavioralChannel->blank(
        name => $name,
        account_id =>  $adserver_status eq PENDING_INACTIVATION ? $cmp : $advertiser,
        keyword_list => $keyword,
        status => $adserver_status ne PENDING_INACTIVATION ? $adserver_status : ACTIVE,
        qa_status => 'A',
        display_status_id => $display_status,
        cpm => $adserver_status eq PENDING_INACTIVATION ? 1 : undef,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P')]));

      $ns->update($behavioral_channels{$name}, { status => $adserver_status })
        if $adserver_status eq PENDING_INACTIVATION;

      $behavioral_channels{$name}->{triggers} = $keyword;

      $ns->output("$name/CHANNEL_ID",
        $behavioral_channels{$name}->channel_id());
      $ns->output("$name/TRIGGERS", $keyword);
      $ns->output("$name/STATUS",
        $adserver_status eq PENDING_INACTIVATION ? ACTIVE : $adserver_status);
      $ns->output("$name/DISPLAY_STATUS", $display_status);
    }
  }

  #$ns->output("SimpleChannelsStaticCases", join(',', keys %behavioral_channels));

  # for channels threshold feature
  my $ct_publisher = $ns->create(Publisher => {name => "B-CT" });
  $ns->output("Tag", $ct_publisher->{tag_id});

  my %ct_cases = ("B-CT-A" => "GN",
                  "B-CT-W" => "GB");

  while (my ($name, $country) = each %ct_cases)
  {
    my $keyword = make_autotest_name($ns, $name);
    my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => $name,
      account_id => $advertiser,
      country_code => $country,
      keyword_list => $keyword,
      display_status_id => $country eq "GB"
        ? DB::BehavioralChannel::DISPLAY_STATUS_NOT_ENOUGH_USERS
        : DB::Defaults::instance()->DISPLAY_STATUS_LIVE,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*24*60*60 )]));

    $channel->{triggers} = $keyword;
    $behavioral_channels{$name} = $channel;

    my $campaign = $ns->create(DisplayCampaign => {
      name => $name,
      account_id => $advertiser,
      channel_id => $channel,
      campaigncreativegroup_display_status_id => $country eq "GB"
        ? DB::CampaignCreativeGroup::DISPLAY_STATUS_CHANNEL_NEED_ATTENTION
        : DB::Defaults::instance()->DISPLAY_STATUS_LIVE,
      site_links => [{site_id => $ct_publisher->{site_id}}] });

    $ns->output("$name/CHANNEL_ID", $channel);
    $ns->output("$name/TRIGGERS", $keyword);
    $ns->output("$name/STATUS", $country eq "GB" ? "W" : "A");
    $ns->output("$name/CC_ID", $campaign->{cc_id});
    $ns->output("$name/CCG_ID", $campaign->{ccg_id});
  }

  $ns->output("SimpleChannelsStaticCases", join(',', keys %behavioral_channels));

  my @expressions = (
    { name => "E-A", expression => "B-A-var", status => 'A', keyword_to_match => ["B-A-var"] },
    { name => "E-I", expression => "B-I-var", status => 'I', keyword_to_match => ["B-I-var"] },
    { name => "E-D", expression => "B-D-var", status => 'I', keyword_to_match => ["B-D-var"] },
    { name => "E-AandI", expression => "B-A-const&B-I-var", status => 'I', keyword_to_match => ["B-A-const", "B-I-var"] },
    { name => "E-AorD", expression => "B-A-var|B-D-const", status => 'A', keyword_to_match => ["B-A-var", "B-D-const"] },
    { name => "E-AandnotD", expression => "B-A-const^B-D-var", status => 'I', keyword_to_match => ["B-A-const"] },
    { name => "E-AandnotI", expression => "B-A-const^B-I-var", status => 'I', keyword_to_match => ["B-A-const"] },
    # Second level
    { name => "E-{AorD}andA", expression => "(B-E-var|B-D-var)&B-A-var", status => 'A', keyword_to_match => ["B-A-const", "B-D-var", "B-A-var"] },
    { name => "E-{AandnotI}andA", expression => "(B-E-var^B-I-var)&B-A-var", status => 'I', keyword_to_match => ["B-A-const", "B-A-var"] },
    { name => "E-{AandI}orA", expression => "(B-A-const&B-I-const)|B-A-var", status => 'A', keyword_to_match => ["B-A-const", "B-I-const", "B-A-var"] },
    { name => "E-{AandnotI}orA", expression => "(B-A-var^B-I-const)|B-A-const", status => 'A', keyword_to_match => ["B-A-const"] },
    { name => "E-{AorI}andnotA", expression => "(B-A-var|B-I-const)^B-A-const", status => 'A', keyword_to_match => ["B-A-var", "B-I-const"] },
    { name => "E-{AandD}andnotA", expression => "(B-A-const&B-D-var)^B-E-const", status => 'I', keyword_to_match => ["B-A-const", "B-D-var"] },
    # Channel threshold feature
    { name => "E-WandA", expression => "B-CT-W&B-A-const", status => 'I', keyword_to_match => ["B-CT-W", "B-A-const"]},
    { name => "E-WorA", expression => "B-CT-W|B-A-const", status => 'A', keyword_to_match => ["B-CT-W", "B-A-const"]},
    { name => "E-WandnotA", expression => "B-CT-W^B-A-const", status => 'I', keyword_to_match => ["B-CT-W"]},
    { name => "E-AorI", expression => "B-CT-A|B-I-const", status => 'A', keyword_to_match => ["B-CT-A", "B-I-const"]},
    # Expression in expression
    { name => "E-eAorI", expression => "E-AorD|B-I-const", status => 'A', keyword_to_match => ["B-A-var", "B-D-const", "B-I-const"] },
    { name => "E-eAandeI", expression => "E-{AandnotI}orA&E-AandI", status => 'I', keyword_to_match => ["B-A-const", "B-I-var"] }
  );

  foreach my $expression_channel (@expressions)
  {
    my $name = $expression_channel->{name};
    die "Channel name ($name) must not conatin symbols that used in expressions!"
      if $name =~ m/[\|\&\^\(\)]/;
    my $expression = $expression_channel->{expression};

    foreach (split(/[\|\&\^\(\)]/, $expression))
    {
      next unless $_;
      my $channel = $behavioral_channels{$_};
      $channel = $expression_channels{$_} unless defined $channel;
      die "Channel ($_) that used in expression hasn't been defined!"
        unless defined $channel;
      my $id = $channel->channel_id();
      $expression =~ s/(\W|^)$_(\W|$)/$1$id$2/g
    }

    my $channel = $ns->create(DB::ExpressionChannel->blank(
      name => $name,
      account_id => $advertiser,
      display_status_id => $expression_channel->{status} eq 'A'
        ? DB::ExpressionChannel::DISPLAY_STATUS_LIVE_CHANNELS_NEED_ATTENTION
        : DB::ExpressionChannel::DISPLAY_STATUS_NOT_LIVE_CHANNELS_NEED_ATTENTION,
      expression => $expression));

    $expression_channels{$name} = $channel;

    $ns->output("$name/CHANNEL_ID", $channel->channel_id());
    $ns->output("$name/TRIGGERS",
      join(',', map {$behavioral_channels{$_}->{triggers}} @{$expression_channel->{keyword_to_match}}));
    $ns->output("$name/STATUS", $expression_channel->{status});

    my $publisher = $ns->create(Publisher => {
      name => $name });

    $ns->output("$name/TAG_ID", $publisher->{tag_id});

    my $campaign = $ns->create(DisplayCampaign => {
      name => $name,
      account_id => $advertiser,
      channel_id => $channel,
      site_links => [{site_id => $publisher->{site_id}}] });

    $ns->output("$name/CC_ID", $campaign->{cc_id});
    $ns->output("$name/TARGETING_CHANNEL_ID", $campaign->{CampaignCreativeGroup}->{targeting_channel_id});
  }

#  create_expression_channels($ns, $advertiser,
#    "A" => { expression => "B-A-var", status => 'A', keyword_to_match => ["B-A-var"] },
#    "I" => { expression => "B-I-var", status => 'I', keyword_to_match => ["B-I-var"] },
#    "D" => { expression => "B-D-var", status => 'I', keyword_to_match => ["B-D-var"] },
#    "AandI" => { expression => "B-A-const&B-I-var", status => 'I', keyword_to_match => ["B-A-const", "B-I-var"] },
#    "AorD" => { expression => "B-A-var|B-D-const", status => 'A', keyword_to_match => ["B-A-var", "B-D-const"] },
#    "AandnotD" => { expression => "B-A-const^B-D-var", status => 'I', keyword_to_match => ["B-A-const"] },
#    "AandnotI" => { expression => "B-A-const^B-I-var", status => 'I', keyword_to_match => ["B-A-const"] },
#    # Second level
#    "(AorD)andA" => { expression => "(B-E-var|B-D-var)&B-A-var", status => 'A', keyword_to_match => ["B-A-const", "B-D-var", "B-A-var"] },
#    "(AandnotI)andA" => { expression => "(B-E-var^B-I-var)&B-A-var", status => 'I', keyword_to_match => ["B-A-const", "B-A-var"] },
#    "(AandI)orA" => { expression => "(B-A-const&B-I-const)|B-A-var", status => 'A', keyword_to_match => ["B-A-const", "B-I-const", "B-A-var"] },
#    "(AandnotI)orA" => { expression => "(B-A-var^B-I-const)|B-A-const", status => 'A', keyword_to_match => ["B-A-const"] },
#    "(AorI)andnotA" => { expression => "(B-A-var|B-I-const)^B-A-const", status => 'A', keyword_to_match => ["B-A-var", "B-I-const"] },
#    "(AandD)andnotA" => { expression => "(B-A-const&B-D-var)^B-E-const", status => 'I', keyword_to_match => ["B-A-const", "B-D-var"] },
#    # Channel threshold feature
#    "WandA" => { expression => "B-CT-W&B-A-const", status => 'I', keyword_to_match => ["B-CT-W", "B-A-const"]},
#    "WorA" => { expression => "B-CT-W|B-A-const", status => 'A', keyword_to_match => ["B-CT-W", "B-A-const"]},
#    "WandnotA" => { expression => "B-CT-W^B-A-const", status => 'I', keyword_to_match => ["B-CT-W"]},
#    "AorI" => { expression => "B-CT-A|B-I-const", status => 'A', keyword_to_match => ["B-CT-A", "B-I-const"]}
#  );
#
#  create_expression_channels($ns, $advertiser,
#    # Expression in expression
#    "eAorI" => { expression => "E-AorD|B-I-const", status => 'A', keyword_to_match => ["B-A-var", "B-D-const", "B-I-const"] },
#    "eAandeI" => { expression => "E--AandnotI-orA&E-AandI", status => 'I', keyword_to_match => ["B-A-const", "B-I-var"] }
#  );

  $ns->output("ExpressionChannelsStaticCases",
    join(',', map { $_->{name} } @expressions));
}

1;

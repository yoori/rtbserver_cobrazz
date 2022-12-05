
package BudgetOverrunsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

use constant DISPLAY => 'DisplayCampaign';
use constant CT_TEXT => 'ChannelTargetedTACampaign';
use constant TEXT => 'TextAdvertisingCampaign';

sub creative_revenue 
{
  my ($self, $params) = @_;
  if (defined $params->{cpm})
  {
    return $params->{cpm} / 1000;
  }
  if (defined $params->{cpc})
  {
    return $params->{cpc};
  }
  if (defined $params->{cpa})
  {
    return $params->{cpa};
  }
  return 0;
}

sub channel_revenue
{
  my ($self, $params) = @_;
  if (defined $params->{channel_rate})
  {
    return $params->{channel_rate} / 1000;
  }
  return 0;
}

sub request_revenue
{
  my ($self, $params) = @_;
  return
    $self->channel_revenue($params) +
    $self->creative_revenue($params);
}

sub budget
{
  my ($self, $params) = @_;
  
  my $min = $params->{campaign_budget} ||
            $params->{ccg_budget} ||
            $params->{campaign_daily_budget} ||
            $params->{ccg_daily_budget};

  return 0 unless $min;

  foreach my $budget_fld (qw(campaign_budget
                             ccg_budget
                             campaign_daily_budget
                             ccg_daily_budget))
  {
    $min = $params->{$budget_fld}
      if $params->{$budget_fld} && $min > $params->{$budget_fld};
  }

  return $min;
}

sub create_campaign
{
  my $self = shift;
  my ($ns, $type, $name, $params) = @_;

  $params->{name} = $name;
  $params->{campaign_flags} = 0;
  $params->{campaigncreativegroup_flags} = 0;
  $params->{account_id} = $ns->create(Account => {
    name => $name,
    role_id => DB::Defaults::instance()->advertiser_role(),
    currency_id => defined $params->{exchange_rate}
      ? $ns->create(Currency => { rate => $params->{exchange_rate} })
      : DB::Defaults::instance()->currency() } ) unless defined $params->{account_id};
  $params->{campaign_budget} = $params->{campaign_budget}
    if defined $params->{campaign_budget};
  $params->{campaigncreativegroup_budget} = $params->{ccg_budget}
    if defined $params->{ccg_budget};
  #delete $params->{ccg_budget};

  if (defined $params->{campaign_daily_budget})
  {
    $params->{campaign_daily_budget} = $params->{campaign_daily_budget};
    $params->{campaign_delivery_pacing} = 'F';
  }

  if (defined $params->{ccg_daily_budget})
  {
    $params->{campaigncreativegroup_daily_budget} = $params->{ccg_daily_budget};
    $params->{campaigncreativegroup_delivery_pacing} = 'F';
    #delete $params->{ccg_daily_budget};
  }

  my $keyword = make_autotest_name($ns, $name);
  if ($type eq TEXT)
  {
    $params->{original_keyword} = $keyword;
    my $channel = $ns->create(DB::BehavioralChannel->blank(
        name => $name,
        channel_type => 'K',
        keyword_list => $keyword,
        account_id => $params->{account_id},
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P') ]));
    $params->{ccgkeyword_channel_id} = $channel->channel_id();
  }
  else
  {
    $params->{channel_id} = $ns->create(DB::BehavioralChannel->blank(
      account_id => defined $params->{channel_rate} ?
        $ns->create(Account => {
          name => "$name-CMP",
          role_id => DB::Defaults::instance()->cmp_role,
          account_type_id => DB::Defaults::instance()->cmp_type })
        : $params->{account_id},
      name => $name,
      keyword_list => $keyword,
      cpm => $params->{channel_rate},
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P") ]));
  }

  $params->{campaigncreativegroup_cpm} = $params->{cpm}
    if defined $params->{cpm};
  if (defined $params->{cpc})
  {
    $params->{campaigncreativegroup_cpm} = 0;
    if ($type ne TEXT)
    {
      $params->{campaigncreativegroup_cpc} = $params->{cpc};
    }
    else
    {
      $params->{max_cpc_bid} = $params->{cpc};
    }
  }

  if ($type ne DISPLAY) # Text
  {
    $params->{template_id} = DB::Defaults::instance()->text_template
        unless defined $params->{template_id};

    unless (defined $params->{size_id})
    {
      $params->{creative_size_type_id} = DB::Defaults::instance()->other_size_type;
      $params->{size_id} = DB::Defaults::instance()->text_size;
    }
  }

  $params->{campaigncreativegroup_ctr} = 0.1 if $type eq CT_TEXT;

  $ns->output("$name/Revenue", $self->request_revenue($params));
  $ns->output("$name/Budget", $self->budget($params));

  delete $params->{ccg_budget};
  delete $params->{ccg_daily_budget};
  delete $params->{cpm};
  delete $params->{cpc};
  delete $params->{channel_rate};
  my $campaign = $ns->create($type => $params);

  $ns->output("$name/Keyword", $keyword);
  $ns->output("$name/CCG", $campaign->{ccg_id});
  $ns->output("$name/CC", $campaign->{cc_id});
  $ns->output("$name/Campaign", $campaign->{campaign_id});
  #$ns->output("$name/Revenue", $self->request_revenue($params));
  #$ns->output("$name/Budget", $self->budget($params));
}

sub init
{
  my ($self, $ns) = @_;

  my $publisher = $ns->create(Publisher => { name => 'Pub' });
  $ns->output("Tag", $publisher->{tag_id});

  my $advertiser = $ns->create(Account => {
    name => "Adv",
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type } );

  # Cases
  $self->create_campaign($ns, DISPLAY, "DisplayCPM",
    { account_id => $advertiser,
      cpm => 1001,
      ccg_budget => 2  });

  $self->create_campaign($ns, DISPLAY, "DisplayCPC",
    { account_id => $advertiser,
      cpc => 120,
      ccg_budget => 100 });

  $self->create_campaign($ns, CT_TEXT, "ChannelTargetedCPC",
    { account_id => $advertiser,
      cpc => 100,
      ccg_budget => 100 });

  $self->create_campaign($ns, TEXT, "TextCPC",
    { account_id => $advertiser,
      cpc => 80,
      ccg_budget => 100 });

  $self->create_campaign($ns, DISPLAY, "DisplayChannelRate",
    { account_id => $advertiser,
      cpm => 5000,
      channel_rate => 5000,
      ccg_budget => 10 });

  $self->create_campaign($ns, CT_TEXT, "TextChannelRate",
    { account_id => $advertiser,
      cpm => 5000,
      channel_rate => 5000,
      ccg_budget => 10 });

  $self->create_campaign($ns, DISPLAY, "DisplayGross",
    { account_id => $ns->create(Account => {
        name => 'GrossAdvSys',
        commission => 0.1,
        agency_account_id => 
          DB::Defaults::instance()->agency_gross(),
        role_id => DB::Defaults::instance()->advertiser_role() }),
      campaign_commission => 0.1,
      cpm => 10000,
      ccg_budget => 10 });

  $self->create_campaign($ns, DISPLAY, "DisplayNet",
    { account_id => $ns->create(Account => {
        name => 'NetAdvSys',
        commission => 0.5,
        role_id => DB::Defaults::instance()->advertiser_role() }),
      campaign_commission => 0.5,
      cpm => 10000,
      ccg_budget => 20 });

  $self->create_campaign($ns, TEXT, "TextGross",
    { account_id => $ns->create(Account => {
        name => 'GrossAdv',
        commission => 0.1,
        agency_account_id => 
          DB::Defaults::instance()->agency_gross(),
        role_id => DB::Defaults::instance()->advertiser_role(),
        currency_id => $ns->create(Currency => { rate => 0.5 }) }),
      campaign_commission => 0.1,
      cpc => 0.1,
      ccg_budget => 0.1,
      ccg_daily_budget => 0.1 });

  $self->create_campaign($ns, TEXT, "TextNet",
    { account_id => $ns->create(Account => {
        name => 'NetAdv',
        commission => 0.5,
        role_id => DB::Defaults::instance()->advertiser_role(),
        currency_id => $ns->create(Currency => { rate => 0.25 }) }),
      campaign_commission => 0.5,
      cpc => 0.01,
      ccg_budget => 0.01,
      ccg_daily_budget => 0.01 });

  $self->create_campaign($ns, DISPLAY, "CampaignLessBudget", # than request revenue
    { account_id => $advertiser,
      cpc => 120,
      campaign_budget => 100  });

  $self->create_campaign($ns, CT_TEXT, "CampaignEqualBudget",
    { account_id => $advertiser,
      cpc => 100,
      campaign_budget => 100 });

  $self->create_campaign($ns, TEXT, "CampaignMoreBudget",
    { account_id => $advertiser,
      cpc => 80,
      campaign_budget => 100 });

  $self->create_campaign($ns, DISPLAY, "CampaignUnlimBudget",
    { account_id => $advertiser,
      cpc => 9999,
      campaign_budget => 100  });

  $self->create_campaign($ns, DISPLAY, "CampaignZeroBudget",
    { account_id => $advertiser,
      cpm => 1000,
      campaign_budget => 0  });

  $self->create_campaign($ns, CT_TEXT, "CTTextBlankBudget",
    { account_id => $advertiser,
      cpc => 100,
      ccg_budget => 100,
      ccg_daily_budget => 1000 });

  $self->create_campaign($ns, TEXT, "KTTextBlankBudget",
    { account_id => $advertiser,
      cpc => 100,
      ccg_budget => 100,
      ccg_daily_budget => 1000 });
}

1;

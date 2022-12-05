package PageLoadsDailyStats;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_simple_case {
  my ($ns, $publisher, $name, $cpms, $tag_cpms) = @_;

  my $account = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $site = $ns->create(Site => {
    name => "Site".$name,
    account_id => $publisher});

  $ns->output("SITE".$name, $site);

  my $i = 1;
  foreach my $cpm (@$cpms)
  {
    my $keyword = make_autotest_name($ns, $name."_".$i);
    $ns->output("KEYWORD".$name."_".$i, $keyword, "keyword for adv request");

    my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel'.$name."-".$i,
      account_id => $account,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P")]));
    
    if ($cpm)
    {
      my $campaign = $ns->create(DisplayCampaign => {
        name => "Display".$name."-".$i,
        account_id => $account,
        channel_id => $channel,
        campaigncreativegroup_cpm => $cpm,
        site_links => [{ site_id => $site }] });

      $ns->output("CC".$name."_".$i, $campaign->{cc_id});
    }

    my $cpm_name = 'cpm' . $i;

    my $tag_cpm = 
      defined $tag_cpms? exists $tag_cpms->{$cpm_name}?
          $tag_cpms->{$cpm_name}: 0: 0;

    my $tag = $ns->create(PricedTag => {
      name => $name."-".$i,
      cpm => $tag_cpm,
      adjustment => 1.0,
      site_id => $site});
    $ns->output("TAG".$name."_".$i, $tag->tag_id());
    $i = $i + 1;
  }
  return $site;
}

sub create_country_case {
  my ($ns, $name, $country_code, $cpms, $tag_cpms) = @_;

  my $publisher = $ns->create(PubAccount => {
    name => "CountryPublisher".$name,
    country_code => $country_code});
  
  my $site = $ns->create(Site => {
    name => "Site".$name,
    account_id => $publisher});

  $ns->output("SITE".$name, $site);

  my $i = 1;
  foreach my $cpm (@$cpms)
  {
    my $keyword = make_autotest_name($ns, $name."_".$i);
    $ns->output("KEYWORD".$name."_".$i, $keyword);
        
    my $campaign = $ns->create(DisplayCampaign => {
      name => "Display".$name."-".$i,
      role_id => DB::Defaults::instance()->advertiser_role,
      country_code => $country_code,
      behavioralchannel_keyword_list => $keyword,
      campaigncreativegroup_cpm => $cpm,
      site_links => [{ site_id => $site }] });

    $ns->output("CC".$name."_".$i, $campaign->{cc_id});

    my $channel = $ns->create(
      DB::BehavioralChannel::BehavioralParameter->blank(
        channel_id => $campaign->{channel_id},
        trigger_type => "P"));

    my $cpm_name = 'cpm' . $i;

    my $tag_cpm = 
      defined $tag_cpms? exists $tag_cpms->{$cpm_name}?
          $tag_cpms->{$cpm_name}: 0: 0;

    my $tag = $ns->create(PricedTag => {
      name => $name."-".$i,
      cpm => $tag_cpm,
      adjustment => 1.0,
      site_id => $site});
    $ns->output("TAG".$name."_".$i, $tag->tag_id());
    $i = $i + 1;
  }
  return $site;
}

sub create_track_case {
  my ($ns, $publisher, $name, $cpas, $tag_cpms) = @_;

 
  my $account = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $site = $ns->create(Site => {
    name => "Site".$name,
    account_id => $publisher});

  $ns->output("SITE".$name, $site);

  my $i = 1;
  foreach my $cpa (@$cpas)
  {
    my $keyword = make_autotest_name($ns, $name."_".$i);
    $ns->output("KEYWORD".$name."_".$i, $keyword);

    my $channel = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel'.$name."-".$i,
      account_id => $account,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P")]));
    
    my $campaign = $ns->create(DisplayCampaign => {
      name => "Display".$name."-".$i,
      account_id => $account,
      channel_id => $channel,
      campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
      campaigncreativegroup_cpa => $cpa,
      campaigncreativegroup_ar => 0.01,
      site_links => [{ site_id => $site }] });

    $ns->output("CC".$name."_".$i, $campaign->{cc_id});

    my $cpm_name = 'cpm' . $i;

    my $tag_cpm = 
      defined $tag_cpms? exists $tag_cpms->{$cpm_name}?
          $tag_cpms->{$cpm_name}: 0: 0;

    my $tag = $ns->create(PricedTag => {
      name => $name."-".$i,
      cpm => $tag_cpm,
      adjustment => 1.0,
      site_id => $site});
    $ns->output("TAG".$name."_".$i, $tag->tag_id());
    $i = $i + 1;
  }
  return $site;
}

sub create_taginv_case {
  my ($ns, $publisher, $name, $tag_cpms) = @_;

  my $account = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $site = $ns->create(Site => {
    name => "Site".$name,
    account_id => $publisher});

  $ns->output("SITE".$name, $site);

  my $i = 1;
  foreach my $cpm (@$tag_cpms)
  {
    my $tag = $ns->create(PricedTag => {
      name => $name."-".$i,
      cpm => $cpm,
      adjustment => 1.0,
      site_id => $site});
    $ns->output("TAG".$name."_".$i++, $tag->tag_id());
  }

  my $tag_inv = $ns->create(Tags => {
    name => $name."-INV",
    flags => DB::Tags::INVENORY_ESTIMATION,
    site_id => $site});

  $ns->output("TAG$name" . "_INV", $tag_inv);
  
  return $site;
}

sub create_ron_case {
  my ($ns, $publisher, $name, $cpms, $tag_cpms) = @_;
 
  my $site = $ns->create(Site => {
    name => "Site".$name,
    account_id => $publisher});

  $ns->output("SITE".$name, $site);

  my $i = 1;
  foreach my $cpm (@$cpms)
  {    
    my $campaign = $ns->create(DisplayCampaign => {
      name => "Display".$name."-".$i,
      channel_id => undef,
      campaigncreativegroup_cpm => $cpm,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES |
        DB::Campaign::RON,
      site_links => [{ site_id => $site }] });

    $ns->output("CC".$name."_".$i, $campaign->{cc_id});

    my $cpm_name = 'cpm' . $i;

    my $tag_cpm = 
      defined $tag_cpms? exists $tag_cpms->{$cpm_name}?
          $tag_cpms->{$cpm_name}: 0: 0;

    my $tag = $ns->create(PricedTag => {
      name => $name."-".$i,
      cpm => $tag_cpm,
      adjustment => 1.0,
      site_id => $site});
    $ns->output("TAG".$name."_".$i, $tag->tag_id());
    $i = $i + 1;
  }
  return $site;
}

sub init {
  my ($self, $ns) = @_;

  my $publisher = $ns->create(PubAccount => {
    name => "Publisher",
  });

  create_simple_case(
    $ns, $publisher, "01", [1, 1],
    { cpm1 => 2000, cpm2 => 2000});
  create_simple_case($ns, $publisher, "02", [500, 1, 1],
    { cpm2 => 2000, cpm3 => 2000});
  create_simple_case(
    $ns, $publisher, "03", [500, 1, 1],
    { cpm2 => 2000, cpm3 => 2000});
  create_simple_case(
    $ns, $publisher, "04", [500, 1, 1],
    { cpm2 => 2000, cpm3 => 2000});
  create_simple_case(
    $ns, $publisher, "05", 
    [500, 1, 500, 500, 500, 1],
    { cpm2 => 2000, cpm6 => 2000});
  create_simple_case($ns, $publisher, "06", [500, 500]);
  create_simple_case(
    $ns, $publisher, "07", [500, 1],
    { cpm2 => 2000});
  create_simple_case(
    $ns, $publisher, "08", [500, 1],
    { cpm2 => 2000});
  create_simple_case($ns, $publisher, "09", [1, 1]);
  create_track_case($ns, $publisher, "10", [10]);
  create_simple_case(
    $ns, $publisher, "11a", [1, 1],
    { cpm1 => 2000, cpm2 => 2000});
  create_simple_case($ns, $publisher, "11b", [1, 500],
    { cpm1 => 2000 });
  create_country_case($ns, "12", 
     DB::Defaults::instance()->test_country_1->{country_code}, [1, 500]);
  create_taginv_case($ns, $publisher, "13", [0, 0]);
  create_simple_case($ns, $publisher, "14a", [500, 1],
    { cpm2 => 2000 });
  create_simple_case(
    $ns, $publisher, "14b", [1, 1],
    { cpm1 => 2000, cpm2 => 2000});
  create_ron_case($ns, $publisher, "15", [500, 1, 1],
    { cpm2 => 2000, cpm3 => 2000 });
}

1;

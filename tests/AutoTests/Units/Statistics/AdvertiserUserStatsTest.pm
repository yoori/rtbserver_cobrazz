
package AdvertiserUserStatsTest::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_account_types
{
  my ($self, $args) = @_;

  foreach my $a (@$args)
  {
    die $self->{ns_}->namespace . ".AccountType '$a->{name}' is redefined!" 
      if exists $self->{accounts_}->{$a->{name}};

    my $account_type = $self->{ns_}->create(AccountType => {
      name => $a->{name},
      account_role_id =>  
        defined $a->{role}?
           $a->{role}: DB::Defaults::instance()->advertiser_role,
      flags => defined $a->{flags}?
        $a->{flags}: 0 });    
    $self->{types_}->{$a->{name}} = $account_type;
  }
}

sub create_accounts
{
  my ($self, $args) = @_;

  foreach my $a (@$args)
  {
    die $self->{ns_}->namespace . ".Account '$a->{name}' is redefined!" 
      if exists $self->{accounts_}->{$a->{name}};

    die  $self->{ns_}->namespace . ".Account '$a->{agency}' is undefined!" 
      if defined $a->{agency} and not exists $self->{accounts_}->{$a->{agency}};

   die  $self->{ns_}->namespace . ".AccountType '$a->{type}' is undefined!" 
      if defined $a->{type} and not exists $self->{types_}->{$a->{type}};

    my %acc_args = %$a;

    $acc_args{agency_account_id} = 
      defined $a->{agency}?
        $self->{accounts_}->{$a->{agency}}: undef;
    delete $acc_args{agency};

    $acc_args{role_id} = 
      defined $a->{role}?
        $a->{role}: DB::Defaults::instance()->advertiser_role;
    delete $acc_args{role};

    $acc_args{timezone_id} = 
      $self->{ns_}->create( 
        TimeZone => { tzname => $a->{timezone} })
          if defined $a->{timezone};
    delete $acc_args{timezone};

    $acc_args{account_type_id} =
      defined $a->{type}?
        $self->{types_}->{$a->{type}}: defined $a->{agency}? 
          undef: DB::Defaults::instance()->advertiser_type;
    delete $acc_args{type};
                                                   
    my $account = $self->{ns_}->create(Account => \%acc_args);

    $self->{ns_}->output("Account/" . $a->{name},  $account);
    $self->{accounts_}->{$a->{name}} = $account;
  }
}

sub create_display_campaigns
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {
    die  $self->{ns_}->namespace . ".Account '$c->{account_id}' is undefined!" 
      if $c->{account_id} and not exists $self->{accounts_}->{$c->{account_id}};

    my $account = $self->{accounts_}->{$c->{account_id}};
    my $keyword = make_autotest_name($self->{ns_}, $c->{name});
    
    my $campaign = $self->{ns_}->create(DisplayCampaign => {
      name => $c->{name},
      account_id => $self->{accounts_}->{$c->{account_id}},
      size_id => $self->{size_},
      cpm => $c->{cpm},
      channel_id => 
        DB::BehavioralChannel->blank(
          name => 'Channel-' . $c->{name},
          account_id => $account,
          keyword_list => $keyword,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P',
              time_to => 
                defined $c->{channel_time_to}? 
                  $c->{channel_time_to}: 0) ]),
      site_links => $self->{sites_} });

    $self->{ns_}->output("Campaign/" . $c->{name}, $campaign->{campaign_id});
    $self->{ns_}->output("CCG/" . $c->{name}, $campaign->{ccg_id});
    $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
    $self->{ns_}->output("KWD/" . $c->{name}, $keyword);
    $self->{campaigns_}->{$c->{name}} = $campaign;
    $self->{ccgs_}->{$c->{name}} = $campaign->{ccg_id};
  }
}

sub create_text_campaigns
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {
    die  $self->{ns_}->namespace . ".Account '$c->{account_id}' is undefined!" 
      if $c->{account_id} and not exists $self->{accounts_}->{$c->{account_id}};

    die  $self->{ns_}->namespace . ".Campaign '$c->{campaign_id}' is undefined!" 
      if $c->{campaign_id} and not exists $self->{campaigns_}->{$c->{campaign_id}};

    my ($account, $head_campaign);
    $account = $self->{accounts_}->{$c->{account_id}} if defined $c->{account_id};
    $head_campaign = $self->{campaigns_}->{$c->{campaign_id}} if defined $c->{campaign_id};
    my $keyword = make_autotest_name($self->{ns_}, $c->{name});

    my $channel = $self->{ns_}->create(
      DB::BehavioralChannel->blank(
        name => 'Channel-' . $c->{name},
        account_id => $account,
        keyword_list => $keyword,
        channel_type => 'K',
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            time_to =>
              defined $c->{channel_time_to}? 
                $c->{channel_time_to}: 0)]));

    my @exclusion_options = ();

    my %cmp_args;
    $cmp_args{name} = $c->{name};
    $cmp_args{size_id} = $self->{size_};
    $cmp_args{account_id} = $account;
    $cmp_args{campaign_id} = $head_campaign->{campaign_id}
      if defined $head_campaign;
    $cmp_args{template_id} = DB::Defaults::instance()->text_template;
    $cmp_args{original_keyword} = $keyword;
    $cmp_args{max_cpc_bid} = $c->{cpc};
    $cmp_args{ccgkeyword_channel_id} = $channel->{channel_id};
    $cmp_args{site_links} = $self->{sites_};
    $cmp_args{creative_description1_value} = 
      $self->{exclusions_}->{$c->{category}} if defined $c->{category};

    my $campaign = $self->{ns_}->create(
      TextAdvertisingCampaign => \%cmp_args);

    if (defined $c->{exclusion})
    {
      $self->create_exclusion_cc(
        $c->{name} . 'A' , $campaign->{ccg_id}, 
        $account, DB::Defaults::instance()->text_template,
        $self->{exclusions_}->{$c->{exclusion}});
    }

    $self->{ns_}->output("Campaign/" . $c->{name}, $campaign->{campaign_id});
    $self->{ns_}->output("CCG/" . $c->{name}, $campaign->{ccg_id});
    $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
    $self->{ns_}->output("KWD/" . $c->{name}, $keyword);
    $self->{campaigns_}->{$c->{name}} = $campaign;
    $self->{ccgs_}->{$c->{name}} = $campaign->{ccg_id};
  }
}

sub create_keyword_campaigns
{
  my ($self, $args) = @_;

  foreach my $c (@$args)
  {

    die  $self->{ns_}->namespace . ".Account '$c->{account_id}' is undefined!" 
      if $c->{account_id} and not exists $self->{accounts_}->{$c->{account_id}};

    my $account = $self->{accounts_}->{$c->{account_id}};
    my $keyword = make_autotest_name($self->{ns_}, $c->{name});

    my $channel = $self->{ns_}->create(
      DB::BehavioralChannel->blank(
        name => 'Channel-' . $c->{name},
        account_id => $account,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => 'P',
            time_to =>
              defined $c->{channel_time_to}? 
                $c->{channel_time_to}: 0)]));

    my $campaign = $self->{ns_}->create(ChannelTargetedTACampaign => { 
      name => $c->{name},
      size_id => $self->{size_},
      account_id => $account,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_cpm => $c->{cpm},
      campaigncreativegroup_ctr => 0.01,
      channel_id => $channel,
      site_links => $self->{sites_} });

    $self->{ns_}->output("Campaign/" . $c->{name}, $campaign->{campaign_id});
    $self->{ns_}->output("CCG/" . $c->{name}, $campaign->{ccg_id});
    $self->{ns_}->output("CC/" . $c->{name}, $campaign->{cc_id});
    $self->{ns_}->output("KWD/" . $c->{name}, $keyword);
    $self->{ns_}->output("Channel/" . $c->{name}, $campaign->{channel_id});
    $self->{campaigns_}->{$c->{name}} = $campaign;
    $self->{ccgs_}->{$c->{name}} = $campaign->{ccg_id};
  }
}

sub create_exclusion_tag
{

  my ($self, $args) = @_;

  foreach my $name (@$args)
  {
    my $excluded_tag = make_autotest_name($self->{ns_}, $name);
    
    my $category = $self->{ns_}->create(CreativeCategory => {
      name => $excluded_tag,
      cct_id => 2 });
  
    my $publisher =
      $self->{ns_}->create(Publisher => { 
        name => 'Publisher-' . $name, 
        pubaccount_type_adv_exclusions => 'S',
        exclusions => [{ creative_category_id => $category }],
        size_id => $self->{size_}});

    $self->{ns_}->output($name, $publisher->{tag_id});                
    push @{$self->{sites_}}, { site_id => $publisher->{site_id} };
    $self->{exclusions_}->{$name} = $excluded_tag;
  }
}

sub create_exclusion_cc
{
  my ($self, $name, $ccg, $account, $template, $exclusion) = @_;

  my $creative = $self->{ns_}->create(Creative => {
    name => $name,
    account_id => $account,
    template_id => $template,
    description1_value => $exclusion,
    size_id => $self->{size_} });

  my $cc = $self->{ns_}->create(CampaignCreative => {
    ccg_id => $ccg,
    creative_id => $creative });
    
  $self->{ns_}->output("CC/" . $name, $cc);
}


sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) 
  {
    $self = bless {}, $self;  
  }
  
  $self->{ns_} = $ns->sub_namespace($prefix);

  $self->{accounts_} = {};
  $self->{campaigns_} = {};
  $self->{ccgs_} = {};
  $self->{creatives_} = {};
  $self->{types_} = {};
  $self->{exclusions_} = {};

  $self->{size_} = $self->{ns_}->create(CreativeSize => {
    name => 'Size',
    max_text_creatives => 2 });

  $self->{publisher_} =
    $self->{ns_}->create(Publisher =>
      { name => 'Publisher', 
        size_id => $self->{size_}});

  push @{$self->{sites_}}, 
    { site_id => $self->{publisher_}->{site_id} };

  foreach my $template (DB::Defaults::instance()->display_template(), 
                        DB::Defaults::instance()->text_template)
  {
    $self->{ns_}->create(TemplateFile => {
      template_id => $template,
      size_id => $self->{size_},
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X',
      flags => DB::TemplateFile::PIXEL_TRACKING,
      app_format_id => DB::Defaults::instance()->app_format_track});

    $self->{ns_}->create(TemplateFile => {
      template_id => $template,
      size_id => $self->{size_},
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X',
      app_format_id => DB::Defaults::instance()->app_format_no_track});
  }
    
  $self->{ns_}->output("Tag", $self->{publisher_}->{tag_id});

  if (defined $args->{free_tag})
  {
    my $publisher =
      $self->{ns_}->create(Publisher =>
        { name => $args->{free_tag}, 
          size_id => $self->{size_}});

    $self->{ns_}->output(
      $args->{free_tag}, $publisher->{tag_id});
  }

  my @creators = (
    [ "account_types", \&create_account_types ],
    [ "accounts", \&create_accounts ],
    [ "exclusions", \&create_exclusion_tag ],
    [ "display_campaigns", \&create_display_campaigns ],
    [ "keyword_campaigns", \&create_keyword_campaigns ],
    [ "text_campaigns", \&create_text_campaigns ] );

  foreach my $creator (@creators)
  {
    my ($dict, $fn) = @$creator;
    if (defined $args->{$dict})
    {
      $fn->($self, $args->{$dict})
    }
  }

  return $self;
}

1;


package AdvertiserUserStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub base
{
  my ($self, $ns) = @_;

  new AdvertiserUserStatsTest::Case(
    $ns, 'BASE',
    { accounts => [
        { name => "Adv1" },
        { name => "Adv2" } ],
      display_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 },
        { name => "2", account_id => "Adv2", cpm => 20 } ]} );
}

sub unique_users
{
  my ($self, $ns) = @_;
    new AdvertiserUserStatsTest::Case(
    $ns, 'UNIQUE',
    { 
      account_types => [ 
       { name => 'Type1', flags => DB::AccountType::ADVERTISER, role => DB::Defaults::instance()->agency_role },
       { name => 'Type2', flags => 0, role => DB::Defaults::instance()->agency_role }],
      accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role, type => 'Type1',
          prepaid_amount => 0, not_invoiced => 0, total_adv_amount => 0,
          text_adserving => 'M',},
        { name => 'Agency2', role => DB::Defaults::instance()->agency_role, type => 'Type2',
          prepaid_amount => 1000000, not_invoiced => 0.0002, 
          total_adv_amount => 0.0002},
        { name => 'Adv1', agency => 'Agency1', 
          prepaid_amount => 1000000, not_invoiced => 0.00072, 
          total_adv_amount => 0.00072},
        { name => 'Adv2', agency => 'Agency2', 
          prepaid_amount => 0, not_invoiced => 0, total_adv_amount => 0},
        { name => 'Adv3', agency => 'Agency2', 
          prepaid_amount => 0, not_invoiced => 0, total_adv_amount => 0},
        { name => "Adv4" } ],
      exclusions => ["TagEx1", "TagEx2"],
      display_campaigns => [
        { name => "Display1", account_id => "Adv1", cpm => 1 },
        { name => "Display2", account_id => "Adv2", cpm => 100 },
        { name => "Display4", account_id => "Adv4", cpm => 1000 } ],
      keyword_campaigns => [
        { name => "Keyword1", account_id => "Adv1", cpm => 2000},
        { name => "Keyword4", account_id => "Adv4", cpm => 10} ],
      text_campaigns => [ 
        { name => "Text1", cpc => 0.1, campaign_id => "Keyword1", 
          account_id => "Adv1", category => "TagEx1", 
          exclusion => "TagEx2" },
        { name => "Text3", cpc => 10, account_id => "Adv3" }] });
}

sub last_usage
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'LASTUSAGE',
    { free_tag => "FreeTag",
      accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role},
        { name => 'Agency2', role => DB::Defaults::instance()->agency_role},
        { name => 'Adv1', agency => 'Agency1'},
        { name => "Adv2", agency => 'Agency2' } ],
      display_campaigns => [
        { name => "Display1", account_id => "Adv1", cpm => 10 },
        { name => "Display2", account_id => "Adv2", cpm => 1000 } ],
      keyword_campaigns => [
        { name => "Text1", account_id => "Adv1", cpm => 10 } ] });
}

sub timezone
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'TZ',
    { accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role, timezone => 'Asia/Tokyo'},
        { name => 'Agency2', role => DB::Defaults::instance()->agency_role, timezone => 'America/Sao_Paulo'},
        { name => 'Adv1', agency => 'Agency1', timezone => 'Asia/Tokyo'},
        { name => "Adv2", agency => 'Agency2', timezone => 'America/Sao_Paulo' } ],
      keyword_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 } ], 
      display_campaigns => [
        { name => "2", account_id => "Adv2", cpm => 10 } ] });
}

sub async
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'ASYNC',
    { accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role},
        { name => 'Adv1', agency => 'Agency1'},
        { name => "Adv2", agency => 'Agency1'},
        { name => "Adv3"} ],
      display_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 },
        { name => "3", account_id => "Adv3", cpm => 10 } ],
      keyword_campaigns => [
        { name => "2", account_id => "Adv2", cpm => 10 } ] });
}

sub big_date_difference
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'BIGDIFF',
    { accounts => [
        { name => 'Adv1'} ],
      display_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 } ] });
}

sub colo
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'COLO',
    { accounts => [
        { name => 'Adv1'} ],
      display_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 } ] });
}

sub temporary_user
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'TEMP',
    { accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role},
        { name => 'Adv1', agency => 'Agency1'},
        { name => "Adv2", agency => 'Agency1'} ],
      keyword_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10, channel_time_to => 3*24*60*60 } ],
      display_campaigns => [
        { name => "2", account_id => "Adv2", cpm => 10 } ] });
}

sub optout_users
{
  my ($self, $ns) = @_;
  
  new AdvertiserUserStatsTest::Case(
    $ns, 'OPTOUT',
    { accounts => [
        { name => 'Agency1', role => DB::Defaults::instance()->agency_role},
        { name => 'Agency2', role => DB::Defaults::instance()->agency_role},
        { name => 'Adv1', agency => 'Agency1'},
        { name => "Adv2", agency => 'Agency2'} ],
      keyword_campaigns => [
        { name => "1", account_id => "Adv1", cpm => 10 } ],
      display_campaigns => [
        { name => "2", account_id => "Adv2", cpm => 10 } ] });
}

sub init {
  my ($self, $ns) = @_;

  $self->base($ns);
  $self->unique_users($ns);
  $self->last_usage($ns);
  $self->timezone($ns);
  $self->async($ns);
  $self->big_date_difference($ns);
  $self->colo($ns);
  $self->temporary_user($ns);
  $self->optout_users($ns);

  $ns->output("DEFAULT_COLO", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("REMOTE_COLO", DB::Defaults::instance()->remote_isp->{colo_id});
  $ns->output("ADS_COLO", DB::Defaults::instance()->ads_isp->{colo_id});
}
1;

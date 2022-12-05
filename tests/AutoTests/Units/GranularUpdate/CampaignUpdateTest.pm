package CampaignUpdateTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub add_ext_channels
{
  my ($self, $args) = @_;

  while (my ($name, $channel) = each %$args) 
  {
    die "$self->{case_name}: Channel '$name' already exists!"
      if exists $self->{channels}->{$name};
   
    $self->{ns}->output("$name/ID", $channel );
    $self->{ns}->output("$name/EXP", $channel->{expression})
      if defined $channel->{expression};
    $self->{channels}->{$name} = $channel;  
  }
}

sub create_channels
{
  my ($self, $args) = @_;
  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Channel '$arg->{name}' already exists!"
      if exists $self->{channels}->{$arg->{name}};

    $arg->{account_id} = $self->{ns}->create(Account => {
      name => $arg->{name},
      role_id => DB::Defaults::instance()->advertiser_role }) if not exists $arg->{account_id};

    delete $arg->{account_id} if not defined $arg->{account_id};

    my $channel_obj;

    if (defined $arg->{expression})
    {
      foreach my $channel (grep {m/.+/} split(/\W+/, $arg->{expression}))
      {
        die "$self->{case_name}: channel '$channel' not defined!"
          unless exists $self->{channels}->{$channel};
        my $channel_id = $self->{channels}->{$channel}->channel_id();
        $arg->{expression} =~ s/$channel/$channel_id/;
      }
      
      $channel_obj =  
        defined $arg->{account_id}?
          $self->{ns}->create(DB::ExpressionChannel->blank(%$arg)):
            $self->{ns}->create(DB::TargetingChannel->blank(%$arg));
    }
    else
    {
      $arg->{keyword_list} = make_autotest_name($self->{ns}, "$arg->{name}-kwd")
        unless defined $arg->{keyword_list};

      foreach my $bp (@{ $arg->{behavioral_parameters} })
      {
        $bp = DB::BehavioralChannel::BehavioralParameter->blank(%$bp);
      }
      $channel_obj = $self->{ns}->create(DB::BehavioralChannel->blank(%$arg));
    }

    $self->{channels}->{$arg->{name}} = $channel_obj;

    $self->{ns}->output("$arg->{name}/ID", $channel_obj->channel_id());
  }
}

sub create_campaigns
{
  my ($self, $args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Campaign with '$arg->{name}' name already exists!"
      if exists $self->{campaigns}->{$arg->{name}};

    if (defined $arg->{channel} )
    {
      die "$self->{case_name}: Link campaign with channel fails: " .
          "'$arg->{channel}' is not defined!"
        unless exists $self->{channels}->{$arg->{channel}};
      $arg->{channel_id} = $self->{channels}->{$arg->{channel}};

      if (defined $arg->{targeting_channel})
      {
        die "$self->{case_name}: Link campaign with channel fails: " .
            "'$arg->{targeting_channel}' is not defined!"
            unless exists $self->{channels}->{$arg->{targeting_channel}};
        $arg->{targeting_channel_id} = 
           $self->{channels}->{$arg->{targeting_channel}}->channel_id();
      }
    }
    else
    {
      $arg->{channel_id} = undef;
      $arg->{targeting_channel_id} = undef;
      $arg->{channel_target} = 'U';
    }

    $arg->{campaigncreativegroup_cpc} = 10;
    delete $arg->{campaign};
    delete $arg->{channel};
    delete $arg->{targeting_channel};

    my $campaign = $self->{ns}->create(DisplayCampaign => $arg);

    $self->{ns}->output("$arg->{name}/CCG", $campaign->{ccg_id});
    $self->{ns}->output("$arg->{name}/CAMPAIGN", $campaign->{campaign_id});
    $self->{ns}->output("$arg->{name}/ACCOUNT", $campaign->{account_id});
  }
}

sub create
{
  my $self = shift;
  my (%entities) = @_;

  my %index;
  foreach my $entity (keys %entities)
  {
    $index{$entity} = 1 unless defined $index{$entity};
    my $object = $self->{ns}->create($entity => $entities{$entity});
    $self->{ns}->output("$entity#$index{$entity}", $object);
  }
}

sub create_unique_name
{
  my ($self, $name) = @_;
  die "$self->{case_name}: Name '$name' is already defined!"
    if exists $self->{unique_names}->{$name};
  my $unique_name = make_autotest_name($self->{ns}, $name);
  $self->{unique_names}->{$name} = $unique_name;
  $self->{ns}->output($name, $unique_name);
}

sub new
{
  my $self = shift;
  my ($ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{ns} = $ns->sub_namespace($case_name);
  $self->{case_name} = $case_name;

  return $self;
}

1;

package CampaignUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub add_campaign_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "AddCampaign");
  $test_case->create_unique_name("NewName");

  $test_case->create(Creative =>
    { name => "creative",
      account_id => $self->{account} });
}

sub currency_change_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "CurrencyChange");
  $test_case->create_campaigns([
    { name => "Campaign" } ] );
  $test_case->create(Currency => { rate => 3 });
}

sub update_channel_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "UpdateChannel");

  $test_case->add_ext_channels(
    { 'GEO' => DB::Defaults::instance()->geo_channel,
      'Device' => DB::Defaults::instance()->device_channel });

  $test_case->create_channels([
    { name => "BChannel1",
      account_id => $self->{account},
      behavioral_parameters => [{ trigger_type => 'P' }] },

    { name => "BChannel2",
      account_id => $self->{account},
      behavioral_parameters => [{ trigger_type => 'P' }] },

    { name => "B1forExpr",
      account_id => $self->{account},
      behavioral_parameters => [{ trigger_type => 'P' }] },

    { name => "B2forExpr",
      account_id => $self->{account},
      behavioral_parameters => [{ trigger_type => 'P' }] },

    { name => "EChannel",
      account_id => $self->{account},
      expression => "B1forExpr|B2forExpr" },

    { name => "TargetingFull",
      account_id => undef,
      expression => "BChannel1&GEO&Device"},

    { name => "TargetingUntarget",
      account_id => undef,
      expression => "GEO&Device"} ]);

  $test_case->create_campaigns([
    { name => "ChangeChannel",
      channel => "BChannel1" },

    { name => "AddChannel" },

    { name => "NullChannel",
      channel => "BChannel1"},

    { name => "ExprChannel",
      channel => "EChannel" },

    { name => "Untargeting",
      channel => "EChannel",
      targeting_channel => "TargetingFull" } ] );
}

sub change_status_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "ChangeStatus");
  $test_case->create_campaigns([
    { name => "CampaignA" },

    { name => "CampaignD",
      campaign_status => 'D' } ] );
}

sub change_date_interval_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "ChangeDateInterval");
  $test_case->create_campaigns([
    { name => "CMPOutside",
      campaign_date_start =>
        DB::Entity::Oracle->sql("to_date('1970-01-01', 'yyyy-mm-dd')"),
      campaign_date_end =>
        DB::Entity::Oracle->sql("to_date('1970-01-02', 'yyyy-mm-dd')") },

    { name => "CCGOutside",
      campaigncreativegroup_date_start =>
        DB::Entity::Oracle->sql("to_date('1970-01-01', 'yyyy-mm-dd')"),
      campaigncreativegroup_date_end =>
        DB::Entity::Oracle->sql("to_date('1970-01-02', 'yyyy-mm-dd')") },

    { name => "CMPExpiration" },

    { name => "CCGExpiration" } ] );
}

sub change_max_pub_share_case_
{
  my ($self, $ns) = @_;
  my $test_case = new CampaignUpdateTest::TestCase($ns, "ChangeMaxPubShare");
  $test_case->create_campaigns([
    { name => "Campaign",
      max_pub_share => 1.0 } ] );
}

sub init
{
  my ($self, $ns) = @_;

  $self->{account} = $ns->create(Account => {
    name => "Advertiser",
    role_id => DB::Defaults::instance()->advertiser_role });

  $self->add_campaign_case_($ns);
  $self->currency_change_case_($ns);
  $self->update_channel_case_($ns);
  $self->change_status_case_($ns);
  $self->change_date_interval_case_($ns);
  $self->change_max_pub_share_case_($ns);

  $ns->output('DefaultCurrency', DB::Defaults::instance()->currency());
  $ns->output('Advertiser', $self->{account});
  $ns->output('DefaultUser', DB::Defaults::instance()->user);
}

1;

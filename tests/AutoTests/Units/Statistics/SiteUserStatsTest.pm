package SiteUserStatsTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_publishers_
{
  my ($self, $args) = @_;
  
  foreach my $p (@$args)
  {
    my %pub_args;
    $pub_args{name} = $p->{name};
    $pub_args{pricedtag_country_code} = $p->{country};

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
    $self->{ns_}->output("PUBLISHER/" . $p->{name}, $publisher->{account_id});
    $self->{ns_}->output("SITE/" . $p->{name}, $publisher->{site_id});
    $self->{ns_}->output("TID/" . $p->{name}, $publisher->{tag_id});
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name, $args) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{ns_} = $ns->sub_namespace($case_name);
  $self->{publishers_} = {};
  $self->create_publishers_($args);

  return $self;
}

1;

package SiteUserStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub unique_users
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, "UNIQUE",
    [{name => '1'},
     {name => '2', site => '1'},
     {name => '3'}]);
}

sub last_usage
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, "LASTUSAGE",
    [{name => '1'},
     {name => '2'}]);
}

sub timezone
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, "TZ",
    [{name => '1'}]);
}

sub colo
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, "COLO",
    [{name => '1'}]);
}

sub async
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, "ASYNC",
    [{name => '1'},
     {name => '2'}]);
}

sub temporary_user
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, 'TEMP',
    [{name => '1'}]);
}

sub optout_users
{
  my ($self, $ns) = @_;
  
  new SiteUserStatsTest::TestCase(
    $ns, 'OPTOUT',
    [{name => '1'}]);
}

sub init {
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "kwd");

  my $advertiser = 
    $ns->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel",
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to =>  3*24*60*60)]));


  $ns->output("COLO", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("ADS_COLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("TZ_COLO", DB::Defaults::instance()->remote_isp->{colo_id});
  $ns->output("TZ", 
    DB::Defaults::instance()->remote_isp->{Account}->{timezone_id}->{tzname});
  $ns->output("CHANNEL", $channel);
  $ns->output("KWD", $keyword);

  $self->unique_users($ns);
  $self->last_usage($ns);
  $self->timezone($ns);
  $self->colo($ns);
  $self->async($ns);
  $self->temporary_user($ns);
  $self->optout_users($ns);

}
1;

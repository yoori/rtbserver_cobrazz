
package CGEndDateTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use POSIX qw(strftime);

use constant DEFAULT_DATE_END => undef;
use constant DEFAULT_DATE_START => DB::Entity::PQ->sql('current_date - 1');

sub to_date
{
  my ($time) = @_;
  return 
    DB::Entity::Oracle->sql("to_date('" .
       strftime("%d-%m-%Y:%H-%M-00", gmtime($time)) . "', 'DD-MM-YYYY:HH24-MI-SS')" );
}

sub create
{
  my ($self, $campaigns) = @_;
  
  my $publisher = 
    $self->{ns}->create(Publisher => { name => 'Pub'});

  my $advertiser = $self->{ns}->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role,
    account_type_id => DB::Defaults::instance()->advertiser_type});

  my $i = 0;

  foreach my $args (@$campaigns)
  {
    my $keyword = make_autotest_name($self->{ns}, "Keyword" . ++$i);
    my $campaign = $self->{ns}->create(DisplayCampaign => {
      name => 'Campaign' . $i,
      account_id => $advertiser,
      campaigncreativegroup_cpm => 
        defined $args->{cpm}? 
          $args->{cpm}: 1, 
      campaigncreativegroup_date_end => 
        defined $args->{ccg_date_end}?
          to_date($args->{ccg_date_end}): DEFAULT_DATE_END,
      campaigncreativegroup_date_start => 
         defined $args->{ccg_date_start}?
           to_date($args->{ccg_date_start}): DEFAULT_DATE_START,
      channel_id => DB::BehavioralChannel->blank(
        name => 'Channel' . $i,
        account_id => $advertiser,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      site_links => 
        [ { site_id =>  $publisher->{site_id} }] });

    $self->{ns}->output("KWD" . $i, $keyword);  
    $self->{ns}->output("CCG" . $i, $campaign->{ccg_id});
    $self->{ns}->output("CC" . $i, $campaign->{cc_id});
    if (defined  $campaign->{CampaignCreativeGroup}->{date_end})
    {
      $self->{ns}->output(
        "CGENDDATE" . $i, 
        $campaign->{CampaignCreativeGroup}->{date_end});
    }
    $self->{ns}->output(
      "CGSTARTDATE" . $i, 
      $campaign->{CampaignCreativeGroup}->{date_start});
  }
  $self->{ns}->output("TAG", $publisher->{tag_id});
}

sub new
{
  my $self = shift;
  my ($ns, $case_name, $args) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{case_name} = $case_name;
  $self->{ns} = $ns->sub_namespace($case_name);

  $self->create($args);

  return $self;
}

1;


package CGEndDateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub gmt_case
{
  my ($self, $ns, $now) = @_;
  
  new CGEndDateTest::TestCase(
     $ns, "GMT",
     [ { ccg_date_end => $now + 10*24*60*60,
         ccg_date_start => $now - 24*60*60 },
       { ccg_date_start => $now + 10*24*60*60 },
       { ccg_date_end => $now - 24*60*60},
       { ccg_date_start => $now - 24*60*60},
       { ccg_date_start => $now - 2*60*60,
         ccg_date_end => $now - 60*60 },
       { ccg_date_start => $now - 60*60,
         ccg_date_end => $now + 6*60*60 },
       { ccg_date_start => $now + 5*60*60,
         ccg_date_end => $now + 6*60*60 } ]);  
}

sub init
{
  my ($self, $ns) = @_;

  my $now = time;
  
  $self->gmt_case($ns, $now);
}

1;

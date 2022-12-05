
# CCG schedule
package DB::CCGSchedule;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  schedule_id => DB::Entity::Type::sequence(),
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup'),
  time_from => DB::Entity::Type::int(),
  time_to =>  DB::Entity::Type::int()
};

1;

# CCG site
package DB::CCGSite;

use warnings;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1),
  site_id => DB::Entity::Type::link('DB::Site', unique => 1)
};

1;

# CCG colocation 
package DB::CCGColocation;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  colo_id => DB::Entity::Type::link('DB:Colocation', unique => 1 ),
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1 )
};

1;

# CCG CTR colocation 
package DB::CCGCTROverride;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1 ),
  ctr => DB::Entity::Type::float()
};

1;

# CCG AR colocation 
package DB::CCGAROverride;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1 ),
  ar => DB::Entity::Type::float()
};

1;

# CCG rate       
package DB::CCGRate;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant DEFAULT_CPM => 50;

use constant STRUCT => 
{
  ccg_rate_id => DB::Entity::Type::sequence(),
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup'),
  rate_type => DB::Entity::Type::enum(['CPM', 'CPC', 'CPA']),
  cpm => DB::Entity::Type::float(nullable => 1),
  cpc => DB::Entity::Type::float(nullable => 1),
  cpa => DB::Entity::Type::float(nullable => 1),
  effective_date => DB::Entity::Type::pq_date('now()')
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  if(!exists($args->{rate_type}))
  {
    my $inited_cost_num = (defined $args->{cpm}? 1 : 0) +
      (defined $args->{cpc}? 1 : 0) +
      (defined $args->{cpa}? 1 : 0);
    if($inited_cost_num)
    {
      $args->{rate_type} =
        ($args->{cpm}? 'CPM' :
          (defined $args->{cpc} && !$args->{cpa}? 'CPC' :
            (defined $args->{cpa}? 'CPA': 'CPM')));
    }
    else
    {
      $args->{rate_type} = 'CPM';
      $args->{cpm} = DEFAULT_CPM;
    }
  }

  $args->{cpm} = $args->{cpm} ? $args->{cpm} : 0;
  $args->{cpc} = $args->{cpc} ? $args->{cpc} : 0;
  $args->{cpa} = $args->{cpa} ? $args->{cpa} : 0;
 
}

1;

# CCG GEO channels
package DB::CCGGEOChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup number', unique => 1),
  geo_channel_id => DB::Entity::Type::link('DB::GEOChannel number', unique => 1)
};

1;

# CCG actions
package DB::CCGAction;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1),
  action_id => DB::Entity::Type::link('DB::Action', unique => 1 )
};

1;

# Campaign creative group
package DB::CampaignCreativeGroup;

use warnings;
use strict;
use DB::Entity::PQ;

# Flags
use constant ONLY_SPECIFIC_SITES   =>  0x200;
use constant INVENTORY_ESTIMATION  =>  0x800;

# Defaults
use constant DEFAULT_BUDGET => 100_000_000;
use constant DISPLAY_STATUS_CHANNEL_NEED_ATTENTION =>
  "Not Live - Channel Target Needs Attention";

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  ccg_rate_id => DB::Entity::Type::link('DB::CCGRate'),
  campaign_id => DB::Entity::Type::link('DB::Campaign'),
  channel_id => DB::Entity::Type::link('DB::CMPChannelBase', nullable => 1),
  flags => DB::Entity::Type::int(default => 0),
  status => DB::Entity::Type::status(),
  freq_cap_id => DB::Entity::Type::link('DB::FreqCap', nullable => 1),
  qa_status => DB::Entity::Type::qa_status(),
  date_start => DB::Entity::Type::pq_date("current_date - 1"),
  date_end => DB::Entity::Type::pq_date(undef, nullable => 1),
  cur_date => DB::Entity::Type::pq_date('now()'),
  ccg_type => DB::Entity::Type::enum(['D', 'T']),
  budget =>  DB::Entity::Type::float(nullable => 1, default => DEFAULT_BUDGET),
  tgt_type =>  DB::Entity::Type::enum(['C', 'K']),
  display_status_id => 
    DB::Entity::Type::display_status('CampaignCreativeGroup'),
  country_code => DB::Entity::Type::country(),
  channel_target => DB::Entity::Type::enum(['N', 'U', 'T']),
  delivery_pacing =>  DB::Entity::Type::enum(['D', 'F', 'U']), 
  daily_budget => DB::Entity::Type::float(nullable => 1), 
  optin_status_targeting => 
    DB::Entity::Type::enum(
      ['YYY', 'YYN', 'YNY', 'YNN', 'NYY', 'NYN', 'NNY'], 
      nullable => 1),
  targeting_channel_id => DB::Entity::Type::link('DB::TargetingChannel', nullable => 1),
  min_uid_age =>  DB::Entity::Type::int(default => 0),
  ctr_reset_id => 0,

  # Private fields
  
  # CCGRate
  cpm => DB::Entity::Type::float(private => 1),
  cpc => DB::Entity::Type::float(private => 1),
  cpa => DB::Entity::Type::float(private => 1),
  rate_type => DB::Entity::Type::enum(['CPM', 'CPC', 'CPA'], private => 1),

  # CCGGEOChannel
  geo_channels => DB::Entity::Type::link_array('DB::GEOChannel', private => 1),

  # CCGColocation
  colocations => DB::Entity::Type::link_array('DB::Colocation', private => 1),

  # CCGCTROverride
  ctr => DB::Entity::Type::float(private => 1),

  # CCGAROverride
  ar => DB::Entity::Type::float(private => 1),

  # CCGAction
  action_id => DB::Entity::Type::link_array('DB::Action', private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  if (!defined $args->{tgt_type} && 
      (defined $args->{ccg_type} && 
        $args->{ccg_type} ne 'D'))
  {
    print $args->{name} . " " .  $args->{ccg_type} . "\n";
  }

  $args->{tgt_type} = 'K' 
    if !defined $args->{tgt_type} && 
      defined $args->{ccg_type} && 
        $args->{ccg_type} ne 'D';

  unless (defined $args->{channel_target})
  {
    if (exists $args->{channel_id} and defined $args->{channel_id})
    {
      $args->{channel_target} = 'T'; 
    }
    else
    {
      $args->{channel_target} = 'U'; 
    }
  }
}

sub precreate_
{
  my ($self, $ns) = @_;

  unless (exists $self->{targeting_channel_id})
  {
    my $channel_id = ref $self->{channel_id}?
      $self->{channel_id}->{channel_id}: $self->{channel_id};
    my $expression = defined $channel_id? "$channel_id": "" ;

    # Since REQ-3191 geo_channel targeting required (by country as minimum)
    my @geo_channels;
    if (defined $self->{geo_channels})
    {
      @geo_channels = ref($self->{geo_channels}) eq 'ARRAY'
        ? @{$self->{geo_channels}}
        : ($self->{geo_channels});
    }
    else
    {
      @geo_channels =
        $ns->pq_dbh->selectrow_array(qq[
          SELECT channel_id
          FROM CHANNEL
          WHERE channel_type = ? AND geo_type = ? AND country_code = ?],
          undef, 'G', 'CNTRY',
          ref $self->{country_code} eq 'CODE'
            ? $self->{country_code}->()
            : ref $self->{country_code}
              ? $self->{country_code}->{country_code}
              : $self->{country_code});
    }
    if (@geo_channels)
    {
      my $geo_expression = join('|',  map(ref $_? $_->{channel_id}: $_, @geo_channels));
      $expression .= (defined $channel_id? "&($geo_expression)": $geo_expression);
    }

    # Get campaign excluded channels
    my @campaign_excluded_channels = $ns->pq_dbh->selectrow_array(qq[
      SELECT channel_id
      FROM CampaignExcludedChannel
      WHERE campaign_id = ?], undef, ref $self->{campaign_id}
        ? $self->{campaign_id}->campaign_id()
        : $self->{campaign_id});

    $expression .= "^(" . join('|', @campaign_excluded_channels) . ")"
      if @campaign_excluded_channels;

    if ($expression)
    {
      my $targeting = 
        $ns->create(
          DB::TargetingChannel->blank(
            name => $self->{__name} . '-T-' . $expression,
            expression => $expression));
     $self->{targeting_channel_id} = $targeting->{channel_id};
    }
  }
}

sub postcreate_
{
  my ($self, $ns) = @_;

  if (exists $self->{action_id} and defined $self->{action_id})
  {
    my @actions = ref($self->{action_id}) eq 'ARRAY'?
        @{$self->{action_id}}: ($self->{action_id});
    foreach my $action (@actions)
    {
      $ns->create(DB::CCGAction->blank(
        action_id => $action,
        ccg_id => $self->{ccg_id} ));
    }
  }

  unless (defined $self->{ccg_rate_id})
  {

    my %rate = (
      ccg_id => $self->{ccg_id},
      cpm => $self->{cpm},
      cpc => $self->{cpc},
      cpa => $self->{cpa} );
    
    $rate{rate_type} = $self->{rate_type} if
      defined $self->{rate_type};  

    $self->__update($ns, {
      ccg_rate_id => DB::CCGRate->blank(%rate) });
  }

  if (defined $self->{ctr})
  {
    $ns->create(DB::CCGCTROverride->blank(
       ccg_id => $self->{ccg_id},
       ctr => $self->{ctr} ));
  }

  if (defined $self->{ar})
  {
    $ns->create(DB::CCGAROverride->blank(
       ccg_id => $self->{ccg_id},
       ar => $self->{ar} ));
  }

  if (defined $self->{geo_channels})
  {
    my @channels = ref($self->{geo_channels}) eq 'ARRAY'?
        @{$self->{geo_channels}}: ($self->{geo_channels});
    foreach my $channel (@channels)
    {
      $ns->create(DB::CCGGEOChannel->blank(
         ccg_id  => $self->{ccg_id},
         geo_channel_id => 
            ref $channel? $channel->{channel_id}: $channel));
    }
  }

 if (defined $self->{colocations})
 {
   my @colocations = ref($self->{colocations}) eq 'ARRAY'?
       @{$self->{colocations}}: ($self->{colocations});
   foreach my $colo (@colocations)
   {
     $ns->create(DB::CCGColocation->blank(
        ccg_id  => $self->{ccg_id},
        colo_id => $colo ));
    }
  }
}

1;

# Keyword text CCG
package DB::CampaignCreativeGroup::Keyword;

use warnings;
use strict;

our @ISA = qw(DB::CampaignCreativeGroup);

sub _table {
    'CampaignCreativeGroup'
}

use constant STRUCT => 
{
  %{ DB::CampaignCreativeGroup->STRUCT },

  channel_id => undef,    
  ccg_type => 'T',
  tgt_type => 'K'
};

1;

# Channel text CCG
package DB::CampaignCreativeGroup::Channel;

use warnings;
use strict;

our @ISA = qw(DB::CampaignCreativeGroup);

sub _table {
    'CampaignCreativeGroup'
}

use constant STRUCT => 
{
 %{ DB::CampaignCreativeGroup->STRUCT },
    
 ccg_type => 'T',
 tgt_type => 'C'
};

1;

# Display text CCG
package DB::CampaignCreativeGroup::Display;

use warnings;
use strict;

our @ISA = qw(DB::CampaignCreativeGroup);

sub _table {
    'CampaignCreativeGroup'
}

use constant STRUCT => 
{
 %{ DB::CampaignCreativeGroup->STRUCT },
    
 ccg_type => 'D',
 tgt_type => 'C'
};

1;

# CCGKeywordCTROverride (used to fix keyword CTR)
package DB::CCGKeywordCTROverride;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  ccg_keyword_id => DB::Entity::Type::link('DB::CCGKeyword', unique => 1),
  ctr => DB::Entity::Type::float(),
  tow => DB::Entity::Type::float()
};

1;

package DB::CCGKeyword;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

# Defaults
use constant DEFAULT_TOW => '1';
use constant DEFAULT_CTR => 0.01;

use constant STRUCT => 
{
  ccg_keyword_id => DB::Entity::Type::sequence(),
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1),
  channel_id => DB::Entity::Type::link('DB::BehavioralChannel', unique => 1),
  original_keyword => DB::Entity::Type::string(),
  max_cpc_bid => DB::Entity::Type::float(nullable => 1),
  click_url => DB::Entity::Type::string(nullable => 1),
  status => DB::Entity::Type::status(),
  trigger_type => DB::Entity::Type::enum(['S', 'P']),
    
  # Private
  # CCGKeywordCTROverride
  ctr => DB::Entity::Type::float(private => 1, default => DEFAULT_CTR),
  tow => DB::Entity::Type::float(private => 1),

  # Name for BehavioralChannel
  name => DB::Entity::Type::string(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  unless (defined $args->{channel_id})
  {
    my ($account) = 
      $ns->pq_dbh->selectrow_array(
        qq[SELECT account_id FROM Campaign where campaign_id in 
           (SELECT campaign_id FROM CampaignCreativeGroup 
            WHERE ccg_id = ?)], undef, $args->{ccg_id});

    $args->{channel_id} =  
      $ns->create(
        DB::BehavioralChannel->blank(
          name => $args->{name},
          channel_type => 'K',
          keyword_list => $args->{original_keyword},
          account_id => $account,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P',
              minimum_visits => 2,
              time_from => 0,
              time_to => 3600) ]));
  }
}

sub postcreate_
{
  my ($self, $ns) = @_;
  
  if (defined $self->{ctr} || defined $self->{tow})
  {
    $ns->create(DB::CCGKeywordCTROverride->blank(
       ccg_keyword_id => $self->{ccg_keyword_id},
       tow => defined $self->{tow}? $self->{tow}: DEFAULT_TOW,
       ctr => defined $self->{ctr}? $self->{ctr}: DEFAULT_CTR));
  }
}

# CCG creative
package DB::CampaignCreative;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  cc_id => DB::Entity::Type::sequence(),
  ccg_id => DB::Entity::Type::link('DB::CampaignCreativeGroup', unique => 1),
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1),
  weight => DB::Entity::Type::int(),
  freq_cap_id => DB::Entity::Type::link('DB::FreqCap', nullable => 1),
  status => DB::Entity::Type::status(),,
  display_status_id => DB::Entity::Type::display_status('CampaignCreative'),
  set_number => 1
};

1;

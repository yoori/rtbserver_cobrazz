package DB::BehavioralChannel::BehavioralParameter;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'BehavioralParameters'
}

use constant STRUCT => 
{
  channel_id => DB::Entity::Type::int(unique => 1, nullable => 1),
  behav_params_id => DB::Entity::Type::sequence(),
  minimum_visits => DB::Entity::Type::int(unique => 1, default => 1),
  time_from => DB::Entity::Type::int(unique => 1, default => 0),
  time_to => DB::Entity::Type::int(unique => 1, default => 0),
  trigger_type => DB::Entity::Type::enum(['P', 'S', 'U', 'R'], unique => 1),
  weight => DB::Entity::Type::int(unique => 1, default => 1),
  behav_params_list_id => DB::Entity::Type::int(unique => 1, nullable => 1)
};

sub __init_args
{
   my ($self, $ns, $args) = @_;

   $self->SUPER::__init_args($ns, $args);
   
   # ADDB-3430 don't set channel_id when behav_params_list_id used
   if (defined $args->{behav_params_list_id})
   {
     $args->{channel_id} = undef;
     delete $args->{channel_id};
   }
}

1;

package DB::ChannelCategory;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  channel_id => DB::Entity::Type::int(unique => 1),
  category_channel_id => DB::Entity::Type::int(unique => 1)
};

1;


package DB::BehavioralChannel::Rate;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'ChannelRate'
}


use constant STRUCT => 
{
  channel_rate_id => DB::Entity::Type::sequence(),
  channel_id => DB::Entity::Type::int(unique => 1, nullable => 1), # unsettable
  cpm => DB::Entity::Type::float(nullable => 1, default => undef),
  cpc => DB::Entity::Type::float(nullable => 1, default => undef),
  rate_type => DB::Entity::Type::enum(['CPM', 'CPC']),
  effective_date => DB::Entity::Type::pq_date('now()'),
  currency_id => DB::Entity::Type::link('DB::Currency')
};

1;

package DB::BehavioralChannel::Trigger;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'Triggers'
}

use constant STRUCT => 
{
  trigger_id => DB::Entity::Type::sequence('triggers_trigger_id_seq'),
  trigger_type => DB::Entity::Type::enum(['K', 'U'], unique => 1),
  normalized_trigger => DB::Entity::Type::string(unique => 1),
  channel_type => DB::Entity::Type::enum(['A', 'D', 'S'], unique => 1),
  country_code => DB::Entity::Type::string(unique => 1),
  qa_status => DB::Entity::Type::qa_status(),
  version => DB::Entity::Type::pq_date("timestamp 'now'"),
  created => DB::Entity::Type::pq_date("timestamp 'now'"),
};

1;

package DB::BehavioralChannel::ChannelTrigger;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'ChannelTrigger'
}


use constant STRUCT => 
{
  channel_trigger_id => DB::Entity::Type::next_sequence(),
  channel_id => DB::Entity::Type::int(unique => 1),
  trigger_id => DB::Entity::Type::link('DB::BehavioralChannel::Trigger'),
  channel_type => DB::Entity::Type::enum(['A', 'D', 'S']),
  trigger_type => DB::Entity::Type::enum(['P', 'U', 'S', 'R'], unique => 1),
  original_trigger => DB::Entity::Type::string(unique => 1),
  trigger_group => DB::Entity::Type::string(nullable => 1),
  masked => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  negative => DB::Entity::Type::enum(['N', 'Y']),
  qa_status => DB::Entity::Type::qa_status()
};

1;

package DB::BehavioralParametersList;

use warnings;
use strict;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT =>
{
  behav_params_list_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  threshold => DB::Entity::Type::int(),
  version => DB::Entity::Type::pq_timestamp("timestamp 'now'")
};

1;

package DB::CMPChannelBase;

use warnings;
use strict;

use constant AUTO_QA => 0x002;
use constant OVERLAP => 0x008;

sub cmp_channel_preinit_
{
  my ($self, $args, $class_name) = @_;

  my $message_prefix = defined($class_name) ? "$class_name: " : "";

  if(exists($self->{channel_rate_id}))
  {
    # don't allow to define channel_rate_id directly,
    # for exclude inconsistent states like
    #   rate.currency_id != account.currency_id,
    #   shared channel rate
    die "${message_prefix}channel_rate_id is private field, " .
      "can't be defined";
  }
  
  if(defined($args->{visibility}))
  {
    if($args->{visibility} ne 'PUB' &&
      $args->{visibility} ne 'PRI' &&
      $args->{visibility} ne 'CMP')
    {
      die "${message_prefix}Incorrect visibility value '" .
        $args->{visibility} . "', allowed only 'PUB', 'PRI', 'CMP'";
    }
  }

  if(exists($args->{channel_rate_}))
  {
    $self->{channel_rate_} = $args->{channel_rate_};
    delete $args->{channel_rate_};
  }
  elsif(defined($args->{cpm}) || defined($args->{cpc}))
  {
    # CMP channel
    if(defined($args->{visibility}) && $args->{visibility} ne 'CMP')
    {
      die "${message_prefix}Incorrect visibility value '" .
        $args->{visibility} . "' for CMP channel(defined cpm or cpc)";
    }

    if(defined($args->{cpm}) &&
       $args->{cpm} != 0 &&
       defined($args->{cpc}) &&
       $args->{cpc} != 0)
    {
      die "${message_prefix}only one of cpm, cpc can be != 0";
    }

    $self->{channel_rate_} = DB::BehavioralChannel::Rate->blank(
      channel_id => 0,
      currency_id => 0,
      cpm => (defined($args->{cpm}) ? $args->{cpm} : 0),
      cpc => (defined($args->{cpc}) ? $args->{cpc} : 0),
      rate_type => (defined($args->{cpc}) ? 'CPC' : 'CPM'));

    $self->{visibility} = 'PUB'; # change visibility to CMP after rate linkage
    delete $args->{visibility};
  }

  delete $args->{cpm};
  delete $args->{cpc};
}

sub cmp_channel_postcreate_
{
  my ($self, $ns) = @_;

  if(exists $self->{channel_rate_})
  {
    $self->{channel_rate_}->{channel_id} = $self->channel_id();
    $self->{channel_rate_}->{currency_id} = $self->{account_id}->currency_id();
    $self->{channel_rate_id} = $ns->create($self->{channel_rate_});
    delete $self->{channel_rate_};
  }

  if(defined($self->{channel_rate_id}))
  {
    $self->__update($ns, { channel_rate_id => $self->{channel_rate_id}, visibility => 'CMP' });
  }
}

1;

package DB::BehavioralChannelBase;

use warnings;
use strict;

sub behavioral_parameters
{
  my ($self) = @_;
  return $self->{behavioral_parameters};
}

sub search_key
{
  my ($self) = @_;
  return $self->key_('S');
}

sub page_key
{
  my ($self) = @_;
  return $self->key_('P');
}

sub url_key
{
  my ($self) = @_;
  return $self->key_('U');
}

sub url_kwd_key
{
  my ($self) = @_;
  return $self->key_('R');
}

sub bp_key
{
  my ($self) = @_;
  my @keys;
  foreach my $bp(@{$self->{behavioral_parameters}})
  {
    push(@keys, $bp->trigger_type() .
      $bp->time_from() . "_" .
      $bp->time_to() . "_" .
      $bp->minimum_visits() . "_" .
      $bp->weight());
  }

  @keys = sort(@keys); # sort by trigger_type
  my $res = "_";
  my $i = 0;
  foreach my $k(@keys)
  {
    $res = ($i > 0 ? $res . "/" : $res) . $k;
    ++$i;
  }
  return $res;
}

sub key_
{
  my ($self, $type) = @_;
  foreach my $bp(@{$self->{behavioral_parameters}})
  {
    if($bp->{trigger_type} eq $type)
    {
      return $self->channel_id() . "$type";
    }
  }
  die "Tring to get undefined key type '$type'";
}

sub triggers
{
  my ($self) = @_;
  return values %{$self->{triggers_}};
}

sub keyword_channel_triggers
{
  my ($self) = @_;
  return $self->{keyword_channel_triggers_};
} 

sub search_channel_triggers
{
  my ($self) = @_;
  return $self->{search_channel_triggers_};
} 

sub url_channel_triggers
{
  my ($self) = @_;
  return $self->{url_channel_triggers_};
} 

sub url_kwd_channel_triggers
{
  my ($self) = @_;
  return $self->{url_kwd_channel_triggers_};
} 

sub behavioral_channel_preinit_
{
  my ($self, $args) = @_;

  if(!exists($args->{country_code}))
  {
    $args->{country_code} = 
      exists $self->{country_code}?
        $self->{country_code}:
           DB::Defaults::instance()->country()->{country_code};
  }
 
  # process alternate arg names
  if(exists $args->{search_list} or exists $args->{keyword_list})
  {
    $self->{search_list_} = 
      exists $args->{search_list}?
        $args->{search_list}: $args->{keyword_list};
    delete $args->{search_list};
  }

  if(exists $args->{keyword_list})
  {
    $self->{keyword_list_} = $args->{keyword_list};
    delete $args->{keyword_list};
  }

  if(exists $args->{url_list})
  {
    $self->{url_list_} = $args->{url_list};
    delete $args->{url_list};
  }

  if(exists $args->{url_kwd_list})
  {
    $self->{url_kwd_list_} = $args->{url_kwd_list};
    delete $args->{url_kwd_list};
  }

  # process relations
  if(exists $args->{behavioral_parameters})
  {
    $self->{behavioral_parameters} = $args->{behavioral_parameters};
    delete $args->{behavioral_parameters};
  }

  if(exists $args->{categories})
  {
    $self->{categories} = $args->{categories};
    delete $args->{categories};
  }
}

sub set_language_
{ 
  my ($self, $ns) = @_;
 
  if (not defined $self->{language} 
      and defined $self->{country_code})
  {
    ($self->{language}) = $ns->pq_dbh->selectrow_array(
      qq[select LANGUAGE from COUNTRY 
         where COUNTRY_CODE='$self->{country_code}']);
  }
}

sub behavioral_channel_precreate_
{
  my ($self, $ns) = @_;
 
  $self->set_language_($ns);
}

sub behavioral_channel_postcreate_
{
  my ($self, $ns) = @_;


  # link behavioral parameters
  my @res_behavioral_parameters;

  if(defined($self->{behavioral_parameters}))
  {
    foreach my $bp(@{$self->{behavioral_parameters}})
    {
      $bp->{channel_id} = $self->channel_id();
      push(@res_behavioral_parameters, $ns->create($bp));
    }
  }

  $self->{behavioral_parameters} = \@res_behavioral_parameters;

  # link categories
  if(defined($self->{categories}))
  {
    foreach my $cat(@{$self->{categories}})
    {
      $ns->create(DB::ChannelCategory->blank(
        channel_id => $self->{channel_id},
        category_channel_id => $cat));
    }
  }
  

  $self->{triggers_} = {};
  $self->{keyword_channel_triggers_} = [];
  $self->{search_channel_triggers_} = [];
  $self->{url_kwd_channel_triggers_} = [];
  $self->{url_channel_triggers_} = [];
   
  $self->fill_triggers_($ns, $self->{keyword_list_}, 'P');
  $self->fill_triggers_($ns, $self->{search_list_}, 'S');
  $self->fill_triggers_($ns, $self->{url_kwd_list_}, 'R');
  $self->fill_triggers_($ns, $self->{url_list_}, 'U');
}

sub normalize_url_
{
  my ($trigger_word) = @_;
  $trigger_word = lc($trigger_word);
  $trigger_word =~ s|^(?:http://)?(?:www.)?(.*)$|$1|;
  $trigger_word =~ s/\#(.*)$//;
  if($trigger_word !~ m/^\".*\"$/) # not exact url
  {
    if($trigger_word =~ m/\?/)
    {
      $trigger_word =~ s|^([^?]*[^?/])\?|$1/?|;
    }
    else
    {
      $trigger_word =~ s|([^/])(\")?$|$1/|;
    }
  }
  return $trigger_word;
}

sub normalize_keyword_
{
  my ($trigger_word) = @_;
  $trigger_word = lc($trigger_word);
  # pack spaces
  $trigger_word =~ s/\s+/ /;
  # trim
  $trigger_word =~ s/^\s+//;
  $trigger_word =~ s/\s+$//;
  return $trigger_word;
}

# Special processing for negative triggers
sub process_raw_trigger_
{
  my ($trigger_word_raw) = @_;
  
  if (substr($trigger_word_raw, 0, 2) eq '\-')
  {
    return (substr($trigger_word_raw, 1), 'N');
  }
  if (substr($trigger_word_raw, 0, 1) eq '-')
  {
    return (substr($trigger_word_raw, 1), 'Y');
  }
  return ($trigger_word_raw, 'N');
}

sub add_trigger
{
  my ($self, $ns, $trigger_word_raw, $trigger_type, $qa_status) = @_;

  $qa_status = 'A' unless defined $qa_status;
  my ($trigger_word, $negative) = 
    process_raw_trigger_($trigger_word_raw);
  my $nomalized_trigger = ($trigger_type eq 'U' ?
    normalize_url_($trigger_word) : normalize_keyword_($trigger_word));

  my $trigger = 
    defined $self->{triggers_}->{$nomalized_trigger}?
      $self->{triggers_}->{$nomalized_trigger}:
        $ns->create(DB::BehavioralChannel::Trigger->blank(
          trigger_type => 
            $trigger_type eq 'U'? $trigger_type: 'K',
          normalized_trigger => $nomalized_trigger,
          qa_status => $qa_status,
          channel_type => 
            ($self->channel_type() eq 'D' || $self->channel_type() eq 'S'? 
             $self->channel_type() : 'A'),
          country_code =>  
            $self->channel_type() eq 'S'? 
              '': $self->{country_code} ));

  if (! defined $self->{triggers_}->{$nomalized_trigger})
  {
    $self->{triggers_}->{$nomalized_trigger} = $trigger;
  }

  my $trigger_group = undef;

  if($trigger_type eq 'U')
  {
    if($trigger_word !~ m|^(?:http://)?([^/?]*).*$|)
    {
      die "Incorrect trigger word '$trigger_word'";
    }
    $trigger_group = $1;
  }

  my $channel_trigger = $ns->create(DB::BehavioralChannel::ChannelTrigger->blank(
    channel_id => $self->channel_id(),
    trigger_id => $trigger->trigger_id(),
    channel_type => 
     ($self->channel_type() eq 'D' || $self->channel_type() eq 'S'? 
        $self->channel_type() : 'A'),
    trigger_type => $trigger_type,
    original_trigger => $trigger_word,
    trigger_group => $trigger_group,
    negative => $negative,
    qa_status => $qa_status,
    masked => ($trigger_type eq 'U' ? 'N' : undef)));

  my $channel_trigger_list = $trigger_type eq 'U'? 
     $self->{url_channel_triggers_}:
     ($trigger_type eq 'S'? 
       $self->{search_channel_triggers_}:
         ($trigger_type eq 'R'?
            $self->{url_kwd_channel_triggers_}:
              $self->{keyword_channel_triggers_}));

  push(@{$channel_trigger_list}, $channel_trigger);

  return $channel_trigger;
}

sub fill_triggers_
{
  my ($self, $ns, $trigger_list, $trigger_type) = @_;

  if(defined($trigger_list))
  {
    my @triggers = split('\n', $trigger_list);
    foreach my $trigger_word_raw(@triggers)
    {
      $self->add_trigger($ns, $trigger_word_raw, $trigger_type);
    }
  }
}

1;

package DB::BehavioralChannel;

use warnings;
use strict;
use DB::Entity::PQ;

use constant DISPLAY_STATUS_PENDING_INACTIVATION =>
  "Live - Pending Inactivation";
use constant DISPLAY_STATUS_TRIGGERS_NEED_ATTENTION =>
  "Live - Triggers Need Attention";
use constant DISPLAY_STATUS_PENDING_INACTIVATION_AND_NEED_ATTENTION =>
  "Live - pending inactivation and needs attention";
use constant DISPLAY_STATUS_NOT_ENOUGH_USERS =>
  "Not Live - Not Enough Unique Users";

our @ISA = qw(DB::Entity::PQ DB::BehavioralChannelBase DB::CMPChannelBase);

sub _table
{
  'Channel'
}

use constant STRUCT =>
{
  #   public
  name => DB::Entity::Type::name(unique => 1),
  account_id => DB::Entity::Type::link(
    'DB::Account',
      default => sub { DB::Defaults::instance()->advertiser }),
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('BehavioralChannel'),
  country_code => DB::Entity::Type::country(), 
  language => DB::Entity::Type::string(nullable => 1), 
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  freq_cap_id => DB::Entity::Type::link('DB::FreqCap', nullable => 1),
  channel_type => DB::Entity::Type::enum(['B', 'K']),
  visibility => DB::Entity::Type::enum(['PUB', 'PRI', 'CMP']),
  trigger_type => DB::Entity::Type::enum(['S', 'P'], nullable => 1),
  behav_params_list_id => DB::Entity::Type::int(),
  triggers_status => DB::Entity::Type::enum(['A', 'L', 'H', 'D'], nullable => 1),
  distinct_url_triggers_count =>  DB::Entity::Type::int(nullable => 1),
  message_sent => 0,

  # only B or K allowed, OIX UI specific (adserver don't differentiate B & K)
  # K required if freq_cap_id isn't null
  # behavioral_parameters : array of BehavioralParameters
  # categories : array of (number|DB::ChannelCategory)
  # cpm, cpc: for CMP channels, only one != 0, and visibility == 'CMP' if one defined

  channel_rate_id => DB::Entity::Type::link('DB::BehavioralChannel::Rate'),
  channel_id => DB::Entity::Type::sequence(),
  flags => DB::Entity::Type::int(default => DB::CMPChannelBase::AUTO_QA),
  namespace =>  DB::Entity::Type::enum(['A', 'K']),

  keyword_list => DB::Entity::Type::string(private => 1),
  search_list => DB::Entity::Type::string(private => 1),
  url_list => DB::Entity::Type::string(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;
  
  if(exists($args->{channel_type}) &&
     $args->{channel_type} eq 'K')
  {
    $args->{visibility} = 'PRI';
    $args->{trigger_type} = 'S';
    $args->{namespace} = 'K';
  }
  else
  {
    $args->{triggers_status} = 'A';
    my @urls =();
    @urls = split('\n', $args->{url_list}) if $args->{url_list};
    $args->{distinct_url_triggers_count} = @urls;
  }

  DB::BehavioralChannelBase::behavioral_channel_preinit_($self, $args);
  
  DB::CMPChannelBase::cmp_channel_preinit_($self, $args);
}

sub precreate_
{
  my ($self, $ns) = @_;

  DB::BehavioralChannelBase::behavioral_channel_precreate_($self, $ns);

}

sub postcreate_
{
  my ($self, $ns) = @_;

  # link behavioral parameters
  DB::BehavioralChannelBase::behavioral_channel_postcreate_($self, $ns);
  
  # link categories
  if(defined($self->{categories}))
  {
    foreach my $cat(@{$self->{categories}})
    {
      $ns->create(
        DB::ChannelCategory->blank(
          channel_id => $self->{channel_id},
          category_channel_id => $cat));
    }
  }

  DB::CMPChannelBase::cmp_channel_postcreate_($self, $ns);

}

sub __init_args
{
   my ($self, $ns, $args) = @_;

   $self->SUPER::__init_args($ns, $args);

}

1;



# ATTENTION! Don not create_campaigns before create_channels
# Usage:
# my $db = new PerformanceDB:DB(...);
# $db->create_channels...);
# $db->create_campaigns(...);
# ....
# $db->tids(); # getting tids
# ...
# $db->commit;
# ...
# $db->keywords() # getting keywords

package PerformanceDB::Database;

use constant {
TAGS_ALL => 0, 
TAGS_FREE => 1, 
TAGS_USED => 2,
REFS_ALL => 0,
REFS_DISCOVER => 1,
REFS_ADVERTISING => 2
};

use constant TAGS => (TAGS_ALL, TAGS_FREE, TAGS_USED);
use constant REFS => (REFS_ALL, REFS_DISCOVER, REFS_ADVERTISING);
use constant CHANNELS_PER_ACCOUNT => 50;

use warnings;
use strict;

use FindBin;
use lib "$FindBin::Dir/../../../Commons/Perl";

use DB::Database;
use DB::EntitiesImpl;
use DB::Defaults;
use DB::Util;

use RandWords;
use CampaignTemplates;
use DBEntityChecker;


sub new {
    my $self = shift;
    my ($host, $db, $user, $password, $namespace_name, $test_name) = @_;

    unless (ref $self) {
        $self = bless {}, $self;
    }

    (my $user_name = $ENV{USER}) =~ s/_/./;
    $self->{_db} = new DB::Database($host, $db, $user, $password,
                                    "$namespace_name-$user_name");
    $self->{_namespace} = $self->{_db}->namespace($test_name);
    $self->{_test_name}                 = $test_name;
    $self->{_bhv_channel_idx}           = 0; 
    $self->{_channel_idx}               = 0;
    $self->{_campaign_idx}              = 0;
    $self->{_set_channel_idx}           = 0;
    $self->{_ron_index}                 = 0;
    $self->{_channels}                  = [];
    $self->{_tids}                      = [];
    $self->{_free_tids}                 = [];
    $self->{_keywords}                  = [];
    $self->{_urls}                      = [];
    $self->{_discover_keywords}         = [];
    $self->{_discover_urls}             = [];
    $self->{_default_size}              = undef;
    $self->{_default_country}           = undef;
    $self->{_acc_id}                    = undef;
    $self->{_default_template_id}       = undef;
    $self->{_advertiser_type}           = undef;
    $self->{_publisher_type}            = undef;
    $self->init_namespace();
    return $self;
}


sub live_display_status {
    my $self = shift;
    my ($object_type_name) = @_;
    my $ns = $self->{_namespace};  
    my $stmt = $ns->pq_dbh->prepare_cached(q[
                                          SELECT display_status_id
                                          FROM DisplayStatus
                                          WHERE object_type_id = (SELECT object_type_id
                                                                  FROM ObjectType
                                                                  WHERE name = ?)
                                          AND disp_status = 'L'
                                          ]);
    
    $stmt->execute($object_type_name);
    my ($display_status_id) = $stmt->fetchrow_array();
    $stmt->finish;
    
    return $display_status_id;
}


sub init_namespace {
  my $self = shift;
  my $database = $self->{_db};
  my $ns = $self->{_namespace};
  my $test_name = $self->{_test_name};
  
  # Namespace defaults
  DB::Defaults::instance()->initialize($database);

  $self->{_default_size} = DB::Defaults::instance()->size;

  $self->{_acc_id} = $ns->create(Advertiser => { 
    name => "$test_name-01"} );
}

sub allow_html_format
{
  my $self = shift;
  my $ns = $self->{_namespace};

  my $format = 
    $ns->create(DB::AppFormat->blank(
      name => 'html',
      mime_type => 'text/html'));

  $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->display_template(),
      size_id => $self->{_default_size},
      app_format_id => $format });
}

sub set_default_country
{
  my $self = shift;
  my ($code) = @_;
  $self->{_default_country} = $self->create(Country => {
    country_code => $code,
    low_channel_threshold => 0,
    high_channel_threshold => 0 });
}

sub get_channel_account
{
  my ($self) = @_;
  my $test_name = $self->{_test_name};
  my $ns = $self->{_namespace};
  my $account_idx = int(@{$self->{_channels}} / CHANNELS_PER_ACCOUNT);

  return $ns->create(Account => { 
    name => "$test_name-Channel-" . $account_idx,
    role_id => DB::Defaults::instance()->advertiser_role });

}

sub create_channels {
  my $self = shift;
  my ($count, $time_from, $time_to, $minimum_visits, $channel_type, $queries) = @_;
  $minimum_visits = 1 unless defined $minimum_visits;
  $channel_type = 'B' unless defined $channel_type;
  my $test_name  = $self->{_test_name};
  for (my $i = 0; $i < $count; ++$i)
  {
    my $idx        = sprintf("%04d",  ++($self->{_bhv_channel_idx}));
    my $trigger_type = rand_trigger_type($self->{_test_name});

    my ($channel, $url, $keyword);

    if ($trigger_type eq 'U')
    {
      $url = RandWords::rand_url($self->{_test_name});
    }
    else
    {
      $keyword = RandWords::rand_keyword($self->{_test_name});
    }   
    
    if ( $channel_type eq 'D' )
    {
      my $query_ind        = $i % scalar @$queries;
      my $discover_query      = $queries->[$query_ind];
      my $discover_annotation = $queries->[$query_ind];
      $channel = $self->create(DB::DiscoverChannel->blank(
        name => "$test_name-$idx",
        account_id => $self->get_channel_account(),
        keyword_list => $keyword,
        url_list => $url, 
        discover_query => $discover_query,
        discover_annotation => $discover_annotation,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => $trigger_type,
            time_from => $time_from,
            time_to => $time_to,
            minimum_visits => $minimum_visits)]));
        push(@{$self->{_discover_urls}}, $url) if defined $url;
        push(@{$self->{_discover_keywords}}, $keyword) if defined $keyword;
    }
    else
    {
      $channel = $self->create(DB::BehavioralChannel->blank(
        name => "$test_name-$idx",
        account_id => $self->get_channel_account(),
        keyword_list => $keyword,
        url_list => $url, 
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => $trigger_type,
            time_from => $time_from,
            time_to => $time_to,
            minimum_visits => $minimum_visits)]));
      push(@{$self->{_urls}}, $url) if defined $url;
      push(@{$self->{_keywords}}, $keyword) if defined $keyword;
    }
    push(@{$self->{_channels}}, $channel->channel_id);
  }
}

sub create_ft_channels {
  my $self = shift;
  my ($ft_phrases, $percentage, $lang) = @_;
  my $size = scalar @$ft_phrases;
  my $test_name  = $self->{_test_name};
  use POSIX qw(ceil);
  my $channels_count = ceil(($size * $percentage) / 100);
  for(my $i=0; $i < $channels_count; $i++)
  {
    my $phrase = @$ft_phrases[int(rand(scalar(@$ft_phrases)))];
    $self->create(DB::BehavioralChannel->blank(
      name => "$test_name-ft-$i",
      keyword_list => $phrase ,
      language => $lang,
      account_id => $self->{_acc_id},
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          time_from => 0,
          time_to => 0,
          minimum_visits => 1)]));
  }
}

sub create_set_channels {
  my $self = shift;
  my ($count)    = @_;
  my $test_name  = $self->{_test_name};
  my $idx        = $self->{_channel_idx};
  my $expression = "";
  for (my $i = 0; $i < $count; ++$i)
  {
    if ($expression ne "")
    {
      $expression .= "|";
    }
    my $channel_id = @{$self->{_channels}}[$idx];
    $expression .= "$channel_id";
    if (++$idx >= scalar(@{$self->{_channels}}))
    {
      $idx = 0;
    }
  }
  $self->{_channel_idx} = $idx;
  my $name = sprintf("%s-Set-%04d", $test_name, ++($self->{_set_channel_idx}));
  return $self->create(DB::ExpressionChannel->blank(
            name => "$name",
            expression => $expression,
            account_id => $self->{_acc_id}));
}

sub create_campaigns {
  my $self = shift;
  my ($count, $channels_count, $flags) = @_;
  my $test_name  = $self->{_test_name};
  for (my $i = 0; $i < $count; ++$i)
  {
    my $idx = sprintf("%04d", ++($self->{_campaign_idx}));
    my $channel  = $self->create_set_channels( $channels_count );
    my $campaign = new PerformanceDB::Campaign($self, "$idx", $flags);
    $campaign->create($channel->channel_id, 1);
  }
}

sub create_ron_campaign {
  my $self = shift;
  my ($tags_count, $site_id) = @_;
  my $campaign = new PerformanceDB::Campaign($self, "RON-".(++($self->{_ron_index})),  
                                             CampaignConfig::CampaignFlags::RONFlag |
                                             CampaignConfig::CampaignFlags::CampaignSpecificSitesFlag);
  $campaign->create(undef, $tags_count, $site_id);
}

sub create_free_tags {
  my $self = shift;
  my ($tags_count) = @_;
  my $size = $self->create(CreativeSize =>
                                { name => "1X2",
                                  width => 1,
                                  height => 2 });

  my $tags = new PerformanceDB::Tags($self, "FREE", $tags_count, $size, 1);
  $tags->create(undef, undef);
}

sub rtb_publisher {
  my $self = shift;

  $self->{_rtb_publisher} ||= $self->{_namespace}->create(Publisher => {
      name => "RTBPublisher",
      account_id => DB::Defaults::instance()->openrtb_account,
      pricedtag_size_id =>
        $self->{_default_size} || DB::Defaults::instance()->size });
}

sub store_tid {
  my $self = shift;
  my ($tag_id, $free) = @_;
  if ($free) 
  {
    push(@{$self->{_free_tids}}, $tag_id);    
  }
  else
  {
    push(@{$self->{_tids}}, $tag_id);
  }
}

sub create {
  my $self = shift;
  my ($class, $args) = @_;
   if (exists $args->{'name'})
   {
    if (!PerformanceDB::EntityChecker::check($self->{_namespace}, $class, $args->{'name'}))
     {
       $self->{_namespace}->pq_dbh->rollback;        
       my $class = "DB::$class";
       my $table = $class;
       unless (!$class->_table) {$table = $class->_table; }
       my $entity = $self->{_namespace}->namespace . "-" . $args->{'name'};
       $self->{_namespace} = undef;
       die "Entity '$entity' already exists in '$table'. You need clean performance test data before running script!";
     }
   }
  return $self->{_namespace}->create($class, $args);
}

sub acc_id {
  my $self = shift;
  return $self->{_acc_id};
}

sub size {
  my $self = shift;
  return $self->{_default_size} || DB::Defaults::instance()->size;
}

sub country {
  my $self = shift;
  return $self->{_default_country} || DB::Defaults::instance()->country;
}

sub tids {
  my $self = shift;
  my $tags_type = shift;
  $tags_type = TAGS_ALL unless defined $tags_type;
  if ($tags_type == TAGS_FREE) {
    return $self->{_free_tids};
  }    
  if ($tags_type == TAGS_USED) {
    return $self->{_tids};
  }
  return ($self->{_tids}, $self->{_free_tids});
}

sub keywords {
  my $self = shift;
  my $ref_type = shift;
  $ref_type = REFS_ALL unless defined $ref_type;
  if ($ref_type == REFS_ADVERTISING) {
    return $self->{_keywords};
  }    
  if ($ref_type == REFS_DISCOVER) {
    return $self->{_discover_keywords};
  }
  return ($self->{_keywords}, $self->{_discover_keywords});
}

sub urls {
  my $self = shift;
  my $ref_type = shift;
  $ref_type = REFS_ALL unless defined $ref_type;
  if ($ref_type == REFS_ADVERTISING) {
    return $self->{_urls};
  }    
  if ($ref_type == REFS_DISCOVER) {
    return $self->{_discover_urls};
  }
  return ($self->{_urls}, $self->{_discover_urls});
}

sub commit {
  my $self = shift;
  $self->{_namespace} = undef;
  calc_ctr_pq($self->{_db});
  $self->{_db}->_pq_dbh()->commit;
}

sub rand_trigger_type {
  my $type = int(rand(2));
  if ($type == 0)
  {
    return 'U';
  }
  return 'P';
}

1;


# File contant example:
#
# time_from: 0, time_to: 10, count: 10
# time_from: 15, time_to: 30, count: 30
# time_to: 20, count: 10
# count: 15

# Default time_from = default time_to = 0

package ChannelConfig::File;

use constant TimeFrom => "time_from";
use constant TimeTo => "time_to";
use constant Count => "count";
use constant ChannelType => "channel_type";
use constant MinimumVisits => "minimum_visits";

use constant DefaultTimeFrom => 0;
use constant DefaultTimeTo   => 0;
use constant DefaultMinimumVisits => 1;

use warnings;
use strict;
use Text::ParseWords;


sub new {
  my $self = shift;
  my ($config_file_path) = @_;
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{file_path}  = $config_file_path;
  $self->{line_count} = 0;
  return $self;
}

sub _parse_line {
  my $self = shift;
  my $line = shift;
  my @pairs = &parse_line('\s*,\s*', 0, $line);
  my $time_from = DefaultTimeFrom;
  my $time_to = DefaultTimeTo;
  my $minimum_visits = DefaultMinimumVisits;
  my $count;
  my $channel_type;
  for my $pair (@pairs)
  {
    if ($pair =~ /^(\w*?)\s*:\s*(\d+)\s*$/o)
    {
      if ($1 eq TimeFrom)
      {
        $time_from = int($2);
      }
      elsif ($1 eq TimeTo)
      {
        $time_to = int($2);
      }
      elsif ($1 eq Count)
      {
        $count = int($2);
      }
      elsif ($1 eq MinimumVisits)
      {
        $minimum_visits = int($2);
      }
      else
      {
        die "Invalid line $self->{line_count} field name $1";
      }
    }
    elsif ($pair =~ /^(\w*?)\s*:\s*(\w+)\s*$/o)
    {
      if ($1 eq ChannelType)
      {
        if (grep /$2/, ('D', 'C'))
        {
          $channel_type = $2;
        }
        else
        {
          die "Invalid line $self->{line_count}: incorrect channel type '$2'";
        }
      }
      else
      {
        die "Invalid line $self->{line_count} field name $1";
      }
    }
    else 
    {
      die "Invalid line $self->{line_count} of the config file $self->{file_path}";
    }
  }
  if (!defined($count))
  {
    die "Invalid line $self->{line_count} count field absent";    
  }
  return ($time_from, $time_to, $minimum_visits, $count, $channel_type);
}


sub get {
  my $self = shift;
  open(CONFIG, $self->{file_path}) || die "Cann't open file $self->{file_path}.";
  my @lines = <CONFIG>;
  close(CONFIG);
  my @result;
  $self->{line_count} = 0;
  foreach my $line (@lines)
  {
    my @channel = $self->_parse_line($line);
    push(@result, \@channel);
    $self->{line_count}++;
  }
  return @result;
}

1;



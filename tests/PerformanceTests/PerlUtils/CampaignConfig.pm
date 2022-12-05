
# File contant example:
#
# FreqCapsCampaign, FreqCapsCreative, FreqCapsCCG: 3
# FreqCapsCreative: 3
# FreqCapsCCG: 3
# :3 
package CampaignConfig::CampaignFlags;

use warnings;
use strict;

use constant FreqCapsCampaignFlag      => 1;
use constant FreqCapsCreativeFlag      => 2;
use constant FreqCapsCCGFlag           => 4;
use constant CampaignSpecificSitesFlag => 16;
use constant RONFlag                   => 32;

our %FLAGS = (FreqCapsCampaign       => FreqCapsCampaignFlag,
              FreqCapsCreative       => FreqCapsCreativeFlag,
              FreqCapsCCG            => FreqCapsCCGFlag,
              CampaignSpecificSites  => CampaignSpecificSitesFlag);
1;

package CampaignConfig::File;

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

sub _get_count_and_flag_str {
  my $self = shift;
  my $s    = shift;  
  if ($s =~ /([\w\W]*?) *: *(\d+) */o)
  {
    return ($1, int($2))
  }
  else 
  {
    die "Invalid line $self->{line_count} of the config file $self->{file_path}";
  }
}

sub  _get_flags {
  my $self  = shift;
  my $s     = shift;
  my $flags = 0;
  my @words = &parse_line('\s*,\s*', 0, $s);
  foreach my $word (@words)
  {
    if (! exists $CampaignConfig::CampaignFlags::FLAGS{$word} )
    {
      die "Invalid flag '$word' in $self->{line_count} line of the config file $self->{file_path}";
    }
    $flags |= $CampaignConfig::CampaignFlags::FLAGS{$word};
  }
  return $flags;
}

sub get {
  my $self = shift;
  open(CONFIG, $self->{file_path}) || die "Cann't open file $self->{file_path}.";
  my @lines = <CONFIG>;
  close(CONFIG);
  my %result;
  $self->{line_count} = 0;
  foreach my $line (@lines)
  {
    my ($flag_str, $count) = $self->_get_count_and_flag_str($line);
    my $flags              = $self->_get_flags($flag_str);
    if ( ! exists $result{$flags})
    {
      $result{$flags} = 0;
    }
    $result{$flags} += $count;
    $self->{line_count}++;
  }

  return %result;
}

1;

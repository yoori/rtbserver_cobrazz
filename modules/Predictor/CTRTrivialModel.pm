package Predictor::CTRTrivialModel;

use strict;
use warnings;

sub new
{
  my $class = shift;

  my $self = {
    'model_' => {}
  };

  bless $self, $class;

  return $self;
}

sub set_ctr
{
  my ($self, $tag_id, $domain, $clicks, $imps) = @_;
  $self->{'model_'}->{"$tag_id,$domain"} = "$clicks,$imps";
}

sub get_ctr
{
  my ($self, $tag_id, $domain) = @_;
  if(exists($self->{'model_'}->{"$tag_id,$domain"}))
  {
    my $l = $self->{'model_'}->{"$tag_id,$domain"};
    my @a = split(',', $l);
    my $clicks = $a[0];
    my $imps = $a[1];
    return [ $clicks, $imps ];
  }

  return [0, 0]
}

sub save
{
  my ($self, $file_path) = @_;

  open(my $fh, '>', $file_path) or die "Could not open '$file_path' $!\n";
  while (my ($key, $val) = each (%{$self->{'model_'}}))
  {
    my @a = split(',', $val);
    my $clicks = $a[0];
    my $imps = $a[1];

    if($imps > 0)
    {
      print $fh "$key,$val\n";
    }
  }
  close($fh);
}

sub load
{
  my ($self, $file_path) = @_;

  open(my $fh, '<', $file_path) or die "Could not open '$file_path' $!\n";

  while(my $line = <$fh>)
  {
    chomp $line;
    my @fields = split(',', $line);

    if(scalar(@fields) > 0)
    {
      # <tag_id>,<domain>,<clicks>,<imps>
      my $tag_id = $fields[0];
      my $domain = $fields[1];

      my $clicks = $fields[2];
      my $imps = $fields[3];

      $self->{'model_'}->{"$tag_id,$domain"} = "$clicks,$imps";
    }
  }

  close($fh);
}

1;

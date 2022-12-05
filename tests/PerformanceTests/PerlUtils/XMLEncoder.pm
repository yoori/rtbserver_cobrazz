
use warnings;
use strict;

package XML::Encoder;

use constant DEFAULT_OFSET          => 2;
use constant NEW_LINE               => " \n";

my $NAMESPACE  = undef;

sub set_namespace {
  $NAMESPACE = shift;
}

sub convert_to_bool {
  my $val = shift;
  if ($val) {
    return "true";
  }
  return "false"
}

sub get_full_name {
  my $name= shift;
  if (defined($NAMESPACE)) 
  {
    return  $NAMESPACE . ":" . $name;
  }
  return $name;
}

sub make_ofset {
  my $ofset  = shift;
  my $buffer = "";
  for( my $ind = 0; $ind < $ofset; ++$ind )
  {
    $buffer .= " ";
  }
  return $buffer;
}


sub set_defaults {
  if (defined($_[0]) && (ref($_[0]) eq 'HASH'))
  {
    return @_;
  }
  else 
  {
    unshift(@_, {});
  }
  return @_;
}

sub make_attributes {
  my $attributes = shift;
  my $ofset      = shift;
  if (!defined($ofset))
  {
    $ofset = 0;
  }
  my $buffer = "";
  my $first  = 1;
  while (my ($name, $value) = each %$attributes)
  {
    if (!$first)
    {
      $buffer .="\n " . make_ofset($ofset)
    }
    $buffer .= " $name=\"$value\"";
    $first = 0;
  }
  return $buffer;
}

sub print_child {
  my ($text, $ofset) = @_;
  my @lines  =  split("\n", $text);
  my $buffer = "";
  foreach  my $line (@lines)
  {
    if ($line eq " ")
    {
      $buffer .= "\n";
    }
    else
    {
      $buffer .= make_ofset($ofset) . $line . "\n";
    }
  }
  return $buffer;
}

sub PARAM {
  my ($attributes, $name, $value) = set_defaults(@_);
  $name = get_full_name($name);
  if (defined($value))
  {
    return "<$name" . make_attributes(\%$attributes, length($name)) . 
        ">" . $value . "</$name>\n";
  }

  return "<$name" . make_attributes(\%$attributes, length($name)) . "/>";
  
}

sub GROUP_PARAM {
  my ($attributes, $name, $childs, $value) = set_defaults(@_); 
  $name = get_full_name($name);
  my $buffer = "<$name" . make_attributes(\%$attributes, length($name)) . ">";
  if (defined($value))
  {
    $buffer .= "$value";
  }
  $buffer .= "\n";
  foreach my $child (@$childs)
  {
    $buffer .= print_child($child, DEFAULT_OFSET);
  }
  $buffer      .= "</$name>\n";
  return $buffer;
}

sub PARAM_LIST {
  my ($attributes, $name, $listname, $values) = set_defaults(@_); 
  $name     = get_full_name($name);
  $listname = get_full_name($listname);
  my $buffer = "<$name" . make_attributes(\%$attributes, length($name)) . ">\n";
  foreach my $value (@{$values})
  {
    $buffer .= make_ofset(DEFAULT_OFSET) . "<$listname>"; 
    $buffer .= $value;
    $buffer .= "</$listname>\n";
  }
  $buffer .= "</$name>\n";
  return $buffer;
}

1;

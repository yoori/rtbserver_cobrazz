package ConfigUtils;

sub print_value
{
  my ($value, $tab) = @_;

  if(ref($value) eq 'HASH')
  {
    my $ret = "$tab\{ \n";

    while(my ($key, $value) = each(%$value))
    {
      $ret = $ret . $tab . "  $key => " . print_value($value, "$tab  ") . ",\n";
    }

    $ret = $ret . $tab . " }\n";

    return $ret;
  }
  elsif(ref($value) eq 'ARRAY')
  {
    my $ret = "$tab\[ \n";

    foreach my $i(@{$value})
    {
      $ret = $ret . print_value($i, "$tab  ") . ",\n";
    }

    $ret = $ret . $tab . " ]\n";

    return $ret;
  }
  else
  {
    return "'$value'";
  }
}

sub save
{
  my ($vars, $out) = @_;

  open(OUTPUT_FILE, "> $out") or
    die "Error: can't open $out for writing.\n";

  print OUTPUT_FILE "%CONFIG = (%CONFIG, \n";
  while(my ($key, $value) = each(%$vars))
  {
    print OUTPUT_FILE "  '$key' => ". print_value($value, "  "). ",\n";
  }

  print OUTPUT_FILE ");\n";
}

1;

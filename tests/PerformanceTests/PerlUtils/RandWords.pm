
package RandWords;

use constant MAX_WORD_LENGTH => 30;

sub rand_symbol {
  my $code = int(rand(36));
  if ($code < 10)
  {
    return chr(ord('0') + $code);
  }
  my $caps = int(rand(2));
  if ($caps)
  {
    return chr(ord('A') + $code -  10);
  }
  return chr(ord('a') + $code -  10);
}

sub rand_string {
  my $length = shift;
  my $buffer = "";
  for (my $i = 0; $i < $length; $i++)
  {
    $buffer .= rand_symbol();
  }
  return $buffer;
}

sub rand_keyword {
  my $prefix = shift;
  my $length = int(rand(MAX_WORD_LENGTH));
  return $prefix . '-' . rand_string($length+1);
}

sub rand_url {
  my $prefix = shift;
  return 'http://' . $prefix . '.' . rand_keyword($prefix) .
      "." . rand_keyword($prefix) . "." . rand_string(3);
}

1;

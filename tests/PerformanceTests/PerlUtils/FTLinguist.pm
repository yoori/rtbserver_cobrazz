
package FTLinguist;

use warnings;
use strict;
use utf8;                      # source is in UTF-8
use warnings qw(FATAL utf8);   # encoding errors raise exceptions
use open qw(:utf8 :std);       # default open mode, `backticks`, and std{in,out,err} are in UTF-8

use constant WORDS_IN_PHRASE => 9;  # in words
use constant FT_SIZE => 512;

sub create_ft
{
  my ($source_path, $destination_path) = @_;
  open(SOURCE, "<:encoding(UTF-8)", $source_path);
  my @phrases = ();
  my $i = 0;
  my $fi = 0;
  my $trigger = "";
  my $text = "";
  for my $line (<SOURCE>)
  {
    $line =~ s/\p{Common}//g;
    $text .= $line;
    my $string = `LinguistUtil -nl segment '$line'` if $line;
    if ($string)
    {
      my @lines = split /\n/, $string;
      for my $l (@lines)
      {
        if ($l =~ /^\s+\d+:\s'(\X+)'$/o)
        {
          my $t = $1;
          $trigger .= $trigger? "\n" . $t: $t;
          $text .= $t;
          if (length($text) + 2 >= FT_SIZE)
          {
            my $filepath = File::Spec->join($destination_path, $fi++ . ".ft");
            store_ft($filepath, $text);
            $text = "";
          }
          if ($i++ == WORDS_IN_PHRASE)
          {
            push @phrases, $trigger;
            $trigger = "";
            $i = 0;
          }
        }
      }
    }
  }
  close(SOURCE);
  push @phrases, $trigger if $trigger;
  if ($text)
  {
    my $filepath = File::Spec->join($destination_path, $fi++ . ".ft");
    store_ft($filepath, $text);

  }
  return @phrases;
}

sub store_ft {
  my ($path, $text) = @_;
  open(my $FILE, ">:encoding(UTF-8)", $path) || die "Cann't open file $path for write.\n";  
  print $FILE $text;
  close($FILE);
}

1;

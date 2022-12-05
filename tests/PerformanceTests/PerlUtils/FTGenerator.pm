
package FTGenerator;

use warnings;
use strict;

use RandWords;

use constant WORDS_IN_PHRASE         => 5;  # in words
use constant MAX_WORDS_IN_SENTENCE   => 12;  # in words
use constant MAX_WORD_LENGTH         => 7; # in bytes

my @delimiters = (",", "\n");

sub create_ft
{
  my ($ft_path, $ft_count, $file_size) = @_;
  my @phrases = ();
  for (my $f_idx = 0; $f_idx < $ft_count; $f_idx++)
  {
    my $text = "";
    while (length($text) < $file_size)
    {
      $text .= create_sentence(\@phrases, length($text), $file_size);
      if (length($text) + 2 < $file_size)
      {
        $text .= create_sentence_delimiter() . " ";
      }
    }
    my $filepath = File::Spec->join($ft_path, ($f_idx+1) . ".ft");
    store_ft($filepath, $text);
  }
  return @phrases;
}

sub create_sentence
{
  my ($phrases, $text_size, $file_size) = @_;
  my $words_count = 0;
  my $sentence = "";
  my $max_words = int(rand(MAX_WORDS_IN_SENTENCE)) + 1;
  while ($words_count < $max_words and
         $text_size + length($sentence) < $file_size)
  {
    my ($phrase, $w_count) = create_phrase($phrases,  $text_size + length($sentence), $file_size);
    $sentence .= $phrase;
    $words_count += $w_count;
    if ($words_count < $max_words)
    {
      $sentence .= " ";
    }
  }
  return $sentence;
}

sub create_phrase
{
  my ($phrases, $text_size, $file_size) = @_;
  my $phrase = "";
  my $words_count = 0;
  my $max_words = WORDS_IN_PHRASE;
  while ($words_count < $max_words and
         $text_size + length($phrase) < $file_size)
  {
    $phrase .= RandWords::rand_string(MAX_WORD_LENGTH);
    $words_count++;
    if ($words_count < $max_words)
    {
      $phrase .= " ";
    }
  }
  push (@$phrases, $phrase);
  return ($phrase, $max_words)
}


sub create_sentence_delimiter
{
  return @delimiters[int(rand(scalar(@delimiters)))];
}

sub store_ft {
  my ($path, $text) = @_;
  open(my $FILE, ">$path") || die "Cann't open file $path for write.\n";  
  print $FILE $text;
  close($FILE);
}

1;

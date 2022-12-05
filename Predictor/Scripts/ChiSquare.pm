package ChiSquare;

use strict;
use Class::Struct;

require 5.000;

require Exporter;
@ChiSquare::ISA = qw( Exporter );

use Carp;

@ChiSquare::EXPORT = qw( chi_square );

my @chilevels = (1, 0.99, 0.95, 0.9, 0.7, 0.5, 0.3, 0.1, 0.05, 0.01);

my %chitable;

$chitable{1} = [0, 0.00016, 0.0039, 0.016, 0.15, 0.46, 1.07, 2.71, 3.84, 6.64];
$chitable{2} = [0, 0.020,   0.10,   0.21,  0.71, 1.39, 2.41, 4.60, 5.99, 9.21];
$chitable{3} = [0, 0.12,   0.35,   0.58,  1.42, 2.37, 3.67, 6.25, 7.82, 11.34];
$chitable{4} = [0, 0.30,   0.71,   1.06,  2.20, 3.36, 4.88, 7.78, 9.49, 13.28];
$chitable{5} = [0, 0.55,  1.14,   1.61,  3.00, 4.35, 6.06, 9.24, 11.07, 15.09];
$chitable{6} = [0, 0.87, 1.64,   2.20,  3.83, 5.35, 7.23, 10.65, 12.59, 16.81];
$chitable{7} = [0, 1.24, 2.17,   2.83,  4.67, 6.35, 8.38, 12.02, 14.07, 18.48];
$chitable{8} = [0, 1.65, 2.73,   3.49,  5.53, 7.34, 9.52, 13.36, 15.51, 20.09];
$chitable{9} = [0, 2.09, 3.33,   4.17, 6.39, 8.34, 10.66, 14.68, 16.92, 21.67];
$chitable{10} = [0, 2.56, 3.94,  4.86, 7.27, 9.34, 11.78, 15.99, 18.31, 23.21];
$chitable{11} = [0, 3.05, 4.58, 5.58, 8.15, 10.34, 12.90, 17.28, 19.68, 24.73];
$chitable{12} = [0, 3.57, 5.23, 6.30, 9.03, 11.34, 14.01, 18.55, 21.03, 26.22];
$chitable{13} = [0, 4.11, 5.89, 7.04, 9.93, 12.34, 15.12, 19.81, 22.36, 27.69];
$chitable{14} = [0, 4.66, 6.57, 7.79, 10.82, 13.34. 16.22, 21.06, 23.69,29.14];
$chitable{15} = [0, 5.23, 7.26, 8.55, 11.72, 14.34, 17.32, 22.31, 25.00,30.58];
$chitable{16} = [0, 5.81, 7.96, 9.31, 12.62, 15.34, 18.42, 23.54, 26.30,32.00];
$chitable{17} = [0, 6.41, 8.67, 10.09, 13.53, 16.34, 19.51, 24.77,27.59,33.41];
$chitable{18} = [0, 7.00, 9.39, 10.87, 14.44, 17.34, 20.60, 25.99,28.87,34.81];
$chitable{19} = [0, 7.63, 10.12, 11.65, 15.35, 18.34, 21.69,27.20,30.14,36.19];
$chitable{20} = [0, 8.26, 10.85, 12.44, 16.27, 19.34, 22.78,28.41,31.41,37.57];

sub chi_square
{
  my ($dataref, $distref) = @_;
  my (@data) = @$dataref;
  my (@dist) = @$distref;
  my ($degrees_of_freedom) = scalar(@data) - 1;
  my ($chisquare, $i, $observed, $expected) = (0, 0, 0, 0);

  exists($chitable{$degrees_of_freedom}) or
    carp("(Can't handle ",scalar(@data)," choices without a better table.)"),
    return undef;
  scalar(@data) && scalar(@data) == scalar(@dist) or 
    carp("(Error: there should be as many data elements as distribution elements.)"), return undef;

  for ($i = 0; $i < @data; $i++)
  {
    $observed += $data[$i];
    $expected += $dist[$i];
  }

  $observed == $expected or
    carp("(Error: $observed observed and $expected expected.  Those should be equal.)"), return undef;

  for ($i = 0; $i < @data; $i++)
  {
    $chisquare += (($data[$i] - $dist[$i]) ** 2) / $dist[$i];
  }

  $i = 0;

  foreach (@{$chitable{$degrees_of_freedom}})
  {
    if ($chisquare < $_)
    {
      return ($i == @chilevels - 1 ? 0 : $chilevels[$i+1], $chilevels[$i]);
    }
    $i++;
  }

  return (0, $chilevels[-1]);
}    

1;

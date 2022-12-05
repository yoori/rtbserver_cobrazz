#!/usr/bin/perl -w

use strict;
use ChiSquare;

my ($min, $max) = chi_square([9, 0], [9 - 3.82048694335814, 3.82048694335814]);
print "$min $max\n";


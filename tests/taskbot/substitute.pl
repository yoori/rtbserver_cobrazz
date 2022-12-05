#! /usr/bin/perl
#
use warnings;
use strict;

=head1 NAME

substitute.pl - perform @@STUB@@ substitution on files.

=head1 SYNOPSIS

  substitute.pl [ -f STUB_FILE ] [ -D STUB=REPLACEMENT... ] FILE...

=head2 OPTIONS

=over

=item C<-D STUB=REPLACEMENT>

Define a substitution.  Every occurrence of I<@@STUB@@> in I<FILE>
will be replaced with I<REPLACEMENT>.  This option may be given
multiple times.

=item C<--file STUB_FILE>
=item C<-f STUB_FILE>

Specify file containing I<STUB=REPLACEMENT> definitions one per line.
Blank lines and comments starting with I<#> are ignored.  Explicit
I<-D> take precedence over definitions from file.

=back

=cut

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %define;
my %option = (D => \%define);
if (! GetOptions(\%option, qw(D=s% file|f=s)) || ! @ARGV) {
    pod2usage(1);
}

if ($option{file}) {
    my $file = $option{file};
    open(my $df, '<', $file)
      or die "open(< $file): $!";

    while (my $line = <$df>) {
        next if $line =~ /^\s*(?:#|$)/;

        chomp $line;
        my ($key, $value) = split("=", $line, 2);
        $define{$key} = $value
            unless exists $define{$key};
    }

    close($df)
      or die "close($file): $!";
}

keys %define
  or pod2usage(1);

foreach my $file (@ARGV) {
    open(my $if, '<', $file)
      or die "open(< $file): $!";
    open(my $of, '>', "$file.new")
      or die "open(> $file.new): $!";

    my $line_no = 0;
    while (my $line = <$if>) {
        ++$line_no;
        $line =~ s<\@\@([^@]+)\@\@>
                  <
                    exists $define{$1}
                    ? $define{$1}
                    : die "$file:$line_no:"
                        . " substitution for \@\@$1\@\@ not defined\n"
                  >eg;
        print $of $line;
    }

    close($if)
      or die "close($file): $!";
    close($of)
      or die "close($file.new): $!";

    unlink($file)
      or die "unlink($file): $!";
    rename("$file.new", $file)
      or die "rename($file.new, $file): $!";
}

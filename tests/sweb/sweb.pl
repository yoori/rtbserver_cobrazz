#! /usr/bin/perl
# -*- cperl -*-
#
use warnings;
use strict;

=head1 NAME

sweb.pl - literate programming tool for writing tests.

=head1 SYNOPSIS

  sweb.pl SOURCE...

=cut

use FindBin;
use lib $FindBin::Dir;
use Lexer;
use Parser;

my @trees;

foreach my $source (@ARGV) {
    open(my $fh, '<:utf8', $source)
      or die "open(< $source): $!";

    my $lexer = new Lexer($source, $fh);
    my $parser = new Parser;
    my $tree = $parser->run($lexer);
    die "$source: parse failed" unless defined $tree;
    push @trees, $tree;

    close($fh)
      or die "close(< $source): $!";
}

if (0) {
    use Data::Dumper;
    $Data::Dumper::Indent = 1;
    print Dumper(\@trees);
    exit;
}

if (0) {
    sub process {
        my ($children, $func) = @_;

        foreach my $c (@$children) {
            if (ref $c) {
                $c->apply($func);
            } else {
                print $c;
            }
        }
    }

    $trees[0]->apply(\&process);
    exit;
}

use Tangle;

my $tangle = new Tangle;
$tangle->init(\@trees);
$tangle->eval('@eval');

#! /usr/bin/perl
# -*- cperl -*-
#
use warnings;
use strict;

=head1 NAME

xmlsh.pl - interactively edit XML file in a shell-like fasion

=head1 SYNOPSIS

  xmlsh.pl XML

=head2 BUGS

The script is a quick-n-dirty solution.

=cut

use Pod::Usage;

my %option;
if (@ARGV != 1) {
    pod2usage(1);
}

my $xml_file = shift @ARGV;


use XML::Twig;

my $xml = new XML::Twig(comments => 'drop',
                        pretty_print => 'indented');
$xml->parsefile($xml_file);
our $node = $xml->root;

my %command;


sub help {
    my ($cmd) = @_;

    if (defined $cmd) {
    } else {
        print join("\n", sort keys %command), "\n";
    }
}

sub cd {
    my ($xpath) = @_;

    die "Usage: cd XPATH\n"
      unless defined $xpath;

    if ($xpath eq '/') {
        $node = $xml->root;
    } else {
        my @nodes = $node->get_xpath($xpath);
        die "More than one node matches the path $xpath\n" if @nodes > 1;
        die "No node matches the path $xpath\n" if @nodes != 1;
        $node = $nodes[0];
    }
}

sub ls {
    my ($xpath) = @_;

    foreach my $c ($node->children($xpath)) {
        my $name = $c->xpath;
        my $xpath = $node->xpath;
        $name =~ s|^\Q$xpath/\E||;
        print $name, "\n";
    }
}

sub rm {
    my ($xpath) = @_;

    foreach my $c ($node->children($xpath)) {
        $c->delete;
    }
}

sub mk {
    my ($args) = @_;

    my ($xpath, $before) = split /\s+/, $args, 2;

    if (defined $before) {
        $node->new($xpath)->paste(before => $node->first_child($before));
    } else {
        $node->new($xpath)->paste(last_child => $node);
    }
}

sub cat {
    my ($xpath) = @_;

    if (defined $xpath) {
        foreach my $c ($node->children($xpath)) {
            $c->print;
        }
    } else {
        $node->print;
    }
    print "\n";

}

sub for_each {
    my ($args) = @_;

    my ($xpath, $cmd, $arg) = $args =~ /^(.+)\s+do\s+(\S+)(?:\s+(.+))?/;

    die "Unknown command\n" unless exists $command{$cmd};
    my $sub = $command{$cmd};

    my @nodes = $node->get_xpath($xpath);

    local $node;
    foreach $node (@nodes) {
        $sub->($arg);
    }
}

sub repeat {
    my ($args) = @_;

    my ($count, $cmd, $arg) = $args =~ /^(\d+)\s+do\s+(\S+)(?:\s+(.+))?/;

    die "Unknown command\n" unless exists $command{$cmd};
    my $sub = $command{$cmd};

    for(my $i = 0; $i < $count; ++$i) {
        $sub->($arg);
    }
}

sub save {
    my ($file) = @_;

    $file = $xml_file unless defined $file;

    open(my $fh, '>', $file)
      or die "open(> $file): $!\n";

    $xml->print($fh);

    close($fh)
      or die "close($file): $!\n";
}


%command = (
    help => \&help,
    cd => \&cd,
    pwd => sub { print $node->xpath, "\n" },
    rm => \&rm,
    ls => \&ls,
    cat => \&cat,
    set => sub { $node->set_att(split '=', $_[0], 2) },
    unset => sub { $node->del_att(split(/\s+/, $_[0])) },
    get => sub { print $node->att($_[0]) },
    mk => \&mk,
    exit => sub { exit },
    print => sub { $xml->print },
    foreach => \&for_each,
    save => \&save,
    repeat => \&repeat,
);


$| = 1;

print "xmlsh> " if -t \*STDIN;
while (defined (my $line = <STDIN>)) {
    next if $line =~ /^\s*(?:#|$)/;

    $line =~ s/\s+$//;
    my ($cmd, $args) = split(/\s+/, $line, 2);

    eval {
        die "Unknown command\n" unless exists $command{$cmd};
        $command{$cmd}->($args);
    };
    print $@ if $@;
} continue {
    print "xmlsh> " if -t \*STDIN;
}

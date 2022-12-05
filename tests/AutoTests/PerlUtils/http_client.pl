#! /usr/bin/perl
#
# Simple interactive HTTP client.  Example session:
#
#   get http://...
#   show headers
#   show body
#   show cookies
#
use warnings;
use strict;


use FindBin;
use Term::ReadLine;

use LWP::UserAgent;
use HTTP::Cookies;
use URI::Escape;


sub execute_command {
    my ($map, $line) = @_;

    unless (defined $line && $line =~ /\w/) {
        print "Undefined command\n";
        return;
    }

    $line =~ s/^\s+//;
    my ($cmd, $args) = split(/\s+/, $line, 2);

    if (exists $map->{$cmd}) {
        $map->{$cmd}->($args);
    } else {
        print "Unknown command '$cmd'\n";
    }
}


sub print_help {
    my ($msg, $map) = @_;

    print $msg, join(", ", map { "'$_'" } sort keys %$map), ".\n";
}


my $term = Term::ReadLine->new($FindBin::Bin);

use LWP::Protocol::http ();
@LWP::Protocol::http::EXTRA_SOCK_OPTS = (MaxLineLength => 4096 * 10);

my $cookie_jar = new HTTP::Cookies;
my $ua = new LWP::UserAgent(cookie_jar => $cookie_jar);
$ua->env_proxy;
$ua->requests_redirectable([]);

my ($request, $response);
my %command;
my %show;

%command = (
    help => sub { print_help("Commands are ", \%command) },
    get => sub { $request = new HTTP::Request(GET => $_[0]);
                 $response = $ua->request($request);
                 print $response->status_line, "\n" },
    show => sub { execute_command(\%show, $_[0]) },
    clear => sub { $cookie_jar->clear },
    encode => sub { print uri_escape($_[0]), "\n" },
    decode => sub { print uri_unescape($_[0]), "\n" },
);

%show = (
    help => sub { print_help("Subcommands are ", \%show) },
    status => sub { print $response->status_line, "\n" if $response },
    headers => sub { print $response->headers->as_string if $response },
    body => sub { print $response->content if $response },
    cookies => sub { print $cookie_jar->as_string },
    request => sub { print $request->as_string },
);


while (1) {
    my $line = $term->readline("> ");

    last unless defined $line;
    next if $line =~ /^\s*$/;

    execute_command(\%command, $line);
}

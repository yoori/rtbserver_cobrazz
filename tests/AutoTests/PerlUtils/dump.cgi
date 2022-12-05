#! /usr/bin/perl
#
use warnings;
use strict;


use CGI;
use CGI::Carp qw (fatalsToBrowser);

my $q = new CGI;
if ($q->cgi_error) {
    print $q->header(-status => $q->cgi_error);

    exit(0);
}

my $params = $q->Vars;
if (%$params) {
    if ($params->{action} eq 'Add table') {
        ++$params->{table_count};
    } elsif ($params->{action} eq 'Dump') {
        my $cmd = '/home/taskbot/taskbot/repo/foros/server/'
                . 'tests/AutoTests/PerlUtils/db_dump.pl';
        (my $cmd_short = $cmd) =~ s|.*/||;
        my @args = map({ ("--$_", $params->{$_}) }
                       qw(host db user password namespace prefix test));
        for (my $i = 0; $i < $params->{table_count}; ++$i) {
            my $table = $params->{"table$i"};
            my $where = $params->{"where$i"};
            push @args, '--table', "$table=$where";
        }

        # Oracle environment.
        $ENV{NLS_NCHAR} = 'AL32UTF8';
        $ENV{NLS_LANG} = '.AL32UTF8';
        $ENV{LD_LIBRARY_PATH} = '/opt/oracle/product/10g/instantclient/lib/';

        my $error_msg =
            "Run the command from command line to learn the exact error\n";
        open(my $p, '-|', $cmd, @args)
            or die "open(-| $cmd_short @args): $!\n$error_msg";
        $/ = '';
        my $output = <$p>;
        close($p)
            or die "close(-| $cmd_short @args): $! (status word $?)\n"
                 . $error_msg;

        if ($output =~ m|<struct>\n</struct>|m) {
            print $q->header(-type => 'text/html; charset=utf-8');
            print $q->start_html('Empty result');
            print "No row has matched your constraints.";
            print $q->end_html;

            exit(0);
        }

        delete $params->{action};
        my $cookie = $q->cookie(-name => 'params',
                                -value => $params,
                                -path => $q->url(-absolute => 1),
                                -expires => '+3M');

        print $q->header(-type => 'text/xml; charset=utf-8',
                         -cookie => $cookie);
        print $output;

        exit(0);
    } else {
        my $shift;
        for (my $i = 0; $i < $params->{table_count}; ++$i) {
            $params->{"table@{[$i-1]}"} = $params->{"table$i"} if $shift;
            $params->{"where@{[$i-1]}"} = $params->{"where$i"} if $shift;
            $shift = 1 if not $shift and $params->{"remove$i"} eq 'Remove';
        }
        --$params->{table_count} if $shift;
    }
} else {
    %$params = $q->cookie('params');
    if (%$params) {
#        print $q->redirect;
#
#        exit(0);
    } else {
        $params->{table_count} = 0;
        $params->{host} = 'stat-dev1';
        $params->{db} = 'ads_dev';
        $params->{password} = 'adserver';
        $params->{namespace} = 'UT';
        $params->{test} = '--';
    }
}

print $q->header(-type => 'text/html; charset=utf-8');
print $q->start_html('DB dump');
print $q->h1('DB dump');
print $q->start_form(-method => 'GET');

print $q->hidden(-name => 'table_count');

print $q->start_table;
print $q->Tr([
    $q->td(["Host:",
            $q->textfield(-name => 'host',
                          -size => 80,
                          -maxsize => 80)]),
    $q->td(["Database:",
            $q->textfield(-name => 'db',
                          -size => 80,
                          -maxsize => 80)]),
    $q->td(["User:", $q->textfield(-name => 'user',
                                   -size => 80,
                                   -maxsize => 80)]),
    $q->td(["Password:", $q->textfield(-name => 'password',
                                       -size => 80,
                                       -maxsize => 80)]),
    $q->td([$q->br, $q->br]),
    $q->td(["Namespace:", $q->textfield(-name => 'namespace',
                                        -size => 80,
                                        -maxsize => 80)]),
    $q->td(["Prefix:", $q->textfield(-name => 'prefix',
                                     -size => 80,
                                     -maxsize => 80)]),
    $q->td(["Test:", $q->textfield(-name => 'test',
                                   -size => 80,
                                   -maxsize => 80),
            "('--' to disable)"])
             ]);

for (my $i = 0; $i < $params->{table_count}; ++$i) {
    my $table = $q->table($q->Tr($q->td([
                                     $q->textfield(-name => "table$i",
                                                   -size => 30,
                                                   -maxsize => 30),
                                     "where",
                                     $q->textfield(-name => "where$i",
                                                   -size => 40,
                                                   -maxsize => 500)])));

    print $q->Tr([$q->td(["Table:",
                          $table,
                          $q->submit(-name => "remove$i",
                                     -value => 'Remove')])]);
}
print $q->Tr($q->td(['Table:',
                     $q->submit(-name => 'action',
                                -value => 'Add table')]));

print $q->end_table;

print $q->br x 1;

print $q->submit(-name => 'action',
                 -value => 'Dump');

print $q->end_form;

print $q->end_html;

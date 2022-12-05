# -*- cperl -*-
#
package Taskbot::Base;

use warnings;
use strict;


use DBI;


sub do_file {
    my ($file) = @_;

    my $res = do $file;
    unless ($res) {
        die "While reading $file: $!" if ($!);
        die "While compiling $file: $@" if ($@);
        die "While executing $file: returned '"
          . (defined $res ? $res : "(undef)") . "'";
    }

    return $res;
}


use fields qw(config dbh);


sub new {
    my $self = shift;
    my ($config_file) = @_;

    my $config = do_file($config_file);

    my $dbh = DBI->connect(@{$config->{taskbot_dbi}}[0..2],
                           { AutoCommit => 0,
                             PrintError => 0, RaiseError => 1,
                             FetchHashKeyName => 'NAME_lc' });

    # The following makes '||' on MySQL to be concatenation and not 'OR'.
    eval {
        $dbh->do(q[
            SET SESSION SQL_MODE='PIPES_AS_CONCAT'
        ]);
    };

    unless (ref $self) {
        $self = fields::new($self);
    }

    $self->{config} = $config;
    $self->{dbh} = $dbh;

    return $self;
}


1;

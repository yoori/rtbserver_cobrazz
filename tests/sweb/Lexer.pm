# -*- cperl -*-
#
package Lexer;

use warnings;
use strict;


use fields qw(file fh lineno line lexer cache);


sub new {
    my $self = shift;
    my ($file, $fh) = @_;

    unless (ref $self) {
        $self = fields::new($self);
    }

    $self->{file} = $file;
    $self->{fh} = $fh;

    return $self;
}


sub get_lexer {
    my $self = shift;

    return sub { $self->_lexer(@_) };
}


sub where {
    my $self = shift;

    my @where = ($self->{file});
    if (defined $self->{line}) {
        push @where, $self->{lineno}, pos($self->{line});
    }

    return join(":", @where);
}


sub push {
    my $self = shift;
    my @tokens = @_;

    my $cache = \$self->{cache}->{join "\0", @tokens};

    if (defined $$cache) {
        push @{$self->{lexer}}, $$cache;
        return;
    }

    my $code = <<'    CODE';
        sub {
            for (${$self->_get_line}) {
                ! defined and return ('', undef);
    CODE
    while (my ($regex, $class) = splice(@tokens, 0, 2)) {
        $class =~ s/'/\\'/g;
        $code .= <<"        CODE";
                /\\G (?: $regex )/xsgc and return ('$class', \$&);
        CODE
    }
    $code .= <<'    CODE';
            }
            die "lexer failed at " . $self->where;
        }
    CODE

    my $sub = eval $code;
    die $@ if $@;

    $$cache = $sub;

    push @{$self->{lexer}}, $sub;
}


sub pop {
    my $self = shift;

    pop @{$self->{lexer}};
}


sub _lexer {
    my $self = shift;
#    my ($parser) = @_;

    return $self->{lexer}->[-1]->();
}


sub _get_line {
    my $self = shift;

    my $line = \$self->{line};
    unless (defined $$line && pos($$line) < length($$line)) {
        my $fh = $self->{fh};
        $$line = <$fh>;
        if (defined $$line) {
            ++$self->{lineno};
            pos($$line) = 0;
        }
    }

    return $line;
}


1;

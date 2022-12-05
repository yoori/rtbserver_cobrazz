# -*- cperl -*-
#
use warnings;
use strict;


package AST;

use fields qw(children);


sub new {
    my $self = shift;

    unless (ref $self) {
        $self = fields::new($self);
    }

    $self->add(@_);

    return $self;
}


sub add {
    my $self = shift;

    push @{$self->{children}}, @_;

    return $self;
}


sub normalize {
    my $self = shift;

    $self->apply(\&flatten);
    $self->apply(\&reassign_empty_lines);
    $self->apply(\&reassign_params);
    $self->apply(\&concat);
}


sub apply {
    my $self = shift;
    my ($func) = @_;

    $func->($self->{children}, $func);
}


sub flatten {
    my ($children, $func) = @_;

    @$children = grep { defined and length } @$children;

    for (my $i = 0; $i < @$children; ++$i) {
        my $ref = ref $children->[$i];

        if ($ref) {
            my $c = $children->[$i];
            if ($ref eq 'ARRAY') {
                flatten($c, $func);
                splice(@$children, $i, 1, @$c);
                $i += @$c - 1;
            } else {
                $children->[$i]->apply($func);
            }
        }
    }
}


sub reassign_empty_lines {
    my ($children, $func) = @_;

    for (my $i = 0; $i < @$children; ++$i) {
        next unless ref $children->[$i] && $children->[$i]->isa('Def');

        my $empty_lines = $children->[$i]->apply(sub {
            my ($children) = @_;

            foreach my $c (@$children) {
                next unless ref $c && $c->isa('Code');

                return $c->apply(sub {
                     my ($children) = @_;

                     my $split = @$children;
                     1 while (--$split >= 0
                              && ref $children->[$split]
                              && $children->[$split]->isa('EmptyLine'));
                     my @empty = splice(@$children, $split + 1);
                     return [ map { $_->apply(sub { @{$_[0]} }) } @empty ];
                });
            }

            return [];
        });

        splice(@$children, $i + 1, 0, @$empty_lines);
        $i += @$empty_lines;
    }
}


sub reassign_params {
    my ($children, $func) = @_;

    foreach my $c (@$children) {
        my $ref = ref $c;

        next unless $ref;

        if ($c->isa('Macro')) {
            $c->apply(sub {
                my ($children) = @_;

                for (my $i = 0; $i < @$children; ++$i) {
                    if (ref $children->[$i] && $children->[$i]->isa('Param')) {
                        my $param = splice(@$children, $i, 1);
                        $param->apply($func);
                        $children->[$i - 1]->add($param);
                        --$i;
                    }
                }
            });
        } elsif (grep { $c->isa($_) } qw(Def Code CodeLine)) {
            $c->apply($func);
        }
    }
}


sub concat {
    my ($children, $func) = @_;

    my @join;
    my $count = @$children;
    while ($count-- > 0) {
        my $c = shift @$children;
        if (ref $c) {
            push @$children, join('', @join) if @join;
            @join = ();

            $c->apply($func);
            push @$children, $c;
        } else {
            push @join, $c;
        }
    }
    push @$children, join('', @join) if @join;
}


1;


package Def;
use base 'AST';

sub macro {
    my $self = shift;

    $self->{children}->[0];
}

sub code {
    my $self = shift;

    $self->{children}->[2];
}

1;


package Macro;
use base 'AST';

use fields qw(where);

sub new {
    my $self = shift;
    my $where = shift;

    $self = $self->SUPER::new(@_);

    $self->{where} = $where;

    return $self;
}

sub where {
    my $self = shift;

    $self->{where};
}

sub name {
    my $self = shift;

    join(" ", map { $_->name } grep { ref } @{ $self->{children} });
}

sub signature {
    my $self = shift;

    join(" ", map { $_->signature } grep { ref } @{ $self->{children} });
}

sub args {
    my $self = shift;

    +{ map { $_->arg } grep { ref } @{ $self->{children} } };
}

1;


package Word;
use base 'AST';

sub name {
    my $self = shift;

    lc $self->{children}->[0];
}

sub signature {
    my $self = shift;

    lc $self->{children}->[0] . (exists $self->{children}->[1] ? ":?" : "");
}

sub arg {
    my $self = shift;

    if (exists $self->{children}->[1]) {
        ($self->name => $self->{children}->[1]->value);
    } else {
        ();
    }
}

1;


package Param;
use base 'AST';

sub value {
    my $self = shift;

    if (exists $self->{children}->[1]) {
        $self->{children}->[1];
    }
}

1;


package Code;
use base 'AST';
1;


package CodeLine;
use base 'AST';
1;


package EmptyLine;
use base 'AST';
1;


package EndCode;
use base 'AST';
1;

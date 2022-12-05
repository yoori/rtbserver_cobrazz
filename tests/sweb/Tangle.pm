# -*- cperl -*-
#
package Tangle;

use warnings;
use strict;

use fields qw(defines column loop head);


sub new {
    my $self = shift;

    unless (ref $self) {
        $self = fields::new($self);
    }

    return $self;
}


sub init {
    my $self = shift;
    my ($trees) = @_;

    my $accumulate_defines = sub {
        my ($children, $func) = @_;

        foreach my $c (@$children) {
            next unless ref $c;
            unless ($c->isa('Def')) {
                $c->apply($func);
                next;
            }

            my $macro = $c->macro;
            my $def = \$self->{defines}->{$macro->name};
            if (defined $$def) {
                match_signature($$def, $macro);
                match_file($$def, $macro);
            } else {
                $$def->{signature} = $macro->signature;
                $$def->{where} = $macro->where;
            }
            push @{ $$def->{code} }, $c->code;
        }
    };

    $accumulate_defines->($trees, $accumulate_defines);
}


sub match_file {
    my ($def, $macro) = @_;

    (my $def_file = $def->{where}) =~ s/:\d+:\d+$//;
    (my $macro_file = $macro->where) =~ s/:\d+:\d+$//;

    if ($def_file ne $macro_file) {
        die "macro '$def->{signature}' defined in more than one file,\n"
          . "  $def->{where}\n"
          . "  " . $macro->where . "\n";
    }
}


sub expand {
    my $self = shift;
    my ($fh, $name) = @_;

    die "No such macro '$name'" unless exists $self->{defines}->{lc $name};

    $self->{column} = 0;
    $self->expand_code($fh, {}, $self->{defines}->{lc $name}->{code});
    print $fh "\n";
}


sub eval {
    my $self = shift;
    my ($name) = @_;

    my $code = <<'EOF';
sub {
    my $sweb = shift;
EOF

    open(my $fh, '>>', \$code);
    $self->expand($fh, $name);
    close($fh);

    $code .= <<'EOF';
}
EOF

    my $sub = eval $code;
    die $@ if $@;

    $sub->($self);
}


sub match_signature {
    my ($def, $macro) = @_;

    if ($def->{signature} ne $macro->signature) {
        die "Signature differs:\n"
          . "  '$def->{signature}' in $def->{where}\n"
          . "  '@{[ $macro->signature ]}' in @{[ $macro->where ]}\n";
    }
}


sub expand_macro {
    my $self = shift;
    my ($fh, $locals, $macro) = @_;

    my $name = $macro->name;
    my $signature = $macro->signature;
    if (exists $locals->{$signature}) {
        $locals->{" context"}->($locals->{$signature});
    } elsif (exists $self->{defines}->{$name}) {
        my $def = $self->{defines}->{$name};

        match_signature($def, $macro);

        # Do not substitute $func in place where it is used: it will give
        #
        #   sub {
        #       f(sub {
        #           g(@_);
        #         });
        #   }
        #
        # Perl 5.8 will (incorrectly) bind @_ to the outer sub, while
        # Perl 5.10 will bind it to the inner sub, which is expected.
        #
        my $func = sub {
            $self->expand_code_line(1, $fh, $locals, @_)
        };
        my $args = $macro->args;
        $args->{" context"} = sub {
            $_[0]->apply($func)
        };

        $self->push_visited($macro);

        $self->expand_code($fh, $args, $def->{code});

        $self->pop_visited($macro);
    } else {
        die "Macro '$name' is not defined (used in @{[ $macro->where ]})\n";
    }
}


sub push_visited {
    my $self = shift;
    my ($macro) = @_;

    my $head = \$self->{loop}->{$macro};
    if (defined $$head) {
        my $msg = "Loop detected:";
        $msg .= "\n  '@{[ $macro->name ]}' in @{[ $macro->where ]}";
        $macro = $self->{head}->{macro};
        $msg .= "\n  '@{[ $macro->name ]}' in @{[ $macro->where ]}";
        while ($self->{head} != $$head) {
            $self->{head} = $self->{head}->{prev};
            $macro = $self->{head}->{macro};
            $msg .= "\n  '@{[ $macro->name ]}' in @{[ $macro->where ]}";
        }
        die "$msg\n";
    }
    $$head->{macro} = $macro;
    $$head->{prev} = $self->{head};
    $self->{head} = $$head;
}


sub pop_visited {
    my $self = shift;
    my ($macro) = @_;

    $self->{head} = $self->{head}->{prev};
    delete $self->{loop}->{$macro};
}


sub expand_code {
    my $self = shift;
    my ($fh, $locals, $chunks) = @_;

    my $indent = $self->{column};
    my $pre = '';
    foreach my $code (@$chunks) {
        $code->apply(sub {
            my ($children) = @_;

            foreach my $c (@$children) {
                next unless ref $c;

                if ($c->isa('CodeLine')) {
                    print $fh $pre;
                    $self->{column} = $indent;
                    $c->apply(sub {
                        $self->expand_code_line(0, $fh, $locals, @_)
                    });
                    $pre = "\n" . " " x $indent;
                } elsif ($c->isa('EmptyLine')) {
                    print $fh "\n";
                }
            }
        });

        $pre = "\n$pre";
    }
}


sub expand_code_line {
    my $self = shift;
    my ($multiline, $fh, $locals, $children) = @_;

    foreach my $c (@$children) {
        if (! ref $c) {
            print $fh $c;
            if ($multiline && $c =~ s/.*(?:\n|\r\n?)//s) {
                $self->{column} = length($c);
            } else {
                $self->{column} += length($c);
            }
        } elsif ($c->isa('Macro')) {
            $self->expand_macro($fh, $locals, $c);
        }
    }
}


1;

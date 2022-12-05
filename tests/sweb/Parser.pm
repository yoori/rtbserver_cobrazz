####################################################################
#
#    This file was generated using Parse::Yapp version 1.05.
#
#        Don't edit this file, use source file instead.
#
#             ANY CHANGE MADE HERE WILL BE LOST !
#
####################################################################
package Parser;
use vars qw ( @ISA );
use strict;

@ISA= qw ( Parse::Yapp::Driver );
#Included Parse/Yapp/Driver.pm file----------------------------------------
{
#
# Module Parse::Yapp::Driver
#
# This module is part of the Parse::Yapp package available on your
# nearest CPAN
#
# Any use of this module in a standalone parser make the included
# text under the same copyright as the Parse::Yapp module itself.
#
# This notice should remain unchanged.
#
# (c) Copyright 1998-2001 Francois Desarmenien, all rights reserved.
# (see the pod text in Parse::Yapp module for use and distribution rights)
#

package Parse::Yapp::Driver;

require 5.004;

use strict;

use vars qw ( $VERSION $COMPATIBLE $FILENAME );

$VERSION = '1.05';
$COMPATIBLE = '0.07';
$FILENAME=__FILE__;

use Carp;

#Known parameters, all starting with YY (leading YY will be discarded)
my(%params)=(YYLEX => 'CODE', 'YYERROR' => 'CODE', YYVERSION => '',
			 YYRULES => 'ARRAY', YYSTATES => 'ARRAY', YYDEBUG => '');
#Mandatory parameters
my(@params)=('LEX','RULES','STATES');

sub new {
    my($class)=shift;
	my($errst,$nberr,$token,$value,$check,$dotpos);
    my($self)={ ERROR => \&_Error,
				ERRST => \$errst,
                NBERR => \$nberr,
				TOKEN => \$token,
				VALUE => \$value,
				DOTPOS => \$dotpos,
				STACK => [],
				DEBUG => 0,
				CHECK => \$check };

	_CheckParams( [], \%params, \@_, $self );

		exists($$self{VERSION})
	and	$$self{VERSION} < $COMPATIBLE
	and	croak "Yapp driver version $VERSION ".
			  "incompatible with version $$self{VERSION}:\n".
			  "Please recompile parser module.";

        ref($class)
    and $class=ref($class);

    bless($self,$class);
}

sub YYParse {
    my($self)=shift;
    my($retval);

	_CheckParams( \@params, \%params, \@_, $self );

	if($$self{DEBUG}) {
		_DBLoad();
		$retval = eval '$self->_DBParse()';#Do not create stab entry on compile
        $@ and die $@;
	}
	else {
		$retval = $self->_Parse();
	}
    $retval
}

sub YYData {
	my($self)=shift;

		exists($$self{USER})
	or	$$self{USER}={};

	$$self{USER};
	
}

sub YYErrok {
	my($self)=shift;

	${$$self{ERRST}}=0;
    undef;
}

sub YYNberr {
	my($self)=shift;

	${$$self{NBERR}};
}

sub YYRecovering {
	my($self)=shift;

	${$$self{ERRST}} != 0;
}

sub YYAbort {
	my($self)=shift;

	${$$self{CHECK}}='ABORT';
    undef;
}

sub YYAccept {
	my($self)=shift;

	${$$self{CHECK}}='ACCEPT';
    undef;
}

sub YYError {
	my($self)=shift;

	${$$self{CHECK}}='ERROR';
    undef;
}

sub YYSemval {
	my($self)=shift;
	my($index)= $_[0] - ${$$self{DOTPOS}} - 1;

		$index < 0
	and	-$index <= @{$$self{STACK}}
	and	return $$self{STACK}[$index][1];

	undef;	#Invalid index
}

sub YYCurtok {
	my($self)=shift;

        @_
    and ${$$self{TOKEN}}=$_[0];
    ${$$self{TOKEN}};
}

sub YYCurval {
	my($self)=shift;

        @_
    and ${$$self{VALUE}}=$_[0];
    ${$$self{VALUE}};
}

sub YYExpect {
    my($self)=shift;

    keys %{$self->{STATES}[$self->{STACK}[-1][0]]{ACTIONS}}
}

sub YYLexer {
    my($self)=shift;

	$$self{LEX};
}


#################
# Private stuff #
#################


sub _CheckParams {
	my($mandatory,$checklist,$inarray,$outhash)=@_;
	my($prm,$value);
	my($prmlst)={};

	while(($prm,$value)=splice(@$inarray,0,2)) {
        $prm=uc($prm);
			exists($$checklist{$prm})
		or	croak("Unknow parameter '$prm'");
			ref($value) eq $$checklist{$prm}
		or	croak("Invalid value for parameter '$prm'");
        $prm=unpack('@2A*',$prm);
		$$outhash{$prm}=$value;
	}
	for (@$mandatory) {
			exists($$outhash{$_})
		or	croak("Missing mandatory parameter '".lc($_)."'");
	}
}

sub _Error {
	print "Parse error.\n";
}

sub _DBLoad {
	{
		no strict 'refs';

			exists(${__PACKAGE__.'::'}{_DBParse})#Already loaded ?
		and	return;
	}
	my($fname)=__FILE__;
	my(@drv);
	open(DRV,"<$fname") or die "Report this as a BUG: Cannot open $fname";
	while(<DRV>) {
                	/^\s*sub\s+_Parse\s*{\s*$/ .. /^\s*}\s*#\s*_Parse\s*$/
        	and     do {
                	s/^#DBG>//;
                	push(@drv,$_);
        	}
	}
	close(DRV);

	$drv[0]=~s/_P/_DBP/;
	eval join('',@drv);
}

#Note that for loading debugging version of the driver,
#this file will be parsed from 'sub _Parse' up to '}#_Parse' inclusive.
#So, DO NOT remove comment at end of sub !!!
sub _Parse {
    my($self)=shift;

	my($rules,$states,$lex,$error)
     = @$self{ 'RULES', 'STATES', 'LEX', 'ERROR' };
	my($errstatus,$nberror,$token,$value,$stack,$check,$dotpos)
     = @$self{ 'ERRST', 'NBERR', 'TOKEN', 'VALUE', 'STACK', 'CHECK', 'DOTPOS' };

#DBG>	my($debug)=$$self{DEBUG};
#DBG>	my($dbgerror)=0;

#DBG>	my($ShowCurToken) = sub {
#DBG>		my($tok)='>';
#DBG>		for (split('',$$token)) {
#DBG>			$tok.=		(ord($_) < 32 or ord($_) > 126)
#DBG>					?	sprintf('<%02X>',ord($_))
#DBG>					:	$_;
#DBG>		}
#DBG>		$tok.='<';
#DBG>	};

	$$errstatus=0;
	$$nberror=0;
	($$token,$$value)=(undef,undef);
	@$stack=( [ 0, undef ] );
	$$check='';

    while(1) {
        my($actions,$act,$stateno);

        $stateno=$$stack[-1][0];
        $actions=$$states[$stateno];

#DBG>	print STDERR ('-' x 40),"\n";
#DBG>		$debug & 0x2
#DBG>	and	print STDERR "In state $stateno:\n";
#DBG>		$debug & 0x08
#DBG>	and	print STDERR "Stack:[".
#DBG>					 join(',',map { $$_[0] } @$stack).
#DBG>					 "]\n";


        if  (exists($$actions{ACTIONS})) {

				defined($$token)
            or	do {
				($$token,$$value)=&$lex($self);
#DBG>				$debug & 0x01
#DBG>			and	print STDERR "Need token. Got ".&$ShowCurToken."\n";
			};

            $act=   exists($$actions{ACTIONS}{$$token})
                    ?   $$actions{ACTIONS}{$$token}
                    :   exists($$actions{DEFAULT})
                        ?   $$actions{DEFAULT}
                        :   undef;
        }
        else {
            $act=$$actions{DEFAULT};
#DBG>			$debug & 0x01
#DBG>		and	print STDERR "Don't need token.\n";
        }

            defined($act)
        and do {

                $act > 0
            and do {        #shift

#DBG>				$debug & 0x04
#DBG>			and	print STDERR "Shift and go to state $act.\n";

					$$errstatus
				and	do {
					--$$errstatus;

#DBG>					$debug & 0x10
#DBG>				and	$dbgerror
#DBG>				and	$$errstatus == 0
#DBG>				and	do {
#DBG>					print STDERR "**End of Error recovery.\n";
#DBG>					$dbgerror=0;
#DBG>				};
				};


                push(@$stack,[ $act, $$value ]);

					$$token ne ''	#Don't eat the eof
				and	$$token=$$value=undef;
                next;
            };

            #reduce
            my($lhs,$len,$code,@sempar,$semval);
            ($lhs,$len,$code)=@{$$rules[-$act]};

#DBG>			$debug & 0x04
#DBG>		and	$act
#DBG>		and	print STDERR "Reduce using rule ".-$act." ($lhs,$len): ";

                $act
            or  $self->YYAccept();

            $$dotpos=$len;

                unpack('A1',$lhs) eq '@'    #In line rule
            and do {
                    $lhs =~ /^\@[0-9]+\-([0-9]+)$/
                or  die "In line rule name '$lhs' ill formed: ".
                        "report it as a BUG.\n";
                $$dotpos = $1;
            };

            @sempar =       $$dotpos
                        ?   map { $$_[1] } @$stack[ -$$dotpos .. -1 ]
                        :   ();

            $semval = $code ? &$code( $self, @sempar )
                            : @sempar ? $sempar[0] : undef;

            splice(@$stack,-$len,$len);

                $$check eq 'ACCEPT'
            and do {

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Accept.\n";

				return($semval);
			};

                $$check eq 'ABORT'
            and	do {

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Abort.\n";

				return(undef);

			};

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Back to state $$stack[-1][0], then ";

                $$check eq 'ERROR'
            or  do {
#DBG>				$debug & 0x04
#DBG>			and	print STDERR 
#DBG>				    "go to state $$states[$$stack[-1][0]]{GOTOS}{$lhs}.\n";

#DBG>				$debug & 0x10
#DBG>			and	$dbgerror
#DBG>			and	$$errstatus == 0
#DBG>			and	do {
#DBG>				print STDERR "**End of Error recovery.\n";
#DBG>				$dbgerror=0;
#DBG>			};

			    push(@$stack,
                     [ $$states[$$stack[-1][0]]{GOTOS}{$lhs}, $semval ]);
                $$check='';
                next;
            };

#DBG>			$debug & 0x04
#DBG>		and	print STDERR "Forced Error recovery.\n";

            $$check='';

        };

        #Error
            $$errstatus
        or   do {

            $$errstatus = 1;
            &$error($self);
                $$errstatus # if 0, then YYErrok has been called
            or  next;       # so continue parsing

#DBG>			$debug & 0x10
#DBG>		and	do {
#DBG>			print STDERR "**Entering Error recovery.\n";
#DBG>			++$dbgerror;
#DBG>		};

            ++$$nberror;

        };

			$$errstatus == 3	#The next token is not valid: discard it
		and	do {
				$$token eq ''	# End of input: no hope
			and	do {
#DBG>				$debug & 0x10
#DBG>			and	print STDERR "**At eof: aborting.\n";
				return(undef);
			};

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Dicard invalid token ".&$ShowCurToken.".\n";

			$$token=$$value=undef;
		};

        $$errstatus=3;

		while(	  @$stack
			  and (		not exists($$states[$$stack[-1][0]]{ACTIONS})
			        or  not exists($$states[$$stack[-1][0]]{ACTIONS}{error})
					or	$$states[$$stack[-1][0]]{ACTIONS}{error} <= 0)) {

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Pop state $$stack[-1][0].\n";

			pop(@$stack);
		}

			@$stack
		or	do {

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**No state left on stack: aborting.\n";

			return(undef);
		};

		#shift the error token

#DBG>			$debug & 0x10
#DBG>		and	print STDERR "**Shift \$error token and go to state ".
#DBG>						 $$states[$$stack[-1][0]]{ACTIONS}{error}.
#DBG>						 ".\n";

		push(@$stack, [ $$states[$$stack[-1][0]]{ACTIONS}{error}, undef ]);

    }

    #never reached
	croak("Error in driver logic. Please, report it as a BUG");

}#_Parse
#DO NOT remove comment

1;

}
#End of include--------------------------------------------------




sub new {
        my($class)=shift;
        ref($class)
    and $class=ref($class);

    my($self)=$class->SUPER::new( yyversion => '1.05',
                                  yystates =>
[
	{#State 0
		ACTIONS => {
			'CHAR' => 1
		},
		DEFAULT => -4,
		GOTOS => {
			'input' => 2,
			'text' => 3,
			'doc' => 4,
			'start' => 5
		}
	},
	{#State 1
		DEFAULT => -7,
		GOTOS => {
			'text_maybe' => 6
		}
	},
	{#State 2
		ACTIONS => {
			"<<MACRO" => -9
		},
		DEFAULT => -1,
		GOTOS => {
			'@1-0' => 7,
			'def' => 8
		}
	},
	{#State 3
		DEFAULT => -5
	},
	{#State 4
		DEFAULT => -2
	},
	{#State 5
		ACTIONS => {
			'' => 9
		}
	},
	{#State 6
		ACTIONS => {
			'CHAR' => 10
		},
		DEFAULT => -6
	},
	{#State 7
		ACTIONS => {
			"<<MACRO" => 11
		},
		GOTOS => {
			'macro' => 12
		}
	},
	{#State 8
		ACTIONS => {
			'CHAR' => 1
		},
		DEFAULT => -4,
		GOTOS => {
			'text' => 3,
			'doc' => 13
		}
	},
	{#State 9
		DEFAULT => 0
	},
	{#State 10
		DEFAULT => -8
	},
	{#State 11
		DEFAULT => -13,
		GOTOS => {
			'@4-1' => 14
		}
	},
	{#State 12
		DEFAULT => -10,
		GOTOS => {
			'@2-2' => 15
		}
	},
	{#State 13
		DEFAULT => -3
	},
	{#State 14
		DEFAULT => -15,
		GOTOS => {
			'whitespace_maybe' => 16
		}
	},
	{#State 15
		ACTIONS => {
			"=" => 17
		}
	},
	{#State 16
		ACTIONS => {
			'CHAR' => 1,
			'WHITESPACE' => 21
		},
		GOTOS => {
			'word' => 18,
			'text' => 19,
			'macro_name' => 20
		}
	},
	{#State 17
		DEFAULT => -11,
		GOTOS => {
			'@3-4' => 22
		}
	},
	{#State 18
		DEFAULT => -18
	},
	{#State 19
		DEFAULT => -20
	},
	{#State 20
		ACTIONS => {
			":" => 23,
			'WHITESPACE' => 28
		},
		DEFAULT => -23,
		GOTOS => {
			'whitespace' => 24,
			'param_whitespace' => 25,
			'param_maybe' => 27,
			'param' => 26
		}
	},
	{#State 21
		DEFAULT => -16
	},
	{#State 22
		DEFAULT => -33,
		GOTOS => {
			'@8-0' => 29,
			'code' => 30
		}
	},
	{#State 23
		DEFAULT => -25,
		GOTOS => {
			'@5-1' => 31
		}
	},
	{#State 24
		DEFAULT => -22
	},
	{#State 25
		ACTIONS => {
			'CHAR' => 1
		},
		DEFAULT => -24,
		GOTOS => {
			'word' => 32,
			'text' => 19
		}
	},
	{#State 26
		DEFAULT => -15,
		GOTOS => {
			'whitespace_maybe' => 33
		}
	},
	{#State 27
		ACTIONS => {
			"MACRO>>" => 34
		}
	},
	{#State 28
		DEFAULT => -15,
		GOTOS => {
			'whitespace_maybe' => 35
		}
	},
	{#State 29
		DEFAULT => -15,
		GOTOS => {
			'whitespace_maybe' => 36
		}
	},
	{#State 30
		DEFAULT => -12
	},
	{#State 31
		ACTIONS => {
			"?" => 37
		},
		DEFAULT => -28,
		GOTOS => {
			'arg' => 38,
			'@6-0' => 39
		}
	},
	{#State 32
		DEFAULT => -19
	},
	{#State 33
		ACTIONS => {
			'WHITESPACE' => 21
		},
		DEFAULT => -21
	},
	{#State 34
		DEFAULT => -14
	},
	{#State 35
		ACTIONS => {
			'WHITESPACE' => 21
		},
		DEFAULT => -17
	},
	{#State 36
		ACTIONS => {
			'WHITESPACE' => 21
		},
		DEFAULT => -34,
		GOTOS => {
			'@9-2' => 40
		}
	},
	{#State 37
		DEFAULT => -27
	},
	{#State 38
		DEFAULT => -26
	},
	{#State 39
		ACTIONS => {
			"<VALUE" => 41
		}
	},
	{#State 40
		ACTIONS => {
			'CHAR' => 1,
			"<<MACRO" => 11,
			'NEWLINE' => 46
		},
		GOTOS => {
			'code_line' => 42,
			'text' => 43,
			'code_block' => 45,
			'macro' => 44
		}
	},
	{#State 41
		DEFAULT => -29,
		GOTOS => {
			'@7-2' => 47
		}
	},
	{#State 42
		ACTIONS => {
			'NEWLINE' => 48
		}
	},
	{#State 43
		DEFAULT => -42,
		GOTOS => {
			'code_line_rest' => 49
		}
	},
	{#State 44
		DEFAULT => -7,
		GOTOS => {
			'text_maybe' => 50
		}
	},
	{#State 45
		DEFAULT => -35
	},
	{#State 46
		DEFAULT => -38,
		GOTOS => {
			'empty_lines_maybe' => 51
		}
	},
	{#State 47
		DEFAULT => -7,
		GOTOS => {
			'text_maybe' => 52,
			'value' => 53
		}
	},
	{#State 48
		DEFAULT => -36
	},
	{#State 49
		ACTIONS => {
			"<<MACRO" => 11
		},
		DEFAULT => -40,
		GOTOS => {
			'macro' => 54
		}
	},
	{#State 50
		ACTIONS => {
			'CHAR' => 10
		},
		DEFAULT => -42,
		GOTOS => {
			'code_line_rest' => 55
		}
	},
	{#State 51
		ACTIONS => {
			'CHAR' => 1,
			'ENDCODE' => 56,
			"<<MACRO" => 11,
			'NEWLINE' => 62,
			'WHITESPACE' => 57
		},
		GOTOS => {
			'endcode' => 59,
			'code_indented' => 58,
			'code_line' => 60,
			'text' => 43,
			'macro' => 44,
			'code_multiline' => 61
		}
	},
	{#State 52
		ACTIONS => {
			'CHAR' => 10
		},
		DEFAULT => -31
	},
	{#State 53
		ACTIONS => {
			"VALUE>" => 63,
			"<<MACRO" => 11
		},
		GOTOS => {
			'macro' => 64
		}
	},
	{#State 54
		DEFAULT => -7,
		GOTOS => {
			'text_maybe' => 65
		}
	},
	{#State 55
		ACTIONS => {
			"<<MACRO" => 11
		},
		DEFAULT => -41,
		GOTOS => {
			'macro' => 54
		}
	},
	{#State 56
		ACTIONS => {
			'NEWLINE' => 66
		}
	},
	{#State 57
		ACTIONS => {
			'CHAR' => 1,
			"<<MACRO" => 11
		},
		GOTOS => {
			'code_line' => 67,
			'text' => 43,
			'macro' => 44
		}
	},
	{#State 58
		ACTIONS => {
			'ENDCODE' => 56,
			'NEWLINE' => 71,
			'WHITESPACE' => 69
		},
		DEFAULT => -48,
		GOTOS => {
			'endcode' => 70,
			'endcode_maybe' => 68
		}
	},
	{#State 59
		DEFAULT => -44
	},
	{#State 60
		ACTIONS => {
			'NEWLINE' => 72
		}
	},
	{#State 61
		DEFAULT => -37
	},
	{#State 62
		DEFAULT => -39
	},
	{#State 63
		DEFAULT => -30
	},
	{#State 64
		DEFAULT => -7,
		GOTOS => {
			'text_maybe' => 73
		}
	},
	{#State 65
		ACTIONS => {
			'CHAR' => 10
		},
		DEFAULT => -43
	},
	{#State 66
		DEFAULT => -47
	},
	{#State 67
		ACTIONS => {
			'NEWLINE' => 74
		}
	},
	{#State 68
		DEFAULT => -46
	},
	{#State 69
		ACTIONS => {
			'CHAR' => 1,
			"<<MACRO" => 11
		},
		GOTOS => {
			'code_line' => 75,
			'text' => 43,
			'macro' => 44
		}
	},
	{#State 70
		DEFAULT => -49
	},
	{#State 71
		DEFAULT => -54
	},
	{#State 72
		DEFAULT => -50,
		GOTOS => {
			'code_maybe' => 76
		}
	},
	{#State 73
		ACTIONS => {
			'CHAR' => 10
		},
		DEFAULT => -32
	},
	{#State 74
		DEFAULT => -53
	},
	{#State 75
		ACTIONS => {
			'NEWLINE' => 77
		}
	},
	{#State 76
		ACTIONS => {
			'ENDCODE' => 56,
			'NEWLINE' => 80
		},
		DEFAULT => -15,
		GOTOS => {
			'endcode' => 79,
			'whitespace_maybe' => 78
		}
	},
	{#State 77
		DEFAULT => -55
	},
	{#State 78
		ACTIONS => {
			'CHAR' => 1,
			"<<MACRO" => 11,
			'WHITESPACE' => 21
		},
		GOTOS => {
			'code_line' => 81,
			'text' => 43,
			'macro' => 44
		}
	},
	{#State 79
		DEFAULT => -45
	},
	{#State 80
		DEFAULT => -51
	},
	{#State 81
		ACTIONS => {
			'NEWLINE' => 82
		}
	},
	{#State 82
		DEFAULT => -52
	}
],
                                  yyrules  =>
[
	[#Rule 0
		 '$start', 2, undef
	],
	[#Rule 1
		 'start', 1,
sub
#line 14 "Parser.yp"
{ new AST($_[1]) }
	],
	[#Rule 2
		 'input', 1, undef
	],
	[#Rule 3
		 'input', 3,
sub
#line 20 "Parser.yp"
{ push @{$_[1]}, @_[2,3]; $_[1] }
	],
	[#Rule 4
		 'doc', 0, undef
	],
	[#Rule 5
		 'doc', 1, undef
	],
	[#Rule 6
		 'text', 2,
sub
#line 30 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 7
		 'text_maybe', 0, undef
	],
	[#Rule 8
		 'text_maybe', 2,
sub
#line 36 "Parser.yp"
{ push @{$_[1]}, $_[2]; $_[1] }
	],
	[#Rule 9
		 '@1-0', 0,
sub
#line 40 "Parser.yp"
{ push @{ $_[0]->YYData->{inside} },
           "define started at " . $_[0]->YYData->{lexer}->where }
	],
	[#Rule 10
		 '@2-2', 0,
sub
#line 43 "Parser.yp"
{ $_[0]->YYData->{lexer}->push(
          '='                    => '=',
          '\s'                   => 'WHITESPACE',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 11
		 '@3-4', 0,
sub
#line 49 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;
      $_[0]->YYData->{in_code} = 1 }
	],
	[#Rule 12
		 'def', 6,
sub
#line 52 "Parser.yp"
{ pop @{ $_[0]->YYData->{inside} };
      $_[0]->YYData->{in_code} = 0;
      new Def(@_[2,4,6]) }
	],
	[#Rule 13
		 '@4-1', 0,
sub
#line 59 "Parser.yp"
{ push @{ $_[0]->YYData->{inside} },
           "macro started at " . $_[0]->YYData->{lexer}->where;

      my $c;
      if ($_[0]->YYData->{in_code}) {
          $c = $_[0]->YYData->{macro_close};
      } else {
          my $o = "\\" . substr($_[1], -1, 1);
          $c = open2close($o);

          $_[0]->YYData->{macro_open} = $o;
          $_[0]->YYData->{macro_close} = $c;
      }

      $_[0]->YYData->{lexer}->push(
          "$c>"                  => 'MACRO>>',
          ':'                    => ':',
          '\?'                   => '?',
          '\n | \r\n? | [ \t]+'  => 'WHITESPACE',
          "[^$c:?\\n\\r \\t]+"   => 'CHAR',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 14
		 'macro', 6,
sub
#line 82 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;
      new Macro(pop @{ $_[0]->YYData->{inside} },
                @_[1,3,4,5,6]) }
	],
	[#Rule 15
		 'whitespace_maybe', 0, undef
	],
	[#Rule 16
		 'whitespace_maybe', 2,
sub
#line 90 "Parser.yp"
{ push @{$_[1]}, $_[2]; $_[1] }
	],
	[#Rule 17
		 'whitespace', 2,
sub
#line 95 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 18
		 'macro_name', 1,
sub
#line 100 "Parser.yp"
{ [ $_[1] ] }
	],
	[#Rule 19
		 'macro_name', 3,
sub
#line 102 "Parser.yp"
{ push @{$_[1]}, @_[2,3]; $_[1] }
	],
	[#Rule 20
		 'word', 1,
sub
#line 107 "Parser.yp"
{ new Word($_[1]) }
	],
	[#Rule 21
		 'param_whitespace', 2,
sub
#line 112 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 22
		 'param_whitespace', 1, undef
	],
	[#Rule 23
		 'param_maybe', 0, undef
	],
	[#Rule 24
		 'param_maybe', 1, undef
	],
	[#Rule 25
		 '@5-1', 0,
sub
#line 123 "Parser.yp"
{ my @value;
      if ($_[0]->YYData->{in_code}) {
          @value = ( '[[:punct:]]' => '<VALUE' );
      } else {
          @value = ( '\?' => '?' );
      }

      $_[0]->YYData->{lexer}->push(
          @value,
          '\s'                   => 'WHITESPACE',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 26
		 'param', 3,
sub
#line 136 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;
      new Param(@_[1,3]) }
	],
	[#Rule 27
		 'arg', 1, undef
	],
	[#Rule 28
		 '@6-0', 0,
sub
#line 142 "Parser.yp"
{ $_[0]->YYError unless $_[0]->YYData->{in_code} }
	],
	[#Rule 29
		 '@7-2', 0,
sub
#line 144 "Parser.yp"
{ push @{ $_[0]->YYData->{inside} },
           "value started at " . $_[0]->YYData->{lexer}->where;

      my $c = "\\" . open2close($_[2]);
      my $o = $_[0]->YYData->{macro_open};

      $_[0]->YYData->{lexer}->push(
          "<$o"                  => '<<MACRO',
          "$c"                   => 'VALUE>',
          "[^<$c]+"              => 'CHAR',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 30
		 'arg', 5,
sub
#line 157 "Parser.yp"
{ pop @{ $_[0]->YYData->{inside} };
      $_[0]->YYData->{lexer}->pop;
      [ $_[2], new CodeLine($_[4]), $_[5] ] }
	],
	[#Rule 31
		 'value', 1, undef
	],
	[#Rule 32
		 'value', 3,
sub
#line 165 "Parser.yp"
{ push @{$_[1]}, @_[2,3]; $_[1] }
	],
	[#Rule 33
		 '@8-0', 0,
sub
#line 169 "Parser.yp"
{ my $o = $_[0]->YYData->{macro_open};

      $_[0]->YYData->{lexer}->push(
          "<$o"                  => '<<MACRO',
          '[ \t]* (?:\n|\r\n?)'  => 'NEWLINE',
          '[ \t]+'               => 'WHITESPACE',
          '[^< \t\n\r]+'         => 'CHAR',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 34
		 '@9-2', 0,
sub
#line 179 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;

      my $o = $_[0]->YYData->{macro_open};
      my $c = $_[0]->YYData->{macro_close};
      $_[0]->YYData->{lexer}->push(
          "^\s*<$o$c>"           => 'ENDCODE',
          "<$o"                  => '<<MACRO',
          '[ \t]* (?:\n|\r\n?)'  => 'NEWLINE',
          '^[ ]*\t'              => 'TAB',
          '^[ ]+'                => 'WHITESPACE',
          '[^< \t\n\r]+'         => 'CHAR',
          '.'                    => 'CHAR'
      ) }
	],
	[#Rule 35
		 'code', 4,
sub
#line 193 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;
      new Code(@_[2,4]) }
	],
	[#Rule 36
		 'code_block', 2,
sub
#line 199 "Parser.yp"
{ [ new CodeLine($_[1]), $_[2] ] }
	],
	[#Rule 37
		 'code_block', 3,
sub
#line 201 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 38
		 'empty_lines_maybe', 0, undef
	],
	[#Rule 39
		 'empty_lines_maybe', 2,
sub
#line 207 "Parser.yp"
{ push @{$_[1]}, $_[2]; $_[1] }
	],
	[#Rule 40
		 'code_line', 2,
sub
#line 212 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 41
		 'code_line', 3,
sub
#line 214 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 42
		 'code_line_rest', 0, undef
	],
	[#Rule 43
		 'code_line_rest', 3,
sub
#line 220 "Parser.yp"
{ push @{$_[1]}, @_[2,3]; $_[1] }
	],
	[#Rule 44
		 'code_multiline', 1, undef
	],
	[#Rule 45
		 'code_multiline', 4,
sub
#line 226 "Parser.yp"
{ [ new CodeLine($_[1]), @_[2,3,4] ] }
	],
	[#Rule 46
		 'code_multiline', 2,
sub
#line 228 "Parser.yp"
{ shift; [@_] }
	],
	[#Rule 47
		 'endcode', 2,
sub
#line 233 "Parser.yp"
{ new EndCode(@_[1,2]) }
	],
	[#Rule 48
		 'endcode_maybe', 0, undef
	],
	[#Rule 49
		 'endcode_maybe', 1, undef
	],
	[#Rule 50
		 'code_maybe', 0, undef
	],
	[#Rule 51
		 'code_maybe', 2,
sub
#line 244 "Parser.yp"
{ push @{$_[1]}, new EmptyLine($_[2]); $_[1] }
	],
	[#Rule 52
		 'code_maybe', 4,
sub
#line 246 "Parser.yp"
{ push @{$_[1]}, new CodeLine(@_[2,3]), $_[4]; $_[1] }
	],
	[#Rule 53
		 'code_indented', 3,
sub
#line 251 "Parser.yp"
{ $_[0]->YYData->{lexer}->pop;

      my $indent = length($_[1]);
      $_[0]->YYData->{indent} = $indent;

      my $o = $_[0]->YYData->{macro_open};
      my $c = $_[0]->YYData->{macro_close};
      $_[0]->YYData->{lexer}->push(
          "^\s*<$o$c>"           => 'ENDCODE',
          "<$o"                  => '<<MACRO',
          '[ \t]* (?:\n|\r\n?)'  => 'NEWLINE',
          "^[ ]{$indent}[ \\t]*" => 'WHITESPACE',
          '^[ ]*\t'              => 'TAB',
          '^[ ]*<[[:punct:]]'    => '<<MACRO',  # Start of the *next* def.
          '[^< \t\n\r]+'         => 'CHAR',
          '.'                    => 'CHAR'
      );
      [ $_[1], new CodeLine($_[2]), $_[3] ] }
	],
	[#Rule 54
		 'code_indented', 2,
sub
#line 270 "Parser.yp"
{ push @{$_[1]}, new EmptyLine($_[2]); $_[1] }
	],
	[#Rule 55
		 'code_indented', 4,
sub
#line 272 "Parser.yp"
{ my $indent = substr($_[2], 0, $_[0]->YYData->{indent}, '');
      push @{$_[1]}, $indent, new CodeLine(@_[2,3]), $_[4]; $_[1] }
	]
],
                                  @_);
    bless($self,$class);
}

#line 277 "Parser.yp"



use strict;
use warnings;

use AST;


sub open2close {
    my ($c) = @_;

    $c =~ tr/[]{}()<>/][}{)(></;

    return $c;
}


sub _error {
    my $self = shift;

    my $symbol;
    if (defined $self->YYCurval) {
        $symbol = "'" . $self->YYCurtok;
        $symbol .= " " . $self->YYCurval
          if $self->YYCurval ne $self->YYCurtok && $self->YYCurval !~ /^\s*$/;
        $symbol .= "'";
    } else {
        $symbol = "end of file";
    }

    my $msg = "Syntax error at " . $self->YYData->{lexer}->where
      . " near $symbol,\n"
      . "expected '" . join("|", $self->YYExpect) . "'";

    while (my $inside = pop @{ $self->YYData->{inside} }) {
        $msg .= "\n  inside $inside";
    }

    die "$msg\n";
}


sub run {
    my $self = shift;
    my ($lexer) = @_;

    $lexer->push(
          '^\s*<[[:punct:]]'     => '<<MACRO',
          '.+'                   => 'CHAR'
    );
    $self->YYData->{lexer} = $lexer;
    my $tree = $self->YYParse(yylex => $lexer->get_lexer,
#                              yydebug => 0x1f,
                              yyerror => \&_error);

    $tree->normalize;

    return $tree;
}

1;

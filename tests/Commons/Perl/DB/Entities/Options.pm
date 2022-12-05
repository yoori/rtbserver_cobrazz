
package DB::Options;

use warnings;
use strict;
use DB::Entity::PQ;

# Recursive tokens
use constant GENERIC => 0x001;
use constant ADVERTISER => 0x002;
use constant PUBLISHER => 0x004;
use constant INTERNAL => 0x008;

our @ISA = qw(DB::Entity::PQ);

our %OPTION_DEFAULTS = (
  'CRCLICK' => {
    type => 'URL',
    recursive_tokens => GENERIC | ADVERTISER | INTERNAL,
    required => 'Y',
    sort_order => 1,
    default_value => 'www.autotest.com'
  },
  'PUBL_TAG_TRACK_PIXEL' => {
    type => 'URL',
    recursive_tokens => GENERIC | ADVERTISER | INTERNAL,
    required => 'N',
    sort_order => 1
  },
  'CRADVTRACKPIXEL' => {
    type => 'URL',
    recursive_tokens => GENERIC | ADVERTISER | INTERNAL,
    required => 'N',
    sort_order => 1
  },
  'CRHTML' => {
    type => 'Dynamic File',
    recursive_tokens => GENERIC | ADVERTISER,
    required => 'N',
    sort_order => 2
  }
);

use constant STRUCT => 
{
  option_id => DB::Entity::Type::sequence(),
  token => DB::Entity::Type::string(unique => 1),
  template_id => DB::Entity::Type::link('DB::Template', unique => 1, nullable => 1),
  size_id => DB::Entity::Type::link('DB::CreativeSize', unique => 1, nullable => 1),
  name => DB::Entity::Type::string(),
  type => 
    DB::Entity::Type::enum(
      ['String', 'Text', 'File', 
       'URL', 'File/URL', 'Integer', 
       'Color', 'Enum', 'Dynamic File']),
  recursive_tokens => DB::Entity::Type::int(nullable => 1),
  required => DB::Entity::Type::enum(['N', 'Y']),  
  sort_order => DB::Entity::Type::int(nullable => 1),
  default_value => DB::Entity::Type::string(nullable => 1),  
  min_value => DB::Entity::Type::int(),  
  max_value => DB::Entity::Type::int(),  
  option_group_id => DB::Entity::Type::link('DB::OptionGroup'),

  # Private
  tag_id => DB::Entity::Type::link('DB::Tag', private => 1),
  creative_id => DB::Entity::Type::link('DB::Creative', private => 1),
  value => DB::Entity::Type::string(private => 1)
};

sub compare
{
  my ($value, $default) = @_;
  defined $value? 
    defined $default? $value eq $default: 0:
      defined $default? 0: 1;
}

sub preinit_
{
  my ($self, $ns, $args) = @_;
  $args->{name} = $args->{token} 
    if not defined $args->{name};

  if (exists $OPTION_DEFAULTS{$args->{token}})
  {
    my %defaults = %{$OPTION_DEFAULTS{$args->{token}}};
    while(my ($f, $v) = each(%defaults))
    {
      if (not exists $args->{$f})
      {
        $args->{$f} = $v;
      }
    }
  }
}

sub postcreate_
{
  my ($self, $ns) = @_;

  if (exists $OPTION_DEFAULTS{$self->{token}})
  {
    my %args_copy = %{$self->{args__}};
    foreach my $f ($self->_external_fields)
    {
      delete $args_copy{$f};
    }
    my %defaults = (%{$OPTION_DEFAULTS{$self->{token}}}, %args_copy);
    my %def_copy = %defaults;
    while(my ($f, $v) = each(%defaults))
    {
      if (!(exists $self->{$f} && compare($self->{$f}, $defaults{$f})))
      {
        $self->__update($ns, \%def_copy) && last;
      }
    }
  }

  if ($self->{creative_id} && defined $self->{value}) {
    $ns->create(CreativeOptionValue => {
      creative_id => $self->{creative_id},
      option_id => $self->{option_id},
      value => $self->{value} });
  }

  if ($self->{tag_id} && defined $self->{value}) {
    $ns->create(TagOptionValue => {
      tag_id => $self->{tag_id},
      option_id => $self->{option_id},
      value => $self->{value} });
  }
}

1;

package DB::OptionGroup;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  option_group_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  template_id => DB::Entity::Type::link('DB::Template', nullable => 1, unique => 1),
  size_id => DB::Entity::Type::link('DB::CreativeSize', nullable => 1, unique => 1),
  type => DB::Entity::Type::enum(['Advertiser','Publisher', 'Hidden'], unique => 1),
  availability => 'A',
  collapsibility => 'N'
};

#sub preinit_
#{
#  my ($self, $ns, $args) = @_;
#
#  die "OptionGroup incorrect type: $args->{type}" 
#    if (defined $args->{type} && 
#      $args->{type} ne 'Advertiser' &&
#        $args->{type} ne 'Publisher');
#}

1;


package DB::Dump::noop;

use warnings;
use strict;

sub new
{
  my $self = shift;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  return $self;
}

sub begin_namespace
{}

sub dump
{}

sub end_namespace
{}

1;

package DB::Dump::XML;

use warnings;
use strict;

use constant XML_ENCODE  => 0x01;
use constant BASE64_ENCODE  => 0x02;

sub new
{
  my $self = shift;
  my ($xml, $xsd) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  $self->{_xml} = $xml;
  $self->{_namespaces} = {};
  open($self->{_fh}, '>:utf8', "$xml.tmp")
    or die "open(> $xml.tmp): $!";

  (my $header = << "  EOF;") =~ s/^ {2}//mg;
    <LocalParams xmlns="http://www.121media.com/xsd/tests/AutoTests"
                 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                 xsi:schemaLocation="http://www.121media.com/xsd/tests/AutoTests
                                     $xsd">

  EOF;

  print { $self->{_fh} } $header;

  return $self;
}


sub begin_namespace
{
  my ($self, $namespace_name) = @_;

  $self->{_namespaces}->{$namespace_name} = ();
}

sub _xml_encode {
    my ($s, $encode) = @_;

    if(!defined($s))
    {
      die "_xml_encode: undefined input value";
    }

    if ($encode)
    {
      $s =~ s/&/&amp;/g;
      $s =~ s/</&lt;/g;
      $s =~ s/>/&gt;/g;
      $s =~ s/\"/&quot;/g;
      $s =~s/\'/&apos;/g;
    }

    return $s;
}


sub dump
{
  my $self = shift;
  my ($name, $v_name, $value, $description, $flags) = @_;

  die "Unknown namespace '$name'" 
    if (not exists $self->{_namespaces}->{$name});

  $value = _xml_encode($value, $flags & XML_ENCODE);
  $description = _xml_encode($description, 1);

  push @{ $self->{_namespaces}->{$name} },
    [$v_name, $value, $description, $flags & BASE64_ENCODE]

}

sub end_namespace
{
  my $self = shift;
  my $name = shift;

  die "Unknown namespace '$name'" 
    if (not exists $self->{_namespaces}->{$name});

  (my $begin = << "  EOF;") =~ s/^ {2}//mg;
    <UnitLocalData UnitName="$name">
  EOF;
  print { $self->{_fh} } $begin;

  foreach my $data (@ {$self->{_namespaces}->{$name} })
  {
    my ($name, $value, $description, $base64) = @$data;
    my $base64_str = $base64? ' base64="true"': '';
    (my $body = << "  EOF;") =~ s/^ {2}//mg;
    <DataElem>
      <Name>$name</Name>
      <Value$base64_str>$value</Value>
      <Description>$description</Description>
    </DataElem>
  EOF;
    print { $self->{_fh} } $body;
  }

  (my $end = << "  EOF;") =~ s/^ {2}//mg;
    </UnitLocalData>

  EOF;

  print { $self->{_fh} } $end;
}


sub DESTROY
{
  my $self = shift;
  
  (my $footer = << '  EOF;') =~ s/^ {4}//mg;
    </LocalParams>
  EOF;

  print { $self->{_fh} } $footer;

  close($self->{_fh}) or
    die "close($self->{_xml}.tmp): $!";

  unlink($self->{_xml});

  rename("$self->{_xml}.tmp", $self->{_xml}) or
    die "rename($self->{_xml}.tmp, $self->{_xml}): $!";
}

1;

package DB::Namespace;

use warnings;
use strict;
use Data::Dumper;
use Encode;
use MIME::Base64;

use constant ENTITY_BASE_CLASS_NAME => 'DB::Entity::Base::Blank';

sub new
{
  my ($self, $db, $namespace, $options, $trace) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  $db->_dumper->begin_namespace($namespace);

  $self->{_db} = $db;
  $self->{_name} = $namespace;
  $self->{_namespace} = $self->{_db}->_namespace . '-' . $namespace;
  $self->{trace_} = defined($trace) ? $trace : 0;
  $self->{options_} = $options;

  return $self;
}

sub options
{
  my $self = shift;
  return $self->{options_};  
}

sub pq_dbh
{
  my $self = shift;
  return $self->{_db}->_pq_dbh;
}

sub db
{
  my $self = shift;
  return $self->{_db};
}

sub sub_namespace
{
  my ($self, $namespace_name, $trace) = @_;
  return new DB::SubNamespace::(
    $self,
    $namespace_name,
    $self->{options_},
    $self->{trace_} || $trace);
}


sub namespace
{
  my $self = shift;
  return $self->{_namespace};
}

sub create
{
  my $self = shift;
  my ($class, $args) = @_;

  my $obj;

  if(ref($class) eq ENTITY_BASE_CLASS_NAME)
  {
    my $blank_obj = $class;

    my $db_entity_class_name = $blank_obj->entity_class_name();
    $obj = $db_entity_class_name->_new($self, $blank_obj->args);
  }
  else
  {
    $class = "DB::$class";
    $obj = $class->_new($self, $args);
  }

  if($self->{trace_} > 0)
  {
    print "created $class" . (
      UNIVERSAL::isa($obj, "DB::Entity::Base") && scalar($obj->_unique()) ?
      " with key: " . join(',', $obj->{$obj->_unique()}) : "") . "\n";
    if($self->{trace_} > 1)
    {
      my $dumper = Data::Dumper->new([$obj]);
      $dumper->Indent(2);
      print "    " . $dumper->Dump . "\n";
    }
  }

  return $obj;
}

sub update
{
  my $self = shift;
  my ($object, $update) = @_;

  $object->__update($self, $update);
}

sub output
{
  my $self = shift;
  my ($key, $value, $description) = @_;
  $description = "" unless defined $description;

  use Scalar::Util qw(blessed);
  if (blessed($value))
  {
    $value = $value->_output();
  }

  $self->{_db}->_dumper->dump(
    $self->{_name}, $key, $value, $description, DB::Dump::XML::XML_ENCODE);
}

sub output_raw
{
  my $self = shift;
  my ($key, $value, $description) = @_;
  $description = "" unless defined $description;

  use Scalar::Util qw(blessed);
  if (blessed($value))
  {
    $value = $value->_output();
  }

  $self->{_db}->_dumper->dump(
    $self->{_name}, $key, $value, $description, 0);
}

sub output_base64
{
  my $self = shift;
  my ($key, $value) = @_;

  my $encoded_base64 = 
    encode_base64($value);

  $self->{_db}->_dumper->dump(
    $self->{_name}, $key, $encoded_base64,
      "", DB::Dump::XML::BASE64_ENCODE);
}

sub DESTROY
{
  my $self = shift;
  $self->{_db}->_dumper->end_namespace($self->{_name});
}

1;

package DB::SubNamespace;

use warnings;
use strict;
our @ISA = qw(DB::Namespace);

sub new
{
  my ($self, $parent, $namespace, $options, $trace) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  $self->{_name} = $parent->{_name};
  $self->{_sub_name} = $namespace;
  $self->{_db} = $parent->db;
  $self->{_namespace} = $parent->namespace . '-' . $namespace;
  $self->{trace_} = defined($trace) ? $trace : 0;
  $self->{options_} = $options;

  return $self;
}

sub output
{
  my $self = shift;
  my ($key, $value, $description, $encode) = @_;
  $encode = 1 unless defined $encode;
  $self->SUPER::output(
    $self->{_sub_name} . "/" . $key,
    $value, $description, $encode);
}

sub DESTROY
{ 
  # Suppress dump namespace name.
}

1;

package DB::Database;

use warnings;
use strict;
use DBI;

use constant DEFAULT_PQ_HOST => 'stat-dev1';
use constant DEFAULT_PQ_USER => 'test_ads';
use constant DEFAULT_PQ_PASSWORD => 'adserver';

sub new
{
  my $self = shift;
  my ($host, $db, $user, $password, $namespace, $dumper, $options, $trace) = @_;

  unless (ref $self)
  {
    $self = bless {}, $self;
  }

  $dumper = new DB::Dump::noop unless $dumper;

  $self->{_namespace} = $namespace;
  $self->{_dumper} = $dumper;
  $self->{options_} = $options;
  $self->{trace_} = $trace;

  $self->{_pq_dbh} = DBI->connect("DBI:Pg:dbname=$db;host=$host",
    $user, $password,
    { AutoCommit => 0,
      PrintError => 0,
      RaiseError => 1,
      pg_prepare_now => 1 });

  return $self;
}

sub DESTROY
{
  my $self = shift;
  $self->{_pq_dbh}->finish(); 
  $self->{_pq_dbh}->disconnect();
  undef($self->{_pq_dbh});
}

sub namespace
{
  my ($self, $namespace_name, $trace) = @_;
  return new DB::Namespace(
    $self,
    $namespace_name,
    $self->{options_},
    $self->{trace_} || $trace);
}

sub commit
{
  my $self = shift;
  $self->{_pq_dbh}->commit;
}

sub _namespace
{
  my $self = shift;
  return $self->{_namespace};
}

sub _pq_dbh
{
  my $self = shift;
  return $self->{_pq_dbh};
}

sub _dumper
{
  my $self = shift;
  return $self->{_dumper};
}

1;


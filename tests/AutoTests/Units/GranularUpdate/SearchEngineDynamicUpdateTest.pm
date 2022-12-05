package SearchEngineDynamicUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub add_engine
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('ADDENGINE');
  my $name = make_autotest_name($ns, 'search-engine');
  $ns->output("NAME", $name);
  $ns->output("REGEXP", '.*[?&]search=([^&]+).*');
  $ns->output("ENCODING", 'utf8');  
  $ns->output("DEC_DEPTH", 1);
  $ns->output("POST_ENCODING", 'utf8');
  $ns->output("HOST", 'dynamic.searchengine.test');
}

sub update_regexp
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('UPDATEREGEX');
  my $regexp = '.*[?&]search=([^&]+).*';
  my $engine = $ns->create(SearchEngine =>
    { name => 'Engine',
      host => 'dynamic.searchengine.test',
      regexp => $regexp }); 

  $ns->output("OLD_REGEXP", $regexp);
  $ns->output("NEW_REGEXP", '.*[?&]find=([^&]+).*');
  $ns->output("ENGINE", $engine);
}

sub update_encoding
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('UPDATEENC');
  my $encoding  = 'utf8';
  my $engine = $ns->create(SearchEngine =>
    { name => 'Engine',
      host => 'dynamic.searchengine.test',
      encoding => $encoding,
      regexp => '.*[?&]search=([^&]+).*' }); 

  $ns->output("OLD_ENCODING", $encoding);
  $ns->output("NEW_ENCODING", 'euc-kr');
  $ns->output("ENGINE", $engine);
}

sub update_depth
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('UPDATEDEPTH');
  my $depth  = 1;
  my $engine = $ns->create(SearchEngine =>
    { name => 'Engine',
      host => 'dynamic.searchengine.test',
      decoding_depth => $depth,
      regexp => '.*[?&]search=([^&]+).*' }); 

  $ns->output("OLD_DEPTH", $depth);
  $ns->output("NEW_DEPTH", 2);
  $ns->output("ENGINE", $engine);
}

sub del_engine
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('DELENGINE');
  my $engine = $ns->create(SearchEngine =>
    { name => 'Engine',
      host => 'dynamic.searchengine.test',
      regexp => '.*[?&]search=([^&]+).*' }); 

  $ns->output("ENGINE", $engine);
}

sub init
{
  my ($self, $ns) = @_;
  
  $self->add_engine($ns);
  $self->update_regexp($ns);
  $self->update_encoding($ns);
  $self->update_depth($ns);
  $self->del_engine($ns);

}

1;

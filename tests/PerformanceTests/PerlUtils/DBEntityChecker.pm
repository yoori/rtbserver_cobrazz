
package PerformanceDB::EntityChecker;

use warnings;
use strict;

use FindBin;
use lib "$FindBin::Dir/../../../Commons/Perl";

use DB::Database;
use DB::EntitiesImpl;

sub check {
  my ($ns, $table, $entity_name) = @_;
  my $class = "DB::$table";
  unless (!$class->_table) { $table = $class->_table; }
  $entity_name = $ns->namespace . "-" .  $entity_name;
  if ( $table eq "BehavioralParameters")
  {
    my $statment = "SELECT behav_params_id FROM BehavioralParameters WHERE " .
        "channel_id in (Select channel_id From Channel where name='$entity_name')";
    return _exec_checker_query($ns, $statment)
  }
  elsif ($class->can('_name'))
  {
    my ($name)        = ($class->_name);
    if ($name) 
    {
      my $statment = "SELECT $name FROM $table WHERE $name='$entity_name'";
      return _exec_checker_query($ns, $statment)
    }
  }
  return 1;
}  

sub _exec_checker_query {

  my ($ns, $statment) = @_;
  my $stmt = $ns->pq_dbh->prepare($statment) or  die $ns->pq_dbh->errstr;
  $stmt->execute();
  if ($stmt->fetchrow_arrayref)
  {
    return 0;
  }
  return 1;
}

1;


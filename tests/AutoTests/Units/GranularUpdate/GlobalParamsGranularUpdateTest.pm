package GlobalParamsGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $stmt = $ns->pq_dbh->prepare("SELECT Max(Currency_exchange_id) From CurrencyExchange");
  $stmt->execute;
  my @result = $stmt->fetchrow_array;
  $stmt->finish;

  $ns->output("CurrencyExchange", $result[0]);
}

1;

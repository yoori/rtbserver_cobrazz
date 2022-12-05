
package TextBiddingCommon;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;


sub expected_revenue_output
{
  my ($self, $ns, $case_index, $clicks, $imps) = @_;
  my $idx = 0;
  foreach my $revenue (@$clicks) {
    $ns->output("CLICKREV" .   $case_index . 
                "_" . ++$idx, $revenue);
  }
  $idx = 0;
  foreach my $revenue (@$imps) {
    $ns->output("IMPREV" .   $case_index . 
                "_" . ++$idx, $revenue);
  }
}

1;



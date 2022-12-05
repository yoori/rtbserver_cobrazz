package ColocationGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $account1 = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->isp_role });

  my $account2 = $ns->create(Account => {
    name => 2,
    role_id => DB::Defaults::instance()->isp_role });

  $ns->output("ColocationName", make_autotest_name($ns, "1"));
  $ns->output("Account1", $account1);
  $ns->output("Account2", $account2);
}

1;

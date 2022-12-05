package OptOutLoggingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $isp = $ns->create(Isp => {
    name => "ISP",
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  $ns->output("COLO", $isp->{colo_id});


  my $isp_tz = $ns->create(Isp => {
    name => "ISP-TZ",
    account_timezone_id => 
        DB::TimeZone->blank(tzname => 'Australia/Sydney'),
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  $ns->output("COLO/TZ", $isp_tz->{colo_id});
  $ns->output("TZ", 'Australia/Sydney');
}

1;

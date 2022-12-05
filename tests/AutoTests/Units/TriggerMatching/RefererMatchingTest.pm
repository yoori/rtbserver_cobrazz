package RefererMatchingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  $ns->output("Tags/Default", DB::Defaults::instance()->tag, "tid");

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 1,
      url_list => "http://dev.ocslab.com/services/",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  $ns->output("RefererMatchingTest/01", $bp->url_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 2,
      url_list => "http://andrey.gusev.com:80/",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  $ns->output("RefererMatchingTest/02", $bp->url_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 3,
      url_list => 
        "http://andrey.gusev.com:80/good/specialist/",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  $ns->output("RefererMatchingTest/03", $bp->url_key());

  $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => 4,
      url_list => 
        "http://andrey.gusev.com:80/good/specialist?wants=1&cool=1",
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'U') ]));

  $ns->output("RefererMatchingTest/04", $bp->url_key());
}

1;

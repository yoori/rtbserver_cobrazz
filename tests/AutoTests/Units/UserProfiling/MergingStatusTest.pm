package MergingStatusTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $site_id = $ns->create(Site => { name => 1});

  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $site_id});

  $ns->output("TID#1", $tag_id);
}

1;

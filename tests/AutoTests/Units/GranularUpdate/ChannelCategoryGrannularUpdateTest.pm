package ChannelCategoryGrannularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $category_name = make_autotest_name($ns, "1");

  my $category = $ns->create(
    CategoryChannel => {
      name => "2",
      account_id => DB::Defaults::instance()->internal_account });

  $ns->output("InternalAccount", DB::Defaults::instance()->internal_account);
  $ns->output("CategoryName", $category_name);
  $ns->output("ChannelCategory", $category);
  
}

1;


package WebStatFrontendProtocolTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use encoding 'utf8';

sub init {
  my ($self, $ns) = @_;

  my $isp = $ns->create(Isp => {
    name => "WebStatFrontendProtocol"});

  my $publisher = 
   $ns->create(Publisher => {
     name => "Publisher" });

   my $tag_deleted = $ns->create(PricedTag => {
    name => "Tag-Deleted",
    site_id => $publisher->{site_id},
    status => 'D' });

  my %campaign = (
    'ACTIVE' => { ccg_status => 'A', cc_status => 'A'},
    'INACTIVECC' => { ccg_status => 'A', cc_status => 'D'},
    'INACTIVECCG' => { ccg_status => 'I', cc_status => 'A'} );

  while (my ($k, $v) = each %campaign )
  {
    my $campaign = $ns->create(DisplayCampaign => {
      name => 'Campaign-' . $k,
      campaigncreativegroup_status => $v->{ccg_status},
      campaigncreativegroup_display_status_id => 
         $v->{ccg_status} ne 'A'? DISPLAY_STATUS_INACTIVE: DISPLAY_STATUS_LIVE,
      campaigncreative_status => $v->{cc_status},
      campaigncreative_display_status_id => 
         $v->{cc_status} eq 'D'?  DISPLAY_STATUS_INACTIVE: DISPLAY_STATUS_LIVE,
    });

    $ns->output("CC/" . $k, $campaign->{cc_id});
  }

  $ns->output("Colo", $isp->{colo_id});

  $ns->output("TAG/ACTIVE", $publisher->{tag_id});
  $ns->output("TAG/INACTIVE", $tag_deleted);

  my @web_operations = (
   ['wstest_app_1', 'wstest_src_1', 'wstest_op_1', 'A', 127],
   ['wstest_app_2', 'wstest_src_2', 'wstest_op_2', 'A', 127],
   ['wstest_app_3', 'wstest_src_3', 'wstest_op_3', 'A', 127],
   ['wstest_app_4', 'wstest_src_4', 'wstest_op_4', 'A', 127],
   ['wstest_app_5', 'wstest_src_5', 'wstest_op_5', 'A', 127],
   ['wstest_app_6', 'wstest_src_6', 'wstest_op_6', 'A', 127],
   ['wstest_app_7', 'wstest_src_7', 'wstest_op_7', 'A', 127],
#   ['wstest_app_8',  undef, 'wstest_op_8', 'A', 127],
   ['wstest_app_8', 'wstest_src_8', 'wstest_op_8', 'A', 127],
   ['wstest_app_9', 'wstest_src_9', 'wstest_op_9', 'A', 127],
   ['wstest_app_10', 'wstest_src_10', 'wstest_op_10', 'A', 127],
   ['wstest_app_11', 'wstest_src_11', 'wstest_op_11', 'A', 127],
   ['wstest_app_12', 'wstest_src_12', 'wstest_op_12', 'A', 127],
   ['wstest_app_13', 'wstest_src_13', 'wstest_op_13', 'A', 127],
   ['wstest_app_14', 'wstest_src_14', 'wstest_op_14', 'A', 127],
   ['wstest_app_15', 'wstest_src_15', 'wstest_op_15', 'A', 127],
   ['wstest_app_16', 'wstest_src_16', 'wstest_op_16', 'A', 127],
   ['wstest_app_17', 'wstest_src_17', 'wstest_op_17', 'D', 127],
   ['wstest_app_18', 'wstest_src_18', 'wstest_op_18', 'A', 127],
   ['wstest_app_19', 'wstest_src_19', 'wstest_op_19', 'A', 127],
   ['wstest_app_20', 'wstest_src_20', 'wstest_op_20', 'A', 6],
   ['wstest_app_21', 'wstest_src_21', 'wstest_op_21', 'A', 123],
   ['wstest_app_22', 'wstest_src_22', 'wstest_op_22', 'A', 125]);

  my $i = 0;
      
  for my $web (@web_operations)
  {
    my ($app, $source, $operation, $status, $flags) = @$web;

    my $wo = $ns->create(WebOperation => {
      app => $app,
      source => $source,
      operation => $operation,
      status => $status,
      flags => $flags });

    $ns->output("App/" . ++$i, $app);
    $ns->output("Source/$i", $source);
    $ns->output("Operation/$i", $operation);
    $ns->output("WebOpId/$i", $wo);
    
  }

  my @oo_operations = (
    [1, 'adserver', 'oo', 'in', 'A', 121],
    [2, 'adserver', 'oo', 'out', 'A', 121],
    [3, 'adserver', 'oo', 'status', 'A', 0]);

  # Weboperations for opt out
  for my $web (@oo_operations)
  {
    my ($id, $app, $source, $operation, $status, $flags) = @$web;

    my $wo = $ns->create(WebOperation => {
      web_operation_id => $id,      
      app => $app,
      source => $source,
      operation => $operation,
      status => $status,
      flags => $flags });

    $ns->output("OO/" . $operation, $wo);
  }
  
  # CT cookies & params
  $ns->output('CT/1', "%D0%94b");
  $ns->output('CURCT/1', qq[\xd0\x94b]);
}

1;

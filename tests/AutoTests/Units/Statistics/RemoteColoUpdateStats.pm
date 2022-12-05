
package RemoteColoUpdateStats;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use File::Basename;
use File::Spec;

sub init {
  my ($self, $ns) = @_;

  my $path = 
     File::Spec->join(dirname($ns->options()->{'local_params_scheme'}),
      '../../../CMS/tests/Configs/devel/envdev.sh');

  open(FILE, "<$path") || die "Can't open '$path'";
  my @array = <FILE>;
  close(FILE);

  my $version = '2.0.0.0';
  foreach my $row (@array) 
  {
    if ($row =~ /export VERSION='(\S+)'/)
    {
      $version = $1;
      last;
    }
  }

  my $advertiser = 
    $ns->create(Advertiser => { 
      name => "Advertiser" });

  my $publisher = 
    $ns->create(Publisher => { 
      name => "Publisher" });

  my $keyword = make_autotest_name($ns, "KWD");

  my $campaign = $ns->create(DisplayCampaign => {
    account_id => $advertiser,
    name => 'Campaign',
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60)]),
    campaigncreativegroup_cpm => 100,
    site_links => [
      { site_id => $publisher->{site_id} }] });

  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CHANNEL", $campaign->{channel_id});
  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("KWD", $keyword);
  $ns->output("VERSION", $version);
  $ns->output("CENTRAL/COLO", DB::Defaults::instance()->no_ads_isp->{colo_id});
  $ns->output("REMOTE/COLO/1", DB::Defaults::instance()->isp->{colo_id});
  $ns->output("REMOTE/COLO/2", DB::Defaults::instance()->remote_isp->{colo_id});
}

1;

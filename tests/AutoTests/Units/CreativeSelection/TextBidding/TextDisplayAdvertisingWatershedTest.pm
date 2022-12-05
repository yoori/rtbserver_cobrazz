
package TextDisplayAdvertisingWatershedTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
    my ($self, $ns) = @_;

    my $tag_cpm = 4.0;

    my $keyword1 = make_autotest_name($ns, "keyword_1");
    my $keyword2 = make_autotest_name($ns, "keyword_2");
    my $keyword3 = make_autotest_name($ns, "keyword_3");

    $ns->output("Keyword_1", $keyword1);
    $ns->output("Keyword_2", $keyword2);
    $ns->output("Keyword_3", $keyword3);

    my $size_id = $ns->create(CreativeSize => { 
      name => 1,
      max_text_creatives => 2 });

    my $publisher = $ns->create(Publisher => {
      name => 'Publisher',
      pricedtag_size_id => $size_id,
      pricedtag_cpm => $tag_cpm,
      pricedtag_adjustment => 1.0 });

     my $account = $ns->create(Account => { 
       name => 'Advertiser',
       role_id => DB::Defaults::instance()->advertiser_role });

    my $channel1 = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel1',
      account_id => $account,
      keyword_list => $keyword1,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P', time_to =>  60) ]));

    my $channel2 = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel2',
      account_id => $account,
      keyword_list => $keyword2,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

    my $channel3 = $ns->create(DB::BehavioralChannel->blank(
      name => 'Channel3',
      account_id => $account,
      keyword_list => $keyword3,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

    # Expression channels
    my $expression1 = $ns->create(DB::ExpressionChannel->blank(
      name => "Expr1",
      account_id => $account,
      expression => 
        $channel1->channel_id . "&" . $channel2->channel_id));

    my $expression2 = $ns->create(DB::ExpressionChannel->blank(
      name => "Expr2",
      account_id => $account,
      expression => 
        $channel1->channel_id . "&" . $channel3->channel_id));


    my $campaign_display1 = $ns->create(DisplayCampaign => {
       name => 'Display1',
       size_id => $size_id,
       account_id => $account,
       creative_template_id => DB::Defaults::instance()->text_template,
       channel_id => $expression1,
       campaigncreativegroup_cpm => 0,
       campaigncreativegroup_ctr => 0.1,
       campaigncreativegroup_cpc => 3.0*$tag_cpm,
       site_links => 
         [{ site_id => $publisher->{site_id} }] });

    my $campaign_display2 = $ns->create(DisplayCampaign => {
       name => 'Display2',
       size_id => $size_id,
       account_id => $account,
       creative_template_id => DB::Defaults::instance()->text_template,
       channel_id => $expression2,
       campaigncreativegroup_cpm => 0,
       campaigncreativegroup_ctr => 0.1,
       campaigncreativegroup_cpc => 5.0*$tag_cpm,
       site_links => 
         [{ site_id => $publisher->{site_id} }] });

    my $campaign_text = $ns->create(TextAdvertisingCampaign => { 
      name => 'Text',
      size_id => $size_id,
      account_id => $account,
      template_id => DB::Defaults::instance()->text_template,
      original_keyword => $keyword2,
      ccgkeyword_channel_id => $channel2,                  
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpc => 4.0*$tag_cpm,
      ccgkeyword_ctr => 0.1,
      max_cpc_bid => 4.0*$tag_cpm,
      site_links =>
        [{ site_id => $publisher->{site_id} }] });

    $ns->output("Tag", $publisher->{tag_id});
    $ns->output("DisplayCC", $campaign_display2->{cc_id});
    $ns->output("TACC", $campaign_text->{cc_id});
    $ns->output("DisplayCPC", 5.0*$tag_cpm);
    $ns->output("TACPC", 4.0*$tag_cpm);
}

1;

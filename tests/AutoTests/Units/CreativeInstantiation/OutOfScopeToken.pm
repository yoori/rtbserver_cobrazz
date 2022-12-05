
package OutOfScopeToken;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $size = $ns->create(CreativeSize => {
    name => 1 ,
    max_text_creatives => 2});
  
  my $tag_cpm = 2.0;
 
  # keywords
  my $keyword = make_autotest_name($ns, "keyword");
  
  # text advertising creatives
  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(ChannelTargetedTACampaign =>
                             { name => 1,
                               size_id => $size,
                               template_id => DB::Defaults::instance()->text_template,
                               account_id => $account,
                               behavioralchannel_keyword_list => $keyword,
                               campaigncreativegroup_cpm => 2 * $tag_cpm,
                               creative_description1_value => '##KEYWORD##',
                               site_links => [{name => 1}] });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P", 
      time_to => 3 * 60 * 60 ));

  
  my $tag = $ns->create(PricedTag =>
                        { name => 1,
                          site_id => $campaign->{Site}[0]->{site_id},
                          size_id => $size,
                          cpm => $tag_cpm});


  my $option = $ns->create(Options => { 
    token => 'DESCRIPTION1',
    option_group_id => 
      DB::Defaults::instance()->text_option_group,
    template_id => DB::Defaults::instance()->text_template,
    });

  $ns->output("KEYWORD", $keyword);
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("TAG", $tag);
  $ns->output("CREATIVE", $campaign->{creative_id});
  $ns->output("OPTION", $option);
   
}

1;

#v2

package StatsHourlyActionsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# cpa must be great for concurent with tag showing cost (0.02) if system level AR is low (0.01 by default)
use constant CPA_VALUE => 1000;
use constant CPC_VALUE => 500;

sub init {
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
      name => "advertiser",
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type});

  my $keyword = make_autotest_name($ns, "keyword");

  $ns->output("Keyword", $keyword);

  $ns->output("Colo", DB::Defaults::instance()->ads_isp->{colo_id});

  my $common_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'CommonChannel',
    account_id => $advertiser,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ]));

  my $keyword_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'KeywordChannel',
    account_id => $advertiser,
    keyword_list => $keyword,
    channel_type => 'K',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        minimum_visits => 1)
    ]));

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => "BaseFunctionality",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    site_links => [{name => 1}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/1", $campaign1->{cc_id});

  my $campaign2_1 = $ns->create(DisplayCampaign => {
    name => "ActionFromDifferentCampaign-first",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => "2/1"}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/2/1", $campaign2_1->{cc_id});
  $ns->output("CCG Id/2/1", $campaign2_1->{ccg_id});

  my $campaign2_2 = $ns->create(DisplayCampaign => {
    name => "ActionFromDifferentCampaign-second",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => "2/2"}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/2/2", $campaign2_2->{cc_id});
  $ns->output("CCG Id/2/2", $campaign2_2->{ccg_id});

  my $campaign2x2 = $ns->create(DisplayCampaign => {
    name => "TripleAction",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => "2x2"}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/2x2", $campaign2x2->{cc_id});

  my $campaign3 = $ns->create(DisplayCampaign => {
    name => "ActionBeforeClick",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 3}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/3", $campaign3->{cc_id});

  my $campaign4 = $ns->create(DisplayCampaign => {
    name => "ActionBeforeImpressionConfirmation",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 4}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/4", $campaign4->{cc_id});

  my $size5_1 = $ns->create(CreativeSize => {name => "5-1"});
  my $size5_2 = $ns->create(CreativeSize => {name => "5-2"});

  my $template5_1 = $ns->create(Template => {
      name => "OneActionForMultipleCreatives1",
      size_id => $size5_1,
      app_format_id => DB::Defaults::instance()->app_format_track,
      flags => DB::TemplateFile::PIXEL_TRACKING,
      });
  my $creative5_1 = $ns->create(Creative => {
      name => "OneActionForMultipleCreatives1",
      account_id => $advertiser,
      size_id => $size5_1,
      template_id =>  $template5_1
      });

  my $template5_2 = $ns->create(Template => {
      name => "OneActionForMultipleCreatives2",
      size_id => $size5_2,
      app_format_id => DB::Defaults::instance()->app_format_track,
      flags => DB::TemplateFile::PIXEL_TRACKING,
      });
  my $creative5_2 = $ns->create(Creative => {
      name => "OneActionForMultipleCreatives2",
      account_id => $advertiser,
      size_id => $size5_2,
      template_id =>  $template5_2
      });

  my $campaign5 = $ns->create(DisplayCampaign => {
    name => "OneActionForMultipleCreatives",
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    creative_id => $creative5_1,
    site_links => [{name => 5}],
    channel_id => $common_channel
    });

  my $cc5_1 = $ns->create(CampaignCreative => {
      ccg_id => $campaign5->{ccg_id},
      creative_id => $creative5_1
      });

  my $cc5_2 = $ns->create(CampaignCreative => {
      ccg_id => $campaign5->{ccg_id},
      creative_id => $creative5_2
      });
  
  $ns->output("CCG Id/5", $campaign5->{ccg_id});
  $ns->output("CC Id/5/1", $cc5_1);
  $ns->output("CC Id/5/2", $cc5_2);

  $ns->output("Adv/5", $campaign5->{account_id});

  my $campaign6 = $ns->create(DisplayCampaign => {
    name => "ActionWithCPCRate",
    account_id => $advertiser,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpc => CPC_VALUE,
    campaigncreativegroup_ctr => 0.01,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 6}],
    channel_id => $common_channel
    });

  $ns->output("CC Id/6", $campaign6->{cc_id});

  my $size7 = $ns->create(CreativeSize => {name => "ActionsForTextAdGroup",
    max_text_creatives => 3});

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size7,
    template_file => 'UnitTests/textad.xsl',
    flags => DB::TemplateFile::PIXEL_TRACKING,
    app_format_id =>  DB::Defaults::instance()->app_format_track,
    template_type => 'X'});

  my $campaign7_1 = $ns->create(ChannelTargetedTACampaign => {
    name => "ActionsForTextAdGroup1",
    size_id => $size7,
    template_id => DB::Defaults::instance()->text_template,
    channel_id => $common_channel,
    campaigncreativegroup_cpa => CPA_VALUE,
    campaigncreativegroup_ar => 0.01,
    site_links => [{name => 7}],
    });
  $ns->output("CC Id/7/1", $campaign7_1->{cc_id});

  my $campaign7_2 = $ns->create(ChannelTargetedTACampaign => {
    name => "ActionsForTextAdGroup2",
    size_id => $size7,
    template_id => DB::Defaults::instance()->text_template,
    channel_id => $common_channel,
    campaigncreativegroup_cpc => CPC_VALUE,
    campaigncreativegroup_ctr => 0.01,
    site_links => [{site_id => $campaign7_1->{Site}[0]->{site_id}}],
    });
  $ns->output("CC Id/7/2", $campaign7_2->{cc_id});

  my $campaign7_3 = $ns->create(TextAdvertisingCampaign => {
    name => "ActionsForTextAdGroup3",
    size_id => $size7,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword,
    ccgkeyword_channel_id => $keyword_channel,
    campaigncreativegroup_cpc => CPC_VALUE,
    campaigncreativegroup_ctr => 0.01,
    site_links => [{site_id => $campaign7_1->{Site}[0]->{site_id}}],
    });
  $ns->output("CC Id/7/3", $campaign7_3->{cc_id});

  ###########

  my $tag_id1 = $ns->create(PricedTag => {
    name => "BaseFunctionality",
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/1", $tag_id1, "");

  my $tag_id2 = $ns->create(PricedTag => {
    name => "ActionFromDifferentCampaign",
    site_id => $campaign2_1->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/2", $tag_id2, "");

  my $tag_id2x2 = $ns->create(PricedTag => {
    name => "TripleAction",
    site_id => $campaign2x2->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/2x2", $tag_id2x2, "");

  my $tag_id3 = $ns->create(PricedTag => {
    name => "ActionBeforeClick",
    site_id => $campaign3->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/3", $tag_id3, "");

  my $tag_id4 = $ns->create(PricedTag => {
    name => "ActionBeforeImpressionConfirmation",
    site_id => $campaign4->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/4", $tag_id4, "");

  my $tag_id5_1 = $ns->create(PricedTag => {
    name => "OneActionForMultipleCreatives1",
    size_id => $size5_1,
    site_id => $campaign5->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/5/1", $tag_id5_1, "");

  my $tag_id5_2 = $ns->create(PricedTag => {
    name => "OneActionForMultipleCreatives2",
    size_id => $size5_2,
    site_id => $campaign5->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/5/2", $tag_id5_2, "");

  my $tag_id6 = $ns->create(PricedTag => {
    name => "ActionWithCPCRate",
    site_id => $campaign6->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/6", $tag_id6, "");

  my $tag_id7 = $ns->create(PricedTag => {
    name => "ActionsForTextAdGroup",
    size_id => $size7,
    site_id => $campaign7_1->{Site}[0]->{site_id},
    cpm => 20 });
  $ns->output("Tag Id/7", $tag_id7, "");

}

1;

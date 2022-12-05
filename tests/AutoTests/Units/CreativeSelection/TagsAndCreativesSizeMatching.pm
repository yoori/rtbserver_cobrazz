
package TagsAndCreativesSizeMatching;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_sizes
{
  my ($self, $ns, $max_sizes, $type, $overlay) = @_;
  my @sizes = ();
  my $count = ref($max_sizes) eq 'ARRAY'? scalar @{ $max_sizes }: $max_sizes;
  my $si = 100;
  for (my $i = 0; $i < $count; $i++)
  {
    my $size = $ns->create(CreativeSize => {
      name => 'Size' . $i,
      protocol_name => sub { $overlay? "rm_" . $_[0]->{name}: $_[0]->{name} },
      width => $overlay? ++$si: 468,
      height => $overlay? ++$si: 60,
      size_type_id =>
        $type? $type:
          DB::Defaults::instance()->other_size_type,
      max_text_creatives => ref($max_sizes) eq 'ARRAY'? $max_sizes->[$i]: 1 });

    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->display_template,
      app_format_id => $overlay?
        DB::Defaults::instance()->html_format: 
         DB::Defaults::instance()->app_format_no_track(),
      size_id => $size});
    
    $ns->create(TemplateFile => {
      template_id => DB::Defaults::instance()->text_template,
      size_id => $size,
      template_file => $overlay?
         'UnitTests/rtb.html': 'UnitTests/textad.xsl',
      app_format_id => $overlay?
        DB::Defaults::instance()->html_format: 
         DB::Defaults::instance()->app_format_no_track(),
      template_type => $overlay? 'T': 'X' });

    push(@sizes, $size);

    $ns->output('Size/' . $i, $size);
    $ns->output('Size/' . $i . '/Protocol', $size->{protocol_name});
  }

  return @sizes;
}

sub same_type_diff_sizes
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('SAMETYPE');

  my @sizes = $self->create_sizes($ns, 2);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0]]});

  my $keyword = make_autotest_name($ns, "KWD");

  my $campaign = $ns->create(ChannelTargetedTACampaign => {
    name => 'Campaign',
    template_id => DB::Defaults::instance()->text_template,
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    creative_tag_sizes => [$sizes[1]],
    campaigncreativegroup_cpm => 10,
    site_links => [{ site_id => $publisher->{site_id} }] });

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
  $ns->output('CC', $campaign->{cc_id});
}


sub actual_size
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('ACTUALSIZE');

  my $size_type = $ns->create(SizeType => {
    name => 'TagsAndCreativesSizeMatchingType' });

  my @sizes = $self->create_sizes($ns, 4, $size_type);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_type_id => $size_type,
    pricedtag_size_id => [$sizes[0], $sizes[1], $sizes[2]]});

  my $keyword = make_autotest_name($ns, "KWD");

  my $campaign = $ns->create(ChannelTargetedTACampaign => {
    name => 'Campaign',
    template_id => DB::Defaults::instance()->text_template,
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    creative_tag_sizes => [$sizes[1], $sizes[2], $sizes[3]],
    campaigncreativegroup_cpm => 10,
    site_links => [{ site_id => $publisher->{site_id} }] });

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
  $ns->output('CC', $campaign->{cc_id});
}

sub diff_sizes
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('DIFFSIZES');

  my @sizes = $self->create_sizes($ns, [1, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1]]});

  my $keyword = make_autotest_name($ns, "KWD");

  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
    name => "Campaign1",
    template_id => DB::Defaults::instance()->text_template,
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_cpc => 0.2,
    creative_tag_sizes => [$sizes[0]],
    site_links => [{site_id => $publisher->{site_id}} ] });

  my $creative1_2 = $ns->create(Creative => {
    name => "1_2",
    account_id => $campaign1->{account_id},
    tag_sizes => [$sizes[0]],
    template_id => DB::Defaults::instance()->text_template });

  my $cc1_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign1->{ccg_id},
    creative_id => $creative1_2 });

  my $campaign2 = $ns->create(ChannelTargetedTACampaign => {
    name => "Campaign2",
    template_id => DB::Defaults::instance()->text_template,
    channel_id => $campaign1->{channel_id},
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_cpc => 0.15,
    creative_tag_sizes => [$sizes[1]],
    site_links => [{site_id => $publisher->{site_id}} ] });

  my $campaign3 = $ns->create(TextAdvertisingCampaign => {
    name => "Campaign3",
    template_id => DB::Defaults::instance()->text_template,
    ccgkeyword_channel_id => $campaign1->{channel_id},
    ccgkeyword_max_cpc_bid => undef,
    ccgkeyword_original_keyword => $keyword,
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_cpc => 0.1,
    creative_tag_sizes => [$sizes[1]],
    site_links => [{site_id => $publisher->{site_id}} ] });

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
  $ns->output('CC/2', $campaign2->{cc_id});
  $ns->output('CC/3', $campaign3->{cc_id});
}

sub max_ads
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('MAXADS');

  my @sizes = $self->create_sizes($ns, [1, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1]]});

  my $keyword = make_autotest_name($ns, "KWD");
  my @cpms = (20, 20, 10, 10);

  my $i = 0;

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]));

  foreach my $cpm (@cpms)
  {
    my $campaign = $ns->create(ChannelTargetedTACampaign => {
      name => "Campaign" . ++$i,
      channel_id => $channel,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_cpm => $cpm,
      creative_tag_sizes => $i > 2? [$sizes[1]]: [$sizes[0]],
      site_links => [{site_id => $publisher->{site_id}} ] });

    $ns->output('CC/' . $i, $campaign->{cc_id});
  }

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
}

sub zero_max_ads
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('ZEROMAXADS');

  my @sizes = $self->create_sizes($ns, [0, 2]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => [$sizes[0], $sizes[1]]});

  my $keyword = make_autotest_name($ns, "KWD");
  my @cpms = (20, 10);

  my $i = 0;

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]));

  foreach my $cpm (@cpms)
  {
    my $campaign = $ns->create(ChannelTargetedTACampaign => {
      name => "Campaign" . ++$i,
      channel_id => $channel,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_cpm => $cpm,
      creative_tag_sizes => $i == 1? [$sizes[0]]: [$sizes[1]],
      site_links => [{site_id => $publisher->{site_id}} ] });

    $ns->output('CC/' . $i, $campaign->{cc_id});
  }

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
}

sub multi_creative_ccg
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('MULTICC');

  my @sizes = $self->create_sizes($ns, [5, 5, 5, 5, 5, 5]);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => 
     [$sizes[0], $sizes[1], $sizes[2], $sizes[3], $sizes[4]]});

  my $keyword = make_autotest_name($ns, "KWD");

  my $campaign = $ns->create(TextAdvertisingCampaign => {
    name => "Campaign",
    template_id => DB::Defaults::instance()->text_template,
    ccgkeyword_channel_id => $ns->create(
      DB::BehavioralChannel->blank(
        name => 'Channel',
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ])),
    ccgkeyword_max_cpc_bid => undef,
    ccgkeyword_original_keyword => $keyword,
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_cpc => 0.1,
    creative_tag_sizes => [$sizes[5]],
    site_links => [{site_id => $publisher->{site_id}} ] });

  my $creative1_2 = $ns->create(Creative => {
    name => "1_2",
    account_id => $campaign->{account_id},
    tag_sizes => 
      [$sizes[0], $sizes[1], $sizes[2], $sizes[3], $sizes[4]],
    template_id => DB::Defaults::instance()->text_template });

  my $cc1_2 = $ns->create(CampaignCreative => {
    ccg_id => $campaign->{ccg_id},
    creative_id => $creative1_2 });

  my $creative1_3 = $ns->create(Creative => {
    name => "1_3",
    account_id => $campaign->{account_id},
    tag_sizes => 
      [$sizes[4]],
    template_id => DB::Defaults::instance()->text_template });

  my $cc1_3 = $ns->create(CampaignCreative => {
    ccg_id => $campaign->{ccg_id},
    creative_id => $creative1_3 });

  $ns->output('Tag', $publisher->{tag_id});
  $ns->output('KWD', $keyword);
  $ns->output('CC/1/1', $campaign->{cc_id});
  $ns->output('CC/1/2', $cc1_2);
  $ns->output('CC/1/3', $cc1_3);
}

sub display_text
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('DISPLAYTEXT');

  my $size = $ns->create(CreativeSize => {
      name => 'Size',
      max_text_creatives => 2 });

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->display_template,
    size_id => $size});

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size,
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X'});
  
  $ns->output('Size', $size);

  my @cpms = ([20, 10], [10, 20], [10, 10]);

  my $i = 0;

  foreach my $cpm (@cpms)
  {
    my ($d_cpm, $t_cpm) = @$cpm;
    my $keyword = make_autotest_name($ns, "KWD" . ++$i);

    my $channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => 'Channel' . $i,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]));

    my $publisher = $ns->create(Publisher => {
      name => "Publisher",
      pricedtag_size_id => [$size]});

    my $t_campaign = $ns->create(ChannelTargetedTACampaign => {
      name => "Text" . $i,
      channel_id => $channel,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_cpm => $t_cpm,
      creative_tag_sizes => [$size],
      site_links => [{site_id => $publisher->{site_id}} ] });

    my $d_campaign = $ns->create(DisplayCampaign => {
      name => "Display" . $i,
      channel_id => $channel,
      campaigncreativegroup_cpm => $d_cpm,
      creative_tag_sizes => [$size],
      site_links => [{site_id => $publisher->{site_id}} ] });

    $ns->output('CC/Display/' . $i, $d_campaign->{cc_id});
    $ns->output('CC/Text/' . $i, $t_campaign->{cc_id});
    $ns->output('Tag/' . $i, $publisher->{tag_id});
    $ns->output('KWD/' . $i, $keyword);
  }

}

sub size_type_level
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('SIZETYPE');

  my $size_type = $ns->create(SizeType => {
    name => 'TagsAndCreativesSizeMatchingTypeLevel',
    flags => DB::SizeType::MULTIPLE_SIZES });

  my $disabled_size = $ns->create(CreativeSize => {
    name => 'Disabled-Size',
    size_type_id => $size_type,
    max_text_creatives => 2 });  
  
  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $disabled_size,
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X'});

  my $enabled_size = $ns->create(CreativeSize => {
    name => 'Enabled-Size',
    size_type_id => $size_type,
    max_text_creatives => 2 });

  $ns->create(TemplateFile => {
    template_id => DB::Defaults::instance()->text_template,
    size_id => $enabled_size,
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X'});

  my $publisher1 = $ns->create(Publisher => {
      name => "Publisher1",
      pricedtag_size_type_id => $size_type,
      pricedtag_size_id => [$disabled_size]});

  my $publisher2 = $ns->create(Publisher => {
      name => "Publisher2",
      pricedtag_size_type_id => $size_type,
      pricedtag_size_id => [$enabled_size]});

  my $keyword = make_autotest_name($ns, "KWD");

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser',
    text_adserving => 'M',
    type_creative_sizes => [$enabled_size],
    type_flags => DB::AccountType::SITE_TARGETING |
      DB::AccountType::COMPANY_OWNERSHIP });

  my $channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]));

  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
    name => "Campaign1",
    channel_id => $channel,
    account_id => $advertiser,
    template_id => DB::Defaults::instance()->text_template,
    campaigncreativegroup_cpm => 20,
    creative_tag_sizes => [],
    creative_tag_size_types => [$size_type],
    site_links => [{ site_id => $publisher1->{site_id} },
                   { site_id => $publisher2->{site_id} } ] });

  my $campaign2 = $ns->create(ChannelTargetedTACampaign => {
    name => "Campaign2",
    account_id => $advertiser,
    channel_id => $channel,
    template_id => DB::Defaults::instance()->text_template,
    campaigncreativegroup_cpm => 10,
    creative_tag_sizes => [],
    creative_flags => DB::Creative::ENABLE_ALL_SIZES,
    site_links => [{ site_id => $publisher1->{site_id} },
                   { site_id => $publisher2->{site_id} } ] });

  $ns->output('CC/1', $campaign1->{cc_id});
  $ns->output('CC/2', $campaign2->{cc_id});
  $ns->output('Tag/1', $publisher1->{tag_id});
  $ns->output('Tag/2', $publisher2->{tag_id});
  $ns->output('KWD', $keyword);
}

sub rtb_case
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace('RTB');

  my $size_type = $ns->create(SizeType => {
    name => 'TagsAndCreativesSizeMatchingRtb' });

  my @sizes = $self->create_sizes($ns, [2, 2, 2], $size_type, 1);

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    account_id => DB::Defaults::instance()->openx_account,
    pricedtag_size_id => 
     [$sizes[0], $sizes[1]]});

  my $keyword = make_autotest_name($ns, "KWD");
  
  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
    name => "Campaign1",
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'S',
          time_to => 3*60*60) ]),
    template_id => 
       DB::Defaults::instance()->text_template,
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_cpm => 10,
    creative_tag_sizes => [$sizes[0], $sizes[1], $sizes[2]],
    site_links => [{ site_id => $publisher->{site_id} } ] });

  my $campaign2 = $ns->create(TextAdvertisingCampaign => {
    name => "Campaign2",
    template_id => 
       DB::Defaults::instance()->text_template,
    ccgkeyword_channel_id => $campaign1->{channel_id},
    ccgkeyword_max_cpc_bid => undef,
    ccgkeyword_original_keyword => $keyword,
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_cpc => 0.1,
    creative_tag_sizes => [$sizes[0], $sizes[1], $sizes[2]],
    site_links => [{site_id => $publisher->{site_id}} ] });


  $ns->output('SEARCH', "http://search.live.de/results.aspx?q=" . $keyword);
  $ns->output('ACCOUNT', DB::Defaults::instance()->openx_account);
  $ns->output('CC/1', $campaign1->{cc_id});
  $ns->output('CC/2', $campaign2->{cc_id});
  $ns->output('CREATIVE/1', $campaign1->{creative_id});
  $ns->output('CREATIVE/2', $campaign2->{creative_id});
  $ns->output('CCG/1', $campaign1->{ccg_id});
  $ns->output('CCG/2', $campaign2->{ccg_id});
}

sub init {
  my ($self, $ns) = @_;

  $self->same_type_diff_sizes($ns);
  $self->actual_size($ns);
  $self->diff_sizes($ns);
  $self->max_ads($ns);
  $self->zero_max_ads($ns);
  $self->multi_creative_ccg($ns);
  $self->display_text($ns);
  $self->size_type_level($ns);
  $self->rtb_case($ns);
}

1;

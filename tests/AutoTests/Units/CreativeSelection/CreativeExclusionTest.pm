package CreativeExclusionTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub SiteLevelExlusions
{
  my ($ns, $name, $approved, $acc_type_id, $site_flags) = @_;

  my $adv_acc = $ns->create(Account => {
    name => "$name-a",
    role_id => DB::Defaults::instance()->advertiser_role });

  my $pub_acc = $ns->create(PubAccount => {
    name => "$name-p",
    account_type_id => $acc_type_id });

  if ($site_flags == 2)
  {
    $ns->create(WalledGarden => {
      pub_account_id => $pub_acc->{account_id},
      agency_account_id => $adv_acc->{account_id},
      pub_marketplace => 'WG',
      agency_marketplace => 'WG'});
  }

  my $campaign = $ns->create(Campaign => {
    name => "$name",
    account_id => $adv_acc });

  my $keyword = make_autotest_name($ns, $name);
  $ns->output("kwd/$name", $keyword);

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "$name",
      keyword_list => $keyword,
      account_id => $adv_acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("$name/01", $name);

  my $ccg = $ns->create(CampaignCreativeGroup => {
    name => "$name",
    campaign_id => $campaign,
    channel_id => $bp->channel_id() });

  if ($approved !~ /^[AR]$/) {
      die "Error in sub SiteLevelExlusions. Got unexpected <approved> parameter: '$approved', instead of 'A' or 'R'.";
  }

  my $site = $ns->create(Site => { 
    name => "$name",
    account_id => $pub_acc,
    flags => $site_flags });

  my @categories;

  $categories[2] = {"$name-2-v" => {cct_id => 0, approval => "A"}};
  $categories[3] = {"$name-3-v" => {cct_id => 0, approval => "P"}};
  $categories[4] = {"$name-4-v" => {cct_id => 0, approval => "R"}};
  $categories[5] = {"$name-5-v" => {cct_id => 0, approval => "A"},
                    "$name-5-c" => {cct_id => 1, approval => "A"}};
  $categories[6] = {"$name-6-v" => {cct_id => 0, approval => "A"},
                    "$name-6-c" => {cct_id => 1, approval => "P"}};
  $categories[7] = {"$name-7-v" => {cct_id => 0, approval => "A"},
                    "$name-7-c" => {cct_id => 1, approval => "R"}};
  $categories[8] = {"$name-8-v" => {cct_id => 0, approval => "P"},
                    "$name-8-c" => {cct_id => 1, approval => "P"}};
  $categories[9] = {"$name-9-v" => {cct_id => 0, approval => "P"},
                    "$name-9-c" => {cct_id => 1, approval => "R"}};
  $categories[10] = {"$name-10-v" => {cct_id => 0, approval => "R"},
                     "$name-10-c" => {cct_id => 1, approval => "R"}};

  my $content_category = $ns->create(CreativeCategory =>
                                     { name => "$name-content",
                                       cct_id => 1});

  for (my $i = 1; $i <= 10 ; ++$i)
  {
    my $size = $ns->create(CreativeSize =>
                           { name => "$name-$i" });

    my $template = $ns->create(Template =>
                               { name => "$name-$i",
                                 size_id => $size,
                                 flags => 2 });

    my $creative = $ns->create(Creative =>
                               { name => "$name-$i",
                                 account_id => $adv_acc,
                                 size_id => $size,
                                 creative_category_id => $content_category,
                                 template_id => $template });

    $ns->create(SiteCreativeApproval => { 
      site_id => $site,
      creative_id => $creative }) if $approved eq 'A';

    my $cc = $ns->create(CampaignCreative =>
                         { ccg_id => $ccg,
                           creative_id => $creative });

    $ns->output("$name/CC Id/$i", $cc, "See ADSC-208");

    my $tag = $ns->create(PricedTag =>
                          { name => "$name-$i",
                            site_id => $site,
                            size_id => $size });

    $ns->output("$name/Tag Id/$i", $tag, "See ADSC-208");

    foreach my $category (keys %{$categories[$i]})
    {
      my $cat_id = $ns->create(CreativeCategory =>
        { name => $category,
          cct_id => $categories[$i]->{$category}->{cct_id} });

      $ns->create(CreativeCategory_Creative =>
                  { creative_category_id => $cat_id,
                    creative_id => $creative });

      $ns->create(SiteCreativeCategoryExclusion =>
                  { site_id => $site,
                    approval => $categories[$i]->{$category}->{approval},
                    creative_category_id => $cat_id })
        if  $categories[$i]->{$category}->{approval} ne "A";
    }
  }
  return;
}

sub SiteTagLevelExclusions
{
  my ($ns, $name, $approved, $acc_type_id, $site_flags) = @_;

  my $adv_acc = $ns->create(Account =>
                            { name => "$name-a",
                              role_id => DB::Defaults::instance()->advertiser_role });

  my $pub_acc = $ns->create(PubAccount =>
                            { name => "$name-p",
                              account_type_id => $acc_type_id });

  if ($site_flags == 2)
  {
    $ns->create(WalledGarden =>
                { pub_account_id => $pub_acc->{account_id},
                  agency_account_id => $adv_acc->{account_id},
                  pub_marketplace => 'WG',
                  agency_marketplace => 'WG'});
  }

  my $campaign = $ns->create(Campaign =>
                             { name => "$name",
                               account_id => $adv_acc });

  my $keyword = make_autotest_name($ns, $name);
  $ns->output("kwd/$name", $keyword);

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => "$name",
      keyword_list => $keyword,
      account_id => $adv_acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  $ns->output("$name/01", $name);

  my $ccg = $ns->create(CampaignCreativeGroup => {
    name => "$name",
    campaign_id => $campaign,
    channel_id => $bp->channel_id() });

  if ($approved !~ /^[AR]$/) {
      die "Error in sub SiteLevelExlusions. Got unexpected <approved> parameter: '$approved', instead of 'A' or 'R'.";
  }

  my $site = $ns->create(Site => {
    name => "$name",
    account_id => $pub_acc,
    flags => $site_flags });

  my @categories;

  $categories[1] = {"$name-1" => {site => "A", tag => "A", cct_id => 0}};
  $categories[2] = {"$name-2" => {site => "A", tag => "R", cct_id => 0}};
  $categories[3] = {"$name-3" => {site => "P", tag => "A", cct_id => 0}};
  $categories[4] = {"$name-4" => {site => "P", tag => "R", cct_id => 0}};
  $categories[5] = {"$name-5" => {site => "R", tag => "A", cct_id => 0}};
  $categories[6] = {"$name-6" => {site => "R", tag => "R", cct_id => 0}};
  $categories[7] = {"$name-7-1" => {site => "A", tag => undef, cct_id => 0},
                    "$name-7-2" => {site => "A", tag => "R", cct_id => 1}};
  $categories[8] = {"$name-8-1" => {site => "A", tag => undef, cct_id => 0},
                    "$name-8-2" => {site => "A", tag => "A", cct_id => 1}};
  $categories[9] = {"$name-9-1" => {site => "P", tag => undef, cct_id => 0},
                    "$name-9-2" => {site => "P", tag => "R", cct_id => 1}};
  $categories[10] = {"$name-10-1" => {site => "P", tag => undef, cct_id => 0},
                     "$name-10-2" => {site => "P", tag => "A", cct_id => 1}};
  $categories[11] = {"$name-11-1" => {site => "R", tag => undef, cct_id => 0},
                     "$name-11-2" => {site => "R", tag => "R", cct_id => 1}};
  $categories[12] = {"$name-12-1" => {site => "R", tag => undef, cct_id => 0},
                     "$name-12-2" => {site => "R", tag => "A", cct_id => 1}};
  $categories[13] = {"$name-13-1" => {site => "A", tag => "A", cct_id => 0},
                     "$name-13-2" => {site => "A", tag => "A", cct_id => 1}};
  $categories[14] = {"$name-14-1" => {site => "A", tag => "A", cct_id => 0},
                     "$name-14-2" => {site => "A", tag => "R", cct_id => 1}};
  $categories[15] = {"$name-15-1" => {site => "A", tag => "R", cct_id => 0},
                     "$name-15-2" => {site => "A", tag => "R", cct_id => 1}};

  # REQ-391. Add one content category for site
  my $category_content = $ns->create(CreativeCategory => {
    name => "$name-content",
    cct_id => 1});

  for (my $i = 1; $i <= 15; ++$i)
  {
    my $size = $ns->create(CreativeSize =>
                           { name => "$name-$i" });

    my $template = $ns->create(Template =>
                               { name => "$name-$i",
                                 size_id => $size,
                                 flags => 2 });

    my $creative = $ns->create(Creative =>
                               { name => "$name-$i",
                                 account_id => $adv_acc,
                                 size_id => $size,
                                 creative_category_id => $category_content,
                                 template_id => $template });

    $ns->create(SiteCreativeApproval => { 
      site_id => $site,
      creative_id => $creative }) if $approved eq 'A';


    my $cc = $ns->create(CampaignCreative =>
                         { ccg_id => $ccg,
                           creative_id => $creative });

    $ns->output("$name/CC Id/$i", $cc, "See ADSC-2499");

    my $tag = $ns->create(PricedTag =>
                          { name => "$name-$i",
                            site_id => $site,
                            size_id => $size });

    $ns->output("$name/Tag Id/$i", $tag, "See ADSC-2499");

    foreach my $cat_name (keys %{$categories[$i]})
    {
      my $category = $ns->create(CreativeCategory =>
                                 { name => $cat_name,
                                   cct_id => $categories[$i]->{$cat_name}->{cct_id} });

      $ns->create(CreativeCategory_Creative =>
                  { creative_category_id => $category,
                    creative_id => $creative });

      $ns->create(SiteCreativeCategoryExclusion =>
                  { site_id => $site,
                    approval => $categories[$i]->{$cat_name}->{site},
                    creative_category_id => $category })
        if $categories[$i]->{$cat_name}->{site} ne "A";

      $ns->create(TagsCreativeCategoryExclusion =>
                  { tag_id => $tag,
                    approval => $categories[$i]->{$cat_name}->{tag},
                    creative_category_id => $category })
        if defined $categories[$i]->{$cat_name}->{tag};
    }
  }

  return;
}

sub many_creatives_in_campaign
{
  my ($ns, $acc_type) = @_;

  my $v_category = $ns->create(CreativeCategory => {
    name => "adsc-default-visual"});

  my $keyword = make_autotest_name($ns, "cmp-2-ccid");

  my $campaign = $ns->create(DisplayCampaign => {
    name => "cmp-with-2-ccid",
    creative_name => "1st",
    behavioralchannel_keyword_list => $keyword,
    creative_category_id => $v_category,
    campaigncreativegroup_flags => 0 });

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));

  my $creative2 = $ns->create(Creative => {
    name => "2nd",
    account_id => $campaign->{account_id},
    creative_category_id => $v_category });

  my $ccid2 = $ns->create(CampaignCreative =>
                          { ccg_id => $campaign->{ccg_id},
                            creative_id => $creative2,
                            weight => 1000 });

  my $c_category_1 = $ns->create(CreativeCategory =>
                                 { name => "Movies",
                                   cct_id => 1 }); # Content

  my $c_category_2 = $ns->create(CreativeCategory =>
                                 { name => "Hotels",
                                   cct_id => 1 }); # Content

  $ns->create(CreativeCategory_Creative =>
              { creative_category_id => $c_category_1,
                creative_id => $campaign->{creative_id} });

  $ns->create(CreativeCategory_Creative =>
              { creative_category_id => $c_category_2,
                creative_id => $creative2 });

  # Publisher
  my $pub_account = $ns->create(PubAccount =>
                                { name => "ADSC-5543-pub1",
                                  account_type_id => $acc_type });

  my $publisher = $ns->create(Publisher =>
                              { name => "ADSC-5543-pub1",
                                account_id => $pub_account,
                                exclusions => [
                                  { creative_category_id => $c_category_2 } ] });

  $ns->output("ADSC-5543-4/TAG", $publisher->{tag_id});
  $ns->output("ADSC-5543-4/CCID1", $campaign->{cc_id});
  $ns->output("ADSC-5543-4/CCID2", $creative2);
  $ns->output("ADSC-5543-4/KWD", $keyword);
}

sub exclusion_by_creative_template_and_tag
{
  my ($ns, $acc_type) = @_;

  my $acc = $ns->create(Account =>
                        { name => "acc",
                          role_id => DB::Defaults::instance()->advertiser_role});

  my $template_id = $ns->create(Template =>
                                { name => "ADSC-5543-5",
                                  flags => 2 });

  my $size = $ns->create(CreativeSize =>
                           { name => 1,
                             max_text_creatives => 1 });

  $ns->create(TemplateFile =>
              { template_id => $template_id,
                size_id => $size,
                template_file => 
                  'UnitTests/banner_img_clk.html',
                flags => 0,
                template_type => 'T'});

  my $v_category = $ns->create(CreativeCategory =>
                               { name => "ADSC-5543-5-v" });

  $ns->create(CreativeCategory_Template =>
              { creative_category_id => $v_category,
                template_id => $template_id});

  my $keyword = make_autotest_name($ns, "ADSC-5543-5");

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "ADSC-5543-5",
    account_id => $acc,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P" )]));

  my $excluded_tag = make_autotest_name($ns, "ADSC-5543-5");

  my $campaign = $ns->create(DisplayCampaign =>
                             { name => "ADSC-5543-5",
                               account_id => $acc,
                               channel_id => $channel->channel_id(),
                               template_id => $template_id,
                               size_id => $size,
                               creative_headline_value => $excluded_tag,
                               campaigncreativegroup_flags => 0 });

  my $c_category = $ns->create(CreativeCategory =>
                               { name => "ADSC-5543-5-c",
                                 cct_id => 1 });

  $ns->create(CreativeCategory_Creative =>
              { creative_category_id => $c_category,
                creative_id => $campaign->{creative_id} });

  my $t_category = $ns->create(CreativeCategory =>
                              { name => $excluded_tag,
                                 cct_id => 2 });

  my $option_group = $template_id->{option_group_id};
  
  my $option =  $ns->create(Options =>
                            { token => "HEADLINE",
                              option_group_id => $option_group,
                              template_id =>  $template_id,
                              creative_id => $campaign->{creative_id},
                              value => $excluded_tag });
  # Publisher
  my $pub_account = $ns->create(PubAccount =>
                                { name => "ADSC-5543-5-p",
                                  account_type_id => $acc_type });

  my $publisher1 = $ns->create(Publisher =>
                               { name => "ADSC-5543-5-s1",
                                 account_id => $pub_account,
                                 size_id => $size });

  my $publisher2 = $ns->create(Publisher =>
                               { name => "ADSC-5543-5-s2",
                                 account_id => $pub_account,
                                 size_id => $size,
                                 exclusions => [
                                   { creative_category_id => $v_category } ] });

  my $publisher3 = $ns->create(Publisher =>
                               { name => "ADSC-5543-5-s3",
                                 account_id => $pub_account,
                                 size_id => $size,
                                 exclusions => [
                                   { creative_category_id => $t_category } ] });

  $ns->output("ADSC-5543-5/TAG1", $publisher1->{tag_id}, "accepted for creative");
  $ns->output("ADSC-5543-5/TAG2", $publisher2->{tag_id}, "rejected for creative");
  $ns->output("ADSC-5543-5/TAG3", $publisher3->{tag_id}, "rejected for creative");
  $ns->output("ADSC-5543-5/KWD", $keyword, "keyword for channel matching");
  $ns->output("ADSC-5543-5/CCID", $campaign->{cc_id});
}

sub text_creatives_exclusion
{
  my ($ns, $acc_type) = @_;

  my $exclusion_tag = $ns->create(CreativeCategory =>
                                  { name => "sony vaio",
                                    cct_id => 2 });

  my $size = $ns->create(CreativeSize =>
                         { name => "TEST-5-6",
                           max_text_creatives => 1 });

  my $account = $ns->create(Account =>
                            { name => "TEST-5-6",
                              role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(Campaign =>
                             { name => "TEST-5-6",
                               campaign_type => 'T',
                               account_id => $account });

  my $pub_account = $ns->create(PubAccount =>
                                { name => "TEST-5-6-p",
                                  account_type_id => $acc_type });

  my $publisher = $ns->create(Publisher =>
                              { name => "TEST-5-6",
                                account_id => $pub_account,
                                size_id => $size,
                                cpm => 1,
                                exclusions => [
                                  { creative_category_id => $exclusion_tag } ] });

  $ns->output("TEST-5-6/TAG", $publisher->{tag_id});

  my @test_case = ({name => "Test 5.1. Order is preserved",
                    options => {HEADLINE => "vaio sony",
                                DESCRIPTION1 => "vaio sony",
                                DESCRIPTION2 => "Description Line 2",
                                DISPLAY_URL => "http://www.sony.com",
                                CLICK_URL => "http://mail.ru"},
                    impression => 1},

                   {name => "Test 5.2. Spaces are normalized",
                    options => {HEADLINE => "sony    vaio",
                                DESCRIPTION1 => "Description Line 1",
                                DESCRIPTION2 => "Description Line 2",
                                DISPLAY_URL => "http://www.sony.com",
                                CLICK_URL => "http://mail.ru"},
                    impression => 0},

                   {name => "Test 5.3. Match is case insensitive",
                    options => {HEADLINE => "HeadLine",
                                DESCRIPTION1 => "Sony Vaio",
                                DESCRIPTION2 => "Description Line 2",
                                DISPLAY_URL => "http://www.sony.com",
                                CLICK_URL => "http://mail.ru"},
                    impression => 0},

                   {name => "Test 5.4. Match at the beginning of the phrase",
                    options => {HEADLINE => "sony vaio laptop",
                                DESCRIPTION1 => "Description Line 1",
                                DESCRIPTION2 => "sony vaio laptop",
                                DISPLAY_URL => "http://www.sony.com",
                                CLICK_URL => "http://mail.ru"},
                    impression => 0},

                   {name => "Test 5.5. Match at the end of the phrase",
                    options => {HEADLINE => "New brand",
                                DESCRIPTION1 => "Description Line 1",
                                DESCRIPTION2 => "Description Line 2",
                                DISPLAY_URL => "http://www.sony.com/brand new sony vaio",
                                CLICK_URL => "http://mail.ru"},
                    impression => 0},

                   {name => "Test 5.6. Soft matching doesn't work",
                    options => {HEADLINE => "sony 1 vaio",
                                DESCRIPTION1 => "sony 1 vaio",
                                DESCRIPTION2 => "sony 1 vaio",
                                DISPLAY_URL => "http://www.sony.com/",
                                CLICK_URL => "http://mail.ru"},
                    impression => 1},

                   {name => "Test 5.7. Match should respect word boundaries (part 1)",
                    options => {HEADLINE => "HeadLine",
                                DESCRIPTION1 => "Description Line 1",
                                DESCRIPTION2 => "sony vaio 312",
                                DISPLAY_URL => "http://www.sony.com/",
                                CLICK_URL => "http://mail.ru"},
                    impression => 0},

                   {name => "Test 5.7. Match should respect word boundaries (part 2)",
                    options => {HEADLINE => "HeadLine",
                                DESCRIPTION1 => "Description Line 1",
                                DESCRIPTION2 => "sony vaio312",
                                DISPLAY_URL => "http://www.sony.com/",
                                CLICK_URL => "http://mail.ru"},
                    impression => 1});

  $ns->output("TEST-5-6/TCcount", scalar @test_case, "test cases count");

  for (my $i = 0; $i < scalar @test_case; ++$i)
  {
    my $keyword = make_autotest_name($ns, "text-" . ($i + 1));
    my $text_campaign = $ns->create(TextAdvertisingCampaign =>
                                    { name => "text-" . ($i + 1),
                                      campaign_id => $campaign,
                                      size_id => $size,
                                      template_id => DB::Defaults::instance()->text_template,
                                      account_id => $account,
                                      original_keyword => $keyword,
                                      campaigncreativegroup_flags => 0,
                                      max_cpc_bid => 100 });

    foreach my $opt (keys %{$test_case[$i]->{options}})
    {
      next if $opt eq "name";

      my $option_group =  DB::Defaults::instance()->text_option_group;

      my $option =  $ns->create(Options =>
        { token => $opt,
          option_group_id => $option_group,
          template_id =>  DB::Defaults::instance()->text_template,
          creative_id => $text_campaign->{creative_id},
          value => $test_case[$i]->{options}->{$opt},
        });
    }

    my $exp_ccid = 0;
    $exp_ccid = $text_campaign->{cc_id} if $test_case[$i]->{impression};

    $ns->output("TEST-5-6/KWD-" . ($i + 1), $keyword, "keyword for creative $text_campaign->{cc_id}");
    $ns->output("TEST-5-6/EXP_CCID-" . ($i + 1), $exp_ccid, "expected ccid for test case #" . ($i + 1));
    $ns->output("TEST-5-6/TestCase-" . ($i + 1), $test_case[$i]->{name}, "test case name");
    
  }

}

sub exclusion_text_by_domain_match_within_creative_token
{
  my ($ns, $acc_type) = @_;

  my $exclusion_tag = $ns->create(CreativeCategory =>
                                  { name => "mdomain.com",
                                    cct_id => 2 });

  my $size = $ns->create(CreativeSize =>
                         { name => "TEST-5-8",
                           max_text_creatives => 1 });

  my $account = $ns->create(Account =>
                            { name => "TEST-5-8",
                              role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(Campaign =>
                             { name => "TEST-5-8",
                               campaign_type => 'T',
                               account_id => $account });

  my $pub_account = $ns->create(PubAccount =>
                                { name => "TEST-5-8-p",
                                  account_type_id => $acc_type });

  my $publisher = $ns->create(Publisher =>
                              { name => "TEST-5-8",
                                account_id => $pub_account,
                                size_id => $size,
                                cpm => 1,
                                exclusions => [
                                  { creative_category_id => $exclusion_tag } ] });

  $ns->output("TEST-5-8/TAG", $publisher->{tag_id});

  my @test_case = ({name => "Test 5.8.1. Short host name in Display URL",
                    options => {DISPLAY_URL => "mdomain.com",
                                CRCLICK => "http://ndomain.com"},
                    impression => 0},

                   {name => "Test 5.8.2. Short host name in Click URL",
                    options => {DISPLAY_URL => "ndomain.com",
                                CRCLICK => "http://mdomain.com"},
                    impression => 0},

                   {name => "Test 5.8.3. Full host name in Display URL",
                    options => {DISPLAY_URL => "www.mdomain.com/",
                                CRCLICK => "http://www.com"},
                    impression => 0},

                   {name => "Test 5.8.4. Full host name in Click URL",
                    options => {DISPLAY_URL => "http://www.autotest.com",
                                CRCLICK => "http://www.mdomain.com/somepath/1.htm"},
                    impression => 0},

                   {name => "Test 5.8.5. Simple string value in Display URL",
                    options => {DISPLAY_URL => "\"http://mdomain.com\"",
                                CRCLICK => "http://www.com"},
                    impression => 1});

  $ns->output("TEST-5-8/TCcount", scalar @test_case, "test cases count");

  for (my $i = 0; $i < scalar @test_case; ++$i)
  {
    my $keyword = make_autotest_name($ns, "TEST-5-8-text-" . ($i + 1));
    my $text_campaign = $ns->create(TextAdvertisingCampaign =>
                                    { name => "TEST-5-8-text-" . ($i + 1),
                                      campaign_id => $campaign,
                                      size_id => $size,
                                      template_id => DB::Defaults::instance()->text_template,
                                      account_id => $account,
                                      original_keyword => $keyword,
                                      ccgkeyword_click_url => undef,
                                      campaigncreativegroup_flags => 0,
                                      max_cpc_bid => 100 });

    my $option_group =  DB::Defaults::instance()->text_option_group;

    foreach my $opt (keys %{$test_case[$i]->{options}})
    {
      next if $opt eq "name";

      my $option =  $ns->create(Options =>
        { token => $opt,
          type => 'URL',
          option_group_id => $option_group,
          template_id =>  DB::Defaults::instance()->text_template,
          creative_id => $text_campaign->{creative_id},
          value => $test_case[$i]->{options}->{$opt},
        });
    }

    my $exp_ccid = 0;
    $exp_ccid = $text_campaign->{cc_id} if $test_case[$i]->{impression};

    $ns->output("TEST-5-8/KWD-" . ($i + 1), $keyword, "keyword for creative $text_campaign->{cc_id}");
    $ns->output("TEST-5-8/EXP_CCID-" . ($i + 1), $exp_ccid, "expected ccid for test case #" . ($i + 1));
    $ns->output("TEST-5-8/TestCase-" . ($i + 1), $test_case[$i]->{name}, "test case name");
    
  }

}

sub exclusion_by_ccg_keyword_click_url
{
  my ($ns, $acc_type) = @_;

  my $exclusion_tag = $ns->create(CreativeCategory =>
                                  { name => "www.nbc.com",
                                    cct_id => 2 });

  my $size = $ns->create(CreativeSize =>
                         { name => "TEST-5-9",
                           max_text_creatives => 1 });

  my $account = $ns->create(Account =>
                            { name => "TEST-5-9",
                              role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(Campaign =>
                             { name => "TEST-5-9",
                               campaign_type => 'T',
                               account_id => $account });

  my $pub_account = $ns->create(PubAccount =>
                                { name => "TEST-5-9-p",
                                  account_type_id => $acc_type });

  my $publisher = $ns->create(Publisher =>
                              { name => "TEST-5-9",
                                account_id => $pub_account,
                                size_id => $size,
                                cpm => 1,
                                exclusions => [
                                  { creative_category_id => $exclusion_tag } ] });

  $ns->output("TEST-5-9/TAG", $publisher->{tag_id});

  my @test_case = ({name => "Test 5.9.1 Short host name in CCG Keyword Click URL",
                    options => {DISPLAY_URL => "http://www.autotest.com",
                                CRCLICK => "http://www.com"},
                    ccgkeyword_click_url =>  "http://nbc.com",
                    impression => 0},

                   {name => "Test 5.9.2 Full host name in CCG Keyword Click URL",
                    options => {DISPLAY_URL => "http://www.autotest.com",
                                CRCLICK => "http://www.com"},
                    ccgkeyword_click_url =>  "http://www.NBC.cOM",
                    impression => 0});

  $ns->output("TEST-5-9/TCcount", scalar @test_case, "test cases count");

  for (my $i = 0; $i < scalar @test_case; ++$i)
  {
    my $keyword = make_autotest_name($ns, "TEST-5-9-text-" . ($i + 1));
    my $text_campaign = $ns->create(TextAdvertisingCampaign =>
                                    { name => "TEST-5-9-text-" . ($i + 1),
                                      campaign_id => $campaign,
                                      size_id => $size,
                                      template_id => DB::Defaults::instance()->text_template,
                                      account_id => $account,
                                      original_keyword => $keyword,
                                      ccgkeyword_click_url => $test_case[$i]->{ccgkeyword_click_url},
                                      campaigncreativegroup_flags => 0,
                                      max_cpc_bid => 100 });

    foreach my $opt (keys %{$test_case[$i]->{options}})
    {
      next if $opt eq "name";

      my $option_group =  DB::Defaults::instance()->text_option_group;

      my $option =  $ns->create(Options =>
        { token => $opt,
          option_group_id => $option_group,
          template_id =>  DB::Defaults::instance()->text_template,
          creative_id => $text_campaign->{creative_id},
          value => $test_case[$i]->{options}->{$opt} });

    }

    my $exp_ccid = 0;
    $exp_ccid = $text_campaign->{cc_id} if $test_case[$i]->{impression};

    $ns->output("TEST-5-9/KWD-" . ($i + 1), $keyword, "keyword for creative $text_campaign->{cc_id}");
    $ns->output("TEST-5-9/EXP_CCID-" . ($i + 1), $exp_ccid, "expected ccid for test case #" . ($i + 1));
    $ns->output("TEST-5-9/TestCase-" . ($i + 1), $test_case[$i]->{name}, "test case name");
    
  }

}

sub init
{
  my ($self, $ns) = @_;

  my($exc_acc_type_id, $inc_acc_type_id);
  $exc_acc_type_id = $ns->create(AccountType =>
                                 { name => "Sexcl",
                                   account_role_id => DB::Defaults::instance()->publisher_role,
                                   adv_exclusions => 'S',
                                   flags => DB::AccountType::FREQ_CAPS });
  $inc_acc_type_id = $ns->create(AccountType =>
                                 { name => "nSexcl",
                                   account_role_id => DB::Defaults::instance()->publisher_role,
                                   flags => DB::AccountType::FREQ_CAPS });

  SiteLevelExlusions($ns, "1-se1", "R", $exc_acc_type_id, 0);
  SiteLevelExlusions($ns, "1-se2", "A", $exc_acc_type_id, 0);

  SiteLevelExlusions($ns, "1-ne1", "R", $inc_acc_type_id, 0);
  SiteLevelExlusions($ns, "1-ne2", "A", $inc_acc_type_id, 0);

  SiteLevelExlusions($ns, "1-wse1", "R", $exc_acc_type_id, 2);
  SiteLevelExlusions($ns, "1-wse2", "A", $exc_acc_type_id, 2);
  SiteLevelExlusions($ns, "1-wne1", "R", $inc_acc_type_id, 2);
  SiteLevelExlusions($ns, "1-wne2", "A", $inc_acc_type_id, 2);

  my $site_tag_exclusion = $ns->create(AccountType =>
                                      { name => "STexcl",
                                        account_role_id => DB::Defaults::instance()->publisher_role,
                                        adv_exclusions => 'T',
                                        flags => DB::AccountType::FREQ_CAPS });

  SiteTagLevelExclusions($ns, "2-ne1", "R", $inc_acc_type_id, 0);
  SiteTagLevelExclusions($ns, "2-ne2", "A", $inc_acc_type_id, 0);

  SiteTagLevelExclusions($ns, "2-se1", "R", $exc_acc_type_id, 0);
  SiteTagLevelExclusions($ns, "2-se2", "A", $exc_acc_type_id, 0);

  SiteTagLevelExclusions($ns, "2-ste1", "R", $site_tag_exclusion, 0);
  SiteTagLevelExclusions($ns, "2-ste2", "A", $site_tag_exclusion, 0);

  SiteTagLevelExclusions($ns, "2-wne1", "R", $inc_acc_type_id, 2);
  SiteTagLevelExclusions($ns, "2-wne2", "A", $inc_acc_type_id, 2);

  SiteTagLevelExclusions($ns, "2-wse1", "R", $exc_acc_type_id, 2);
  SiteTagLevelExclusions($ns, "2-wse2", "A", $exc_acc_type_id, 2);

  SiteTagLevelExclusions($ns, "2-wste1", "R", $site_tag_exclusion, 2);
  SiteTagLevelExclusions($ns, "2-wste2", "A", $site_tag_exclusion, 2);

  #ADSC-5543
  many_creatives_in_campaign($ns, $exc_acc_type_id);
  exclusion_by_creative_template_and_tag($ns, $exc_acc_type_id);
  text_creatives_exclusion($ns, $exc_acc_type_id);
  #ADSC-5997
  exclusion_text_by_domain_match_within_creative_token($ns, $site_tag_exclusion);
  exclusion_by_ccg_keyword_click_url($ns, $site_tag_exclusion);
}

1;

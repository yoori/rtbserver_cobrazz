package CreativeFilesPresenceTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub CreateCreativeOptions
{
  my ($ns, $acc, $num, $args) = @_;

  my $creative_id = undef;
  if (defined $args)
  {
    if (ref($args) eq 'ARRAY')
    {
      my @opts = @{$args};
      my $template = $ns->create(Template => {
          name => 'Templ'.$num,
          template_file => 'UnitTests/banner_img_clk.html',
          app_format_id => DB::Defaults::instance()->app_format_track,
          flags => 0 });
      $creative_id = $ns->create(Creative => {
          name => 'Creative'.$num,
          template_id => $template,
          account_id => $acc });
      foreach my $opt (@opts)
      {
        my $o = $ns->create(Options => {
            token => $opt->{token},
            type => $opt->{type},
            template_id => $template,
            default_value => exists $opt->{default} ? $opt->{default} : undef,
            option_group_id => $template->{option_group_id}});
        my $v = $ns->create(CreativeOptionValue => {
            option_id => $o,
            creative_id => $creative_id,
            value => $opt->{value}}) if $opt->{value};
      }
    }
    else
    {
      $creative_id = $ns->create(Creative => {
          name => $num,
          template_id => $args,
          account_id => $acc });
    }
  }
  else
  {
    $creative_id = $ns->create(Creative => {
      name => $num,
      account_id => $acc });
  }
  return $creative_id;
}

sub CreateCreativeOptionValue
{
  my ($ns, $acc, $num, $option_group, $creative_id, $opt) = @_;

  my $o = $ns->create(Options => {
      token => $opt->{token},
      type => $opt->{type},
      template_id => $option_group->{template_id},
      default_value => exists $opt->{default} ? $opt->{default} : undef,
      option_group_id => $option_group});
  my $v = $ns->create(CreativeOptionValue => {
      option_id => $o,
      creative_id => $creative_id,
      value => $opt->{value}});
}

sub CreateCreativeOptionValues
{
  my ($ns, $acc, $num, $option_group, $creative_id, $args) = @_;

  if (defined $args)
  {
    if (ref($args) eq 'ARRAY')
    {
      my @opts = @{$args};
      foreach my $opt (@opts)
      {
        CreateCreativeOptionValue($ns, $acc, $num, $option_group, $creative_id, $opt);
      }
    }
  }
}

sub CreateHelper
{
  my ($ns, $acc, $num, $args) = @_;
    
  my $keyword = make_autotest_name($ns, "Keyword".$num);
  $ns->output("KEYWORD".$num, $keyword, "keyword for ad request");

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => $num,
      keyword_list => $keyword,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $publisher = $ns->create(Publisher => {
      name => $num});

  my $creative_id = CreateCreativeOptions($ns, $acc, $num, $args);

  my $campaign = $ns->create(DisplayCampaign => {
    name => "$num",
    account_id => $acc,
    channel_id => $bp->channel_id,
    creative_id => $creative_id,
    site_links => [ { site_id => $publisher->{site_id} } ]  });

  $ns->output("CCG".$num, $campaign->{ccg_id});
  $ns->output("TID".$num, $publisher->{tag_id});
  $ns->output("CCID".$num, $campaign->{cc_id});

  return $creative_id;
}

sub CreateHelperText
{
  my ($ns, $acc, $num, $publisher, $size, $args) = @_;
 
  my $keyword = make_autotest_name($ns, "Keyword".$num);
  $ns->output("KEYWORD".$num, $keyword, "keyword for ad request");

  my $template = DB::Defaults::instance()->text_template;
  my $option_group = DB::Defaults::instance()->text_option_group;

  my $campaign = $ns->create(TextAdvertisingCampaign => { 
    name => "Text".$num,
    size_id => $size,
    template_id => $template,
    original_keyword => $keyword,
    max_cpc_bid => 0.12,
    ccgkeyword_ctr => 0.1,
    site_links => [{site_id => $publisher->{site_id} }] });

  my $creative = $campaign->{creative_id};

  CreateCreativeOptionValues($ns, $acc, $num, $option_group,
    $creative, $args);

  $ns->output("CCG".$num, $campaign->{ccg_id}, "Campaign id");

  $ns->output("CCID".$num, $campaign->{cc_id}, "Creative#".$num);

  return $campaign->{creative_id};
}

sub CreateHelperTextN
{
  my ($ns, $acc, $num, $publisher, $size, $args) = @_;
 
  my $keyword = make_autotest_name($ns, "Keyword".$num);
  $ns->output("KEYWORD".$num, $keyword, "keyword for ad request");

  my $template = DB::Defaults::instance()->text_template;
  my $option_group = DB::Defaults::instance()->text_option_group;

  my $campaign = $ns->create(TextAdvertisingCampaign => { 
    name => "Text".$num,
    size_id => $size,
    template_id => $template,
    original_keyword => $keyword,
    max_cpc_bid => 0.12,
    ccgkeyword_ctr => 0.1,
    site_links => [{site_id => $publisher->{site_id} }] });

  my $ccg_id = $campaign->{ccg_id};

  my @opts = @{$args};
  my $cnum = 1;
  foreach my $opt (@opts)
  {    
    my $creative = undef;
    my $cc_id = undef;
    if ($cnum eq 1)
    {
      $creative = $campaign->{creative_id};
      $cc_id = $campaign->{cc_id};
    }
    else
    {
      $creative = $ns->create(Creative => {
        name => 'Creative'.$num."_".$cnum,
        account_id => $campaign->{account_id},
        size_id => $size,
        template_id => $template });
      $cc_id = $ns->create(CampaignCreative => {
        ccg_id => $ccg_id,
        creative_id => $creative
        });
    }
    CreateCreativeOptionValue($ns, $acc, $num, $option_group,
      $creative, $opt);
    $ns->output("CCID".$num."_".$cnum, $cc_id, "Creative#".$num."_".$cnum);
    $cnum = $cnum + 1;
  }
  $ns->output("CCG".$num, $ccg_id, "Campaign id");
}

sub CreateHelper6
{
  my ($ns, $acc, $num, $args) = @_;
    
  my $keyword = make_autotest_name($ns, "Keyword".$num);
  $ns->output("KEYWORD".$num, $keyword, "keyword for ad request");

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => $num,
      keyword_list => $keyword,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $publisher = $ns->create(Publisher => {
      name => $num});

  my $creativea_id = $ns->create(Creative => {
      name => $num."a",      
      account_id => $acc });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "$num",
    account_id => $acc,
    channel_id => $bp->channel_id,
    creative_id => $creativea_id,
    campaigncreative_weight => 1,
    site_links => [ { site_id => $publisher->{site_id} } ]  });


  $ns->output("CCG".$num, $campaign->{ccg_id});
  $ns->output("TID".$num, $publisher->{tag_id});

  my $creativew_id = CreateCreativeOptions($ns, $acc, $num."w", $args);
  $ns->output("CCID".$num."A", $campaign->{cc_id});

  my $ccw_id = $ns->create(CampaignCreative => {
    ccg_id => $campaign->{ccg_id},
    weight => 10000,
    creative_id => $creativew_id });

  $ns->output("CCID".$num."W", $ccw_id);

  return $creativew_id;
}

sub CreateHelper7
{
  my ($ns, $acc, $num) = @_;
    
  my $keyword = make_autotest_name($ns, "Keyword".$num);
  $ns->output("KEYWORD".$num, $keyword, "keyword for ad request");

  my $bp = $ns->create(
    DB::BehavioralChannel->blank(
      name => $num,
      keyword_list => $keyword,
      account_id => $acc,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P') ]));

  my $size_id = $ns->create(CreativeSize => {
      name => $num,
      max_text_creatives => 2});

  my $publisher = $ns->create(Publisher => {
      name => $num,
      pricedtag_size_id => $size_id});

  my $template_id = $ns->create(Template => {
      name => $num,
      flags => 0 });

  my $tafilea = $ns->create(TemplateFile => {
      template_id => $template_id,
      size_id => $size_id,
      template_file => 'UnitTests/simple.html',
      flags => 0,
      app_format_id => DB::Defaults::instance()->app_format_no_track,
      template_type => 'T'});

  my $tafilew = $ns->create(TemplateFile => {
      template_id => $template_id,
      size_id => $size_id,
      template_file => 'no_such_file.html',
      flags => 0,
      app_format_id => DB::Defaults::instance()->app_format_track,
      template_type => 'T'});
  
  my $creative_id = $ns->create(Creative => {
      name => $num,
      size_id => $size_id,
      template_id => $template_id,
      account_id => $acc });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "$num",
    account_id => $acc,
    channel_id => $bp->channel_id,
    creative_id => $creative_id,
    site_links => [ { site_id => $publisher->{site_id} } ]  });

  $ns->output("TID".$num, $publisher->{tag_id});
  $ns->output("CCID".$num, $campaign->{cc_id});

  return $creative_id;
}

sub ADSC_8367
{
  my ($ns, $account, $file) = @_;
 
  my $kwd = make_autotest_name($ns, "ADSC-8367");
  $ns->output('ADSC-8367/KEYWORD', $kwd);

  my $publisher = $ns->create(Publisher => {
    name => "ADSC-8367" });

  $ns->output('ADSC-8367/TID', $publisher->{tag_id});

  my $template = $ns->create(Template => {
    name => 'ADSC-8367',
    template_file => 'UnitTests/banner_img_clk.html',
    app_format_id => DB::Defaults::instance()->app_format_no_track,
    flags => 0 });

  my $dynamic_file = $ns->create(Options => {
    token => 'ADSC8367_FILE',
    type => 'Dynamic File',
    template_id => $template,
    option_group_id =>
      $ns->create(DB::OptionGroup->blank(
        name => "Template-" . $template->{template_id} . "-h",
        template_id => $template,
        type => 'Hidden' )) });

  my $creative_A = $ns->create(Creative => {
    name => 'ADSC-8367-A',
    template_id => $template,
    account_id => $account });

  $ns->create(CreativeOptionValue => {
    creative_id => $creative_A,
    option_id => $dynamic_file,
    value => $file });

  my $creative_W = $ns->create(Creative => {
    name => 'ADSC-8367-W',
    template_id => $template,
    account_id => $account });

  $ns->create(CreativeOptionValue => {
    creative_id => $creative_W,
    option_id => $dynamic_file,
    value => '/no_such_file.html' });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "ADSC-8367-1",
    account_id => $account,
    behavioralchannel_keyword_list => $kwd,
    creative_id => $creative_W,
    campaigncreative_weight => 200,
    site_links =>
      [{ site_id => $publisher->{site_id} }] });

  my $cc_A = $ns->create(CampaignCreative => {
    ccg_id => $campaign->{ccg_id},
    creative_id => $creative_A,
    weight => 2 });

  $ns->output('ADSC-8367/CCID/A', $cc_A);
  $ns->output('ADSC-8367/CCID/W', $campaign->{cc_id});

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" )); 
}

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Advertiser => {
    name => 'Adveriser'});

  my $existing_file = "/2/103714/test_folder/Penguins.jpg";

  CreateHelper($ns, $acc, 1, [
    {token => "ABSENTFILE",
     type => "File",
     value => "/CreativeFilesPresenceTest-AbsentFile.txt"}]);

  CreateHelper($ns, $acc, 2);

  CreateHelper($ns, $acc, 3, [
    {token => "ABSENTURL",
     type => "File/URL",
     value => "no such url"}]);

  CreateHelper($ns, $acc, 4,
  [
   {token => "PRESENTURL",
    type => "File/URL",
    value => "http://www.nic.com/nic/whois"},
   {token => "PRESENTFILE",
    type => "File",
    value => $existing_file}
   ]
  );

  #
  my $template_id = $ns->create(Template => {
    name => 5,
    template_file => '/no_such_file',
    template_file_type => 'T',
    flags => 0 });

  CreateHelper($ns, $acc, 5, $template_id);

  CreateHelper6($ns, $acc, 6, [
    {token => "ABSENTURL",
     type => "File/URL",
     value => "no such url"}]);

  CreateHelper7($ns, $acc, 7);

  my $size8 = $ns->create(CreativeSize => { 
    name => "Size8",
    max_text_creatives => 2 });

  my $publisher8 = $ns->create(Publisher => {
    name => "Publisher8",
    pricedtag_size_id => $size8,
    pricedtag_cpm => 1 });

  $ns->output("TID8", $publisher8->{tag_id}, "tid for ad request");

  CreateHelperText($ns, $acc, "8_1", $publisher8, $size8, [
    {token => "PRESENTFILE",
     type => "File",
     default_value => $existing_file,
     value => $existing_file}]);

  CreateHelperText($ns, $acc, "8_2", $publisher8, $size8, [
    {token => "ABSENTFILE",
     type => "File",
     default_value => $existing_file,
     value => "/CreativeFilesPresenceTest-AbsentFile.txt"}]);

  my $size9 = $ns->create(CreativeSize => { 
    name => "Size9",
    max_text_creatives => 2 });

  my $publisher9 = $ns->create(Publisher => {
    name => "Publisher9",
    pricedtag_size_id => $size9,
    pricedtag_cpm => 1 });

  $ns->output("TID9", $publisher9->{tag_id}, "tid for ad request");

  CreateHelperTextN($ns, $acc, "9", $publisher9, $size9, [
    {token => "ABSENTFILE",
     type => "File",
     default_value => $existing_file,
     value => "/CreativeFilesPresenceTest-AbsentFile.txt"},
    {token => "PRESENTFILE",
     type => "File",
     default_value => $existing_file,
     value => $existing_file}]);

  ADSC_8367($ns, $acc, $existing_file);
}

1;

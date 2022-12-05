
package TextAdNetAndGrossTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub helper
{
  my ($ns, $self) = @_;

  my $keyword1 = make_autotest_name($ns, $self->{prefix}."-1");
  my $keyword2 = make_autotest_name($ns, $self->{prefix}."-2");
  my $keyword3 = make_autotest_name($ns, $self->{prefix}."-3");

  $ns->output("min_cpm/".$self->{prefix}, $self->{tag}{cpm});

  my $campaign1_max_cpc_bid = $self->{campaigns}[0]{max_cpc_bid} * $self->{tag}{cpm}; 
  my $campaign2_max_cpc_bid = $self->{campaigns}[1]{max_cpc_bid} * $self->{tag}{cpm}; 
  my $campaign3_max_cpc_bid = $self->{campaigns}[2]{max_cpc_bid} * $self->{tag}{cpm}; 

  my $min_cpm_tag = $self->{tag}{cpm} * (1.0 - $self->{tag}{commission});
  $ns->output("min_cpm_tag/".$self->{prefix}, $min_cpm_tag);

  $ns->output("KEYWORD/".$self->{prefix}."/1", $keyword1);
  $ns->output("KEYWORD/".$self->{prefix}."/2", $keyword2);
  $ns->output("KEYWORD/".$self->{prefix}."/3", $keyword3);

  my $publisher = $ns->create(PubAccount => {
      # use only for publisher (tag cpm)
      name => "pub".$self->{prefix},
      commission =>  $self->{tag}{commission}
      });

  my $size = $ns->create(CreativeSize => {
      name => $self->{prefix} . 'Size',
      max_text_creatives => 
        $self->{max_text_creatives} });

  my $campaign1 = $ns->create(TextAdvertisingCampaign => { 
      # Common
      name => $self->{prefix}."-1",
      # CreativeSize
      size_id => $size,
      # Creative
      template_id => DB::Defaults::instance()->text_template,
      advertiser_agency_account_id => 
        DB::Defaults::instance()->agency_gross(),
      # Campaign
      campaign_commission => $self->{campaigns}[0]{commission},
      # CCGKeyword
      original_keyword => $keyword1,
      max_cpc_bid => $campaign1_max_cpc_bid,
      site_links => [{name => $self->{prefix}."-1"},
                     {name => $self->{prefix}."-2",
                      account_id => $publisher }] } );

  my $site1 = $campaign1->{Site}[0]->{site_id};                        
  my $site2 = $campaign1->{Site}[1]->{site_id};                        
  $ns->output("max_cpc_bid/".$self->{prefix}."/1",
     $campaign1_max_cpc_bid*(1.0 - $self->{campaigns}[0]{commission}));

  # NET TextAdNetAndGrossTest campaign
  my $campaign2 = $ns->create(TextAdvertisingCampaign => { 
      # Common
      name => $self->{prefix}."-2",
      # Use campaign1 CreativeSize
      size_id => $size,
      # Campaign
      campaign_commission => $self->{campaigns}[1]{commission},
      # Creative
      template_id => DB::Defaults::instance()->text_template,
      # Use campaign1 TemplateFile
      template_file_id => $campaign1->{template_file_id},
      # CCGKeyword
      original_keyword => $keyword2,
      max_cpc_bid => $campaign2_max_cpc_bid ,
      site_links => [{site_id => $site1},
                     {site_id => $site2}] } );
  $ns->output("max_cpc_bid/".$self->{prefix}."/2",
     $campaign2_max_cpc_bid*(1.0 - $self->{campaigns}[1]{commission}));
    
  # Simple TextAdNetAndGrossTest campaign
  my $campaign3 = $ns->create(TextAdvertisingCampaign => { 
      # Common
      name => $self->{prefix}."-3",
      # Use campaign1 CreativeSize
      size_id => $size,
      # Campaign
      campaign_commission => $self->{campaigns}[2]{commission},
      # Creative
      template_id => DB::Defaults::instance()->text_template,
      # Use campaign1 TemplateFile
      template_file_id => $campaign1->{template_file_id},
      # CCGKeyword
      original_keyword => $keyword3,
      max_cpc_bid => $campaign3_max_cpc_bid,
      site_links => [{site_id => $site1},
                     {site_id => $site2}] } );
  $ns->output("max_cpc_bid/".$self->{prefix}."/3", $campaign3_max_cpc_bid);

  $ns->output("CCID/".$self->{prefix}."/1", $campaign1->{cc_id});
  $ns->output("CCID/".$self->{prefix}."/2", $campaign2->{cc_id});
  $ns->output("CCID/".$self->{prefix}."/3", $campaign3->{cc_id});


  # Create 2 tags for text advertising showing
  $ns->output("tag_cpm/".$self->{prefix}, $self->{tag}{cpm});
  my $tag1   = $ns->create(PricedTag => {
      name => $self->{prefix}."-1",
      site_id => $site1,
      size_id => $size,
      cpm => $self->{tag}{cpm} } );

  $ns->output("TAG/".$self->{prefix}."/1", $tag1);

  my $tag2   = $ns->create(PricedTag => { 
      name => $self->{prefix}."-2",
      site_id => $site2,
      size_id => $size,
      cpm => $self->{tag}{cpm} } );
  
  $ns->output("TAG/".$self->{prefix}."/2", $tag2);

  $ns->output("PUB/".$self->{prefix}."/1",
    $campaign1->{Site}[0]->{account_id}, "acc_id");
  $ns->output("PUB/".$self->{prefix}."/2",
    $campaign1->{Site}[1]->{account_id}, "acc_id");
}

sub helper_2gross
{
  my ($ns, $self) = @_;
 
  my $min_cpm_tag1 = 
    $self->{tags}[0]{cpm} / (1.0 - $self->{publisher_commission});
  my $min_cpm_tag2 = 
   $self->{tags}[1]{cpm} / (1.0 - $self->{publisher_commission});
  $ns->output("min_cpm_tag/".$self->{prefix}."/1", $min_cpm_tag1);
  $ns->output("min_cpm_tag/".$self->{prefix}."/2", $min_cpm_tag2);

  my $keyword1 = make_autotest_name($ns, $self->{prefix}."-1");
  my $keyword2 = make_autotest_name($ns, $self->{prefix}."-2");
  $ns->output("KEYWORD/".$self->{prefix}."/1", $keyword1);
  $ns->output("KEYWORD/".$self->{prefix}."/2", $keyword2);

  my $publisher = $ns->create(PubAccount => {
      # use only for publisher (tag cpm)
      name => "pub-".$self->{prefix},
      commission => $self->{publisher_commission}
      });

  $ns->output("PUB/".$self->{prefix}, $publisher, "acc_id");

  my $size = $ns->create(CreativeSize => {
      name => $self->{prefix} . 'Size',
      max_text_creatives => 
        $self->{max_text_creatives} });

  my $campaign1 = $ns->create(TextAdvertisingCampaign => { 
      # Common
      name => $self->{prefix}."-1",
      # CreativeSize
      size_id => $size,
      # Creative
      template_id => DB::Defaults::instance()->text_template,
      # Account
      advertiser_agency_account_id => 
        DB::Defaults::instance()->agency_gross(),
      # Campaign
      campaign_commission => $self->{campaigns}[0]{commission},
      # CCGKeyword
      original_keyword => $keyword1,
      max_cpc_bid => $self->{campaigns}[0]{max_cpc_bid},
      site_links => [{name => $self->{prefix}."-1",
                      account_id => $publisher}] } );

  $ns->output("CCID/".$self->{prefix}."/1", $campaign1->{cc_id});
  $ns->output("max_cpc_bid/".$self->{prefix}."/1",
    $self->{campaigns}[0]{max_cpc_bid} 
      * (1.0 - $self->{campaigns}[0]{commission}));

  my $site1 = $campaign1->{Site}[0]->{site_id};

  my $campaign2 = $ns->create(TextAdvertisingCampaign => { 
      # Common
      name => $self->{prefix}."-2",
      # Use campaign1 CreativeSize
      size_id => $size,
      # Creative
      template_id => DB::Defaults::instance()->text_template,
      # Use campaign1 TemplateFile
      template_file_id => $campaign1->{template_file_id},
      # Account
      advertiser_agency_account_id => 
        DB::Defaults::instance()->agency_gross(),
      # Campaign
      campaign_commission => $self->{campaigns}[1]{commission},
      # CCGKeyword
      original_keyword => $keyword2,
      max_cpc_bid => $self->{campaigns}[1]{max_cpc_bid} ,
      site_links => [{site_id => $site1}] } );
  
  $ns->output("CCID/".$self->{prefix}."/2", $campaign2->{cc_id});
  $ns->output("max_cpc_bid/".$self->{prefix}."/2",
    $self->{campaigns}[1]{max_cpc_bid}
      * (1.0 - $self->{campaigns}[1]{commission}));

  # Create 2 tags for text advertising showing
  my $tag1 = $ns->create(PricedTag => {
      name => $self->{prefix}."-1",
      site_id => $site1,
      size_id => $size,
      cpm => $min_cpm_tag1 });

  $ns->output("TAG/".$self->{prefix}."/1", $tag1);

  my $tag2 = $ns->create(PricedTag => { 
      name => $self->{prefix}."-2",
      site_id => $site1,
      size_id => $size,
      cpm => $min_cpm_tag2 } );

  $ns->output("TAG/".$self->{prefix}."/2", $tag2);
}

sub init
{
  my ($self, $ns) = @_;

  helper($ns,
         { prefix => "TWAWC", # tag with and without commission
           max_text_creatives => 3,
           tag => {
               cpm => 3, 
               commission => 0.95},
           campaigns => 
               [{max_cpc_bid => 0.3,  commission => 0.9}, #gross
                {max_cpc_bid => 0.25, commission => 0.6}, #net
                {max_cpc_bid => 0.01, commission => 0.0}]}); #simple

  helper($ns,
         { prefix => "COMPETITION",
           max_text_creatives => 2,
           tag => {
               cpm => 5, 
               commission => 0.3},
           campaigns => 
               [{max_cpc_bid => 10, commission => 0.3}, #gross
                {max_cpc_bid => 8,  commission => 0.0}, #net
                {max_cpc_bid => 7,  commission => 0.0}]}); #simple

  helper_2gross($ns, 
               { prefix => "PUBLISHER-COMMISSION",
                 max_text_creatives => 1,
                 # first campaign not shown because percent
                 # second campaign shown because 0 percent
                 # cpm calculated from commission - 3.5 -> 5.83, 53 -> 88.3
                 publisher_commission => 0.4,
                 tags => [{cpm => 3.5}, {cpm => 53}],
                 campaigns =>
                     [{max_cpc_bid => 7.1429, commission => 0.3},
                      {max_cpc_bid => 7.1429, commission => 0.0}]});
}

1;

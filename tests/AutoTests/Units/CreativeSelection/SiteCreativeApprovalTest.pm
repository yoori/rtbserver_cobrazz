
package SiteCreativeApprovalTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_publishers {
  my ($self, $ns, $creative, $visual, $content) = @_;

  my @app_ex = (
    ['P', [], [] ],
    ['A', [], [] ],
    ['R', [], [] ],
    ['P', [{ approval => 'P', creative_category_id => $content }], [] ],
    ['A', [{ approval => 'P', creative_category_id => $content }], [] ],
    ['R', [{ approval => 'P', creative_category_id => $content }], [] ],
    ['A', [{ approval => 'R', creative_category_id => $visual },
           { approval => 'R', creative_category_id => $content }], [] ],
    ['R', [{ approval => 'R', creative_category_id => $visual }], [] ],
    ['A', [{ approval => 'R', creative_category_id => $visual }],
          [{ approval => 'A', creative_category_id => $visual }]],
    ['A', [], [{ approval => 'R', creative_category_id => $visual }] ]);

  my $i = 0;
  my @publishers = ();

  my $account_type = 
    $ns->create( AccountType => { 
      name => "ExclAccType",
      account_role_id => DB::Defaults::instance()->publisher_role,
      adv_exclusion_approval => 'A',
      adv_exclusions => 'T'});

  for my $app_ex (@app_ex)
  {
    my ($site_approval, $site_categories, $tag_categories) = @$app_ex;

    my $publisher = 
      $ns->create(Publisher => {
        name => "Publisher" . ++$i,
        pubaccount_account_type_id => $account_type,
        pricedtag_flags => scalar(@$tag_categories)?
          DB::Tags::TAG_LEVEL_EXCLUSION: 0,
        exclusions => $site_categories,
        tag_exclusions => $tag_categories,
        creative_links => defined $site_approval? [{ creative_id => $creative, approval => $site_approval }]: [] });

    $ns->output("TAG/$i", $publisher->{tag_id});
    push @publishers, $publisher;
  }
  return @publishers;
}

sub init {
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, "KWD");

  my $advertiser = 
    $ns->create(Advertiser => {
      name => "Advertiser"});

  my $visual = 
    $ns->create(CreativeCategory => {
       name => "Visual",
       cct_id => 0 });

  my $content = 
    $ns->create(CreativeCategory => {
       name => "Content",
       cct_id => 1 });

  my $template = 
   $ns->create( Template => {
      name => "Template",
      creative_category_id => $visual });

  $ns->create(TemplateFile => {
    template_id => $template,
    template_file => 'UnitTests/textad.xsl',
    template_type => 'X',
    flags => 0});

  my $creative = 
    $ns->create(Creative =>{
      name => "Creative",
      account_id => $advertiser,
      creative_category_id => $content,
      template_id => $template });

  my @site_links = map
    { site_id => $_->{site_id} },
     $self->create_publishers(
       $ns, $creative, $visual, $content);

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    channel_id => DB::BehavioralChannel->blank(
      account_id => $advertiser,
      name => 'Channel',
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
    campaigncreativegroup_cpm => 100,
    creative_id => $creative,
    site_links => \@site_links });

  $ns->output("KWD", $keyword);
  $ns->output("CC", $campaign->{cc_id});
}

1;

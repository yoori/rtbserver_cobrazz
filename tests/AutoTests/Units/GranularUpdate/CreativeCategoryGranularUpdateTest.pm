package CreativeCategoryGranularUpdateTest::TestCase;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_campaign
{
  my ($self, $args) = @_;

  my $keyword = make_autotest_name($self->{ns_}, "Keyword");

  die "Category '$args->{headline}' is not defined" .
    " in the category list of '$self->{prefix_}'!" 
      if defined $args->{headline} and 
        not defined $self->{category_names_}->{$args->{headline}};

  my $advertiser = $self->{ns_}->create(
    Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  $self->{campaign_} = $self->{ns_}->create(
    DisplayCampaign => {
      name => 'Campaign',
      account_id => $advertiser,
      template_id => $self->{template_},
      size_id => $self->{size_},
      creative_headline_value => 
        defined $args->{headline}?
          $self->{category_names_}->{$args->{headline}}: undef,
      channel_id => DB::BehavioralChannel->blank(
        name => 'Channel',
        account_id => $advertiser,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => "P") ]),
      campaigncreativegroup_cpm => 10, 
        site_links => [
          {site_id => $self->{publisher_}->{site_id}}] });

  $self->{ns_}->output("CC",  $self->{campaign_}->{cc_id});
  $self->{ns_}->output("CCG",  $self->{campaign_}->{ccg_id});
  $self->{ns_}->output("CREATIVE",  $self->{campaign_}->{creative_id});
  $self->{ns_}->output("KWD", $keyword);
}

sub create_publisher
{
  my ($self, $args) = @_;

  $self->{publisher_} = 
    $self->{ns_}->create(
      Publisher => {
        name => "Publisher",
        pubaccount_type_adv_exclusions => 'S',
        size_id => $self->{size_} });

  $self->{ns_}->output("TAG",  $self->{publisher_}->{tag_id});
}

sub create_template
{
  my ($self, $args) = @_;

  $self->{template_} = $self->{ns_}->create(Template => {
    name => 'Template',
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0 });

  $self->{ns_}->create(TemplateFile => {
    template_id => $self->{template_},
    size_id =>  $self->{size_},
    template_file => 'UnitTests/banner_img_clk.html',
    flags => 0,
    template_type => 'T'});
}

sub create_categories
{
  my ($self, $args) = @_;
  my $name;
  for my $cat (@{$args->{categories}})
  {
    if (defined $cat->{cct_id})
    {
      my $category = $self->{ns_}->create(
        CreativeCategory => {
          name => $cat->{name},
          cct_id => $cat->{cct_id} });

      $self->{ns_}->output(
        "CATEGORY/" .  $cat->{name},  $category);

      push (@{$self->{categories_}}, $category);

      $name =  $category->{name};
    }
    else
    {
      $name = 
        make_autotest_name($self->{ns_}, $cat->{name});
      $self->{ns_}->output(
        "CATEGORYNAME/" .  $cat->{name}, $name);
    }
    $self->{category_names_}->{$cat->{name}} = $name;
  }
}

sub link_categories
{
  my ($self, $args) = @_;
  for my $category (@{$self->{categories_}})
  {
    if ( $category->{cct_id} != 2 )
    {
      $self->{ns_}->create(
        CreativeCategory_Creative => {
          creative_category_id => $category,
          creative_id => $self->{campaign_}->{creative_id} });
    }
  }
}

sub create_site_exclusion
{
  my ($self, $args) = @_;

  if (defined $args->{exclusion_category})
  {
    my $category = $self->{ns_}->create(
      CreativeCategory => {
        name => $args->{exclusion_category} });
    
    $self->{ns_}->create(SiteCreativeCategoryExclusion => {
      site_id => $self->{publisher_}->{site_id},
      creative_category_id => $category });

    if ( defined $args->{link_exclusion} and
           $args->{link_exclusion} )
    {
      $self->{ns_}->create(
        CreativeCategory_Creative => {
          creative_category_id => $category,
          creative_id => $self->{campaign_}->{creative_id} });
    }

    $self->{ns_}->output(
        "EXCLUSIONCATEGORY",  $category);

  }
}

sub new
{
  my $self = shift;
  my ($ns, $prefix, $args) = @_;
  
  unless (ref $self) {
    $self = bless {}, $self;  }
  $self->{ns_} = $ns->sub_namespace($prefix);
  $self->{prefix_} = $prefix;
  $self->{categoriy_names_} = ();
  $self->{categories_} = ();
  $self->{size_} = $self->{ns_}->create(
    CreativeSize => {
      name => 'Size',
      max_text_creatives => 1 });

  $self->create_publisher($args);
  $self->create_template($args);
  $self->create_categories($args);
  $self->create_campaign($args);
  $self->link_categories($args);
  $self->create_site_exclusion($args);
}

1;

package CreativeCategoryGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_creative_category
{
  my ($self, $ns) = @_;
  CreativeCategoryGranularUpdateTest::TestCase->new(
    $ns, 'CREATE', 
    { headline => 'Tags',
      categories => 
        [ { name => "Visual"},
          { name => "Content"},
          { name => "Tags"} ]});  
}

sub unlink_creative_category
{
  my ($self, $ns) = @_;
  CreativeCategoryGranularUpdateTest::TestCase->new(
    $ns, 'UNLINK', 
    { headline => 'Tags',
      categories => 
        [ { name => "Tags", cct_id => 2} ]});
}

sub site_exclision_add
{
  my ($self, $ns) = @_;
  CreativeCategoryGranularUpdateTest::TestCase->new(
    $ns, 'ADDEXCLUSION', 
    { categories =>
        [ { name => "Visual", cct_id => 0},
          { name => "Content", cct_id => 1} ],
      exclusion_category => 'Exclusion'});
}

sub site_exclision_del
{
  my ($self, $ns) = @_;
  CreativeCategoryGranularUpdateTest::TestCase->new(
    $ns, 'DELEXCLUSION', 
    { categories => 
        [ { name => "Visual", cct_id => 0},
          { name => "Content", cct_id => 1} ],
      link_exclusion => 1,
      exclusion_category => 'Exclusion'});
}

sub init
{
  my ($self, $ns) = @_;
  $self->create_creative_category($ns);
  $self->unlink_creative_category($ns);
  $self->site_exclision_add($ns);
  $self->site_exclision_del($ns);
}

1;

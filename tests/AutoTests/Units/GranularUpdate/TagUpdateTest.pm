# $Id$
# Creates DB entites required for TagUpdateTest.
package TagUpdateTest::TestCase;

use strict;
use warnings;

sub create_tags
{
  my ($self, $args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Tag '$arg->{name}' id redefined!"
      if exists $self->{tags}->{$arg->{name}};

    my %tag_args = %$arg;
    delete $tag_args{approved_categories};
    delete $tag_args{rejected_categories};
    my $tag = $self->{ns}->create(PricedTag => \%tag_args);
    $self->{tags}->{$arg->{name}} = $tag;

    $self->{ns}->output("$arg->{name}/ID", $tag);

    if (defined $arg->{approved_categories})
    {
      foreach my $category (@{$arg->{approved_categories}})
      {
        $self->{ns}->create(TagsCreativeCategoryExclusion => {
          tag_id => $tag,
          approval => 'A',
          creative_category_id => $category })
      }
    }

    if (defined $arg->{rejected_categories})
    {
      foreach my $category (@{$arg->{rejected_categories}})
      {
        $self->{ns}->create(TagsCreativeCategoryExclusion => {
          tag_id => $tag,
          approval => 'R',
          creative_category_id => $category })
      }
    }
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless {}, $self };

  $self->{ns} = $ns->sub_namespace($case_name);
  $self->{case_name} = $case_name;

  return $self;
}



1;

package TagUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub tag_create_case
{
  my ($self, $parent_ns) = @_;

  my $ns = $parent_ns->sub_namespace("InsertTag");
  $ns->output("Tag1/NAME", make_autotest_name($ns, "Tag1"));

}

sub tag_update_creative_category_exclusion_case
{
  my ($self, $ns) = @_;

  my $test_case = new TagUpdateTest::TestCase($ns, "UpdateCategory");

  $test_case->create_tags([
    { name => "Tag1",
      site_id => $self->{site} },

    { name => "Tag2",
      site_id => $self->{site},
      approved_categories => [ $self->{visual_category} ] },

    { name => "Tag3",
      site_id => $self->{site},
      rejected_categories => [ $self->{visual_category} ] }
  ]);
}

sub tag_update_size_case
{
  my ($self, $ns) = @_;

  my $test_case = new TagUpdateTest::TestCase($ns, "UpdateSize");

  $test_case->create_tags([
    { name => "Tag1",
      site_id => $self->{site} }
  ]);
}

sub tag_remove
{
  my ($self, $ns) = @_;

  my $test_case = new TagUpdateTest::TestCase($ns, "DeleteTag");

  $test_case->create_tags([
    { name => "Tag1",
      site_id => $self->{site} }
  ]);
}

sub init
{
  my ($self, $ns) = @_;

  my $publisher_type = $ns->create(AccountType => {
    name => "Exclusion",
    account_role_id => DB::Defaults::instance()->publisher_role,
    adv_exclusions => 'T',
    flags => ( DB::AccountType::FREQ_CAPS )
    });

  my $publisher = $ns->create(PubAccount => {
    name => 'Pub',
    account_type_id => $publisher_type });

  $self->{site} = $ns->create(Site => {
    name => 'Site',
    account_id => $publisher });

  $self->{content_category} = $ns->create(CreativeCategory => {
    name => "C",
    cct_id => 1 });

  $self->{visual_category} = $ns->create(CreativeCategory => {
    name => "V" });

  # init control creative
  my $keyword = make_autotest_name($ns, "kwd");

#  my $advertiser = $ns->create(Account => {
#    name => 'Adv',
#    role_id => DB::Defaults::instance()->advertiser_role });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    campaign_flags => 0,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    creative_size_id => DB::Defaults::instance()->size(),
    site_links => [ { site_id => $self->{site} } ],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $ns->create(Account => {
                                name => 'Adv',
                                role_id => DB::Defaults::instance()->advertiser_role }),
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])
    });

  $ns->create(CreativeCategory_Creative => {
    creative_category_id => $self->{visual_category},
    creative_id => $campaign->{creative_id} }); 

  $ns->create(CreativeCategory_Creative => {
    creative_category_id => $self->{content_category},
    creative_id => $campaign->{creative_id} });

  $ns->output('NamePrefix', make_autotest_name($ns, ''));
  $ns->output('VisualCategory',$self->{visual_category});
  $ns->output('ContentCategory',$self->{content_category});
  $ns->output('SiteId', $self->{site});
  $ns->output('SizeType', DB::Defaults::instance()->other_size_type);
  $ns->output('Size1', DB::Defaults::instance()->size());
  $ns->output('Size1Name', DB::Defaults::instance()->size()->{name});

  my $size =  $ns->create(CreativeSize => {
      name => 'Size',
      size_type_id => DB::SizeType->blank(name => 'Banner') });

  $ns->output('Size2', $size);
  $ns->output('Size2Name', $size->{name});
  $ns->output('Size2Type', $size->{size_type_id});
  # ad frontend request traits
  $ns->output("Keyword", $keyword, "keyword for matching channel");
  $ns->output('CCID', $campaign->{cc_id}, "ad creative");

  # Run test cases
  $self->tag_create_case($ns);
  $self->tag_update_creative_category_exclusion_case($ns);
  $self->tag_update_size_case($ns);
  $self->tag_remove($ns);
}

1;

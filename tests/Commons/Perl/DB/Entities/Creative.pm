
# RTBConnector 
package DB::RTBConnector;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::DictionaryMixin DB::Entity::PQ);

use constant STRUCT => 
{
  rtb_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::string(unique => 1)
};

1;

package DB::RTBCategory;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  rtb_category_id => DB::Entity::Type::sequence(),
  creative_category_id => DB::Entity::Type::link('DB::CreativeCategory', unique => 1),
  rtb_id =>
    DB::Entity::Type::link(
      'DB::RTBConnector',
      default => sub { DB::Defaults::instance()->openx_connector },
      unique => 1),
  rtb_category_key => DB::Entity::Type::string()
};

1;

package DB::CreativeCategory;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_category_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::string(unique => 1),
  cct_id =>
    DB::Entity::Type::link(
      'DB::CreativeCategoryType',
      default => sub { DB::Defaults::instance()->creative_category_type }),
  qa_status => 'A',
  # Private
  iab_key => DB::Entity::Type::string(private => 1),
  tanx_key => DB::Entity::Type::string(private => 1),
  openx_key => DB::Entity::Type::string(private => 1),
  allyes_key => DB::Entity::Type::string(private => 1),
  baidu_key => DB::Entity::Type::string(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;
  $args->{name} = $ns->namespace . '-' . $args->{name}
    unless exists $args->{cct_id} and $args->{cct_id} == 2;
}

sub postcreate_
{
  my ($self, $ns) = @_;
  my %rtbs = (
    'tanx_key' => DB::Defaults::instance()->tanx_connector,
    'openx_key' => DB::Defaults::instance()->openx_connector,
    'iab_key' => DB::Defaults::instance()->iab_connector,
    'allyes_key' => DB::Defaults::instance()->allyes_connector,
    'baidu_key' => DB::Defaults::instance()->baidu_connector);

  while(my ($key, $connector) = each(%rtbs))
  {
    if (defined($self->{$key}))
    {
      $self->{$key} =
        $ns->create(
          RTBCategory => {
            creative_category_id => $self->{creative_category_id},
            rtb_category_key => $self->{$key},
            rtb_id => $connector });
    }
  }
}

1;

package DB::SizeType;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant MULTIPLE_SIZES => 0x01;       # allow multiple sizes for the tag with this type
use constant SIZE_LEVEL_SELECTION => 0x02; # allow selection at size level (for advertisers)

use constant STRUCT => 
{
    size_type_id => DB::Entity::Type::sequence(),
    name => DB::Entity::Type::string(unique => 1),
    version => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
    flags => DB::Entity::Type::int(default => MULTIPLE_SIZES | SIZE_LEVEL_SELECTION)
};

1;


package DB::CreativeSize;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  size_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(),
  protocol_name => DB::Entity::Type::string(unique => 1),
  width => DB::Entity::Type::int(default => 468, nullable => 1),
  height => DB::Entity::Type::int(default => 60, nullable => 1),
  size_type_id => DB::Entity::Type::link('DB::SizeType',
    default => sub { DB::Defaults::instance()->other_size_type }),
  status => DB::Entity::Type::status(),
  # Private
  # Options
  max_text_creatives => DB::Entity::Type::int(private => 1, default => 1)
};

sub precreate_
{
  my ($self, $ns) = @_;

  $self->{protocol_name} = $self->{name}
    if not exists $self->{protocol_name};
}

sub postcreate_
{
  my ($self, $ns) = @_;

  $self->{option_group_id} =
    $ns->create(
      OptionGroup =>
      { name => "Size-" . $self->{size_id},
        type => 'Publisher',
        size_id => $self->{size_id} });

  if (defined($self->{max_text_creatives}))
  {
    $self->{option_id} = 
      $ns->create(
         Options => {
           name => 'Maximum Number of Text Ads',
           token => 'MAX_ADS_PER_TAG',
           type => 'Integer',
           option_group_id => $self->{option_group_id},
           default_value => 
             $self->{max_text_creatives},
           min_value => 1,
           required => 'Y',
           sort_order => 0,
           template_id => undef,
           size_id => $self->{size_id},
           max_value => 
             $self->{max_text_creatives} });
  }
}

1;

package DB::TemplateFile;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant PIXEL_TRACKING => 1;

use constant STRUCT => 
{
  template_file_id => DB::Entity::Type::sequence(),
  template_id => DB::Entity::Type::link('DB::Template', unique => 1),
  size_id => 
    DB::Entity::Type::link(
      'DB::CreativeSize',
      unique => 1,
      default => sub { DB::Defaults::instance()->size }),
  app_format_id =>
    DB::Entity::Type::link(
      'DB::AppFormat',
      unique => 1,
      default => sub { DB::Defaults::instance()->app_format_no_track }),
  template_file => DB::Entity::Type::string(default => 'UnitTests/banner_img_clk.html'),
  flags => DB::Entity::Type::int(default => 0),
  template_type => DB::Entity::Type::enum(['T', 'X'])
};

1;

package DB::Template;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant OPTIONS => 
  qw(WIDTH HEIGHT ALTTEXT WNDTITLE ADIMAGE CRHTML
     CRSCRIPT CRADVTRACKPIXEL CRHPOS CRVPOS CRCSS
     CRDURATION CRCLICK IMAGETITLE DESCRIPTION1
     HEADLINE DESCRIPTION2 ADHTML PUBL_TAG_TRACK_PIXEL);

use constant STRUCT => 
{
  template_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  template_type => DB::Entity::Type::enum(['CREATIVE', 'DISCOVER']),
  status => DB::Entity::Type::status(),
  expandable => DB::Entity::Type::enum(['N', 'Y']),

  # Private
  # TemplateFile
  size_id => DB::Entity::Type::link('DB::CreativeSize',  private => 1),
  app_format_id => DB::Entity::Type::link('DB::AppFormat', private => 1),
  template_file => DB::Entity::Type::string(private => 1),
  flags => DB::Entity::Type::int(default => 0, private => 1),
  template_file_type => DB::Entity::Type::enum(['T', 'X'], private => 1),

  # Private

  # CreativeCategory
  creative_category_id => DB::Entity::Type::link_array('DB::CreativeCategory', private => 1),

  # Options
  width_value => DB::Entity::Type::int(private => 1, default => 640),
  height_value => DB::Entity::Type::int(private => 1, default => 480),
  alttext_value => DB::Entity::Type::string(private => 1, default => 'alttext'),
  wndtitle_value => DB::Entity::Type::string(private => 1, default => 'UNITTEST'),
  adimage_value => DB::Entity::Type::string_obj(private => 1, default => 'http://www.unittest.com/1.bmp'),
  crhtml_value => DB::Entity::Type::string_obj(private => 1, default => undef),
  crscript_value => DB::Entity::Type::string(private => 1, default => undef),
  cradvtrackpixel_value => DB::Entity::Type::string(private => 1, default => undef),
  crhpos_value => DB::Entity::Type::int(private => 1, default => 1),
  crvpos_value => DB::Entity::Type::int(private => 1, default => 1),
  crcss_value => DB::Entity::Type::string(private => 1, default => undef),
  crduration_value => DB::Entity::Type::int(private => 1, default => 0),
  crclick_value => DB::Entity::Type::string(private => 1, default => undef),
  imagetitle_value => DB::Entity::Type::string(private => 1),
  description1_value => DB::Entity::Type::string(private => 1),
  headline_value => DB::Entity::Type::string(private => 1),
  description2_value => DB::Entity::Type::string(private => 1),
  adhtml_value => DB::Entity::Type::string(private => 1),
  publ_tag_track_pixel_value => DB::Entity::Type::string(private => 1),
};

sub postcreate_
{
  my ($self, $ns) = @_;

  my @file_args =
    grep { exists $self->{$_} }
      qw(size_id app_format_id template_file flags template_file_type);

  my %args;
  @args{@file_args} = @$self{@file_args};
  $args{template_type} = $args{template_file_type}
    if exists $args{template_file_type};
  delete($args{template_file_type});
  $args{template_id} = $self->{template_id};
  $self->{template_file_id} = 
    $ns->create(DB::TemplateFile->blank(%args));

  $self->{option_group_id} =
    $ns->create(DB::OptionGroup->blank(
      name => "Template-" . $self->{template_id},
      type => 'Advertiser',
      template_id => $self->{template_id} ));

  foreach my $opt (OPTIONS)
  {
    my $value_name = lc($opt) . "_value";
    if (exists $self->{$value_name}) 
    {
      my $value = $self->{$value_name};
      my $option = 
        ref($value) eq 'DB::Entity::Base::Blank'? $value:
          DB::Options->blank(value => $value);
      $option->{token} = $opt;
      $option->{option_group_id} = $self->{option_group_id};
      $option->{template_id} = $self->{template_id};
        $ns->create($option);
    }
  }

  if (exists $self->{creative_category_id})
  {
    my @categories =
      ref($self->{creative_category_id}) eq 'ARRAY'?
        @{$self->{creative_category_id}}:
          ($self->{creative_category_id});

    foreach my $category (@categories)
    {
      $ns->create(CreativeCategory_Template => {
        creative_category_id => $category,
        template_id => $self->{template_id} });
    }
  }
}

1;

package DB::CreativeOptionValue;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  option_id => DB::Entity::Type::link('DB::Options', unique => 1),
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1),
  value => DB::Entity::Type::string(nullable => 1),

  # Private
  # Options
  token => DB::Entity::Type::string(private => 1),
  template_id => DB::Entity::Type::link('DB::Template', private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  if (not exists $args->{option_id} and defined $args->{token})
  {
    my $option_group_id = 
      ref($args->{template_id})? 
        $args->{template_id}->{option_group_id}:
        $ns->create(DB::OptionGroup->blank( 
          name => "Template-" . $args->{template_id},
          type =>  'Advertiser',
          template_id => $args->{template_id} ));

    $args->{option_id} = 
        $ns->create(
          DB::Options->blank(
           option_group_id => $option_group_id,
           template_id => $args->{template_id},
           token =>  $args->{token} ));
  }
}

sub postcreate_
{
  my ($self, $ns) = @_;

  {
    no warnings 'uninitialized';
    $self->__update($ns, 
      { value => $self->{args__}->{value} })
      if ($self->{args__}->{value} ne $self->{value});
  }
}

1;

package DB::CreativeCategory_Creative;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_category_id => DB::Entity::Type::link('DB::CreativeCategory', unique => 1),
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1)
};

1;

package DB::CreativeCategory_Template;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_category_id => DB::Entity::Type::link('DB::CreativeCategory'),
  template_id => DB::Entity::Type::link('DB::Template', unique => 1)
};

1;

package DB::Creative;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_id => DB::Entity::Type::link('DB::Account'),
  size_id =>
   DB::Entity::Type::link(
     'DB::CreativeSize',
     default => sub { DB::Defaults::instance()->size }),
  template_id => 
    DB::Entity::Type::link(
     'DB::Template',
     default => sub { DB::Defaults::instance()->display_template }),
  flags => DB::Entity::Type::int(default => 0),
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(), 
  display_status_id => DB::Entity::Type::display_status('Creative'),
  version => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  expandable => 'N',
  # Private
  # Options
  width_value => DB::Entity::Type::int(private => 1),
  height_value => DB::Entity::Type::int(private => 1),
  alttext_value => DB::Entity::Type::string(private => 1),
  wndtitle_value => DB::Entity::Type::string(private => 1),
  adimage_value => DB::Entity::Type::string_obj(private => 1),
  crhtml_value => DB::Entity::Type::string_obj(private => 1),
  crscript_value => DB::Entity::Type::string(private => 1),
  cradvtrackpixel_value => DB::Entity::Type::string(private => 1),
  crhpos_value => DB::Entity::Type::int(private => 1),
  crvpos_value => DB::Entity::Type::int(private => 1),
  crcss_value => DB::Entity::Type::string(private => 1),
  crduration_value => DB::Entity::Type::int(private => 1),
  crclick_value => DB::Entity::Type::string(private => 1),
  imagetitle_value => DB::Entity::Type::string(private => 1),
  description1_value => DB::Entity::Type::string(private => 1),
  headline_value => DB::Entity::Type::string(private => 1),
  description2_value => DB::Entity::Type::string(private => 1),
  adhtml_value => DB::Entity::Type::string(private => 1),
  publ_tag_track_pixel_value => DB::Entity::Type::string(private => 1),

  # CreativeCategory
  creative_category_id => DB::Entity::Type::link_array('DB::CreativeCategory', private => 1),

  # SizeType
  size_type_id => DB::Entity::Type::link('DB::SizeType', private => 1),
  # Tag sizes
  tag_sizes => DB::Entity::Type::link_array('DB::CreativeSize', private => 1),
  tag_size_types => DB::Entity::Type::link_array('DB::SizeType', private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;
  $args->{size_id} = $args->{tag_sizes}->[0] 
    if not defined $args->{size_id} and $args->{tag_sizes};
}

sub postcreate_
{
  my ($self, $ns) = @_;

  my @sizes = $self->{tag_sizes}? 
    @{$self->{tag_sizes}}: ($self->{size_id});

  for my $size (@sizes)
  {
    $ns->create(
      DB::Creative_TagSize->blank(
        creative_id => $self->{creative_id},
        size_id => $size)) if $size;
  }

  if ($self->{tag_size_types})
  {
    for my $size_type (@{ $self->{tag_size_types }})
    {
      $ns->create(
        DB::Creative_TagSizeType->blank(
          creative_id => $self->{creative_id},
          size_type_id => $size_type));
    }
  }

  $ns->create(
    DB::Creative_TagSizeType->blank(
      creative_id => $self->{creative_id},
      size_type_id => $self->{size_type_id} ))
    if defined $self->{size_type_id};

  # NOTE when adding new tokens: for token type 'F' value is a file
  # path relative to data_root.  The file must exists.  For token
  # type 'U' value is either a URL starting with 'http://', or the
  # same as for 'F'.

  foreach my $opt (DB::Template::OPTIONS)
  {
    my $value_name = lc($opt) . "_value";
    if (exists $self->{$value_name}) 
    {
      my $value = $self->{$value_name};
      my $option = 
        ref($value) eq 'DB::Entity::Base::Blank'? $value:
          DB::CreativeOptionValue->blank(value => $value);
      $option->{creative_id} = $self->{creative_id};
      $option->{token} = $opt;
      $option->{template_id} = $self->{template_id};
      $ns->create($option);
    }
  }
  if (exists $self->{creative_category_id})
  {
    my @categories = 
      ref($self->{creative_category_id}) eq 'ARRAY'?
        @{$self->{creative_category_id}}: 
          ($self->{creative_category_id});

    foreach my $category (@categories)
    {
      $ns->create(CreativeCategory_Creative => {
        creative_category_id => $category,
        creative_id => $self->{creative_id} });
    }
  }
}

use constant ENABLE_ALL_SIZES => 0x01;

1;

package DB::CreativeCategoryType;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::DictionaryMixin DB::Entity::PQ);

use constant STRUCT => 
{
  name => DB::Entity::Type::string(unique => 1),
  cct_id => DB::Entity::Type::int()
};

1;

package DB::Creative_TagSizeType;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1),
  size_type_id => DB::Entity::Type::link('DB::SizeType', unique => 1)
};

1;

package DB::Creative_TagSize;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1),
  size_id => DB::Entity::Type::link('DB::CreativeSize', unique => 1)
};

1;

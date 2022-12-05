
# SiteCreativeCategoryExclusion
package DB::SiteCreativeCategoryExclusion;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  site_id => DB::Entity::Type::link('DB::Site', unique => 1),
  creative_category_id => DB::Entity::Type::link('DB::CreativeCategory', unique => 1),
  approval => DB::Entity::Type::enum(['R', 'P'])
};

1;

# SiteCreativepproval
package DB::SiteCreativeApproval;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  site_id => DB::Entity::Type::link('DB::Site', unique => 1),
  creative_id => DB::Entity::Type::link('DB::Creative', unique => 1),
  approval => DB::Entity::Type::enum(['A', 'R', 'P']),
  approval_date => DB::Entity::Type::pq_date('now()')
};

1;

# Site
package DB::Site;

use warnings;
use strict;
use DB::Entity::PQ;

use constant INVENTORY_ESTIMATION =>  0x001;
use constant WALLED_GARDEN =>  0x002;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  site_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  site_url => DB::Entity::Type::string(default => 'www.unittest.com'),
  notes =>  DB::Entity::Type::string(default => ''),
  account_id => 
    DB::Entity::Type::link( 
      'DB::Account',
      default => sub { DB::Defaults::instance()->publisher_account }), 
  freq_cap_id => DB::Entity::Type::link('DB::FreqCap', nullable => 1), 
  no_ads_timeout => DB::Entity::Type::int(default => 0), 
  flags => DB::Entity::Type::int(default => 0), 
  status => DB::Entity::Type::status(), 
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('Site')
};

1;

# Publisher composite
package DB::Publisher;

use warnings;
use strict;

our @ISA = qw(DB::Entity::Composite);

sub _tables {
  qw(PubAccount Site PricedTag)
}

sub links
{
  {
    ccg_links => [
      DB::CampaignCreativeGroup->blank(),
      DB::CCGSite->blank() ],
    creative_links => [
      DB::Creative->blank(),
      DB::SiteCreativeApproval->blank() ],
    exclusions => [
      DB::CreativeCategory->blank(),
      DB::SiteCreativeCategoryExclusion->blank() ],
    tag_exclusions => [
      DB::CreativeCategory->blank(),
      DB::TagsCreativeCategoryExclusion->blank() ],
  }
}

sub tables
{
 (
   DB::PubAccount->blank(),
   DB::Site->blank(),
   DB::PricedTag->blank()
  )
}

1;

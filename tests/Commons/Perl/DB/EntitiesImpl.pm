# For flag values see https://confluence.ocslab.com/display/OIX/Object+Statuses

# include all DB entities definition for fetch its in db_clean.pl
use DB::Entities::Account;
use DB::Entities::TargetingChannel;
use DB::Entities::BehavioralChannel;
use DB::Entities::Campaign;
use DB::Entities::CategoryChannel;
use DB::Entities::CCG;
use DB::Entities::ChannelInventory;
use DB::Entities::Country;
use DB::Entities::Colocation;
use DB::Entities::Creative;
use DB::Entities::Currency;
use DB::Entities::DeviceChannel;
use DB::Entities::AudienceChannel;
use DB::Entities::ExpressionChannel;
use DB::Entities::FreqCap;
use DB::Entities::GEOChannel;
use DB::Entities::Global;
use DB::Entities::Options;
use DB::Entities::Site;
use DB::Entities::SpecialChannel;
use DB::Entities::Tags;
use DB::Entities::Users;
use DB::Entities::WDTags;


# workaround for
# RPM perl dependencies checker (path and package names different)
=for comment
package DB::EntitiesImpl;
package DB::Entities::Account;
package DB::Entities::TargetingChannel;
package DB::Entities::BehavioralChannel;
package DB::Entities::Campaign;
package DB::Entities::CategoryChannel;
package DB::Entities::CCG;
package DB::Entities::ChannelInventory;
package DB::Entities::Country;
package DB::Entities::Colocation;
package DB::Entities::Creative;
package DB::Entities::Currency;
package DB::Entities::DeviceChannel;
package DB::Entities::AudienceChannel;
package DB::Entities::ExpressionChannel;
package DB::Entities::FreqCap;
package DB::Entities::GEOChannel;
package DB::Entities::Global;
package DB::Entities::Options;
package DB::Entities::Site;
package DB::Entities::SpecialChannel;
package DB::Entities::Tags;
package DB::Entities::Users;
package DB::Entities::WDTags;
=cut

package DB::ChannelUtils;

use warnings;
use strict;

sub namespace
{
  my ($channel_type) = @_;
  die "Cann't set Channel.namespace, because channel_type is undefined"
     if (!defined $channel_type);
  if ($channel_type eq 'B' || $channel_type eq 'E') {
    return 'A'; }
  if ($channel_type eq 'D' || $channel_type eq 'L') {
    return 'D'; }
  return $channel_type;
}

1;


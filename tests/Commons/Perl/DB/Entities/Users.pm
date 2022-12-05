
package DB::UserRole;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  user_role_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_role_id => DB::Entity::Type::link('DB::AccountRole'),
  flags => 0
};

1;

package DB::Users;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  user_id => DB::Entity::Type::sequence(),
  ldap_dn => DB::Entity::Type::name(unique => 1),
  first_name => DB::Entity::Type::string(), 
  last_name => DB::Entity::Type::string(), 
  status => DB::Entity::Type::status(),
  account_id => DB::Entity::Type::link('DB::Account'),
  email => DB::Entity::Type::string(), 
  phone => DB::Entity::Type::string(),  
  user_role_id => DB::Entity::Type::link('DB::UserRole'),
  language => DB::Entity::Type::string(), 
  auth_type => DB::Entity::Type::string(),
  flags => 0
};

1;

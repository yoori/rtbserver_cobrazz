

# Adserver global configuration
package DB::AdsConfig;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  param_name => DB::Entity::Type::string(unique => 1),
  param_value => DB::Entity::Type::string()
};

1;

# Application formats dictionary
package DB::AppFormat;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  app_format_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::string(unique => 1),
  mime_type => DB::Entity::Type::string()
};

1;

# Timezone dictionary
package DB::TimeZone;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::DictionaryMixin DB::Entity::PQ);

use constant STRUCT => 
{
  tzname => DB::Entity::Type::string(unique => 1),
  timezone_id => DB::Entity::Type::int(),
};

1;

# Local channel category names (use DynamicResources table)
package DB::CategoryLocalName;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table {
  'DynamicResources'
}

use constant STRUCT => 
{
  key => DB::Entity::Type::string(unique => 1),
  lang => DB::Entity::Type::string(unique => 1),
  value => DB::Entity::Type::string(),
  
  # Private
  channel_category_id => DB::Entity::Type::int(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  if (defined $args->{channel_category_id} && !exists $args->{key})
  {
    $args->{key} = 
      "CategoryChannel." . $args->{channel_category_id};    
  }
}

1;

# Webwise discover request mapping
package DB::WDRequestMapping;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  wd_req_mapping_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::string(unique => 1),
  description =>  DB::Entity::Type::name(),
  request => DB::Entity::Type::string()
};

sub preinit_
{
  my ($self, $ns, $args) = @_;
  
  $args->{description} =  $args->{name}
    if !exists $args->{description};
}

1;


# Fraud condition
package DB::FraudCondition;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{ 
  fraud_condition_id => DB::Entity::Type::sequence(),
  type => DB::Entity::Type::enum(['IMP', 'CLK']),
  period => DB::Entity::Type::int(),
  limit => DB::Entity::Type::int()
};

1;

# Search engine
package DB::SearchEngine;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  search_engine_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  host => DB::Entity::Type::string(),
  regexp => DB::Entity::Type::string(),
  encoding => DB::Entity::Type::string(),
  decoding_depth => DB::Entity::Type::int(default => 1)
};

1;


# Dictionaries, used by web statistics 

# Web application
package DB::WebOperation;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::UpdateMixin DB::Entity::PQ);

use constant STRUCT => 
{
  web_operation_id => DB::Entity::Type::raw_sequence(),
  app => DB::Entity::Type::string(unique => 1),
  source => DB::Entity::Type::string(unique => 1, nullable => 1),
  operation => DB::Entity::Type::string(unique => 1),
  status => DB::Entity::Type::status(),
  flags => DB::Entity::Type::int(default => 0)
};


1;


package ForbiddenUserAgentsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my @user_agents = ('AbachoBOT', 'Accoona-AI-Agent/1.1.1',
    'Aberja Checkomat', 'RixBot (http://babelserver.org/rix)',
    'Robozilla/1.0', 'yarienavoir.net/0.2', 'ZoomSpider - wrensoft.com'
    );

  my $num = 0;
  foreach my $user_agent (@user_agents)
  {
    $ns->output("Agent".$num, $user_agent, "UserAgent");
    $num = $num + 1;
  }
}

1;

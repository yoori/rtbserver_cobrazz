
package Utils;

use warnings;
use strict;

use constant DEFAULT_HTTP_PORT    => 80;

sub make_server_url {
  my $server = shift;
  if (!($server =~ /([\w\W]*?):([\w\W]*)/o))
  {
    $server .= ":" . DEFAULT_HTTP_PORT;
  }
  return "http://$server";
}

sub store_list {
  my ($path, $list) = @_;
  open(my $FILE, ">$path") || die "Cann't open file $path for write.\n";  
  for my $elem (@$list)
  {
    print $FILE "$elem\n";
  }
  close($FILE);
}

1;

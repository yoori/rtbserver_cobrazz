
package GeoDataMergingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use POSIX qw(ceil);

sub init {
  my ($self, $ns) = @_;

  my $start = -50.0;

  for (my $i = 1; $i <= 100; $i++)
  {
    my $cc = $start + $i;

    my $channel =
      $ns->create(DB::AddressChannel->blank(
        name => 'GeoDataMergingTest-Address-' . $i,
        country_code      => 'RU',
        parent_channel_id => DB::Defaults::instance()->geo_ru_country->{channel_id},
        latitude          => "$cc.5556",
        longitude         => "$cc.4444",
        address           => "Russia, Moscow, Street $i",
        radius            => $i + 100,
        radius_units      => 'm'));
    $ns->output("Location/" . $i, "$cc.555555/$cc.444444/" . ($i + 100));
    $ns->output("Address/" .$i , $channel);
  }

  my @locations = (['L1', 55.666666, 37.888888, 800],
                   ['L2', 55.888888, 37.666666, 600],
                   ['L3', 65.555555, 34.444444, 1000],
                   ['L4', 75.555555, 24.444444, 500]);

  foreach my $l (@locations)
  {
    my ($name, $latitude, $longitude, $radius) = @$l;
    my $channel =
      $ns->create(DB::AddressChannel->blank(
        name => 'GeoDataMergingTest-' . $name,
        country_code      => 'RU',
        parent_channel_id => DB::Defaults::instance()->geo_ru_country->{channel_id},
        latitude          => sprintf("%.4f", $latitude),
        longitude         => sprintf("%.4f", $longitude),
        address           => 'Russia, Moscow, ' . $name,
        radius            => $radius,
        radius_units      => 'm'));
    $ns->output("Location/" . $name, $latitude . '/' . $longitude . '/' . $radius);
    $ns->output("Address/" . $name , $channel);
  }
}

1;

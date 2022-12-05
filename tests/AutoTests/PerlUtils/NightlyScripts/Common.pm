
use warnings;
use strict;

use File::Basename;
use Sys::Hostname;

package TestCommon;

# suffix for error path
use constant ERROR_PATH_SUFFIX => "Errors";
# suffix for log path
use constant OUT_PATH_SUFFIX   => "Logs";

# tests status
use constant SUCCESS_STATUS => "success";
use constant FAILED_STATUS  => "failed";

# file extensions
use constant ERROR_EXT  => ".err";
use constant OUT_EXT    => ".out";
use constant HTML_EXT   => ".html";
use constant LOG_STORE_EXT => ".log";

# getting hostname
sub get_host_name {
  return Sys::Hostname::hostname();
}

# get formatted local time
sub get_local_time {
  my ($second, $minute, $hour, $dayOfMonth,
      $month, $yearOfset, $dayOfWeek, $dayOfYear, 
      $dayLightSavings) = localtime();
  my $year = $yearOfset + 1900;
  return sprintf("%02d-%02d-%04d-%02d-%02d", 
                 $dayOfMonth, $month, $year, $hour, $minute);
}

# getting name of last subdirectory in 
sub get_last_dir_name {
  my $path = shift;
  my $dir  = File::Basename::dirname($path);
  my @dir_parts = split("/", $dir);
  my $len = scalar(@dir_parts);
  return $dir_parts[$len-1];
}

sub get_path_without_last_name {
  my $path = shift;
  return File::Basename::dirname($path);
}

# normalize path names (replace spaces)
sub normalize {
  my $path = shift;
  $path =~ s/ /_/g;
  return $path;
}

1;

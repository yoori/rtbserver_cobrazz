package cleanup;
use strict;

our %SERVICE_CONFIG = (
  pid_file => "./LogsCleanUp.pid",
  check_period => 0, # in seconds
);

# 'time', in minutes, must be >= 200 to hold 20 filetimes sequence, dT=10 minutes.
# 'size', in bytes, dS=1 byte.
our %CLEANUP_CONFIG = (
 "$ENV{'TEST_TMP_DIR'}/clean_test1/clean_test.log" =>
   { 'time' => 240,
     'size' => 145, # Constrain: HALF files by size
     'standard' => [('half', 'half', 'one', 'zero')],
   },
 "$ENV{'TEST_TMP_DIR'}/clean_test2/clean_test.log" =>
   { 'time' => 580,
     'size' => 190,      # Constrain: All Files by size exactly
     'standard' => [('all', 'half', 'one', 'zero')],
   },
 "$ENV{'TEST_TMP_DIR'}/clean_test3/clean_test.log" =>
   { 'time' => 3600,
     'size' => 19,  # Constrain: ONE file by size
     'standard' => [('one', 'one', 'one', 'zero')],
   },
 "$ENV{'TEST_TMP_DIR'}/clean_test4/clean_test.log" =>
   { 'time' => 0, # in minutes
     'size' => 0, # in bytes
     'standard' => [('zero', 'zero', 'zero', 'zero')],
   },
);

1;

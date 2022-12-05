
use CGI qw/:standard/;
use File::Basename;
use File::Spec;
use Common;


# TestHTML Common utilities & constants
package HTMLCommon;

use constant MAX_HISTORY_SIZE => 7;

use constant UNNAMED_CATEGORY => "";
use constant VALGRIND_SUFFIX  => "vg";
use constant VALGRIND_NAME    => "Under valgrind";

# History statuses
use constant HS_NEW_FAIL    => "NEW FAIL";
use constant HS_NEW_PASS    => "NEW PASS";
use constant HS_FAILED      => "FAILED";
use constant HS_PASSED      => "PASSED";
use constant HS_UNDEFINED   => "";

# And its colors
use constant COLOR_NEW_FAIL    => "RED";
use constant COLOR_NEW_PASS    => "LIGHTGREEN";
use constant COLOR_FAILED      => "ORANGE";
use constant COLOR_PASSED      => "GREEN";

our $doc_style = undef;

sub set_doc_style {
  $doc_style = shift;
}

sub get_doc_style {
  return $doc_style;
}


sub status_to_td {
  my $status = shift;
  if ($status eq HS_NEW_PASS)
  {
    return CGI::td({bgcolor=>COLOR_NEW_PASS, align=>center}, $status);
  }
  elsif ($status eq HS_NEW_FAIL)
  {
    return CGI::td({bgcolor=>COLOR_NEW_FAIL, align=>center}, $status);
  }
  elsif ($status eq HS_FAILED)
  {
    return CGI::td({bgcolor=>COLOR_FAILED, align=>center}, $status);
  }
  elsif ($status eq HS_PASSED)
  {
    return CGI::td({bgcolor=>COLOR_PASSED, align=>center}, $status);
  }
  elsif ($status eq TestCommon::FAILED_STATUS)
  {
    return CGI::td({bgcolor=>COLOR_NEW_FAIL, align=>center}, HS_FAILED);
  }
  elsif ($status eq TestCommon::SUCCESS_STATUS)
  {
    return CGI::td({bgcolor=>COLOR_PASSED, align=>center}, HS_PASSED);
  }
  return CGI::td("");
}

sub diffs_to_color_tag {
  my $diffs = shift;
  if ($diffs > 0)
  {
    return CGI::font({-color=>"GREEN"}, $diffs);
  }
  elsif ($diffs < 0)
  {
    return CGI::font({-color=>"RED"}, $diffs);
  }
  return $diffs;
}

sub test_key {
  my $filename = shift;
  my ($file, $dir, $ext) = File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT));
  my @file_parts = split(/\./, $file);
  if (scalar(@file_parts) eq 2)
  {
    return $file_parts[1] eq VALGRIND_SUFFIX?
        (VALGRIND_NAME, $file_parts[0]) : ($file_parts[0], $file_parts[1]);
  }
  elsif (scalar(@file_parts) eq 1)
  {
    return (UNNAMED_CATEGORY, $file);
  }
  return (undef, undef);
}

sub is_error {
  my $filename = shift;
  my ($file, $dir, $ext) = File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT,
                                                      TestCommon::LOG_STORE_EXT));
  if ($ext eq TestCommon::LOG_STORE_EXT)
  {
    ($file, $dir, $ext) =  File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT));
  }
  return $ext eq TestCommon::ERROR_EXT;
}

sub is_log {
  my $filename = shift;
  my ($file, $dir, $ext) = File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT,
                                                      TestCommon::LOG_STORE_EXT));
  if ($ext eq TestCommon::LOG_STORE_EXT)
  {
    ($file, $dir, $ext) =  File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT));
  }
  return $ext eq TestCommon::OUT_EXT;
}

sub is_contain_error 
{
  my $filename = shift;
  if (-e $filename)
  {
    my $filesize  = -s "$filename";
    return $filesize ne 0; 
  }
  return 0;
}

sub normalize_path {
  my ($filename, $link_suffix, $new_ext) = @_;
  my ($file, $dir, $ext) = File::Basename::fileparse($filename, 
                                                     (TestCommon::ERROR_EXT, 
                                                      TestCommon::OUT_EXT));
  $new_ext = $ext if !defined $new_ext;
  return TestCommon::normalize(File::Spec->join($link_suffix, $file . $new_ext));
}

# get class name for 'Test results' table row
sub tr_class {
  my $odd    = shift;
  my $status = shift;
  my $prefix = "";
  if ($status eq TestCommon::FAILED_STATUS)
  {
    $prefix = TestCommon::FAILED_STATUS . "-";
  }
  if ($odd)
  {
    return $prefix . "odd";
  }
  return $prefix . "even";
}

1;

# Base HTML Document class
package HTMLBaseDocument;

use warnings;
use strict;

sub new {
  my $self               = shift;
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{html_buffer}  = "";     # buffer for HTML body
  $self->{DOC_NAME}     = shift;  # HTML file name
  $self->{HTML_PATH}    = shift;
  $self->{WORKSPACE}    = shift;
  $self->{DST_PATH}     = shift;
  if (!defined $self->{DST_PATH})
  { 
    $self->{DST_PATH}     = "";
  }
  return $self;
} 

# HTML header
sub header {
  return CGI::start_html();
}

# HTML footer
sub footer {
  return CGI::end_html();
}

# Body
sub get_body {
  return "";
}

sub relative_path {
  my ($self, $path) = @_;
  if (!defined $self->{HTML_PATH} || !defined $self->{WORKSPACE})
  {
    return $path;
  }
  my $html_path  = File::Spec->catfile($self->{HTML_PATH}, $self->{WORKSPACE});
  return File::Spec->abs2rel($path, $html_path);
}

sub file_link {
  my ($self, $linkpath, $filepath, $link_name, $empty_link) = @_;
  $empty_link = 1 if !defined $empty_link;
  if (!defined $filepath)
  {
    return "";
  }
  if (-f $filepath)
  {
    my $filesize  = -s "$filepath";
    if ($filesize ne 0 || $empty_link) 
    {
      if ($linkpath eq $filepath)
      {
        return CGI::a({href=>$self->relative_path($linkpath),
                       type=>"text/html"}, $link_name);
      }
      return CGI::a({href=>$linkpath,
                     type=>"text/html"}, $link_name);
    }
  }
  return "";
}


# store HTML data in file
sub flush {
  my $self     = shift;
  my $path     = shift;  # HTML file path
  open(my $HTMLFILE, ">$path") || die "Error when openning file $path";
  print $HTMLFILE $self->header(); 
  print $HTMLFILE $self->get_body(); 
  print $HTMLFILE $self->footer();
  close($HTMLFILE);
}

1;

# Base HTML Tests Results page  
package HTMLTestsResults;

use warnings;
use strict;
use File::stat;
use Time::localtime;
use POSIX qw(strftime);
use File::Find;

our @ISA = qw(HTMLBaseDocument);

sub new {
  my $self                = shift;
  my $doc_name            = shift;
  my $html_path           = shift;
  my $workspace           = shift;
  my $dst_path            = shift;
  $self                   = $self->SUPER::new($doc_name, $html_path, $workspace, $dst_path);
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{LOG_PATH}        = shift;
  $self->{TIME}            = shift;
  $self->{HISTORY_PATH}    = shift;
  $self->{ADD_PATHS}       = shift;
  $self->{RESULT_PATH}     = "";
  $self->{tests}           = {};
  $self->{histories}       = {};
  $self->{add_logs}        = {};
  @{$self->{history_paths}} = ();
  # Tests statistics
  $self->{tests_count}     = 0; # total tests count
  $self->{errors_count}    = 0; # errors count
  $self->{l_tests_count}   = 0; # prior run, total tests count
  $self->{l_errors_count}  = 0; # prior run, errors count
  return $self;
}

sub wanted {
  my ($self, $fullpath, $file,  $search_path, $paths) = @_;
  my ($name, $dir, $ext) = File::Basename::fileparse($fullpath);
  if ($dir =~ /[\w\W]*$search_path[\w\W]*$/ && $file eq $name)
  {
    print "Find path: $dir\n";
    push @$paths, $dir;
  }
}

sub prepare
{
  my $self    = shift;
  my $subpath = shift;
  my $search_path = $self->{WORKSPACE};
  if ($subpath)
  {
    $search_path = File::Spec->join($subpath, $search_path);
  }
  $search_path = TestCommon::normalize($search_path);
  if (-d $self->{HISTORY_PATH})
  {
    my @paths = ();
    File::Find::find({wanted => sub { $self->wanted($File::Find::name, 
                                                    "tests.html", $search_path, \@paths);} }, 
                     $self->{HISTORY_PATH});
      @{$self->{history_paths}} = 
          sort {-M "$a" <=> -M  "$b"} @paths;
  }
  $self->{RESULT_PATH} = $search_path;
}

# adding test category
sub add_category {
  my ($self, $category)= @_;
  if ( ! exists  $self->{tests}{$category} )  
  {
    $self->{tests}{$category} = {};
  }
}

sub add_error {
  my ($self, $category, $name, $status, $filename) = @_;
  $self->{tests}{$category}{$name}{error_file} = $filename;
  $self->{tests}{$category}{$name}{status} = $status;
  $self->{tests}{$category}{$name}{error}  = 
  HTMLCommon::normalize_path($filename, 
                             TestCommon::ERROR_PATH_SUFFIX);
  if (! exists $self->{tests}{$category}{$name}{log})
  {
    $self->{tests}{$category}{$name}{log} =  
        HTMLCommon::normalize_path($filename, 
                                   TestCommon::OUT_PATH_SUFFIX,
                                   TestCommon::OUT_EXT);
  }
}

sub add_log {
  my ($self, $category, $name, $status, $filename) = @_;
  $self->{tests}{$category}{$name}{log}  = 
      HTMLCommon::normalize_path($filename, 
                                 TestCommon::OUT_PATH_SUFFIX);
  $self->{tests}{$category}{$name}{log_file}  = $filename;
  if (! exists $self->{tests}{$category}{$name}{status})
  {
    $self->{tests}{$category}{$name}{status}  = $status;
  }
  if (! exists $self->{tests}{$category}{$name}{error})
  {
    $self->{tests}{$category}{$name}{error} =  
        HTMLCommon::normalize_path($filename, 
                                   TestCommon::ERROR_PATH_SUFFIX,
                                   TestCommon::ERROR_EXT);
  }
}

# adding test  
sub add_test {
  my ($self, $filename) = @_;
  my ($category, $name) = HTMLCommon::test_key($filename);
  $self->add_category($category);
  my $status       = TestCommon::SUCCESS_STATUS;
  ++$self->{tests_count} if (! exists $self->{tests}{$category}{$name});
  if (HTMLCommon::is_error($filename))
  {
    if (HTMLCommon::is_contain_error($filename)) 
    {
      $status       = TestCommon::FAILED_STATUS;
      ++$self->{errors_count};
    }
    $self->add_error($category, $name, $status, $filename);
  }
  else
  {
    $self->add_log($category, $name, $status, $filename);
  }
  return $status;
}

# Result page body
sub get_body {
  my $self   = shift;
  my $buffer = "";      
  my $idx    = 0;
  my %tests = % { $self->{tests} };
  foreach my $category ( sort keys(%tests) )
  {
    if ($category ne HTMLCommon::UNNAMED_CATEGORY || $idx != 0)
    {
      $buffer .=  CGI::TR({-class=>"category"}, [CGI::th({-colspan=>5}, [$category])]);
    }
    foreach my $name (sort keys( %{ $tests{$category} } ) )
    {
      $idx++;
      my %test = % { $tests{$category}{$name} };
      my $tr_class  = HTMLCommon::tr_class($idx % 2, $test{status} );
      $buffer .= CGI::TR({-class=>"$tr_class"}, 
                         CGI::td($name),
                         HTMLCommon::status_to_td($self->get_history_status($category, $name)),
                         CGI::td($self->file_link($test{error}  . TestCommon::LOG_STORE_EXT, 
                                                  $test{error_file}, 
                                                  "error", 0 )),
                         CGI::td($self->file_link($test{log}  . TestCommon::LOG_STORE_EXT, 
                                                  $test{log_file}, "debug")),
                         CGI::td($self->get_history_tag($category, $name)));
    }
  }
  return $buffer;
}


sub summary {
  my $self = shift;
  my @dirs = @{$self->{history_paths}};
  my $buffer = "";
  if (@dirs)
  {
    my $prior_error_path  = File::Spec->join($dirs[0], TestCommon::ERROR_PATH_SUFFIX);
    my $prior_log_path    = File::Spec->join($dirs[0], TestCommon::OUT_PATH_SUFFIX);
    opendir(DIR, $prior_error_path);
    my @errorfiles = grep (-f $_, map (File::Spec->join($prior_error_path, $_), readdir(DIR)));
    closedir(DIR);    
    opendir(DIR, $prior_log_path);
    my @logfiles = grep (-f $_, map (File::Spec->join($prior_log_path, $_), readdir(DIR)));
    closedir(DIR);    
    $self->{l_tests_count}   = scalar(grep (HTMLCommon::is_log($_),  @logfiles));
    $self->{l_errors_count}  = scalar(grep (HTMLCommon::is_error($_) && HTMLCommon::is_contain_error($_), @errorfiles));
  }
  my @list;
  for my $dir (splice(@dirs, 0, HTMLCommon::MAX_HISTORY_SIZE))
  {
    my $docpath  = File::Spec->join($dir, "tests.html");    
    if (-e $docpath)
    {
      my $time     = strftime "%d-%m-%y %H:%M", gmtime(stat($docpath)->mtime);
      my $root_path = File::Spec->catfile($self->{HISTORY_PATH}, $self->{DST_PATH}, 
                                          $self->{RESULT_PATH});
      my $tag = CGI::li(CGI::a({href=>File::Spec->abs2rel($docpath, $root_path),
                                type=>"text/html"}, $time));
      push @list, $tag;
    }
  }
  if (@list)
  {
    $buffer .= CGI::h2("History") . CGI::hr({color=>"black", noshade=>"true"}),;
    $buffer .= CGI::ol({-type=>"1"}, @list); 
  }
  $buffer .= CGI::h2("Summary ") . CGI::hr({color=>"black", noshade=>"true"}),;
  my $passed     = $self->{tests_count} - $self->{errors_count};
  my $new_passed = $self->{l_errors_count} - $self->{errors_count};
  my $new_test   = $self->{tests_count} - $self->{l_tests_count};
  $buffer .= CGI::start_table() . 
             CGI::TR({}, CGI::td("Total tests"), 
                     CGI::td($self->{tests_count}), 
                     CGI::td(HTMLCommon::diffs_to_color_tag($new_test) ) ) .
             CGI::TR({}, 
                     CGI::td("Passed"), 
                     CGI::td($passed), 
                     CGI::td(HTMLCommon::diffs_to_color_tag($new_passed))) .
             CGI::end_table();
  return $buffer;
}


# Get test status according to prior result
sub get_history_status {
  my ($self, $category, $test) = @_;
  my @dirs     = @{$self->{history_paths}};

  my @err_files = map(File::Spec->join($_,
                                       $self->{tests}->{$category}->{$test}->{error}) . TestCommon::LOG_STORE_EXT, @dirs);
  my @out_files = map(File::Spec->join($_,
                                       $self->{tests}->{$category}->{$test}->{log}) . TestCommon::LOG_STORE_EXT, @dirs);
  #@err_files = grep ( -e $_, @err_files);
  #@out_files = grep ( -e $_, @out_files);
  if (!exists $self->{tests}->{$category}->{$test}->{status})
  {
    return HTMLCommon::HS_UNDEFINED;
  }
  my $status = $self->{tests}->{$category}->{$test}->{status};
  if (@err_files)
  {
    my $err_file = $err_files[0];
    my $search_status       = TestCommon::SUCCESS_STATUS;
    if (HTMLCommon::is_contain_error($err_file)) 
    {
      $search_status = TestCommon::FAILED_STATUS;
    }
    if ($search_status eq TestCommon::FAILED_STATUS && $status eq TestCommon::SUCCESS_STATUS)
    {
      return HTMLCommon::HS_NEW_PASS;
    }
    elsif ($search_status eq TestCommon::FAILED_STATUS)
    {
      return HTMLCommon::HS_FAILED;
    }
    elsif ($search_status eq TestCommon::SUCCESS_STATUS && $status eq TestCommon::FAILED_STATUS)
    {
      return HTMLCommon::HS_NEW_FAIL;
    }
    return HTMLCommon::HS_PASSED;
  }
  elsif (@out_files)
  {
    return ($status eq TestCommon::FAILED_STATUS) ? 
        HTMLCommon::HS_NEW_FAIL : HTMLCommon::HS_PASSED;
  }
  return ($status eq TestCommon::FAILED_STATUS) ? 
      HTMLCommon::HS_NEW_FAIL : HTMLCommon::HS_NEW_PASS;
}

sub get_history_tag
{
  my ($self, $category, $test) = @_;
  my $filename = $test;
  if ($category ne "")
  {
    $filename = join(".", ($category, $filename));
  }
  $filename = TestCommon::normalize($filename);
  my $history_doc = HTMLTestsHistory->new($self->{HTML_PATH},  
                                          File::Spec->catfile($self->{HISTORY_PATH}, $self->{DST_PATH}, $self->{RESULT_PATH}),
                                          $filename, 
                                          $category, $test,
                                          File::Basename::basename($self->{tests}->{$category}->{$test}->{error} . TestCommon::LOG_STORE_EXT),
                                          File::Basename::basename($self->{tests}->{$category}->{$test}->{log} . TestCommon::LOG_STORE_EXT),
                                          \@{$self->{history_paths}});
  if (length($history_doc->get_body()) != 0)
  {
    $filename .= TestCommon::HTML_EXT;
    $self->{histories}{$filename} = $history_doc;
    return  CGI::a({href=>$filename,
                    type=>"text/html"}, "History");
  }
  return ""
}


# my HTML header 
sub header {
  my $self     = shift;
  my $doc_name = $self->{DOC_NAME};
  my @list;
  foreach my $path (@ {$self->{ADD_PATHS} })
  {
    my $log_doc = HTMLServerLogs->new($self->{HTML_PATH}, $path); 
    if (length($log_doc->get_body()) != 0)
    {
      my $filename = TestCommon::normalize(File::Basename::basename($path)) . TestCommon::HTML_EXT;
      $self->{add_logs}{$filename} = $log_doc;
      push @list, CGI::li(CGI::a({href=>$filename,
                                  type=>"text/html"}, File::Basename::basename($path)));
    }
  }
  my $buffer = CGI::ol({-type=>"1"}, @list);
  return 
      CGI::start_html(-title=>"$doc_name",
                      -style=>{'src' => HTMLCommon::get_doc_style}), 
      CGI::h1("$doc_name"),
      CGI::hr({color=>"black", noshade=>"true"}),
      CGI::h3("Ran on host: " . TestCommon::get_host_name()),
      CGI::h3("Processed directory: " . $self->{LOG_PATH}),
      CGI::h3("Start processing time: " . $self->{TIME}),
      $buffer,
      CGI::start_table(),
      CGI::TR({}, [CGI::th(['Test', 'Status', 'Error', 'Log', 'History'])]);
}

# my HTML footer  
sub footer {
  my $self     = shift;
  return CGI::end_table(), CGI::br(), $self->summary(), CGI::end_html();
}

sub flush {
  my $self     = shift;
  my $path     = shift;  # HTML file path
  $self->SUPER::flush($path);  
  my %histories = % { $self->{histories} };
  my %logs      = % { $self->{add_logs} };
  my ($file, $dir, $ext) = File::Basename::fileparse($path);
  foreach my $log (keys %logs)
  {
    my $log_path = File::Spec->join($dir, $log);
    $logs{$log}->flush($log_path);
  }
  foreach my $history (keys %histories)
  {
    my $history_path = File::Spec->join($dir, $history);
    $histories{$history}->flush($history_path);
  }
}

1;

# Test history page  
package HTMLTestsHistory;

use warnings;
use strict;
use File::stat;
use Time::localtime;
use POSIX qw(strftime);

our @ISA = qw(HTMLBaseDocument);

sub new {
  my $self             = shift;
  my $html_path        = shift;
  my $workspace        = shift;
  $self             = $self->SUPER::new("History", $html_path, $workspace);
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{NAME}          = shift;
  $self->{CATEGORY}      = shift;
  $self->{TEST}          = shift;
  $self->{ERROR_FILE}    = shift;
  $self->{LOG_FILE}      = shift;
  $self->{history_paths} = shift;
  return $self;
}

sub _get_category_tag {
  my $self = shift;
  if ($self->{CATEGORY})
  {
    return  CGI::h3("Category: $self->{CATEGORY}");
  }
  return "";
}

# my HTML header 
sub header {
  my $self     = shift;
  return 
      CGI::start_html(-title=>"$self->{NAME} history",
                      -style=>{'src' => HTMLCommon::get_doc_style}), 
      CGI::h2("History"),
      CGI::hr({color=>"black", noshade=>"true"}),
      $self->_get_category_tag,
      CGI::h3("Name: $self->{TEST}"),
      CGI::start_table(),
      CGI::TR({}, [CGI::th(['Time (GMT)', 'Stamp', 'Status', 'Error', 'Log'])]);
}


# my HTML footer  
sub footer {
  my $self     = shift;
  return CGI::end_table(), CGI::end_html();
}


# Result page body
sub get_body {
  my $self = shift;
  my $row_count = 0;
  my $buffer = "";
  foreach my $dir ( @{$self->{history_paths}} )
  {
    if ( $row_count < HTMLCommon::MAX_HISTORY_SIZE)
    {
      my $time     = strftime "%d-%m-%y %H:%M", gmtime(stat($dir)->mtime);
      {
        my $row = $self->row($dir, $row_count, $time);
        if ($row ne "")
        {
          $buffer.= $row;
          $row_count++;
        }
      }
    }
  }
  return $buffer;

}

sub get_status {
  my $error_file = shift;
  if (HTMLCommon::is_contain_error($error_file))
  {
    return TestCommon::FAILED_STATUS
  }
  return TestCommon::SUCCESS_STATUS
}

sub file_link
{
  my ($self, $path, $linkname) = @_;
  if ( -f $path)
  {
    return CGI::a({href=>File::Spec->abs2rel($path, 
                                             $self->{WORKSPACE}),
                   type=>"text/html"}, $linkname);
  }
  return ""
}


sub row {
  my $self                = shift;
  my ($path, $idx, $time) = @_;
  my $errorfile_path  = File::Spec->join($path, "Errors", $self->{ERROR_FILE});  
  my $logfile_path    = File::Spec->join($path, "Logs", $self->{LOG_FILE});  
  if (-f $errorfile_path || -f $logfile_path)
  {
    my $logname         = File::Basename::basename($path);
    my $status          = get_status($errorfile_path);
    my $tr_class        = HTMLCommon::tr_class($idx % 2, $status);


    return  CGI::TR({-class=>"$tr_class"}, 
                    CGI::td($time),
                    CGI::td($logname), 
                    HTMLCommon::status_to_td($status),
                    CGI::td($self->file_link($errorfile_path, "error")),
                    CGI::td($self->file_link($logfile_path, "log")));
  }
  return "";
}

1;


# IndexPage
package HTMLIndex;

use warnings;
use strict;
use File::Find;

our @ISA = qw(HTMLBaseDocument);


sub new {
  my $self        = shift;
  my $path        = shift;
  $self           = $self->SUPER::new("Index", "", $path);
  unless (ref $self) {
    $self = bless {}, $self;
  }
  return $self;
}

sub wanted
{
  my ($self, $fullpath, $links) = @_;
  my $path = $self->relative_path($fullpath);
  my ($file, $dir, $ext) = File::Basename::fileparse($path);
  if ($file eq "tests.html")
  {
    
    (my $linkname = substr($dir, 0, -1)) =~ s/\//-/g;
    
    $links->{$linkname} = CGI::a({href=>$self->relative_path($fullpath),
                                  type=>"text/html"}, $linkname);
  }
}

sub get_body 
{
  my $self = shift;
  my %links;
  File::Find::find({wanted => sub { $self->wanted($File::Find::name, \%links);} }, 
                   ($self->{WORKSPACE}));
  my @list = ();
  foreach my $linkname (sort keys( %links ) )
  {
    my $tag=CGI::li($links{$linkname});
    push @list, $tag;
  }
  return CGI::ol({-type=>"1"}, @list); 
}

1;


# IndexPage
package HTMLServerLogs;

use warnings;
use strict;
use File::Find;

our @ISA = qw(HTMLBaseDocument);


sub new {
  my $self        = shift;
  my $dst_path    = shift;
  my $src_path    = shift;
  $self           = $self->SUPER::new("ServerLogs", $dst_path, $src_path);
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{files}  = {};
  return $self;
}

sub header{
  my $self     = shift;
  return 
      CGI::start_html(-title=>"$self->{DOC_NAME}",
                      -style=>{'src' => HTMLCommon::get_doc_style}), 
      CGI::h1("$self->{DOC_NAME}"),
      CGI::start_table(),
      CGI::TR({}, [CGI::th(['File'])]);
}

sub footer {
  my $self     = shift;
  return CGI::end_table(), CGI::end_html();
}

sub wanted
{
  my ($self, $path) = @_;
  if (-f $path)
  {
    my $srcpath = File::Spec->abs2rel($path, 
                                      $self->{WORKSPACE});
    (my $dstname = $srcpath) =~ s/\//-/g;
    my $dst_path =  File::Spec->catfile(TestCommon::OUT_PATH_SUFFIX, $dstname) . TestCommon::LOG_STORE_EXT;
    $self->{files}{$dst_path} = $path;
  }
}

sub get_body
{
  my $self = shift;
  File::Find::find({wanted => sub { $self->wanted($File::Find::name);} }, 
                   $self->{WORKSPACE});
  my $buffer = "";
  my %files = % { $self->{files} };
  foreach my $file (sort keys( %files ) )
  {
    my $linkname = File::Spec->abs2rel($files{$file}, 
                                      $self->{WORKSPACE});
    $buffer.=CGI::TR(CGI::td(CGI::a({href=>$file,
                              type=>"text/html"}, $linkname)));
  }
  return $buffer;
}

sub flush {
  my $self     = shift;
  my $path     = shift;  # HTML file path
  $self->SUPER::flush($path);  
  my ($file, $dir, $ext) = File::Basename::fileparse($path);
  my %files = % { $self->{files} };
  foreach my $file (sort keys( %files ) )
  {
    my $dstfile = File::Spec->catfile($dir, $file);
    File::Copy::copy($files{$file},$dstfile);
  }
}
1;

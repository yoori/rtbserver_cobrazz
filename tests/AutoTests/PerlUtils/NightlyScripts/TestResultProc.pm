
use warnings;
use strict;

use warnings;
use strict;

use File::Basename;
use File::Copy;
use File::Path;
use File::Spec;

use TestHTML;
use Common;


# class for making report
{
  use constant TEST_SUBDIR_NAME  => "tests";

  # Log processor
  package TestResultProcessor;
  
  sub new {
    my $class             = shift;
    my $self              = {};
    $self->{LOG_PATH}     = shift;  # path, where logs for processing may lies (absoliute)
    $self->{HTML_PATH}    = shift;  # root path of HTTP server (absoliute)
    $self->{HTML_TR_PATH} = shift;  # relative HTML path for test results (relative)
    $self->{ROOT_PATH}    = shift;  # root path, container for all test logs (absoliute)
    $self->{TEST_TIME}    = shift;  # test start time
    $self->{HISTORY_PATH} = shift;  # history test result path
    $self->{DST_PATH}     = shift;  # destination path
    $self->{stylesheet}   = shift;  # stylesheet file
    $self->{add_paths}    = shift;  # additional paths;
    $self->{ERROR_IDX}    = 0;
    bless($self, $class);
    return $self;
  }


  sub get_status {
    my $self      = shift;
    return ($self->{ERROR_IDX} == 0);
  }
  
  # get workspace name by workspace path
  sub get_workspace_name {
    my $self       = shift;
    my $with_time  = shift;
    my $relative_path = File::Spec->abs2rel($self->{LOG_PATH}, $self->{ROOT_PATH});
    if ($with_time)
    {
      return $relative_path . "-" . $self->{TEST_TIME};
    }
    return $relative_path;
  }
  
  # get HTML relative or absolute path for file
  sub get_file_path {
    my $self          = shift;
    my $filename      = shift;
    my $ext           = shift;
    my $is_abs        = shift;
    my @path_parts    = ($self->get_workspace_name(1));
    if ($self->{HTML_TR_PATH} ne "")
    {
      @path_parts = ($self->{HTML_TR_PATH}, @path_parts);
    }
    if ($is_abs)
    {
      @path_parts = ($self->{HTML_PATH}, @path_parts);
    }
    if ($ext eq TestCommon::ERROR_EXT)
    {
      @path_parts = (@path_parts, TestCommon::ERROR_PATH_SUFFIX);
    }
    elsif ($ext eq TestCommon::OUT_EXT)
    {
      @path_parts = (@path_parts, TestCommon::OUT_PATH_SUFFIX);
    }
    @path_parts = (@path_parts, $filename . $ext);
    return TestCommon::normalize(join ("/", @path_parts));
  }

  # create dir for path, if it needed
  sub check_and_create_dir {
    my $path = shift;
    my $dirname = File::Basename::dirname($path);
    if (! -d $dirname)
    {
      File::Path::mkpath($dirname) || die "Cann't create directory $dirname";
    }
  }

  # run log processor
  sub makeHTML {
    my $self    = shift;
    opendir(DIR, $self->{LOG_PATH});
    my $htmlpath = $self->get_file_path(::TEST_SUBDIR_NAME, TestCommon::HTML_EXT, 1);
    my $html_header = sprintf("Log processing result");
    my $html = HTMLTestsResults->new($html_header,  
                                     File::Spec->join($self->{HTML_PATH}, $self->{HTML_TR_PATH}),
                                     $self->get_workspace_name(0),
                                     $self->{DST_PATH},
                                     $self->{LOG_PATH}, 
                                     $self->{TEST_TIME},
                                     $self->{HISTORY_PATH},
                                     \@{ $self->{add_paths} });
    $html->prepare($self->{HTML_TR_PATH});
    my $filename;
    my $have_logs = 0;
    
    foreach $filename (readdir(DIR))
    {
      my $filepath = File::Spec->join($self->{LOG_PATH}, $filename);
      if ( -f $filepath )
      {
        my ($file, $dir, $ext) = File::Basename::fileparse($filename, 
                                                           (TestCommon::ERROR_EXT, 
                                                            TestCommon::OUT_EXT));
        if ($ext eq TestCommon::ERROR_EXT || $ext eq TestCommon::OUT_EXT)
        {
          $have_logs = 1;
          $self->{ERROR_IDX}++ if $html->add_test($filepath) eq TestCommon::FAILED_STATUS;
          # move logs to HTML path
          my $newpath=$self->get_file_path($file, $ext, 1) . TestCommon::LOG_STORE_EXT;
          check_and_create_dir($newpath);
          File::Copy::copy($filepath, $newpath) || die "Cann't copy $filepath to $newpath";
        }
      }
    }
    closedir(DIR);
    # return HTML-path of created document
    if ($have_logs)
    {
      check_and_create_dir($htmlpath);
      if (defined $self->{stylesheet})
      {
        my $csspath = $self->get_file_path(HTMLCommon::get_doc_style(), "", 1);
        check_and_create_dir($csspath);
        File::Copy::copy($self->{stylesheet}, $csspath) || die "Cann't copy $self->{stylesheet} to $csspath";
      }
      $html->flush($htmlpath);
      return $self->get_file_path(::TEST_SUBDIR_NAME, TestCommon::HTML_EXT, 0);
    }
    return ""
  }
}

1;


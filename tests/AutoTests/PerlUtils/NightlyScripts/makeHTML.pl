#!/usr/bin/perl
use warnings;
use strict;

=head1 NAME

makeHTML.pl - processing test results logs to HTML reports

=head1 SYNOPSIS

  makeHTML.pl OPTIONS

=head1 OPTIONS

=over

=item C<--desc, -d tests description>

=item C<--in, -i input test results logs path>

B<Required.>

=item C<--timestamp, -t test results timestamp>

S<Default, timestamp=<current time>("YYYY-MM-DD-hh-mm")>

=item C<--stylesheet, -s CSS stylesheet for test results>

S<Default, stylesheet is not used>

=item C<--url, -u HTTP server URL>

S<Default, http:://<HOSTNAME>>

=item C<--http_root, -hr HTTP server root path (absolute)>

S<Default, /var/www>

=item C<--http_test_path, -ht path to test result (relative)>

S<Default, empty - results stored in the HTTP server root path>

=item C<--history_path, -hp path with tests result history>

S<Default, empty - http_root is history>

=item C<--dst_sub_path, -dp destination path for test results>

S<Default, empty>

=item C<--additional_log_path, -al path with additional logs>

S<Default, additional paths is not set>

=item C<--maillist, -m e-mail list to sending test reports>

S<Default, mail is not sent>

=back

=cut

use Getopt::Long qw(:config gnu_getopt bundling);
use Pod::Usage;
use File::Basename;
use File::Copy;
use File::Path;

use TestResultProc;
use EmailNotifySend;
use TestHTML;
use Common;

my $host = TestCommon::get_host_name;

my @maillist;
my @add_paths;
my %options = ( timestamp       => TestCommon::get_local_time,
                stylesheet      => undef,
                url             => "http://$host",
                http_root       => "/var/www",
                http_test_path  => "",
                dst_sub_path    => "",
                additional_log_path => \@add_paths,
                maillist        => \@maillist );

if (! GetOptions(\%options, qw(desc|d=s in|i=s timestamp|t=s 
                               stylesheet|s=s url|u=s http_root|hr=s
                               dst_sub_path|dp=s history_path|hp=s
                               http_test_path|ht=s  
                               additional_log_path|al=s@
                               maillist|m=s@)))
{
  pod2usage(1);
}

my @requered    = ("desc", "in");
my @not_defined = grep {not defined $options{$_}} @requered;

if (@not_defined)
{
  print "Options '". join(", ", @not_defined) . "' is not defined!\n";
  pod2usage(1);
}

if (! defined $options{history_path})
{
  $options{history_path} = join("/", ($options{http_root}, $options{http_test_path}));
}


my $style_file      = File::Basename::basename($options{stylesheet});
HTMLCommon::set_doc_style("$style_file");

# processing logs in subdirectories
my @documents     = ();
my $tests_result  = round_dirs(File::Basename::dirname($options{in}), $options{in}, \@documents);

# sending e-mail notification
EmailNotification::send_mail(\@maillist, $options{desc}, \@documents,  $tests_result);

sub round_dirs {
  my $root_path = shift;
  my $path      = shift;
  my $docs      = shift;
  my $result    = 0;
  my $log_proc  = TestResultProcessor->new($path, $options{http_root}, 
                                           $options{http_test_path}, 
                                           $root_path, 
                                           $options{timestamp},
                                           $options{history_path},
                                           $options{dst_sub_path},
                                           $options{stylesheet},
                                           \@add_paths);
  my $doc_path      = $log_proc->makeHTML();
  if ($doc_path ne "")
  {
    @$docs  = (@$docs, join("/", ($options{url}, $doc_path)));
    $result = $result || $log_proc->get_status();
  }
  return $result;
}






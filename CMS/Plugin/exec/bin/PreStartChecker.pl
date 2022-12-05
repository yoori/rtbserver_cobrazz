#!/usr/bin/perl

use strict;
use warnings;
use Args;

sub check_service;
sub filter_comments;
sub print_list;
sub system_for_output;

my $usage =
  "Usage: $0 --serv-<service name>=<host name>#<corba ref>,.. --ssh-identity=<> --env=<> [options]\n" .
  "  'serv' encoded location of service to be checked\n" .
  "  'ssh-identity' identity file\n" .
  "  'env' environment arguments\n" .
  "Common Options:\n" .
  "  '--verbose' print extended starting information about the cluster\n" .
  "  '--log-file' save output to provided file\n";

my $args = Args::parse(@ARGV);
my %services;

if(!defined($args->{'ssh-identity'}) || !defined($args->{'env'}))
{
  die "Required ssh-identity and env arguments\n$usage";
}

my $log_file = $args->{'log-file'};

if(defined($log_file))
{
  open(LOG, ">>$log_file");
}
else
{
  open(LOG, ">&STDOUT");
}
my $verbose = exists $args->{'verbose'} ? 1 : 0;

while(my ($arg_key, $arg_value) = each(%$args))
{
  if($arg_key =~ m/^serv-(.*)$/)
  {
    my $service_name = $1;
    my @service_refs = split(',', $arg_value);
    foreach my $service_ref(@service_refs)
    {
      if($service_ref ne '')
      {
        if($service_ref !~ m|^(.*)#(.*)$|)
        {
          die "Invalid reference format: $service_ref";
        }

        $services{$service_name}->{$1} = $2;
      }
    }
  }
}

my %check_result;

foreach my $service_name(keys(%services))
{
  # check service all references
  my $service_refs = $services{$service_name};
  my @failed_hosts;
  my @exit_codes;
  my @comments;
  foreach my $service_host(keys(%$service_refs))
  {
    # check corba ref
    my $status = 'ready';
    my ($res, $comment) = check_service(
      $service_host, $service_refs->{$service_host}, $status, $args);

    if ($res)
    {
      if ($comment eq "\n")
      {
        $comment = 'not ready';
      }
      if (not $service_name eq "ChannelServer")
      {
# ChannelController contain hosts into comment already.
        $comment = $service_host . ": " . $comment;
      }
      push @exit_codes, $res;
      $comment =~ s/^\s+//;
      $comment =~ s/\s+$//;
      push @comments, $comment;
    }
  }
  if (scalar(@exit_codes))
  {
    $check_result{$service_name}{'codes'} = \@exit_codes;
    $check_result{$service_name}{'comments'} = \@comments;
  }
}

if (scalar(keys %check_result))
{
  my $message = '';
  foreach my $service_name(keys(%check_result))
  {
    if ($message ne '')
    {
      $message .= "\n";
    }
    my $exit_codes = $check_result{$service_name}{'codes'};
    my $comments = $check_result{$service_name}{'comments'};

    my $is_service_controller = (($service_name eq "ChannelServer") or ($service_name eq "UserInfoManager"));

    if (scalar(@$comments) > 0)
    {
      $message .= "$service_name " . ($verbose ?
       " exit codes [" . print_list(',', $exit_codes) . "], " .
       print_list(';', $comments) :
        filter_comments($is_service_controller, $comments));
    }
  }
  print LOG "$message\n";
}

exit (scalar(keys %check_result) != 0);

sub filter_comments
{
  my ($is_service_controller, $comments_ref) = @_;
  my @current_comment = $is_service_controller ?
    split (';', @$comments_ref[0]) : @$comments_ref;

  my @comments = ();

  my $rec = 0;
  foreach my $comment(@current_comment)
  {
    my $idx = index($comment, ',');
    my $short_comment = $idx > 0 ? substr($comment, 0, $idx) : $comment;
    my $comma_count = () = $comment =~ /,/g;
    my ($before, $progress_bar, $n1, $total_part, $n2) =
      $short_comment =~ m/(\D\w*\s*)*\s*((\d+)(\/(\d+))?)?/;
    if (not defined($n1))
    {
      $n1 = ~0;
    }
    if (defined $n2 and $n2 > 0)
    {
      $n1 = $n1 / $n2;
    }

    # key - comma_count, n1(, n2 ?)
    push @{ $comments[$rec] }, $comma_count;
    push @{ $comments[$rec] }, $n1;
    push @{ $comments[$rec] }, $short_comment;
    ++$rec;
  }
  @comments = sort {($b->[0] <=> $a->[0]) or ($a->[1] <=> $b->[1])} @comments;
  my @sel;
  for (my $i = 0; $i <= $#comments and $i < 3; ++$i)
  {
    push @sel, $comments[$i][2];
  }
  my $current_comment = join(",\n  ", @sel);
  if (length($current_comment) > 120)
  {
    $current_comment = substr($current_comment, 0, 120) . "...";
  }
  return $current_comment;
}

sub print_list
{
  my ($separator, $list_ref) = @_;
  my $list_output_maxlen = 256;
  my $str = '';
  while (length($str) < $list_output_maxlen and scalar(@$list_ref))
  {
    if ($str ne '')
    {
      $str .= "$separator ";
    }
    $str .= shift @$list_ref;
  }
  if (scalar(@$list_ref))
  {
    $str .= "$separator ...";
  }
  return $str;
}

sub check_service
{
  my ($host, $reference, $status, $args) = @_;
  my $probe_util = "ProbeObj";

  my $command =
      "ssh -i " . $args->{'ssh-identity'} . " $host '" .
    "source " . $args->{'env'} . " && " .
    "$probe_util -status $status $reference || " .
    "{ $probe_util -comment $reference 2>/dev/null ; exit 1 ; }'";

  my @res = system_for_output($command);
  return @res;
}

sub system_for_output
{
  open(SSO, $_[0] . '|') or die "system_for_output: can't open pipe.";
  my $output = "";

  while (my $line = <SSO>)
  {
    $output .= $line;
  }

  close(SSO);
  my $ret = $?;

  return ($ret, $output);
}

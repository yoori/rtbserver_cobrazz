#!/usr/bin/perl

use strict;
use warnings;

use Text::CSV_XS;
use open qw( :std :encoding(UTF-8) );

use Cwd;
use Class::Struct;
use List::Util qw( min minstr );
use File::Find;
use File::Basename;
use Path::Iterator::Rule;
use Log::Dispatch;
use XML::Twig;
use File::Spec::Functions;
use Pod::Usage;
use Proc::Daemon;
use Getopt::Long qw(:config gnu_getopt bundling);
use Date::Parse;
use DateTime;
use DateTime::Duration;
use POSIX qw(strftime);

use Predictor::CTRTrivialModel;
use Predictor::BidCostModel;
use Predictor::BidCostModelEvaluator;

#use Carp qw/longmess cluck confess/;

my $continue = 1;

use constant DUMP_MAX_SIZE => 100000;
use constant DEFAULT_SLEEP => 2;
use constant DEFAULT_SLEEP_TIMEOUT => 60 * 60;
use constant MAX_PROCESS_FILES => 10000;

my $pid_file_name = 'bidcost_predictor_merger.pid';

#struct(Agg => [bids => '$', imps => '$', clicks => '$']);
struct(ResFile => [tmp_file => '$', result_file => '$']);

my $log_format = sub
{
  my %p = @_;
  my $t = time;
  my $time = strftime("%a %d %b %Y %H:%M:%S", localtime $t);
  $time .= sprintf(".%03d", ($t-int($t)) * 1000);
  $time .= strftime(" %Z", localtime $t);
  return $time . " [" . uc($p{level}) . "] " . $p{message};
};

my $logger = Log::Dispatch->new(
  callbacks => $log_format,
  outputs => [
      [ 'Screen', min_level => 'debug', newline => 1, ],
      [ 'Screen', min_level => 'warning', stderr => 1, newline => 1 ],
    ],
  );

sub get_log_level
{
  my ($log_level) = @_;
  my @levels = (
    'emergency', # EMERGENCY
    'alert',     # ALERT
    'critical',  # CRITICAL
    'error',     # ERROR
    'warning',   # WARNING
    'notice',    # NOTICE
    'info',      # INFO
    'debug',     # DEBUG
    'debug'      # TRACE
    );

  if (defined $log_level and $levels[$log_level])
  {
    return $levels[$log_level];
  }
  return 'debug';
}

sub DEBUG { $logger->debug(@_); }
sub ERROR { $logger->error(@_); }
sub TRACE { $logger->debug(@_); }
sub WARN  { $logger->warn(@_); }
sub INFO  { $logger->info(@_); }

# utils
sub dump_file
{
  my ($dump_file_path, $date_agg) = @_;

  INFO("Dump agg into " . $dump_file_path);
  open(my $dump_fh, '>>', $dump_file_path) or die "Could not open '$dump_file_path' $!\n";

  while (my ($key, $agg) = each (%$date_agg))
  {
    print $dump_fh "$key\t" . $agg->bids() . "\t" . $agg->imps() . "\t" . $agg->clicks() . "\n";
  }

  close($dump_fh);
}

sub dump_date
{
  my ($aggs_by_date, $date, $result_files, $output_folder) = @_;

  my $dump_agg = $aggs_by_date->{$date};
  my $dump_size = keys %$dump_agg;

  my $dump_file_path = '';

  if(exists($result_files->{$date}))
  {
    $dump_file_path = $result_files->{$date}->tmp_file();
  }
  else
  {
    my $base_dump_file_path = "BidCostAggStat." . $date . "." . sprintf( "%06d", rand(999999) );
    #print("output_folder: $output_folder\n");
    #print("base_dump_file_path: $base_dump_file_path\n");
    $dump_file_path = $output_folder . "/~" . $base_dump_file_path;
    $result_files->{$date} = new ResFile(
      tmp_file => $dump_file_path,
      result_file => $output_folder . "/" . $base_dump_file_path);
  }

  dump_file($dump_file_path, $dump_agg);

  delete $aggs_by_date->{$date};

  return $dump_size;
}

# reaggregate:
#   BidCostStatAgg.* => BidCostStatAgg.*
sub reaggregate
{
  my ($input_folder, $output_folder, $continue) = @_;

  INFO("Reaggregate: started for folder = $input_folder");

  eval
  {
    # search input files
    my $rule = Path::Iterator::Rule->new;
    $rule->name("BidCostAggStat.*");
    my @all_files = $rule->all(($input_folder));
    my %agg_files;

    foreach my $file(@all_files)
    {
      my ($file_name, $file_path) = fileparse($file);
      if($file_name =~ m/BidCostAggStat[.]([^.]+)[.].*/)
      {
        my $date = $1;
        if(!exists($agg_files{$date}))
        {
          $agg_files{$date} = [];
        }
        push(@{$agg_files{$date}}, $file);
      }
    }

    my %date_to_agg;

    while(my ($date, $files) = each(%agg_files))
    {
      if($$continue == 0)
      {
        last;
      }

      if(scalar(@$files) > 1)
      {
        INFO("Reaggregate: reaggregate @$files");

        # aggregate
        my $res_agg = {};
        foreach my $file(@$files)
        {
          $res_agg = Predictor::BidCostModelEvaluator::load_agg_file($file, $res_agg);
        }

        # save & unlink source files
        my $base_dump_file_path = "BidCostAggStat." . $date . "." . sprintf( "%06d", rand(999999) );
        my $dump_tmp_file_path = $output_folder . "/~" . $base_dump_file_path;
        my $dump_file_path = $output_folder . "/" . $base_dump_file_path;

        dump_file($dump_tmp_file_path, $res_agg);

        rename($dump_tmp_file_path, $dump_file_path) ||
          die("can't rename $dump_tmp_file_path to $dump_file_path");

        foreach my $file(@$files)
        {
          unlink($file);
        }
      }
    }
  };

  if($@)
  {
    ERROR("Reaggregate: caught error $@");
  }

  INFO("Reaggregate: finished");
}

# fetch_input_files:
#   BidCostStat.* => BidCostStatAgg.*
sub aggregate
{
  my ($input_folder, $output_folder, $continue) = @_;

  INFO("Aggregate: started");

  # search input files
  my $rule = Path::Iterator::Rule->new;
  $rule->name("BidCostStat.*");
  my @all_files = sort $rule->all(($input_folder));

  eval
  {
    while(scalar(@all_files) > 0 && $$continue > 0)
    {
      my @process_files = splice(@all_files, 0, MAX_PROCESS_FILES);

      INFO("Aggregate: to process files");

      my $record_count = 0;
      my %aggs_by_date;

      my %result_files; # date to ResFile
      my @processed_files;

      foreach my $file(@process_files)
      {
        if($$continue == 0)
        {
          last;
        }

        INFO("Check $file");

        if(open(my $fh, '<', $file))
        {
          # read head
          my $head = <$fh>;
          chomp $head;

          #print("Head <" . $head . ">\n");
          if($head =~ /^BidCostStat/)
          {
            INFO("To process $file");

            # read file content
            my $date = '';
            while(my $line = <$fh>)
            {
              chomp $line;
              #print("LINE: $line\n");

              if($line =~ /^(\d{4}-\d{2}-\d{2})$/)
              {
                $date = $1;
              }
              elsif($line)
              {
                # <tag id> <ext tag id> <domain> <cost> <bids> <imps> <clicks>
                my @fields = split('\t', $line);
                #print("FIELDS: ");
                #print(@fields);
                #print("\n");
                if(scalar @fields >= 7)
                {
                  my $tag_id = $fields[0];
                  my $ext_tag_id = $fields[1];
                  my $domain = $fields[2];
                  my $cost = $fields[3];
                  my $bids = $fields[4];
                  my $imps = $fields[5];
                  my $clicks = $fields[6];

                  if(!exists($aggs_by_date{$date}))
                  {
                    $aggs_by_date{$date} = {};
                  }

                  my $key = "$tag_id\t$domain\t$cost";
                  if(!exists($aggs_by_date{$date}->{$key}))
                  {
                    $aggs_by_date{$date}->{$key} = new Agg(bids => 0, imps => 0, clicks => 0);
                    $record_count = $record_count + 1;
                  }

                  my $res_agg = $aggs_by_date{$date}->{$key};
                  $res_agg->bids($res_agg->bids() + $bids);
                  $res_agg->imps($res_agg->imps() + $imps);
                  $res_agg->clicks($res_agg->clicks() + $clicks);
                }
              }
            }

            push(@processed_files, $file);
            INFO("Processed " . $file . "(record count = " . $record_count . ")");

            # check dump or not
            while($record_count > DUMP_MAX_SIZE)
            {
              my $min_date = minstr(keys(%aggs_by_date));
              $record_count = $record_count - dump_date(
                \%aggs_by_date, $min_date, \%result_files, $output_folder);
            }
          }

          close($fh);
        } # if open
      }

      # dump aggs_by_date
      my @vals;

      while(my ($d, $res_file) = each(%aggs_by_date))
      {
        push(@vals, [$d, $res_file]);
      }

      foreach my $el(@vals)
      {
        my $d = $el->[0];
        dump_date(\%aggs_by_date, $d, \%result_files, $output_folder);
      }

      # rename result files & unlink source files
      while (my ($d, $res_file) = each (%result_files))
      {
        INFO("Rename " . $res_file->tmp_file() . " to " . $res_file->result_file());
        rename($res_file->tmp_file(), $res_file->result_file()) ||
          die("can't rename " . $res_file->tmp_file() . " to " . $res_file->result_file());
      }

      foreach my $file(@processed_files)
      {
        unlink($file);
      }

    }
  };

  if($@)
  {
    ERROR("Aggregate: caught error $@");
  }

  INFO("Aggregate: finished");
}

# generate_model:
#   BidCostStatAgg.* => bid cost model
#   BidCostStatAgg.* => ctr model
sub generate_model
{
  my ($agg_folder, $model_folder, $tmp_model_folder,
    $ctr_model_folder, $ctr_tmp_model_folder,
    $continue) = @_;

  INFO("Generate model: started");

  my $rule = Path::Iterator::Rule->new;
  $rule->name("BidCostAggStat.*");
  my @all_files = sort $rule->all(($agg_folder));

  my $eval_model = new Predictor::BidCostModelEvaluator();
  foreach my $file(reverse(@all_files))
  {
    if($$continue == 0)
    {
      last;
    }

    INFO("Generate model: load aggregate $file");
    # read aggregate
    $eval_model->add_file($file);
  }

  if($$continue > 0)
  {
    INFO("Generate model: to evaluate model");
    my $model = $eval_model->evaluate([0.95, 0.75, 0.5, 0.25], $continue, $logger);
    INFO("Generate model: from evaluate model");

    if(defined($model))
    {
      my $model_dir = strftime("%Y%m%d.%H%M%S", gmtime);
      my $out_model_dir = $tmp_model_folder . "/" . $model_dir;
      mkdir($out_model_dir);
      my $out_model_file = $out_model_dir . "/bid_cost.csv";
      $model->save($out_model_file);

      my $res_model_dir = $model_folder . "/" . $model_dir;
      rename($out_model_dir, $res_model_dir);
      INFO("Generate model: model saved into $res_model_dir");
    }
  }

  if($$continue > 0 && defined($ctr_model_folder))
  {
    INFO("Generate model: to evaluate ctr model");
    my $model = $eval_model->evaluate_ctr($continue, $logger);
    INFO("Generate model: from evaluate ctr model");

    if(defined($model))
    {
      my $model_dir = strftime("%Y%m%d.%H%M%S", gmtime);
      my $out_model_dir = $ctr_tmp_model_folder . "/" . $model_dir;
      mkdir($out_model_dir);
      my $out_model_file = $out_model_dir . "/trivial_ctr.csv";
      $model->save($out_model_file);

      my $res_model_dir = $ctr_model_folder . "/" . $model_dir;
      rename($out_model_dir, $res_model_dir);
      INFO("Generate model: ctr model saved into $res_model_dir");
    }
  }

  INFO("Generate model: finished");
}

##
sub start
{
  my ($daemon, $pid, $pid_file, $config_file) = @_;

  if (!$pid)
  {
    my $work_dir = getcwd();

    $daemon->Init({ work_dir => $work_dir, pid_file => $pid_file });

    $SIG{INT} = \&shutdown;
    $SIG{TERM} = \&shutdown;
    $SIG{__DIE__} = sub { ERROR("Caught error: @_"); exit(1); };

    my $twig = XML::Twig->new;
    eval { $twig->parsefile($config_file) } or 
      die "Invalid xml config: $@";

    my $config = $twig->root;

    my $logger_config = $config->first_child('cfg:Logger') or
      die "Invalid xml config: logger config is undefined";
    my $stat_config = $config->first_child('cfg:BidCost') or
      die "Invalid xml config: impression config is undefined";

    my $processing_period = 
      $stat_config->att('process_period') || DEFAULT_SLEEP_TIMEOUT;

    my @outputs;
    my $config_log_level = $logger_config->att('log_level');
    my $log_name = $logger_config->att('filename');

    for my $suff ($logger_config->children('cfg:Suffix'))
    {
      my $log_level = $suff->att('max_log_level');
      my $suffix = $suff->att('name');
      push(@outputs, ['File', mode => 'append', newline => 1,
        min_level => get_log_level(
          min($config_log_level, $log_level)),
          filename => $log_name . $suffix ]);
    }


    $logger = Log::Dispatch->new(callbacks => $log_format, outputs => \@outputs);

    INFO("Start BidCost processor");

    my $in_stat_path = $stat_config->att('path');
    my $agg_bid_cost_path = $stat_config->att('cache');

    my $model_path = $stat_config->att('model_path');
    my $tmp_model_path = $stat_config->att('tmp_model_path');

    my $ctr_model_path = $stat_config->att('ctr_model_path');
    my $tmp_ctr_model_path = $stat_config->att('tmp_ctr_model_path');

    my $aggregate_period = DateTime::Duration->new(seconds => $stat_config->att('aggregate_period'));
    my $reaggregate_period = DateTime::Duration->new(seconds => 600);
    my $model_period = DateTime::Duration->new(seconds => $stat_config->att('model_period'));

    my $last_fetch_input_files_time = DateTime->now()->add_duration(DateTime::Duration->new(days => -1));
    my $last_model_generate_time = DateTime->now()->add_duration(DateTime::Duration->new(days => -1));
    my $last_reaggregate_time = DateTime->now()->add_duration(DateTime::Duration->new(days => -1));

    while ($continue)
    {
      my $now = DateTime->now();

      #INFO("XXX " . ($now->strftime('%F %T')));
      #INFO("T1 = " . $last_fetch_input_files_time->clone()->add_duration($aggregate_period)->strftime('%F %T'));
      #INFO("T2 = " . $last_model_generate_time->clone()->add_duration($model_period)->strftime('%F %T'));
      #INFO("T3 = " . $last_reaggregate_time->clone()->add_duration($reaggregate_period)->strftime('%F %T'));

      if(DateTime->compare($last_fetch_input_files_time->clone()->add_duration($aggregate_period), $now) < 0)
      {
        #INFO("XXX1");
        aggregate($in_stat_path, $agg_bid_cost_path, \$continue);
        $last_fetch_input_files_time = $now;
      }

      if(DateTime->compare($last_model_generate_time->clone()->add_duration($model_period), $now) < 0)
      {
        #INFO("XXX2");
        generate_model($agg_bid_cost_path,
          $model_path, $tmp_model_path,
          $ctr_model_path, $tmp_ctr_model_path,
          \$continue);
        $last_model_generate_time = $now;
      }

      if(DateTime->compare($last_reaggregate_time->clone()->add_duration($reaggregate_period), $now) < 0)
      {
        #INFO("XXX3");
        reaggregate($agg_bid_cost_path, $agg_bid_cost_path, \$continue);
        $last_reaggregate_time = $now;
      }

      sleep(DEFAULT_SLEEP);
    }

    INFO("BidCost processor finished");
    unlink $pid_file;
  }
  else
  {
    ERROR("Already Running with pid $pid");
  }
}

sub stop
{
  my ($daemon, $pid, $pid_file) = @_;
  if ($pid)
  {
    INFO("Stopping pid $pid ...");
    if ($daemon->Kill_Daemon($pid_file, 15))
    {
      INFO("Successfully stopped.");
    }
    else
    {
      ERROR("Could not find $pid. Was it running ?");
    }
  }
  else
  {
    INFO("Not running, nothing to stop.");
  }
}

sub status
{
  my ($pid) = @_;
  if ($pid) {
    print "Running with pid $pid.\n";
  } else {
    print "Not running.\n";
  }
}

sub shutdown
{
  $continue = 0;
}

sub main
{
  my $config_file;
  my $pid_file;

  my %options = (
    "config-file-path|c=s" => \$config_file,
    "pid-file-path|l=s" => \$pid_file,
  );

  if (! GetOptions(%options))
  {
    pod2usage(1);
  }

  if (scalar(@ARGV) != 1)
  {
    print "You have to define operation: start, stop or status !\n";
    pod2usage(1);
  }

  if (!defined($pid_file))
  {
    $pid_file = catfile(getcwd(), $pid_file_name);
  }

  my $operation = $ARGV[0];
  my $daemon = Proc::Daemon->new;
  my $pid = $daemon->Status($pid_file);

  if ($operation eq 'start')
  {
    start($daemon, $pid, $pid_file, $config_file);
  }
  elsif ($operation eq 'stop')
  {
    stop($daemon, $pid, $pid_file);
  }
  elsif ($operation eq 'status')
  {
    status($pid);
  }
  else
  {
    print "Unknown operation: $operation!\n";
  }
}

my $exit_code = main();
exit $exit_code;

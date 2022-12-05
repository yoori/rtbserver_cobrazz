#!/usr/bin/perl

use strict;
use warnings;

use POSIX qw( strftime );
use Time::HiRes qw( gettimeofday );

use Common::ModifierExecImpl;
use Common::ModifierExecTestImpl;
use Common::ChunkDescription;

#use UserInfo::Modifier;
#use ExpressionMatcher::Modifier;

my $usage =
  "Usage: $0 <command> <command params> [options]\n".
  "Commands: \n".
  "  'check' check that chunks require redistribution (dry running of reconf).\n".
  "  'reconf' create, move chunks without chunk existing requirements\n".
  "    params:\n".
  "      --chunks-count\n".
  "      --target-hosts\n".
  "      [--check-hosts]\n".
  "      [--chunks-root]\n".
  "      [--environment]\n".
  "Common Options:\n".
  "  [--chunks-prefix] - chunks directory name, must be ommitted for UIM chunks \n".
  "  [--transport] - transport type (ssh by default):\n".
  "    test:<test folder>\n".
  "    ssh:<ssh identity>\n".
  "    rsync:<rsync path>:<rsync port>\n".
  "  [--print] - print progress information while perform slow operations]\n".
  "  [-v] - print verbose information\n";

my $test_mode = 0;
my $test_folder_path = "./cache";
my $print = 0;
my $verbose = 0;
my $chunk_modificator;

sub reconfigure_chunks;

package LegalException;

use overload ('""' => 'stringify');

sub new
{
  my ($class, $msg) = @_;
  my $self = { MSG_ => $msg };
  bless($self, $class);
  return $self;  
}

sub stringify
{
  my ($this) = @_;
  return $this->{MSG_};
}

package Logger;

sub new
{
  my ($class, $out, $verbose) = @_;
  my $self = { verbose_ => $verbose, verbose_out_ => $out };
  bless($self, $class);
  return $self;  
}

sub log_prefix
{
  my ($level) = @_;
  my $now = [ Time::HiRes::gettimeofday() ];
  return POSIX::strftime(
    "%a %e %b %Y %H:%M:%S", gmtime($now->[0])) .
    ":" . sprintf("%06d", $now->[1]) . " [] [$level] [ConfigureChunks] : ";
}

sub trace
{
  my ($this, @messages) = @_;
  if($this->{verbose_})
  {
    my $now = [ Time::HiRes::gettimeofday() ];
    print {$this->{verbose_out_}} Logger::log_prefix("TRACE");
    print {$this->{verbose_out_}} @messages;
    print {$this->{verbose_out_}} "\n";
  }
}

# UserChunkSet class
package UserChunkSet;

use POSIX qw(ceil floor);

# sub new
# sub add
# sub use_hosts
# sub supply
# sub distribute_full_hosts
# sub fill_incomplete_hosts
# sub remove_all
# sub missed_chunks
# sub duplicated_chunks
# sub change_chunks_number

# sub as_string
# sub as_xml

# sub unref_
# sub move_
# sub host_is_full_
# sub find_min_loaded_host_
# sub find_max_loaded_host_

sub new
{
  my ($class, $modifier, $host_array_ref, $chunks_number) = @_;

  my $self = {
    modifier_ => $modifier,
    chunks_number_ => $chunks_number,
    print_ => $print };

  bless($self, $class);
  
  $self->collect_chunks_info_($host_array_ref);
  
  return $self;
}

sub add
{
  my ($this, $chunks) = @_;

  my $ret = 0;

  foreach my $chunk(@$chunks)
  {
    if(exists($this->{chunks_}->{$chunk->index()}))
    {
      $this->{modifier_}->move(
        $chunk,
        $this->{chunks_}->{$chunk->index()}->[0]->host());
    }
    else
    {
      my $min_loaded_host = $this->find_min_loaded_host_();
      $this->move_($chunk, $min_loaded_host);
    }

    ++$ret;
  }

  return $ret;
}

sub use_hosts
{
  my ($this, $host_array_ref) = @_;
  my @res;
  my %new_hosts = map { $_ => 1 } @$host_array_ref;
  my %host_chunks_copy = %{$this->{host_chunks_}};

  while(my ($host, $chunks) = each(%host_chunks_copy))
  {
    if(!exists($new_hosts{$host}))
    {
      my %host_chunks = %{$this->{host_chunks_}->{$host}};
      while(my ($chunk_index, $chunk) = each(%host_chunks))
      {
        push(@res, $this->unref_($chunk));
      }
      delete $this->{host_chunks_}->{$host};
    }
  }

  $this->{max_chunks_per_host_} = ceil(
    $this->{chunks_number_} / keys(%{$this->{host_chunks_}}));
  $this->{min_chunks_per_host_} = floor(
    $this->{chunks_number_} / keys(%{$this->{host_chunks_}}));
  return \@res;
}

sub remove_all
{
  my ($this) = @_;

  while(my ($chunk_index, $chunks) = each(%{$this->{chunks_}}))
  {
    foreach my $chunk(@$chunks)
    {
      $this->{modifier_}->remove($chunk);
    }
  }

  while(my ($host) = each(%{$this->{host_chunks_}}))
  {
    $this->{host_chunks_}->{$host} = {};
  }

  $this->{chunks_} = {};
}

sub missed_chunks
{
  my ($this) = @_;

  my @res;

  for(my $chunk_index = 0; $chunk_index < $this->{chunks_number_}; ++$chunk_index)
  {
    if(!exists($this->{chunks_}->{$chunk_index}))
    {
      push(@res, $chunk_index);
    }
  }

  return sort(@res);
}

sub duplicated_chunks
{
  my ($this) = @_;

  my %res;

  while(my ($chunk_index, $chunks) = each(%{$this->{chunks_}}))
  {
    if(@$chunks > 1)
    {
      $res{$chunk_index} = $chunks;
    }
  }

  while(my ($chunk_index, $chunks) = each(%{$this->{unfinished_chunks_}}))
  {
    if(@$chunks > 1)
    {
      $res{$chunk_index} = $chunks;
    }
  }

  return \%res;
}

sub supply
{
  my ($this) = @_;

  my $ret = 0;
  for(my $chunk_index = 0; $chunk_index < $this->{chunks_number_}; ++$chunk_index)
  {
    if(!exists($this->{chunks_}->{$chunk_index}))
    {
      my $min_loaded_host = $this->find_min_loaded_host_();
      my $chunk = new ChunkDescription(index => $chunk_index, host => $min_loaded_host);
      $this->{modifier_}->create($chunk);
      $this->{host_chunks_}->{$min_loaded_host}->{$chunk_index} = $chunk;
      $this->{chunks_}->{$chunk_index} = [ $chunk ];
      ++$ret;
    }
    else
    {
      my $chunks_array = $this->{chunks_}->{$chunk_index};
      if(@$chunks_array > 1)
      {
        die "UserChunkSet::supply: chunk #$chunk_index duplicated:\n" .
          $this->state_as_string("  ");
      }
      # chunk recreation - no changes in dry run mode (only if migration done)
      $ret += $this->{modifier_}->create($chunks_array->[0]);
    }
  }

  return $ret;
}

sub state_as_string
{
  my ($this, $offset_val) = @_;
  my $offset = defined $offset_val ? $offset_val : '';
  return $offset . "UserChunkSet state:\n" .
    $offset . "  chunks-number: " . $this->{chunks_number_} . "\n" .
    $offset . "  max-chunks-per-host: " . $this->{max_chunks_per_host_} . "\n" .
    $offset . "  min-chunks-per-host: " . $this->{min_chunks_per_host_} . "\n" .
    $offset . "  chunks:\n" . $this->as_string($offset . '    ');
}

sub as_string
{
  my ($this, $offset_val) = @_;
  my $res = '';
  my $offset = defined $offset_val ? $offset_val : '';

  while(my ($host, $chunks) = each(%{$this->{host_chunks_}}))
  {
    $res .= "$offset$host(" . keys(%$chunks) . "): \n";

    foreach my $chunk_index(sort keys %$chunks)
    {
      $res .= "$offset  $chunk_index (" .
        "version : " . $chunks->{$chunk_index}->version() .
        ", total_chunks : " . $chunks->{$chunk_index}->total_chunks() .
        ") => " . $chunks->{$chunk_index}->path() . "\n";
    }
  }

  $res .= $offset . "Unmerged chunks: \n";
  while(my ($chunk_index, $chunk) = each(%{$this->{unmerged_chunks_}}))
  {
    $res .= "$offset  Chunk # " . $chunk_index . ": => " .
      join(", ", @{$chunk->hosts()}) . ":" . $chunk->path() . "\n";
  }

  $res .= $offset . "Unfinished chunks: \n";
  while(my ($chunk_index, $chunks) = each(%{$this->{unfinished_chunks_}}))
  {
    foreach my $chunk(@$chunks)
    {
      $res .= "$offset  " . $chunk->index(). " (" . $chunk->version() . ") => " .
        $chunk->host() . ":" . $chunk->path() . "\n";
    }
  }  

  return $res;
}

sub as_xml
{
  my ($this) = @_;

  my $res = <<END;
<?xml version="1.0" encoding="utf-8"?>
<cfg:DistributionIndexFile
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:colo="http://www.foros.com/cms/colocation">
  <cfg:hosts index_limit="$this->{chunks_number_}">
END

  while(my ($host, $chunks) = each(%{$this->{host_chunks_}}))
  {
    $res .= "    <cfg:host name=\"$host\">\n";
    foreach my $chunk_index(sort keys %$chunks)
    {
      $res .= "      <cfg:index value=\"$chunk_index\"/>\n";
    }
    $res .= "    </cfg:host>\n";
  }
  $res .= <<END;
  </cfg:hosts>
</cfg:DistributionIndexFile>
END

  return $res;
}

sub distribute_full_hosts
{
  my ($this) = @_;

  my $ret = 0;
  my %host_chunks_copy = %{$this->{host_chunks_}};

  while(my ($host, $chunks) = each(%host_chunks_copy))
  {
    if($this->host_is_full_($host, $chunks))
    {
      my $move_chunks_count = scalar(keys(%$chunks)) - $this->{max_chunks_per_host_};

      while((my ($chunk_index, $chunk) = each(%$chunks)) &&
        $move_chunks_count > 0)
      {
        my $min_loaded_host = $this->find_min_loaded_host_();
        $this->move_($chunk, $min_loaded_host);
        ++$ret;
        --$move_chunks_count;
      }
    }
  }

  return $ret;
}

sub fill_incomplete_hosts
{
  my ($this) = @_;

  my $ret = 0;
  my %host_chunks_copy = %{$this->{host_chunks_}};

  while(my ($host, $chunks) = each(%host_chunks_copy))
  {
    if($this->host_is_incomplete_($host, $chunks))
    {
      my $move_chunks_count = $this->{min_chunks_per_host_} - scalar(keys(%$chunks));

      for(my $i = 0; $i < $move_chunks_count; ++$i)
      {
        my $max_loaded_host = $this->find_max_loaded_host_();
        my $max_loaded_host_chunks = $this->{host_chunks_}->{$max_loaded_host};
        if(scalar(keys(%$max_loaded_host_chunks)) <= scalar(keys(%$chunks)))
        {
          last;
        }
        my ($chunk_index, $chunk) = each(%$max_loaded_host_chunks);
        $this->move_($chunk, $host);
        ++$ret;
        --$move_chunks_count;
      }
    }
  }

  return $ret;
}

sub is_redistribution_required
{
  my ($this) = @_;
  my $prev_chunks_number = -1;
  my $redistribution_required = 0;
  my $wrong_chunks_number = 0;
  my $err = 'Different chunks distribution: ';
  while(my ($chunk_index, $chunks) = each(%{$this->{chunks_}}))
  {
    foreach my $chunk(@$chunks)
    {
      if(!defined($chunk->total_chunks()))
      {
        $redistribution_required = 1;
        if($prev_chunks_number < 0)
        {
          $prev_chunks_number = 0;
        }
        next;
      }

      if($chunk->total_chunks() == $this->{chunks_number_})
      {
        next;
      }

      $redistribution_required = 1;

      if($prev_chunks_number < 0)
      {
        $prev_chunks_number = $chunk->total_chunks();
      }
      elsif($chunk->total_chunks() != $prev_chunks_number)
      {
        $err .= $chunk->host() . ' - chunk# ' . $chunk->index() .
          ', chunks num - ' . $chunk->total_chunks() . '; ';
        $wrong_chunks_number = 1;
      }
    }
  }

  if(%{$this->{unfinished_chunks_}} || %{$this->{unmerged_chunks_}})
  {
    $redistribution_required = 1;
    while(my ($chunk_index, $unfinished_chunks) = each(%{$this->{unfinished_chunks_}}))
    {
      foreach my $unfinished_chunk(@$unfinished_chunks)
      {
        if($unfinished_chunk->total_chunks() != $this->{chunks_number_})
        {
          $err .= $unfinished_chunk->host() . ' - chunk# ' . $unfinished_chunk->index() .
            ', chunks num - ' . $unfinished_chunk->total_chunks() . '; ';
          $wrong_chunks_number = 1;
        }
      }
    }

    while(my ($chunk_index, $unmerged_chunk) = each(%{$this->{unmerged_chunks_}}))
    {
      if($unmerged_chunk->total_chunks() != $this->{chunks_number_})
      {
        $err .= 'unmerged chunk# ' . $unmerged_chunk->index() .
          ', chunks num - ' . $unmerged_chunk->total_chunks() . '; ';
        $wrong_chunks_number = 1;
      }
    }
  }

  if($wrong_chunks_number)
  {
    die LegalException->new($err);
  }

  return $redistribution_required;
}

sub divide_chunks
{
  my ($this) = @_;

  # process chunks that already divided
  while(my ($chunk_index, $unfinished_chunks) = each(%{$this->{unfinished_chunks_}}))
  {
    foreach my $unfinished_chunk(@$unfinished_chunks)
    {
      if(exists($this->{chunks_}->{$unfinished_chunk->index()}))
      {
        my $chunk = @{$this->{chunks_}->{$unfinished_chunk->index()}}[0];
        $this->unref_($chunk);
        $this->{modifier_}->remove($chunk);
      }
      $this->{modifier_}->divide_chunk($unfinished_chunk);
    }
  }

  # divide chunks that have unexpected divider (total_chunks)
  while(my ($chunk_index, $chunks) = each(%{$this->{chunks_}}))
  {
    foreach my $chunk(@$chunks)
    {
      if(!defined($chunk->total_chunks()) ||
         $chunk->total_chunks() != $this->{chunks_number_})
      {
        $this->{modifier_}->divide_chunk($chunk);
      }
    }
  }

  # recollect chunks after dividing
  my @hosts = keys(%{$this->{host_chunks_}});
  $this->collect_chunks_info_(\@hosts);
}

sub merge_chunks
{
  my ($this, $host_array_ref) = @_;
  
  if(!%{$this->{unmerged_chunks_}})
  {
    return 0;
  }

  my $ret = 0;

  my %new_hosts = map { $_ => 1 } @$host_array_ref;

  my %unmerged_chunks = %{$this->{unmerged_chunks_}};
  while(my ($chunk_index, $unmerged_chunk) = each(%{$this->{unmerged_chunks_}}))
  {
    if(exists($this->{chunks_}->{$chunk_index}))
    {
      $this->{modifier_}->merge_chunk(
        $this->{chunks_}->{$chunk_index}->[0]->host(), $unmerged_chunk);
      delete $this->{unmerged_chunks_}->{$chunk_index};
      ++$ret;
    }
  }

  my @chunks_to_merge = sort {
      scalar(@{$b->hosts()}) <=> scalar(@{$a->hosts()}) ||
      $b->index() <=> $a->index() 
    } values(%{$this->{unmerged_chunks_}});

  for (my $i = $#chunks_to_merge; $i > -1; $i--)
  {
    my $chunk_to_merge = $chunks_to_merge[$i];
    my $merged = 0;
    foreach my $host(@{$chunk_to_merge->hosts()})
    {
      $merged = 0;

      if(!exists($new_hosts{$host}) ||
         $this->host_is_full_($host))
      {
        next;
      }
  
      my $new_chunk_ref = $this->{modifier_}->merge_chunk($host, $chunk_to_merge);

      push(@{$this->{chunks_}->{$new_chunk_ref->index()}}, $new_chunk_ref);
      $this->{host_chunks_}->{$host}->{$new_chunk_ref->index()} = $new_chunk_ref;

      $merged = 1;
      last;
    }

    if($merged)
    {
      splice(@chunks_to_merge, $i, 1);
      ++$ret;
      next;
    }
  }

  foreach my $chunk_to_merge(@chunks_to_merge)
  {
    my $min_loaded_host = $this->find_min_loaded_host_();

    my $new_chunk_ref = $this->{modifier_}->merge_chunk($min_loaded_host, $chunk_to_merge);

    push(@{$this->{chunks_}->{$new_chunk_ref->index()}}, $new_chunk_ref);
    $this->{host_chunks_}->{$min_loaded_host}->{$new_chunk_ref->index()} = $new_chunk_ref;
    ++$ret;
  }

  $this->{unmerged_chunks_} = {};
  return $ret; 
}

sub unref_
{
  my ($this, $chunk) = @_;
  delete $this->{host_chunks_}->{$chunk->host()}->{$chunk->index()};
  my $i = 0;
  my $chunks_array = $this->{chunks_}->{$chunk->index()};
  foreach my $delete_chunk(@$chunks_array)
  {
    if($delete_chunk->host() eq $chunk->host())
    {
      splice(@$chunks_array, $i, 1);
      last;
    }
    ++$i;
  }
  if(scalar(@{$this->{chunks_}->{$chunk->index()}}) == 0)
  {
    delete $this->{chunks_}->{$chunk->index()};
  }
  return $chunk;
}

sub move_
{
  my ($this, $chunk, $target_host) = @_;

  if(exists($this->{host_chunks_}->{$chunk->host()}) &&
     exists($this->{host_chunks_}->{$chunk->host()}->{$chunk->index()}))
  {
    # owned chunk
    delete $this->{host_chunks_}->{$chunk->host()}->{$chunk->index()};
  }
  else
  {
    push(@{$this->{chunks_}->{$chunk->index()}}, $chunk);
  }
  $this->{modifier_}->move($chunk, $target_host);
  $this->{host_chunks_}->{$chunk->host()}->{$chunk->index()} = $chunk;
}

sub host_is_full_
{
  my ($this, $host, $chunks) = @_;

  if(!defined($chunks))
  {
    return keys %{ $this->{host_chunks_}->{$host} } >= $this->{max_chunks_per_host_};
  }

  return keys %$chunks >= $this->{max_chunks_per_host_};
}

sub host_is_incomplete_
{
  my ($this, $host, $chunks) = @_;

  if(!defined($chunks))
  {
    return keys %{ $this->{host_chunks_}->{$host} } < $this->{min_chunks_per_host_};
  }

  return keys %$chunks < $this->{min_chunks_per_host_};
}

sub find_min_loaded_host_
{
  my ($this) = @_;
  my $min_chunks = undef;
  my $host_with_min_chunks = undef;
  while(my ($host, $chunks) = each(%{$this->{host_chunks_}}))
  {
    my $chunks_size = scalar(keys(%$chunks));
    if(!defined($min_chunks) ||
       $chunks_size < $min_chunks ||
       $chunks_size == $min_chunks && $host lt $host_with_min_chunks
         # select host with minimal host name (lexicaly) for determined behavior
       )
    {
      $min_chunks = scalar(keys(%$chunks));
      $host_with_min_chunks = $host;
    }
  }
  return $host_with_min_chunks;
}

sub find_max_loaded_host_
{
  my ($this) = @_;
  my $max_chunks = undef;
  my $host_with_max_chunks = undef;
  while(my ($host, $chunks) = each(%{$this->{host_chunks_}}))
  {
    my $chunks_size = scalar(keys(%$chunks));
    if(!defined($max_chunks) ||
       $chunks_size > $max_chunks ||
       $chunks_size == $max_chunks && $host lt $host_with_max_chunks)
    {
      $max_chunks = scalar(keys(%$chunks));
      $host_with_max_chunks = $host;
    }
  }
  return $host_with_max_chunks;
}

sub collect_chunks_info_
{
  my ($this, $host_array_ref) = @_;

  my %host_chunks;
  my %chunks;
  my %unmerged_chunks;
  my %unfinished_chunks;

  foreach my $host(@$host_array_ref)
  {
    if(!exists($host_chunks{$host}))
    {
      my ($host_chunks_ref, $host_unmerged_chunks_ref, $host_unfinished_chunks_ref) =
        $this->{modifier_}->exists_chunks($host);

      $host_chunks{$host} = $host_chunks_ref;
      while(my ($chunk_index, $chunk) = each(%$host_chunks_ref))
      {
        if(exists($chunks{$chunk_index}))
        {
          push(@{$chunks{$chunk_index}}, $chunk);
        }
        else
        {
          $chunks{$chunk_index} = [ $chunk ];
        }
      }

      while(my ($chunk_index, $unmerged_chunk) = each(%$host_unmerged_chunks_ref))
      {
        if(exists($unmerged_chunks{$chunk_index}))
        {
          push(@{$unmerged_chunks{$chunk_index}->hosts()}, @{$unmerged_chunk->hosts()}[0]);
        }
        else
        {
          $unmerged_chunks{$chunk_index} = $unmerged_chunk;
        }
      }

      while(my ($chunk_index, $unfinished_chunk) = each(%$host_unfinished_chunks_ref))
      {
        if(exists($unfinished_chunks{$chunk_index}))
        {
          push(@{$unfinished_chunks{$chunk_index}}, $unfinished_chunk);
        }
        else
        {
          $unfinished_chunks{$chunk_index} = [ $unfinished_chunk ];
        }
      }
    }
  }

  $this->{host_chunks_} = \%host_chunks;
  $this->{chunks_} = \%chunks;
  $this->{unmerged_chunks_} = \%unmerged_chunks;
  $this->{unfinished_chunks_} = \%unfinished_chunks;
  $this->{max_chunks_per_host_} = ceil($this->{chunks_number_} / keys(%host_chunks));
}

package main;

use Args;

if(@ARGV < 1)
{
  die $usage . "\nError: command not defined.";
}

eval
{
  my $command = $ARGV[0];
  my $args = Args::parse(@ARGV); # CHANGE

  $verbose = exists $args->{'v'} ? 1 : 0;
  my $chunks_root = exists $args->{'chunks-root'} ? $args->{'chunks-root'} :
    "/opt/foros/server/var/cache";
  my @additional_chunk_roots = exists $args->{'add-chunks-root'} ?
    split(':', $args->{'add-chunks-root'}) : ();

  my $chunks_count = $args->{'chunks-count'};
  my @target_hosts = split(/[\s,;]/, $args->{'target-hosts'});
  my @check_hosts = exists $args->{'check-hosts'} && defined $args->{'check-hosts'} ?
    split(/[\s,;]/, $args->{'check-hosts'}) : ();
  my $force = exists $args->{'force'} &&
    (!defined $args->{'force'} || $args->{'force'} != "0") ?  1 : 0;
  my $ssh_identity = $ENV{'adserver_ssh_identity'};
  $print = exists $args->{'print'} ? 1 : 0;

  my $environment_cmd = (defined $args->{'environment'}) ? ". " . $args->{'environment'} . " && " : "";

  if(!defined $args->{'modifier'})
  {
    die "modifier package is not specified";
  }
  
  my $modifier_module = $args->{'modifier'};
  
  if(exists $args->{'transport'})
  {
    my $transport = $args->{'transport'};
    if($transport =~ m/^test:(.*)$/)
    {
      $test_mode = 1;
      $test_folder_path = $1;
    }
    elsif($transport =~ m/^ssh:(.*)$/)
    {
      $ssh_identity = $1;
    }
    else
    {
      die "incorrect transport value '$transport'. See usage";
    }
  }

  if(!defined($chunks_count) || $chunks_count eq 0)
  {
    die "chunks_count defined as zero.";
  }

  if(scalar @target_hosts eq 0)
  {
    die "chunk hosts is empty.";
  }

  my $logger = new Logger(*STDOUT, $verbose);

  {
    my $modifier_exec_impl;

    if($test_mode)
    {
      $modifier_exec_impl = new Common::ModifierExecTestImpl(
        $chunks_root,
        $test_folder_path);
    }
    else
    {
      if (!defined($ssh_identity))
      {
        die "error: adserver_ssh_identity env variable".
          " is not defined and -ssh param is not set.";
      }

      $modifier_exec_impl = new Common::ModifierExecImpl(
        $chunks_root, $ssh_identity);
    }

    eval "require $modifier_module";

    if ($@)
    {
      die "can't open modifier module '$modifier_module': $@";
    }

    $chunk_modificator = $modifier_module->new(
      $modifier_exec_impl,
      $logger,
      $verbose,
      $command eq 'check' ? 1 : 0, # 0 - means dry run execution
      $chunks_count,
      $environment_cmd
      );
  }

  $logger->trace("To $command with params:\n".
    "  chunks-count = $chunks_count\n".
    "  target-hosts: ", join(', ', @target_hosts),
    "\n  check-hosts: ", join(', ', @check_hosts));

  my $xml = '';

  if($command eq 'check')
  {
    if(reconfigure_chunks(
      $logger,
      $chunks_count,
      [ @check_hosts, @target_hosts ],
      \@target_hosts,
      $force,
      1,
      \$xml) > 0)
    {
#     print "redistribution required\n";
      exit 1;
    }

#   print "redistribution isn't required\n";
  }
  elsif($command eq 'reconf')
  {
    reconfigure_chunks(
      $logger,
      $chunks_count,
      [ @check_hosts, @target_hosts ],
      \@target_hosts,
      $force);
  }
  else
  {
    die "Unknown command '$command'\n";
  }

  if(defined($args->{'xml-out'}))
  {
    if($args->{'xml-out'} eq '-')
    {
      print STDOUT $xml;
    }
    else
    {
      open XML_FILE, "> " . $args->{'xml-out'};
      print XML_FILE $xml;
      close XML_FILE;
    }
  }    
}; # eval

if($@)
{
  if ($@->isa('LegalException'))
  {
    print STDERR $@;
    exit 1;
  }
  else
  {
    my $err = $@;
    print STDERR Logger::log_prefix('ERROR') . $err . "\n";
    exit -1;
  }
}

0;

sub packed_seq_string
{
  my ($seq) = @_;
  my $seq_first_val = undef;
  my $seq_last_val = undef;
  my $res = '';
  foreach my $i(@$seq)
  {
    if(defined $seq_first_val)
    {
      if($i != $seq_last_val + 1)
      {
        # close sequence
        if($seq_first_val == $seq_last_val)
        {
          $res = $res . (length($res) > 0 ? ',' : '') . $seq_first_val;
        }
        else
        {
          $res = $res . (length($res) > 0 ? ',' : '') . $seq_first_val .
            ($seq_last_val - $seq_first_val > 1 ? '-' : ',') . $seq_last_val;
        }
        $seq_first_val = $i;
      }
    }
    else
    {
      $seq_first_val = $i;
    }

    $seq_last_val = $i;
  }

  if($seq_first_val == $seq_last_val)
  {
    $res = $res . (length($res) > 0 ? ',' : '') . $seq_first_val;
  }
  else
  {
    $res = $res . (length($res) > 0 ? ',' : '') . $seq_first_val .
      ($seq_last_val - $seq_first_val > 1 ? '-' : ',') . $seq_last_val;
  }

  return "$res";
}

#####
# reconfigure_chunks is facade for reconfiguration chunks distribution.
# args:
#   logger
#   new-common-chunks-number
#   old-hosts - array of hosts that contain chunk files.
#   new-hosts - array of hosts that will contain chunk files.
#####
sub reconfigure_chunks
{
  my ($logger,
    $new_chunks_number,
    $old_hosts_ref,
    $new_hosts_ref,
    $force,
    $dry_run,
    $xml_out) = @_;

  my $ret = 0;

  my @all_hosts = keys %{{map {($_ => 1)} (@$old_hosts_ref, @$new_hosts_ref)}};

  $logger->trace("To check exist chunks at all hosts : @$old_hosts_ref...");
  my $chunk_set = new UserChunkSet(
    $chunk_modificator,
    [ @$old_hosts_ref, @$new_hosts_ref ],
    $new_chunks_number,
    $logger);

  $logger->trace("Found chunks:\n", $chunk_set->as_string("  "));

  # check missed chunks
  {
    my @missed_chunks = $chunk_set->missed_chunks();
    if((defined $dry_run && $dry_run > 0 ? 1 : 0) != 1 &&
         scalar @missed_chunks > 0 && $force == 0)
    {
      die LegalException->new("Missed chunks: " . packed_seq_string(\@missed_chunks) .
        ". Use 'force' option for create new chunks");
    }
  }

  # check duplicate chunks
  {
    my $dup_chunks = $chunk_set->duplicated_chunks();

    if(scalar keys(%$dup_chunks) > 0)
    {
      my $err = 'Duplicated chunks: ';
      my $first = 1;
      foreach my $chunk_index(sort(keys(%$dup_chunks)))
      {
        $err .= (defined($first) ? "" : ", ") . "chunk #" .
          $chunk_index . " present at";
        my $chunks = $dup_chunks->{$chunk_index};
        foreach my $chunk(@$chunks)
        {
          $err .= " " . $chunk->host();
        }
        $first = undef;
      }
      die LegalException->new($err);
    }
  }

  if($chunk_set->is_redistribution_required())
  {
    if((defined $dry_run && $dry_run > 0 ? 1 : 0) != 1 && $force == 0)
    {
      die LegalException->new("Number of chunks is changed. " .
        "Use 'force' option for create new chunks");
    }

    $logger->trace("Dividing chunks...");
    $chunk_set->divide_chunks();
    $logger->trace("Found chunks:\n", $chunk_set->as_string("  "));
  }

  $logger->trace("To collect chunks at removed hosts...");
  my $chunks_from_removed_hosts = $chunk_set->use_hosts($new_hosts_ref);
  $logger->trace("From collect chunks at removed hosts...");

  $logger->trace("Merging chunks...");
  my $local_ret = $chunk_set->merge_chunks($new_hosts_ref);
  $logger->trace("Merging chunks: chunks are ",
    ($local_ret > 0 ? "" : "not "),
    "modified");
  $ret += $local_ret;

  $logger->trace("To move chunks from removed hosts...");
  $local_ret = $chunk_set->add($chunks_from_removed_hosts);
  $logger->trace("From move chunks from removed hosts, chunks ",
    ($local_ret > 0 ? "" : "not "),
    "modified");
  $ret += $local_ret;

  $logger->trace("To create missing chunks:\n", $chunk_set->as_string("  "));
  $local_ret = $chunk_set->supply();
  $logger->trace("From create missing chunks: ",
    ($local_ret > 0 ? "" : "not "),
    "modified");
  $ret += $local_ret;

  $logger->trace("To redistribute chunks from full hosts:\n", $chunk_set->state_as_string("  "));
  $local_ret = $chunk_set->distribute_full_hosts();
  $logger->trace("From redistribute chunks from full hosts: ",
    ($local_ret > 0 ? "" : "not "),
    "modified");
  $ret += $local_ret;

  $logger->trace("To fill chunks at incomplete hosts:\n", $chunk_set->state_as_string("  "));
  $local_ret = $chunk_set->fill_incomplete_hosts();
  $logger->trace("From fill chunks at incomplete hosts: ",
    ($local_ret > 0 ? "" : "not "),
    "modified");
  $ret += $local_ret;

  if(defined($xml_out))
  {
    $$xml_out = $chunk_set->as_xml();
  }

  $logger->trace("Result chunks (",
    ($ret > 0 ? "" : "not "), "modified):\n", $chunk_set->as_string("  "));

  return $ret;
}

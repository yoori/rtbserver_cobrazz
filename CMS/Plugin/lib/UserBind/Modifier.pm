package UserBind::Modifier;

use warnings;
use strict;
use Exporter;
use Common::ChunkDescription;

use vars qw ( $VERSION @ISA @EXPORT );

@ISA = qw(Exporter);
$VERSION = '1.00';
@EXPORT = qw(new exists_chunks create move remove divide_chunk merge_chunk);

# exists_chunks(host): return exist at host chunk set : %{ index => \ChunkDescription }
#   detect old format chunks - its migration must be done with using create
sub exists_chunks;

# create(host, index): migrate old format chunk if exists or create it
sub create;

# move(src_host, dst_host, index)
sub move;

# remove(host, index)
sub remove;

# divide_chunk(chunk)
sub divide_chunk;

# merge_chunk(host, chunk)
sub merge_chunk;

use constant CHUNK_PREFIX => 'Chunk';
use constant TO_MERGE_CHUNK_PREFIX => CHUNK_PREFIX . "_to_merge";
use constant MERGED_CHUNK_PREFIX => CHUNK_PREFIX . "_merged";
use constant DIVIDED_CHUNK_PREFIX => CHUNK_PREFIX . "_divided";
use constant TEMP_CHUNK_PREFIX => CHUNK_PREFIX . "_temp";

## Constructor
#  prefixes : ({ add => add_prefix, base => base_prefix, ...})
#  exec_impl : ModifierExec implementation:
#    ModifierExec(all pathes relative chunks root):
#      list($host, $path, $recursive)
#      remove($host, $path)
#      move($src_host, $src_path, $tgt_host, $tgt_path)
#      mkdir($host, $path)
#
sub new
{
  my (
    $class,
    $exec_impl,
    $logger,
    $verbose,
    $dry_run,
    $chunks_number,
    $environment_cmd) = @_;

  my $this = {
    exec_impl_ => $exec_impl,
    logger_ => $logger,
    verbose_ => $verbose,
    dry_run_ => defined $dry_run && $dry_run > 0 ? 1 : undef,
    chunks_number_ => $chunks_number
    };

  # command that divide existing chunk
  # <source folder with file prefix> <destination folder with chunk prefix>
  $this->{chunks_divide_cmd_} = $environment_cmd .
    "UserBindUtil --chunks-number=" . $chunks_number . " divide-to-chunks";

  # command that merge two chunks
  # <destination folder with chunk prefix> <source folders with prefixes>
  $this->{chunks_merge_cmd_} = $environment_cmd . "UserBindUtil merge";

  bless($this, $class);
  return $this;
}

# check exists chunks - return array of chunk indexes for specified host
sub exists_chunks
{
  my ($this, $host) = @_;
  my %chunks;
  my %unmerged_chunks;
  my %unfinished_chunks;

  # do non recursive list
  my @chunk_folders = $this->{exec_impl_}->list($host, "", 0);
  my %ignore_old_chunks;

  #print "chunk_folders: " . join(",", @chunk_folders) . "\n";

  # chunks checking for 3.4 format:
  #   <root>/(<CHUNK_PREFIX>_\d_\d)
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[CHUNK_PREFIX]}_(\d+)_(\d+)$|)
    {
      $chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => $folder,
        version => '3.4');
    }
    elsif($folder =~ m|^@{[TO_MERGE_CHUNK_PREFIX]}_(\d+)_(\d+)$| ||
      $folder =~ m|^@{[MERGED_CHUNK_PREFIX]}_(\d+)_(\d+)$|)
    {
      if(!exists($unmerged_chunks{$1}))
      {
        $unmerged_chunks{$1} = new ChunkDescription(
          index => $1,
          total_chunks => $2,
          hosts => [$host],
          path => $folder,
          version => '3.4');
      }
    }
    elsif($folder =~ m|^@{[DIVIDED_CHUNK_PREFIX]}_(\d+)_(\d+)$|)
    {
      $unfinished_chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => $folder,
        version => '3.4',
        divided => 1);
    }
  }

  return (\%chunks, \%unmerged_chunks, \%unfinished_chunks);
}

# create chunk files
sub create
{
  my ($this, $chunk, $dry_run) = @_;

  my $dst_host = $chunk->host();

  # create required dirs
  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: creating chunk #" . $chunk->index() . " at '$dst_host'");
  }

  # convert old chunk if required
  my $res = $this->adapt_chunk_($chunk);

  if(!defined($chunk->total_chunks()))
  {
    $chunk->total_chunks($this->{chunks_number_});
  }
  if(!defined($chunk->path()))
  {
    $chunk->path($this->get_chunk_path_($chunk->index()));
  }

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->mkdir(
      $dst_host,
      $chunk->path()) &&
      die "Modifier: Can't create $dst_host:" . $chunk->path();
  }

  $chunk->version('3.4');

  return $res;
}

# move chunk files
sub move
{
  my ($this, $chunk, $dst_host) = @_;

  # convert old chunk if required
  $this->adapt_chunk_($chunk);

  my $src_host = $chunk->host();
  my $chunk_root = $chunk->path();

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to move chunk #" .
      $chunk->index() . " from '$src_host' to '$dst_host': " . $chunk_root);
  }

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->move(
      $src_host,
      $chunk_root,
      $dst_host,
      $chunk_root) &&
      die "Modifier: Can't move $src_host:$chunk_root to $dst_host:$chunk_root";
  }

  $chunk->host($dst_host);

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: chunk #" . $chunk->index() .
      " moved from '$src_host' to '$dst_host'");
  }
}

# remove chunk files
sub remove
{
  my ($this, $chunk) = @_;

  # convert old chunk if required
  $this->adapt_chunk_($chunk);

  my $dst_host = $chunk->host();

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to remove chunk #" .
      $chunk->index() . " from '$dst_host'(move into _)");
  }

  my $chunk_root = $chunk->path();
  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->move(
      $dst_host,
      $chunk_root,
      $dst_host,
      $chunk_root . "_") &&
      die "Modifier: Can't move $dst_host:$chunk_root to $dst_host:$chunk_root\_";
  }
}

# divides chunk due to new distribution 
sub divide_chunk
{
  my ($this, $chunk) = @_;

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Redistribution: dividing chunk #" . $chunk->index() .
      " on the host " . $chunk->host() . " to " .
      $this->{chunks_number_} . " chunks");
  }

  if(defined $this->{dry_run_})
  {
    return;
  }

  my $divided_chunk_root = DIVIDED_CHUNK_PREFIX . "_" .
    $chunk->index() . "_" . $this->{chunks_number_} . "/";

  # dividing chunk into temp directory
  if(!defined($chunk->divided()))
  {
    $this->adapt_chunk_($chunk);

    my $temp_chunk_root = TEMP_CHUNK_PREFIX . "_" . $chunk->index() . "/";

    $this->{exec_impl_}->remove($chunk->host(), $temp_chunk_root, 1);
    $this->{exec_impl_}->mkdir($chunk->host(), $temp_chunk_root);

    my @chunk_files = $this->{exec_impl_}->list($chunk->host(), $chunk->path());

    my %processed_prefixes;
    foreach my $file(@chunk_files)
    {
      #print "divide_chunk: file '$file'\n";

      if($file !~ m|^([^.]*)[.].*$|)
      {
        die "Can't parse file name for prefix : '$file'";
      }

      my $prefix = $1;

      if(exists($processed_prefixes{$prefix}))
      {
        next;
      }

      my $opt_hash_type;
      if($prefix eq 'UserSeen')
      {
        $opt_hash_type = '--hash-type=value ';
      }
      elsif($prefix eq 'UserBind')
      {
        $opt_hash_type = '--hash-type=string ';
      }
      else
      {
        die "Unknown file prefix '$prefix'";
      }

      # use empty folder prefix for result chunks
      $this->{exec_impl_}->execute_command(
        $chunk->host(),
        $this->{chunks_divide_cmd_},
        $opt_hash_type,
        Common::ModifierExec::path_wrapper($chunk->path() . "/" . $1),
        Common::ModifierExec::path_wrapper($temp_chunk_root . "/")) &&
        die "Modifier: Can't divide chunk: " . $chunk->path();

      $processed_prefixes{$1} = 1;
    }

    # TO FIX: divided chunk can be lost !!!
    $this->{exec_impl_}->remove($chunk->host(), $divided_chunk_root, 1);

    $this->{exec_impl_}->move(
      $chunk->host(),
      $temp_chunk_root,
      $chunk->host(),
      $divided_chunk_root) &&
      die "Modifier: Can't move temporary chunk to divided: '$temp_chunk_root' " .
        "to '$divided_chunk_root'";

    $this->{exec_impl_}->remove($chunk->host(), $chunk->path(), 1) &&
      die "Modifier: Can't remove original chunk '" . $chunk->path() . "'";
  }

  # collecting divided chunk data into to_merge folder
  my @new_chunks_dirs = $this->{exec_impl_}->list(
    $chunk->host(), $divided_chunk_root);

  foreach my $dir(@new_chunks_dirs)
  {
    if($dir !~ m|^\d*$|) # divided chunks have empty folder prefix
    {
      die "Unexpected directory name '$dir' after dividing";
    }

    my $new_chunk_root = TO_MERGE_CHUNK_PREFIX . "_" . $dir . "_" . $this->{chunks_number_} . "/";
    my $new_chunk_dir = $new_chunk_root . $this->random_number_();

    $this->{exec_impl_}->mkdir(
      $chunk->host(),
      $new_chunk_root) &&
      die "Can't create directory " . $chunk->host() . ":" . $new_chunk_root;

    $this->{exec_impl_}->move(
      $chunk->host(),
      $divided_chunk_root . "/" . $dir,
      $chunk->host(),
      $new_chunk_dir) &&
      die "Modifier: Can't move " . $chunk->host() . ":" . $divided_chunk_root .
        "/" . $dir . " to " . $chunk->host() . ":" . $new_chunk_dir;
  }

  # remove 
  $this->{exec_impl_}->remove($chunk->host(), $divided_chunk_root, 1);
}

# collects chunk data from hosts that contains it and merges chunk
sub merge_chunk
{
  my ($this, $new_chunk_host, $chunk) = @_;

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Redistribution: merging chunk #" . $chunk->index() . " on the host $new_chunk_host");
  }

  my $new_chunk_path = $this->get_chunk_path_($chunk->index()); 
  my $chunk_desc = new ChunkDescription(
    index => $chunk->index(),
    total_chunks => $this->{chunks_number_},
    host => $new_chunk_host,
    path => $new_chunk_path,
    version => '3.4');

  # chunk is merged but not renamed - just rename it
  if($chunk->path() =~ m|^@{[MERGED_CHUNK_PREFIX]}_(\d+)_(\d+)$|)
  {
    if(!defined $this->{dry_run_})
    {
      $this->{exec_impl_}->move(
        $new_chunk_host,
        $chunk->path(),
        $new_chunk_host,
        $new_chunk_path) &&
        die "Modifier: Can't move $new_chunk_host:". $chunk->path() .
          " to $new_chunk_host:$new_chunk_path";
    }
    return $chunk_desc;
  }

  # collecting all chunk data on the new chunk host
  foreach my $host(@{$chunk->hosts()})
  {
    if ($host eq $new_chunk_host)
    {
      next;
    }

    if($this->{verbose_} && defined $this->{logger_})
    {
      $this->{logger_}->trace("Moving chunk data from host $host");
    }

    if(defined $this->{dry_run_})
    {
      next;
    }

    $this->{exec_impl_}->mkdir(
      $new_chunk_host,
      $chunk->path()) &&
      die "Can't create directory " . $new_chunk_host . ":" . $chunk->path();

    my @chunk_dirs = $this->{exec_impl_}->list($host, $chunk->path());
    foreach my $dir(@chunk_dirs)
    {
      if($dir !~ m|^\d*$|)
      {
        die "Unexpected directory name '#dir' in '" . $chunk->path();
      }

      my $full_merge_chunk_dir = $chunk->path() . "/" . $dir;

      $this->{exec_impl_}->move(
        $host,
        $full_merge_chunk_dir,
        $new_chunk_host,
        $full_merge_chunk_dir) &&
        die "Modifier: Can't move $host:" . $full_merge_chunk_dir .
          " to $new_chunk_host:" . $full_merge_chunk_dir;
    }
    $this->{exec_impl_}->remove($host, $chunk->path(), 1);
  }

  if(defined $this->{dry_run_})
  {
    return $chunk_desc;
  }

  # preparing chunk for merging
  my $merged_chunk_dir = MERGED_CHUNK_PREFIX .
    "_" . $chunk->index() . "_" . $this->{chunks_number_};
  $this->{exec_impl_}->remove($new_chunk_host, $merged_chunk_dir, 1);

  if ($this->{exec_impl_}->dir_exists($new_chunk_host, $new_chunk_path))
  {
    $this->{exec_impl_}->copy(
      $new_chunk_host,
      $new_chunk_path,
      $new_chunk_host,
      $merged_chunk_dir);
  }
  else
  {
    $this->{exec_impl_}->mkdir(
      $new_chunk_host,
      $merged_chunk_dir) &&
      die "Can't create directory $new_chunk_host:$merged_chunk_dir";
  }

  # collecting data to merge in format {chunk_prefix => paths}
  my %merge_paths;
  my @chunks_to_merge = $this->{exec_impl_}->list($new_chunk_host, $chunk->path());
  foreach my $chunk_dir(@chunks_to_merge)
  {
    my $chunk_path = $chunk->path() . "/" . $chunk_dir;
    my @chunks_files = $this->{exec_impl_}->list($new_chunk_host, $chunk_path);
    my %prefixes;
    foreach my $file(@chunks_files)
    {
      if($file =~ m|^([^.]*)[.].*$| &&
         !exists($prefixes{$1}))
      {
        $prefixes{$1} = 1;
      }
    }

    foreach my $prefix(keys %prefixes)
    {
      push(@{$merge_paths{$prefix}}, $chunk_path);
    }
  }

  # merging chunk
  while(my ($prefix, $chunk_dirs) = each(%merge_paths))
  {
    my @chunk_dirs_with_prefixes;
    foreach my $chunk_dir(@$chunk_dirs)
    {
      push(@chunk_dirs_with_prefixes, $chunk_dir . "/" . $prefix);
    }

    $this->{exec_impl_}->execute_command(
      $new_chunk_host,
      $this->{chunks_merge_cmd_},
      Common::ModifierExec::path_wrapper($merged_chunk_dir),
      Common::ModifierExec::path_wrapper(@chunk_dirs_with_prefixes)) &&
      die "Modifier: Can't merge chunk: " . $merged_chunk_dir . " at host " . $new_chunk_host;
  }

  # chunk is merged, just renaming is left
  $this->{exec_impl_}->remove($new_chunk_host, $chunk->path(), 1);
  $this->{exec_impl_}->remove($new_chunk_host, $new_chunk_path, 1);

  $this->{exec_impl_}->move(
    $new_chunk_host,
    $merged_chunk_dir,
    $new_chunk_host,
    $new_chunk_path) &&
    die "Modifier: Can't move $new_chunk_host:$merged_chunk_dir to" .
      " $new_chunk_host:$new_chunk_path";

  return $chunk_desc;
}

# upgrade chunk to the latest version
sub adapt_chunk_
{
  my ($this, $chunk) = @_;
  my $res = 0;

  return $res;
}

sub get_chunk_path_
{
  my ($this, $chunk_index) = @_;
  return CHUNK_PREFIX . "_" . $chunk_index . "_" . $this->{chunks_number_};
}

sub random_number_
{
  my $random_number = "";
  for (my $i=0; $i <= 9; $i++)
  {
    $random_number .= int(rand(10));
  }
  return $random_number; 
}

1;


package UserInfo::Modifier;

use warnings;
use strict;
use Exporter;
use Common::ChunkDescription;
use Common::ModifierExec;

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

use constant USER_CHUNK_PREFIX => 'UserChunk';
use constant CHUNK_3_3_UPDATER => 'PlainStorageUtil';
use constant CHUNK_REDISTRIBUTOR => 'LevelCheckUtil';

use constant OLD_PROFILE_FOLDER_PREFIXES => {
  add      => "AddChunk_",
  base     => "Chunk_",
  temp     => "TempChunk_",
  history  => "HistoryChunk_",
  activity => "ActivityChunk_",
  pref     => "PrefChunk_",
  wd_imps  => "WDImpsChunk_",
  };

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

  $this->{chunk_3_3_update_cmd_} = $environment_cmd . CHUNK_3_3_UPDATER .
    " --rw-level-max-size=104857600 --max-undumped-size=262144000".
    " --max-levels0=20 convert-mem-to-level ";

  my $chunks_options = " --rw-level-max-size=104857600 --max-undumped-size=262144000" .
    " --max-levels0=20 --chunks-number=" . $chunks_number;

  # command that divide existing chunk
  # <source folder with file prefix> <destination folder with chunk prefix>
  $this->{chunks_divide_cmd_} = $environment_cmd . CHUNK_REDISTRIBUTOR .
    " divide-to-chunks" . $chunks_options;

  # command that merge two chunks
  # <destination folder with chunk prefix> <source folders with prefixes>
  $this->{chunks_merge_cmd_} = $environment_cmd . CHUNK_REDISTRIBUTOR .
    " merge" . $chunks_options;

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

  my @chunk_folders = $this->{exec_impl_}->list($host, "");
  my %ignore_old_chunks;

  # chunks checking for 3.4 format:
  #   Users/(UserChunk_\d_\d)/$prefix...
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d+)_(\d+)(/.*)?$|)
    {
      $chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => USER_CHUNK_PREFIX . '_' . $1 . '_' . $2,
        version => '3.4');
    }
  }

  # checking for chunks with unfinished distribution, format:
  #   Users/(to_merge_\d_\d)/$prefix...
  #   Users/(divided_\d_\d)/$prefix...
  #   Users/(UserChunk_\d_\d_merged)/$prefix...
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^to_merge_(\d+)_(\d+)(/.*)?$|)
    {
      $unmerged_chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        hosts => [$host],
        path => 'to_merge_' . $1 . '_' . $2,
        version => '3.4');
    }
  }

  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d+)_(\d+)_merged(/.*)?$|)
    {
      if(!exists($unmerged_chunks{$1}))
      {
        $unmerged_chunks{$1} = new ChunkDescription(
          index => $1,
          total_chunks => $2,
          hosts => [$host],
          path => USER_CHUNK_PREFIX . '_' . $1 . '_' . $2 . '_merged',
          version => '3.4');
      }
    }

    if($folder =~ m|^divided_(\d+)_(\d+)(/.*)?$|)
    {
      $unfinished_chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => 'divided_' . $1 . '_' . $2,
        version => '3.4',
        divided => 1);
    }
  }

  # chunks checking for 2.4 format:
  #   Users/(UserChunk_\d)/$prefix...
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d+)(/.*)?$|)
    {
      $chunks{$1} = new ChunkDescription(
        index => $1,
        host => $host,
        path => USER_CHUNK_PREFIX . '_' . $1,
        version => '2.4');
    }
    elsif($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d+)[.]migrated$|)
    {
      $ignore_old_chunks{$1} = 1;
    }
  }

  # reconstruct chunks root for previous version
  my $chunks_root_v23_prefix = $this->chunks_root_v23_prefix_();

  if(defined($chunks_root_v23_prefix))
  {
    my @old_chunk_files = $this->{exec_impl_}->list(
      $host, "$chunks_root_v23_prefix");

    foreach my $folder(@old_chunk_files)
    {
      if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d+)(/.*)?$|)
      {
        if(!exists($ignore_old_chunks{$1}))
        {
          $chunks{$1} = new ChunkDescription(
            index => $1,
            host => $host,
            path => $chunks_root_v23_prefix,
            version => '2.3');
        }
      }
    }

    my @all_chunk_files = $this->{exec_impl_}->list($host, "$chunks_root_v23_prefix", 1);

    # check chunks with version <= 2.2

    # chunks checking for 1.12 format:
    #   Chunk_\d/BaseProfiles...,
    #   AddChunk_\d/AddProfiles...
    foreach my $folder(@all_chunk_files)
    {
      my $base_folder_prefix = OLD_PROFILE_FOLDER_PREFIXES->{base};
      if($folder =~ m|^$base_folder_prefix(\d+)$|)
      {
        if(!exists($ignore_old_chunks{$1}))
        {
          $chunks{$1} = new ChunkDescription(
            index => $1,
            host => $host,
            path => $chunks_root_v23_prefix . "/" . $folder,
            version => '1.12');
        }
      }
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

  $chunk->version('2.0');

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
      " on the host " . $chunk->host() . " to " . $this->{chunks_number_} . " chunks");
  }

  if(defined $this->{dry_run_})
  {
    return;
  }

  my $divided_chunk_root = "divided_" . $chunk->index() . "_" . $this->{chunks_number_} . "/";

  # dividing chunk into temp directory
  if(!defined($chunk->divided()))
  {
    $this->adapt_chunk_($chunk);

    my $temp_chunk_root = "temp_" . $chunk->index() . "/";

    $this->{exec_impl_}->remove($chunk->host(), $temp_chunk_root, 1);
    $this->{exec_impl_}->mkdir($chunk->host(), $temp_chunk_root);

    my @chunk_files = $this->{exec_impl_}->list($chunk->host(), $chunk->path());
    my %processed_prefixes;
    foreach my $file(@chunk_files)
    {
      if($file !~ m|^(.*?)\..*\.index$| ||
         exists($processed_prefixes{$1}))
      {
        next;
      }

      my $prefix = $1;

      $this->{exec_impl_}->execute_command(
        $chunk->host(),
        $this->{chunks_divide_cmd_},
        Common::ModifierExec::path_wrapper($chunk->path() . "/" . $prefix),
        Common::ModifierExec::path_wrapper($temp_chunk_root)) &&
        die "Modifier: Can't divide chunk: " . $chunk->path();
      $processed_prefixes{$prefix} = 1;
    }

    $this->{exec_impl_}->remove($chunk->host(), $divided_chunk_root, 1);

    $this->{exec_impl_}->move(
      $chunk->host(),
      $temp_chunk_root,
      $chunk->host(),
      $divided_chunk_root);

    $this->{exec_impl_}->remove($chunk->host(), $chunk->path(), 1);
  }

  # collecting divided chunk data in to_merge folder
  my @new_chunks_dirs = $this->{exec_impl_}->list($chunk->host(), $divided_chunk_root);
  foreach my $dir(@new_chunks_dirs)
  {
    if($dir !~ m|^\d+$|)
    {
      next;
    }

    my $new_chunk_root = "to_merge_" . $dir . "_" . $this->{chunks_number_} . "/";
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

  $this->{exec_impl_}->remove($chunk->host(), $divided_chunk_root, 1);
}

# collects chunk data from hosts that contain it and merges chunk
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
  if($chunk->path() =~ m|^.*_merged$|)
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
      if($dir !~ m|^\d+$|)
      {
        next;
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
  my $merged_chunk_dir = $this->get_chunk_path_($chunk->index()) . "_merged";
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
      if($file =~ m|^(.*?)\..*\.index$| &&
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
      Common::ModifierExec::path_wrapper(@chunk_dirs_with_prefixes),
      Common::ModifierExec::path_wrapper($merged_chunk_dir . "/" . $prefix)) &&
      die "Modifier: Can't merge chunk: " . $chunk->path() . "/" . $1;
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

  if(defined($chunk->version()))
  {
    if($chunk->version() eq '1.12')
    {
      $this->migrate_1_12_($chunk);
      $res = 1;
    }

    if($chunk->version() eq '2.3')
    {
      $this->migrate_2_3_($chunk);
      $res = 1;
    }

    if($chunk->version() eq '2.4')
    {
      $this->migrate_3_4_($chunk);
      $res = 1;
    }
  }

  return $res;
}

sub migrate_1_12_
{
  my ($this, $chunk) = @_;

  my $dst_host = $chunk->host();
  my $command = '';

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to migrate chunk #" .
      $chunk->index() . ": 1.12 => 2.3 at '$dst_host'");
  }

  my $chunks_root_v23_prefix = $this->chunks_root_v23_prefix_();

  my $new_chunk_root = "$chunks_root_v23_prefix/" .
    USER_CHUNK_PREFIX . "_" . $chunk->index() . "/";

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->mkdir($dst_host, $new_chunk_root) &&
      die "Modifier: Can't create directory $dst_host:$new_chunk_root";

    while(my ($prefix_type, $prefix_value) = each(%{OLD_PROFILE_FOLDER_PREFIXES()}))
    {
      my $old_chunk_root = "$chunks_root_v23_prefix/" . $prefix_value . $chunk->index();
      my @files_to_convert = $this->{exec_impl_}->list($dst_host, $old_chunk_root, 1);

      foreach my $old_file(@files_to_convert)
      {
        if($old_file =~ m/Profiles[.]/ || $old_file =~ m/WDImp[.]/)
        {
          my $new_file = $old_file;
          my $basename = $old_file;
          $basename =~ s|Profiles||;
          $basename =~ s|WDImp([^s])|WDImps$1|;

          $this->{exec_impl_}->move(
            $dst_host,
            $old_chunk_root . "/" . $old_file,
            $dst_host,
            $new_chunk_root . "/" . $basename) &&
            die "Modifier: Can't move $dst_host:$old_file to $dst_host:$new_chunk_root/$basename";
        }
      }

      my @res_files = $this->{exec_impl_}->list($dst_host, $old_chunk_root, 0);
      if(scalar @res_files == 0)
      {
        $this->{exec_impl_}->remove($dst_host, $old_chunk_root) &&
          die "Modifier: Can't remove $dst_host:$old_chunk_root";
      }
      else
      {
        $this->{exec_impl_}->move(
          $dst_host, $old_chunk_root, $dst_host, $old_chunk_root . ".bak") &&
          die "Modifier: Can't move $dst_host:$old_chunk_root to $dst_host:${old_chunk_root}.bak";
      }
    }
  }

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: migrated chunk #" .
      $chunk->index() . ": 1.12 => 2.3 at '$dst_host'");
  }

  $chunk->version('2.3');
  $chunk->path($new_chunk_root);
}

sub migrate_2_3_
{
  my ($this, $chunk) = @_;

  my $chunks_root_v23_prefix = $this->chunks_root_v23_prefix_();
  my $dst_host = $chunk->host();

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to migrate chunk #" .
      $chunk->index() . ": 2.3 => 2.4 at '$dst_host'");
  }

  if(!defined $this->{dry_run_})
  {
    my $old_chunk_root = "$chunks_root_v23_prefix/" . USER_CHUNK_PREFIX . "_" . $chunk->index();
    my $new_chunk_root = USER_CHUNK_PREFIX . "_" . $chunk->index();

    $this->{exec_impl_}->remove($dst_host, $new_chunk_root, 1) &&
          die "Modifier: Can't remove $dst_host:$new_chunk_root";

    $this->{exec_impl_}->copy(
      $dst_host,
      $old_chunk_root,
      $dst_host,
      $new_chunk_root) &&
      die "Modifier: Can't copy $dst_host:$old_chunk_root to $dst_host:$new_chunk_root";

    $this->{exec_impl_}->mkdir(
      $dst_host,
      USER_CHUNK_PREFIX . "_" . $chunk->index() . ".migrated");
  }

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: migrated chunk #" .
      $chunk->index() . ": 2.3 => 2.4 at '$dst_host'");
  }

  $chunk->version('2.4');
}

sub migrate_3_4_
{
  my ($this, $chunk) = @_;

  my $dst_host = $chunk->host();
  my $new_chunk_root = $this->get_chunk_path_($chunk->index());

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to migrate chunk #" .
      $chunk->index() . ": 2.4 => 3.4 at '$dst_host'");
  }

  if(!defined $this->{dry_run_})
  {
    my $chunk_root = USER_CHUNK_PREFIX . "_" . $chunk->index();
    my $temp_chunk_root = USER_CHUNK_PREFIX . "_" . $chunk->index() . ".temp";
    my $back_chunk_root = USER_CHUNK_PREFIX . "_" . $chunk->index() . ".back";

    $this->{exec_impl_}->remove($dst_host, $temp_chunk_root, 1) &&
          die "Modifier: Can't remove $dst_host:$temp_chunk_root";

    $this->{exec_impl_}->copy(
      $dst_host,
      $chunk_root,
      $dst_host,
      $temp_chunk_root) &&
      die "Modifier: Can't copy $dst_host:$chunk_root to $dst_host:$temp_chunk_root";

    my %processed;
    my @chunk_files = $this->{exec_impl_}->list($dst_host, $temp_chunk_root);
    foreach my $file(@chunk_files)
    {
      if($file =~ m|^(.*?)\..*$| && 
         !($file =~ m|^.*\.index$| || $file =~ m|^.*\.data$|))
      {
        my $prefix = $1;
        if(!exists($processed{$prefix}))
        {
          $this->{exec_impl_}->execute_command(
            $dst_host,
            $this->{chunk_3_3_update_cmd_},
            Common::ModifierExec::path_wrapper($temp_chunk_root),
            $prefix,
            Common::ModifierExec::path_wrapper($temp_chunk_root),
            $prefix) &&
            die "Modifier: Can't update file $dst_host:$temp_chunk_root/$file";

          $processed{$prefix} = 1;
        }

        $this->{exec_impl_}->remove($dst_host, $temp_chunk_root . "/" . $file) &&
          die "Modifier: Can't remove $dst_host:$temp_chunk_root/$file";
      }
    }

    $this->{exec_impl_}->remove($dst_host, $back_chunk_root, 1) &&
          die "Modifier: Can't remove $dst_host:$back_chunk_root";

    $this->{exec_impl_}->move(
      $dst_host,
      $chunk_root,
      $dst_host,
      $back_chunk_root) &&
      die "Modifier: Can't move $dst_host:$chunk_root to $dst_host:$back_chunk_root";

    $this->{exec_impl_}->move(
      $dst_host,
      $temp_chunk_root,
      $dst_host,
      $new_chunk_root) &&
      die "Modifier: Can't move $dst_host:$temp_chunk_root to $dst_host:$new_chunk_root";
  }  

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: migrated chunk #" .
      $chunk->index() . ": 2.4 => 3.4 at '$dst_host'");
  }

  $chunk->version('3.4');
  $chunk->path($new_chunk_root);
}

sub chunks_root_v23_prefix_
{
  my ($this) = @_;

  if($this->{exec_impl_}->chunks_root() =~ m|^(?:.*/)?([^\d/]+)\d+[/]?$|)
  {
    return "../" . $1;
  }

  return undef;
}

sub get_chunk_path_
{
  my ($this, $chunk_index) = @_;
  return USER_CHUNK_PREFIX . "_" . $chunk_index . "_" . $this->{chunks_number_};
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


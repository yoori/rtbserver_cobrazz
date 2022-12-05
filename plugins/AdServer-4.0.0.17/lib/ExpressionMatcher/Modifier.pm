package ExpressionMatcher::Modifier;

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


#root: /cache/ExpressionMatcher
#old_root: ../

use constant PROFILE_FOLDERS_PREFIXES => {
  "Inventory" => ["Inventory_"],
  "HouseholdColoReach" => ["HouseholdColoReach_"],
  "UserTriggerMatch" => ["UserTriggerMatch_", "TempUserTriggerMatch_"],  
  };
use constant CHUNK_PREFIX => 'Chunk';
use constant CHUNK_REDISTRIBUTOR => 'LevelCheckUtil';

## Constructor
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
    $environment_cmd
  ) = @_;

  my $this = {
    exec_impl_ => $exec_impl,
    logger_ => $logger,
    verbose_ => $verbose,
    dry_run_ => defined $dry_run && $dry_run > 0 ? 1 : undef,
    chunks_number_ => $chunks_number,
    };

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

  # chunks checking for 3.4 format:
  #   ../Inventory/
  #   ../HouseholdColoReach/
  #   ../UserTriggerMatch/
  my @chunk_folders = $this->{exec_impl_}->list($host, "../");
  my $chunk_3_4_detected = 0;
  foreach my $chunk_folder(@chunk_folders)
  {
    foreach my $folder(keys %{PROFILE_FOLDERS_PREFIXES()})
    {
      if($chunk_folder =~ m|^$folder(/.*)?$|)
      {
        my $chunk_index = $this->random_number_(); 
        $chunks{$chunk_index} = new ChunkDescription(
          index => $chunk_index,
          total_chunks => 0,
          host => $host,
          path => '../',
          version => '3.4');
        $chunk_3_4_detected = 1;
        last;
      }
    }

    if ($chunk_3_4_detected)
    {
      last;
    }
  }

  # chunks checking for 3.5 format:
  #   /Chunk_\d_\d/Inventory/
  #   /Chunk_\d_\d/HouseholdColoReach/
  #   /Chunk_\d_\d/UserTriggerMatch/
  @chunk_folders = $this->{exec_impl_}->list($host, "/");
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[CHUNK_PREFIX]}_(\d+)_(\d+)(/.*)?$|)
    {
      $chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => CHUNK_PREFIX . '_' . $1 . '_' . $2,
        version => '3.5');
    }
  }

  # checking for chunks with unfinished distribution, format:
  #   /(to_merge_\d_\d)/...
  #   /(divided_\d_\d)/...
  #   /(Chunk_\d_\d_merged)/...
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^to_merge_(\d+)_(\d+)(/.*)?$|)
    {
      $unmerged_chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        hosts => [$host],
        path => 'to_merge_' . $1 . '_' . $2,
        version => '3.5');
    }
  }
  
  foreach my $folder(@chunk_folders)
  {
    if($folder =~ m|^@{[CHUNK_PREFIX]}_(\d+)_(\d+)_merged(/.*)?$|)
    {
      if(!exists($unmerged_chunks{$1}))
      {
        $unmerged_chunks{$1} = new ChunkDescription(
          index => $1,
          total_chunks => $2,
          hosts => [$host],
          path => CHUNK_PREFIX . '_' . $1 . '_' . $2 . '_merged',
          version => '3.5');
      }
    }

    if($folder =~ m|^divided_(\d+)_(\d+)(/.*)?$|)
    {
      $unfinished_chunks{$1} = new ChunkDescription(
        index => $1,
        total_chunks => $2,
        host => $host,
        path => 'divided_' . $1 . '_' . $2,
        version => '3.5',
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

    foreach my $folder(keys %{PROFILE_FOLDERS_PREFIXES()})
    {
      $this->{exec_impl_}->mkdir(
        $dst_host,
        $chunk->path(). "/". $folder) &&
        die "Modifier: Can't create $dst_host:" . $chunk->path();
    }
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
  my ($this, $chunk, $copied) = @_;

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

    while(my ($folder, $prefixes) = each(%{PROFILE_FOLDERS_PREFIXES()}))
    {
      if (!$this->{exec_impl_}->dir_exists($chunk->host(), $chunk->path() . "/" . $folder))
      {
        next;
      }

      $this->{exec_impl_}->mkdir($chunk->host(), $temp_chunk_root. "/" . $folder);

      foreach my $prefix(@$prefixes)
      {
        $this->{exec_impl_}->execute_command(
          $chunk->host(),
          $this->{chunks_divide_cmd_},
          Common::ModifierExec::path_wrapper($chunk->path() . "/" . $folder . "/" . $prefix),
          Common::ModifierExec::path_wrapper($temp_chunk_root. "/" . $folder . "/")) &&
          die "Modifier: Can't divide chunk: " . $chunk->path();
      }
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
  foreach my $folder(keys %{PROFILE_FOLDERS_PREFIXES()})
  {
    my $divided_chunk_folder = $divided_chunk_root . $folder;
    if (!$this->{exec_impl_}->dir_exists($chunk->host(), $divided_chunk_folder))
    {
      next;
    }

    my @new_chunks_dirs = $this->{exec_impl_}->list($chunk->host(), $divided_chunk_folder);
    
    foreach my $dir(@new_chunks_dirs)
    {
      if($dir !~ m|^\d+$|)
      {
        next;
      }

      my $new_chunk_root = "to_merge_" . $dir . "_" . $this->{chunks_number_} . "/" . $folder . "/";
      my $new_chunk_folder = $new_chunk_root . $this->random_number_();

      $this->{exec_impl_}->mkdir(
        $chunk->host(),
        $new_chunk_root) &&
        die "Can't create directory " . $chunk->host() . ":" . $new_chunk_root;

      $this->{exec_impl_}->move(
        $chunk->host(),
        $divided_chunk_folder . "/" . $dir,
        $chunk->host(),
        $new_chunk_folder) &&
        die "Modifier: Can't move " . $chunk->host() . ":" . $divided_chunk_folder .
          $dir . " to " . $chunk->host() . ":" . $new_chunk_folder;
    }
  }

  $this->{exec_impl_}->remove($chunk->host(), $divided_chunk_root, 1);
}

# collects chunk data from hosts that contains it ans mereges chunk
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
    version => '3.5');

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

    foreach my $chunk_folder(keys %{PROFILE_FOLDERS_PREFIXES()})
    {
      my $chunk_root = $chunk->path() . "/" . $chunk_folder . "/";
      
      if (!$this->{exec_impl_}->dir_exists($host, $chunk_root))
      {
        next;
      }

      $this->{exec_impl_}->mkdir(
        $new_chunk_host,
        $chunk_root) &&
        die "Can't create directory " . $new_chunk_host . ":" . $chunk_root;

      my @chunk_folders = $this->{exec_impl_}->list($host, $chunk_root);
      foreach my $folder(@chunk_folders)
      {
        if($folder !~ m|^\d+$|)
        {
          next;
        }

        my $full_merge_chunk_dir = $chunk_root . "/" . $folder;

        $this->{exec_impl_}->move(
          $host,
          $full_merge_chunk_dir,
          $new_chunk_host,
          $full_merge_chunk_dir) &&
          die "Modifier: Can't move $host:" . $full_merge_chunk_dir .
            " to $new_chunk_host:" . $full_merge_chunk_dir;
      }
    }
    $this->{exec_impl_}->remove($host, $chunk->path(), 1);
  }

  if(defined $this->{dry_run_})
  {
    return $chunk_desc;
  }

  # collecting data for merging
  my $merged_chunk_folder = $this->get_chunk_path_($chunk->index()) . "_merged";
  $this->{exec_impl_}->remove($new_chunk_host, $merged_chunk_folder, 1);

  if ($this->{exec_impl_}->dir_exists($new_chunk_host, $new_chunk_path))
  {
    $this->{exec_impl_}->copy(
      $new_chunk_host,
      $new_chunk_path,
      $new_chunk_host,
      $merged_chunk_folder);
  }
  else
  {
    $this->{exec_impl_}->mkdir(
      $new_chunk_host,
      $merged_chunk_folder) &&
      die "Can't create directory $new_chunk_host:$merged_chunk_folder";
  }

  foreach my $chunk_folder(keys %{PROFILE_FOLDERS_PREFIXES()})
  {
    my $chunk_root = $chunk->path() . "/" . $chunk_folder . "/";
    my %merge_paths;

    my @folders_to_merge = $this->{exec_impl_}->list($new_chunk_host, $chunk_root);
    foreach my $folder(@folders_to_merge)
    {
      my @chunks_files = $this->{exec_impl_}->list($new_chunk_host,
        $chunk_root . "/" . $folder);

      foreach my $prefix(@{PROFILE_FOLDERS_PREFIXES->{$chunk_folder}})
      {
        foreach my $file(@chunks_files)
        {
          if($file =~ m|^$prefix.*\.index$|)
          {
            push(@{$merge_paths{$prefix}}, $chunk_root . "/" . $folder);
            last;
          }
        }
      }
    } 

    # merging chunk
    $this->{exec_impl_}->mkdir(
      $new_chunk_host,
      $merged_chunk_folder . "/" . $chunk_folder) &&
      die "Can't create directory $new_chunk_host:$merged_chunk_folder/$chunk_folder";

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
        Common::ModifierExec::path_wrapper($merged_chunk_folder . "/" . $chunk_folder . "/" . $prefix)) &&
        die "Modifier: Can't merge chunk: " . $chunk->path() . "/" . $1;
    }
  }

  # chunk is merged, just renaming is left
  $this->{exec_impl_}->remove($new_chunk_host, $chunk->path(), 1);
  $this->{exec_impl_}->remove($new_chunk_host, $new_chunk_path, 1);

  $this->{exec_impl_}->move(
    $new_chunk_host,
    $merged_chunk_folder,
    $new_chunk_host,
    $new_chunk_path) &&
    die "Modifier: Can't move $new_chunk_host:$merged_chunk_folder to" .
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
    if($chunk->version() eq '3.4')
    {
      $this->migrate_3_5_($chunk);
      $res = 1;
    }
  }

  return $res;
}

sub migrate_3_5_
{
  my ($this, $chunk) = @_;

  my $dst_host = $chunk->host();
  my $new_chunk_root = CHUNK_PREFIX . '_' . $chunk->index() . '_0';

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to migrate chunk #" .
      $chunk->index() . ": 3.4 => 3.5 at '$dst_host'");
  }

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->mkdir(
      $dst_host,
      $new_chunk_root);

    my @chunk_items = $this->{exec_impl_}->list($dst_host, "../");
    foreach my $folder(@chunk_items)
    {
      foreach my $prefix(keys %{PROFILE_FOLDERS_PREFIXES()})
      {
        if($folder =~ m|^$prefix(/)?$|)
        {
          $this->{exec_impl_}->move(
            $dst_host,
            "../" . $folder,
            $dst_host,
            $new_chunk_root) &&
          die "Modifier: Can't move $dst_host:$folder to $dst_host:$new_chunk_root";
        }
      }        
    }
  }

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: migrated chunk #" .
      $chunk->index() . ": 3.4 => 3.5 at '$dst_host'");
  }

  $chunk->version('3.5');
  $chunk->path($new_chunk_root);
}

sub get_chunk_path_
{
  my ($this, $chunk_index) = @_;
  return CHUNK_PREFIX . '_' . $chunk_index . "_" . $this->{chunks_number_};
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

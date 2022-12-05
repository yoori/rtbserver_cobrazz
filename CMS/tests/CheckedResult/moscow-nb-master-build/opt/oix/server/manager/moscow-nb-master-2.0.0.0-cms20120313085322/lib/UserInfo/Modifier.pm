package UserInfo::Modifier;

use warnings;
use strict;
use Exporter;
use Class::Struct
  ChunkDescription => [
    index => '$',
    host => '$',
    path => '$',
    version => '$'
  ];

use vars qw ( $VERSION @ISA @EXPORT );

@ISA = qw(Exporter);
$VERSION = '1.00';
@EXPORT = qw(new exists_chunks create move remove);

# exists_chunks(host): return exist at host chunk set : %{ index => \ChunkDescription }
#   detect old format chunks - its migration must be done with using create
sub exists_chunks;

# create(host, index): migrate old format chunk if exists or create it
sub create;

# move(src_host, dst_host, index)
sub move;

# remove(host, index)
sub remove;

use constant USER_CHUNK_PREFIX => 'UserChunk';

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
  my ($class, $exec_impl, $logger, $verbose, $dry_run) = @_;

  my $this = {
    exec_impl_ => $exec_impl,
    logger_ => $logger,
    verbose_ => $verbose,
    dry_run_ => defined $dry_run && $dry_run > 0 ? 1 : undef
    };

  bless($this, $class);
  return $this;
}

# check exists chunks - return array of chunk indexes for specified host
sub exists_chunks
{
  my ($this, $host) = @_;
  my %chunks = ();

  my @chunk_files = $this->{exec_impl_}->list($host, "");
  my %ignore_old_chunks;

  # chunks checking for current format:
  #   Users/(UserChunk_\d)/$prefix...
  foreach my $folder(@chunk_files)
  {
    if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d*)(/.*)?$|)
    {
      $chunks{$1} = new ChunkDescription(
        index => $1,
        host => $host,
        path => USER_CHUNK_PREFIX . '_' . $1,
        version => '2.4');
    }
    elsif($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d*)[.]migrated$|)
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
      if($folder =~ m|^@{[USER_CHUNK_PREFIX]}_(\d*)(/.*)?$|)
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
      if($folder =~ m|^$base_folder_prefix(\d*)$|)
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

  return \%chunks;
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

  if(!defined($chunk->path()))
  {
    $chunk->path(USER_CHUNK_PREFIX . "_" . $chunk->index() . "/");
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

  if($this->{verbose_} && defined $this->{logger_})
  {
    $this->{logger_}->trace(
      "Modifier: to move chunk #" .
      $chunk->index() . " from '$src_host' to '$dst_host': " .
      $src_host . ":" . USER_CHUNK_PREFIX . "_" . $chunk->index() . "/ to $dst_host:/");
  }

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->move(
      $src_host,
      USER_CHUNK_PREFIX . "_" . $chunk->index() . "/",
      $dst_host,
      "") &&
      die "Modifier: Can't move $src_host:" . (USER_CHUNK_PREFIX . "_" . $chunk->index() . "/") .
        " to $dst_host:";
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

  if(!defined $this->{dry_run_})
  {
    $this->{exec_impl_}->move(
      $dst_host,
      USER_CHUNK_PREFIX . "_" . $chunk->index() . "/",
      $dst_host,
      USER_CHUNK_PREFIX . "_" . $chunk->index() . "_/") &&
      die "Modifier: Can't move $dst_host:" . (USER_CHUNK_PREFIX . "_" . $chunk->index() . "/") .
        " to $dst_host:" . (USER_CHUNK_PREFIX . "_" . $chunk->index() . "_/");
  }
}

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

sub chunks_root_v23_prefix_
{
  my ($this) = @_;

  if($this->{exec_impl_}->chunks_root() =~ m|^(?:.*/)?([^\d/]+)\d+[/]?$|)
  {
    return "../" . $1;
  }

  return undef;
}

1;

#! /usr/bin/perl
# -*- perl -*-
#
use warnings;
use strict;

=head1 NAME

taskbot.pl - run shell script and record outcome of every command

=head1 SYNOPSIS

  taskbot.pl [OPTIONS] CONFIG SCRIPT [ARGS]
  taskbot.pl [OPTIONS] CONFIG -c COMMAND [ARGS]

=head2 OPTIONS

=over

=item C<--description DESCRITION>

=item C<-d DESCRITION>

Create root task I<"DESCRIPTION">.

=item C<--trace>

=item C<--notrace>

Trace execution.  Print every command, its stdout and stderr, and its
exit status.  Default is I<--notrace>.

=item C<-c COMMAND>

Execute I<COMMAND>.  This option is mutually exclusive with specifying
I<SCRIPT>.

=item C<--run_timeout=N>

Run timeout in seconds.  Zero (or negative) value means indefinite.
Overrides corresponding setting in config file.

=back

=cut

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %option = (
    trace => 0,
);
if (! GetOptions(\%option, qw(description|d=s trace! c=s run_timeout=i))
    || @ARGV < (exists $option{c} ? 1 : 2)) {
    pod2usage(1);
}

my @taskbot_options;
push @taskbot_options, '--trace' if $option{trace};

my $config_file = shift @ARGV;

my $script;
if (defined $option{c}) {
    $script = \$option{c};
} else {
    $script = shift @ARGV;
}

my $config = do $config_file;
unless ($config) {
    die "While reading $config_file: $!" if ($!);
    die "While compiling $config_file: $@" if ($@);
    die "While executing $config_file: returned '$config'";
}

if (exists $option{run_timeout}) {
    $config->{run_timeout} = $option{run_timeout};
}


my %dbh;

sub db_connect {
    use DBI;

    $dbh{dbh} = DBI->connect(@{$config->{taskbot_dbi}}[0..2],
                             { AutoCommit => 0,
                               PrintError => 0, RaiseError => 1,
                               FetchHashKeyName => 'NAME_lc' });

    $dbh{start_task} = $dbh{dbh}->prepare(q[
        INSERT INTO task (root_task_id, parent_task_id, branch_id, 
                          description, is_command, start, finish)
        VALUES (?, ?, ?, ?, ?, ?, 0)
    ]);

    $dbh{finish_task} = $dbh{dbh}->prepare(q[
        UPDATE task SET finish = ?, error_message = ?
        WHERE id = ?
    ]);

    $dbh{finish_command} = $dbh{dbh}->prepare(q[
        INSERT INTO command (task_id, stdout_gzipped, stdout,
                             stderr_gzipped, stderr, exit_status)
        VALUES (?, ?, ?, ?, ?, ?)
    ]);

    $dbh{create_running_task} = $dbh{dbh}->prepare(q[
        INSERT INTO running_task (task_id, host, pid, user)
        VALUES (?, ?, ?, ?)
    ]);

    $dbh{delete_running_task} = $dbh{dbh}->prepare(q[
        DELETE FROM running_task
        WHERE task_id = ?
    ]);
}

sub db_ensure_connect {
    unless ($dbh{dbh}->ping) {
        use POSIX qw(strftime);
        print strftime("%Y-%m-%d %H:%M:%S  Reconnecting to the database\n",
                       localtime(time));
        db_connect;
    }
}


use Time::HiRes qw(time);


db_connect;

my @task_stack;
my $root_task_id;

if (exists $ENV{TASKBOT_PARENT_PID}) {
    # Send SIGUSR1 to parent taskbot to disable select_timeout there.
    kill('USR1', $ENV{TASKBOT_PARENT_PID})
      or die "taskbot with pid $ENV{TASKBOT_PARENT_PID} is not running\n";

    my ($task_id) = $dbh{dbh}->selectrow_array(q[
        SELECT MAX(task_id) FROM running_task
        WHERE host = ? AND pid = ?
    ], undef, $ENV{HOSTNAME}, $ENV{TASKBOT_PARENT_PID});

    ($root_task_id) = $dbh{dbh}->selectrow_array(q[
        SELECT root_task_id FROM task
        WHERE id = ?
    ], undef, $task_id);

    push @task_stack, { task_id => $task_id };
} else {
    push @task_stack, { task_id => undef };
}


sub select_branch {
  if (exists $ENV{BRANCH_NAME}) {
    my ($branch_id) = $dbh{dbh}->selectrow_array(q[
        SELECT id FROM branch
        WHERE description = ?], undef, $ENV{BRANCH_NAME});
    return $branch_id;
  }
  return undef;
}


sub delete_stale_records {
    my $select = $dbh{dbh}->prepare(q[
        SELECT DISTINCT pid
        FROM running_task
        WHERE host = ? AND user = ?
    ]);
    $select->execute($ENV{HOSTNAME}, $ENV{USER});
    my @to_delete;
    while (my $row = $select->fetchrow_hashref) {
        $row->{pid} != $$ and kill(0, $row->{pid})
          or push @to_delete, $row;
    }
    if (@to_delete) {
        print "Deleting stale records from running_task table for user"
          . " $ENV{USER} on $ENV{HOSTNAME}:\n";

        my $clean = $dbh{dbh}->prepare(q[
            DELETE FROM running_task
            WHERE host = ? AND pid = ?
        ]);

        foreach my $row (@to_delete) {
            print "  pid = $row->{pid}\n";
            $clean->execute($ENV{HOSTNAME}, $row->{pid});
        }
        # Commit will be done later on start_task().
    }
}


delete_stale_records;


sub start_task {
    my ($description, $is_command) = @_;

    my $parent_task_id = $task_stack[-1]->{task_id};

    $dbh{start_task}->execute($root_task_id, $parent_task_id,  select_branch, 
                              $description,  $is_command || 0, time);

    my $task_id = $dbh{dbh}->last_insert_id((undef) x 4);
    unless (defined $root_task_id) {
        $root_task_id = $task_id;
    }
    push @task_stack, { task_id => $task_id, is_command => $is_command };

    $dbh{create_running_task}->execute($task_id, $ENV{HOSTNAME},
                                       $$, $ENV{USER});
    $dbh{dbh}->commit;
}

sub finish_task {
    my ($in_transaction) = @_;

    db_ensure_connect unless $in_transaction;

    my $task = pop @task_stack;
    $dbh{finish_task}->execute(time, $task->{error_message},
                               $task->{task_id});
    $dbh{delete_running_task}->execute($task->{task_id});
    if (@task_stack > 1) {
        $task_stack[-1]->{error_message} = $task->{error_message};
    }

    $dbh{dbh}->commit;
}

sub finish_tasks_to_level {
    my ($level) = @_;

    finish_task while @task_stack > $level + 1;
}

my ($out, $err);

sub start_command {
    my ($command) = @_;

    ($out, $err) = ('', '');
    start_task($command, 1);
}

sub compress_maybe {
    my ($buf) = @_;

    if (length($buf) >= $config->{gzip_threshold}) {
        use Compress::Zlib;
        my $buf_gz = Compress::Zlib::memGzip($buf);
        if (length($buf_gz) <= length($buf) * $config->{gzip_ratio}) {
            return (1, $buf_gz);
        }
    }

    return (0, $buf);
}

sub finish_command {
    my ($status) = @_;

    db_ensure_connect;

    my $task = $task_stack[-1];
    $dbh{finish_command}->execute($task->{task_id}, compress_maybe($out),
                                  compress_maybe($err), $status);
    if (defined $status && $status != 0) {
        $task->{error_message} = "non-zero exit status";
        if ($status > 128) {
            my $signum = $status - 128;
            use Config;
            my ($name) = $Config{sig_name} =~ /^(?:\w+ ){$signum}(\w+)/;
            if ($name) {
                $task->{error_message} .= ", probably killed by SIG$name";
            }
        }
    }
    finish_task(1);
}


if (defined $option{description}) {
    start_task($option{description});
}

my $pid;

$SIG{__DIE__} = sub {
    my ($msg) = @_;

    $SIG{__DIE__} = 'DEFAULT';

    if (defined $pid) {
        $SIG{CHLD} = 'IGNORE';
        kill('TERM', -$pid)
          or $msg .= "\nkill('TERM', -$pid): $!";
    }
    chomp $msg;
    $task_stack[-1]->{error_message} = $msg;
    finish_command if $task_stack[-1]->{is_command};
    finish_tasks_to_level(0);
    exit(1);
};

$SIG{__WARN__} = sub {
    die("warn: @_");
};

# Starting from here all warn and die messages will go to the
# database.

$| = 1 if $option{trace};

pipe my $rin, my $win
  or die "pipe(): $!";
pipe my $rout, my $wout
  or die "pipe(): $!";
pipe my $rerr, my $werr
  or die "pipe(): $!";
pipe my $rstatus, my $wstatus
  or die "pipe(): $!";


sub write_command {
    my ($buf) = @_;

    my $len = length($buf);
    my $offset = 0;
    while ($len > 0
           && (my $count = syswrite($win, $buf, $len, $offset))) {
        $offset += $count;
        $len -= $count;
    }
    die "syswrite(): $!" if $len > 0;
}

sub trace {
    print "@_" if $option{trace};
}

sub sh_exit_status {
    my ($status) = @_;

    use POSIX qw(WEXITSTATUS WIFSIGNALED WTERMSIG);

    # Compute exit status how sh does ('info bash basic executing
    # exit').  Yep, this makes it impossible to distinguish between
    # exit(137) and kill(KILL), but this is how things are.
    return (WIFSIGNALED($?) ? 128 + WTERMSIG($?) : WEXITSTATUS($?));
}

sub pretty_size {
    my ($bytes) = @_;

    return unless defined $bytes;

    use constant SUFFIX => ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];

    my $i = 0;
    while ($bytes >= 1024) {
        $bytes /= 1024;
        ++$i;
    }

    if ($i > 0) {
        return sprintf("%.1f%s", $bytes, SUFFIX->[$i]);
    } else {
        return "${bytes}B";
    }
}


sub min {
    my ($min, @rest) = @_;

    foreach my $r (@rest) {
        $min = $r if defined $r && (! defined $min || $min > $r);
    }

    return $min;
}


sub max {
    my ($max, @rest) = @_;

    foreach my $r (@rest) {
        $max = $r if defined $r && (! defined $max || $max < $r);
    }

    return $max;
}


my $pretty_max_output_size = pretty_size($config->{max_output_size});
my $select_read = '';
my $child_died = 0;
my $fd_count = 3;
my $select_timeout = $config->{select_timeout};
my $command_timeout = $config->{command_timeout};

sub read_output {
    my $status = '';
    my ($event_read, $event_exception);

    my $get_status = sub  {
      if (waitpid($pid, 0) > 0) {
        $pid = undef;
      }
      $status = sh_exit_status($?);
      finish_command($status);
      if ($status != 0) {
        die "non-zero exit status, $config->{sh} exited\n";
      } else {
        finish_tasks_to_level(0);
        exit 0;
      }
    };

    my $process_command_output = sub {
        my ($fh, $buf) = @_;

        if (vec($event_read, fileno($fh), 1)) {
            my $count;
            my $chunk = '';
            1 while $count = sysread($fh, $chunk, 4096, length($chunk));
            trace($chunk);

            # Truncation from the start of the string is done via
            # offset (see perlguts).  In order to not use too much
            # memory we copy the value to force memory release.
            my $tmp = $$buf . $chunk;
            $$buf = undef;
            my $len = length($tmp);
            if (defined $pretty_max_output_size
                && $len > $config->{max_output_size}) {
                # Works fine if max_output_size is large enough.
                substr($tmp, 0, $len - $config->{max_output_size} + 80,
                       "WARNING: stored only last $pretty_max_output_size"
                       . " of the output\n\n...");
            }
            $$buf = $tmp;

            if (defined $count && $count == 0) {
                vec($select_read, fileno($fh), 1) = 0;
                if (--$fd_count == 0) {
                  &$get_status();
                }
            }
        }

        if (vec($event_exception, fileno($fh), 1)) {
          # wstatus can close on SIGCHILD
          die "select() exception on some of descriptors" if $fh != $rstatus;
        }
    };

    my $command_deadline = time + ($command_timeout || 2 ** 31);
    while ($status !~ /\n$/) {
        my $n = select($event_read = $select_read,
                       undef,
                       $event_exception = $select_read,
                       min($select_timeout, max($command_deadline - time, 0)));


        if ($n == -1) {
          # wstatus can close on SIGCHILD
          use Errno qw(EINTR);
          next if $! == EINTR;
          die "select(): $!" if not vec($event_exception, fileno($rstatus), 1);
        }
        if ($n == 0) {
            if (time + 0.5 < $command_deadline) {
                die "select_timeout"
                    . " ($config->{select_timeout} secs) has expired\n";
            } else {
                die "command_timeout"
                    . " ($command_timeout secs) has expired\n";
            }
        }

        &$process_command_output($rout, \$out);
        &$process_command_output($rerr, \$err);
        &$process_command_output($rstatus, \$status);

        # Zombie protection
        if ($child_died and defined $pid and $fd_count == 1)
        {
          $SIG{CHLD} = 'IGNORE';
          kill('TERM', -$pid);
          &$get_status();
        }
        
        $select_timeout = $config->{select_timeout};
    }

    chomp $status;
    finish_command($status);
    $command_timeout = $config->{command_timeout};
}


$SIG{USR1} = sub {
    $select_timeout = undef;
};


$pid = fork;
if ($pid) {
    # Parent process continues below.
} elsif (defined $pid) {
    # Child process.
    $SIG{__DIE__} = 'DEFAULT';
    $SIG{__WARN__} = 'DEFAULT';
    $SIG{USR1} = 'DEFAULT';

    use POSIX qw(dup2);

    setpgrp()
      or die "setpgrp(): $!";

    dup2(fileno($rin), fileno(STDIN))
      or die "dup2(): $!";
    dup2(fileno($wout), fileno(STDOUT))
      or die "dup2(): $!";
    dup2(fileno($werr), fileno(STDERR))
      or die "dup2(): $!";

    # By default Perl sets close-on-exec flag on every descriptor
    # above the value of $^F (== 2).  This is almost what we want,
    # except that we have to pass opened $wstatus to the shell.
    use Fcntl qw(F_GETFD F_SETFD FD_CLOEXEC);
    my $flags = fcntl($wstatus, F_GETFD, 0)
        or die "fcntl(F_GETFD): $!";
    fcntl($wstatus, F_SETFD, $flags & ~FD_CLOEXEC)
        or die "fcntn(F_SETFD): $!";

    while (my ($k, $v) = each %{$config->{environment}}) {
        if (defined $v) {
            $ENV{$k} = $v;
        } else {
            delete $ENV{$k};
        }
    }
    unless ($config_file =~ m|^/|) {
        use Cwd;
        $config_file = join('/', cwd, $config_file);
    }

    use FindBin;
    $ENV{TASKBOT} = "$FindBin::Bin/$FindBin::Script";
    $ENV{TASKBOT_OPTIONS} = "@taskbot_options";
    $ENV{TASKBOT_CONFIG} = $config_file;
    $ENV{TASKBOT_PARENT_PID} = getppid;

    exec { $config->{sh} } $config->{sh}, '-s', '--', @ARGV;
    die "exec failed: $!";
} else {
    die "fork(): $!";
}


# Parent process.

# Close wstatus when child finished
$SIG{CHLD} = sub {
  $child_died = 1;
  close($wstatus);
};

$SIG{TERM} = $SIG{INT} = $SIG{HUP} = $SIG{QUIT} = sub {
    die "got SIG$_[0]\n";
};

$SIG{PIPE} = sub {
    $SIG{PIPE} = 'IGNORE';
    eval {
        local $SIG{__DIE__} = 'DEFAULT';
        # Accumulate child exit status.
        read_output;
    };
    die "got SIGPIPE\n";
};

if (defined $config->{run_timeout} && $config->{run_timeout} > 0) {
    $SIG{ALRM} = sub {
        die "run_timeout ($config->{run_timeout} secs) has expired\n";
    };
    alarm($config->{run_timeout});
}

close($rin);
close($wout);
close($werr);

my $wstatus_fileno = fileno($wstatus);
close($wstatus)
  or die "While closing status pipe write end: $!";

foreach my $fh ($rout, $rerr, $rstatus) {
    vec($select_read, fileno($fh), 1) = 1;

    use Fcntl qw(F_GETFL F_SETFL O_NONBLOCK);
    my $flags = fcntl($fh, F_GETFL, 0)
        or die "fcntl(F_GETFL): $!";
    fcntl($fh, F_SETFL, $flags | O_NONBLOCK)
        or die "fcntn(F_SETFL): $!";
}


sub process_command {
    my ($command) = @_;

    unless ($command =~ /^\s*[^\#]/m) {
        # To get shell line counting right we output comment lines,
        # and then one more line to fake control/empty line (see
        # below).
        write_command($command . "\n");
        return;
    }

    trace($command);

    chomp $command;
    start_command($command);

    # NOTE 1: we use external /bin/echo rather than shell built-in on
    # purpose: to flush stdout/stderr after possible built-in before
    # it.
    #
    # NOTE 2: we have to use block ('{ LIST; }') to make shell parse
    # all commands before executing them.  Otherwise, if some command
    # would be started earlier, latter data could be treated as the
    # input to that command.
    write_command("{ $command\n/bin/echo \$? >\&$wstatus_fileno; }\n");

    read_output;
}


open(my $fh, '<', $script)
  or die "open(< $option{description}): $!";

my $command = '';
while (my $line = <$fh>) {
    my ($level, $var, $value) =
      $line =~ /^\s*#\s*(?:(\++)|!\s*(timeout)\s*=)\s*(.*\S)\s*$/;

    if ($value || $line =~ /^\s*$/) {
        process_command($command);
        $command = '';
        # To get shell line counting right we _do not_ output one
        # control/empty line.
    } else {
        $command .= $line;
    }

    if (defined $value) {
        if ($level) {
            finish_tasks_to_level(length($level) - 1
                                  + (defined $option{description} ? 1 : 0));
            start_task($value);
        } else {
            if ($var eq 'timeout') {
                $command_timeout = $value;
            }
        }
    }
}
process_command($command);

finish_tasks_to_level(0);

close($fh);

close($win);
close($rout);
close($rerr);

if (waitpid($pid, 0) > 0) {
    $pid = undef;
}

exit(sh_exit_status($?));


# Work around DBD::SQLite bug.
END {
    my $dbh = $dbh{dbh};
    %dbh = ();
    $dbh->disconnect;
}

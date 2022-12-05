# -*- cperl -*-
#
package Taskbot::Report;

use warnings;
use strict;

use base qw(Taskbot::Base);


use fields qw(args link_task_id root_task find_task find_task_by_root
              find_failed_task insert_report insert_to_report add_history
              get_stdout get_stderr prev_root_task files_list file_history
              get_file_content history_count);


sub new {
    my $self = shift;
    my ($config_file, $args, $link_task_id) = @_;

    unless (ref $self) {
        $self = $self->SUPER::new($config_file);
    }

    $self->{args} = $args;
    $self->{root_task} = _find_root($self->{dbh});
    $self->{link_task_id} = $link_task_id || $self->{root_task}->{id};
    $self->{history_count} = 0;

    $self->{find_task} = $self->{dbh}->prepare(q[
        SELECT id, description, is_command, start, finish, error_message
        FROM task
        WHERE parent_task_id = ? AND description LIKE ?
        ORDER BY id
    ]);

    $self->{find_task_by_root} = $self->{dbh}->prepare(q[
        SELECT id, description, is_command, start, finish, error_message
        FROM task
        WHERE root_task_id = ? AND description LIKE ?
        ORDER BY id
    ]);

    $self->{find_failed_task} = $self->{dbh}->prepare(q[
        SELECT id, description, is_command, start, finish, error_message
        FROM task
        WHERE parent_task_id = ? AND error_message IS NOT NULL
    ]);

    $self->{insert_report} = $self->{dbh}->prepare(q[
        INSERT INTO report (task_id, gzipped, html)
        VALUES (?, ?, ?)
    ]);

    $self->{insert_to_report} = $self->{dbh}->prepare(q[
        UPDATE report set gzipped = ?, html = ?
        WHERE id = ?
    ]);

    $self->{add_history} = $self->{dbh}->prepare(q[
        UPDATE history
        SET html_table_row = html_table_row||?
        WHERE task_id = ?
    ]);

    $self->{get_stdout} = $self->{dbh}->prepare(q[
        SELECT stdout_gzipped, stdout
        FROM command
        WHERE task_id = ?
    ]);

    $self->{get_stderr} = $self->{dbh}->prepare(q[
        SELECT stderr_gzipped, stderr
        FROM command
        WHERE task_id = ?
    ]);

    $self->{prev_root_task} = $self->{dbh}->prepare(q[
        SELECT id, description, is_command, start, finish, error_message
        FROM task
        WHERE id = (SELECT MAX(h.task_id)
                    FROM history h
                    JOIN task t ON h.task_id = t.id
                    WHERE h.task_id < ?
                    AND t.description = ?)]);

    $self->{files_list} = $self->{dbh}->prepare(q[
        SELECT id, name, fullpath, task_id
        FROM file
        WHERE task_id = ?]);

    $self->{file_history} = $self->{dbh}->prepare(q[
        SELECT f.id as id, 
               t.id as task_id,
               t.root_task_id as root_task_id,
               t.start as start,
               t.finish as finish
        FROM file f
        JOIN task t on t.id = f.task_id
        WHERE f.name = ? AND
              f.fullpath = ? AND
              f.task_id < ?
        ORDER BY t.id DESC
        LIMIT ?, ?]);

    $self->{get_file_content} = $self->{dbh}->prepare(q[
        SELECT content
        FROM file
        WHERE id = ?]);

    return $self;
}


sub _find_root {
    my ($dbh) = @_;

    my $root_task;
    ($root_task->{parent_task_id}) = $dbh->selectrow_array(q[
        SELECT MIN(task_id)
        FROM running_task
        WHERE host = ? AND pid = ?
    ], undef, $ENV{HOSTNAME}, $ENV{TASKBOT_PARENT_PID});

    my $task = $dbh->prepare(q[
        SELECT id, parent_task_id, description
        FROM task
        WHERE id = ?
    ]);

    do {
        $task->execute($root_task->{parent_task_id});
        $root_task = $task->fetchrow_hashref;
    } while (defined $root_task->{parent_task_id});

    return $root_task;
}


sub current_task_id {
    my $self = shift;

    my ($task_id) = $self->{dbh}->selectrow_array(q[
        SELECT MAX(task_id)
        FROM running_task
        WHERE host = ? AND pid = ?
    ], undef, $ENV{HOSTNAME}, $ENV{TASKBOT_PARENT_PID});

    return $task_id;
}

sub last_task_id_by_dsc {
    my $self = shift;
    my ($dsc) = @_;

    my $task = $self->{dbh}->prepare(q[
        SELECT id, parent_task_id, description
        FROM task
        WHERE id = (Select Max(id) FROM task 
                    WHERE description LIKE ?)
    ]);

    $task->execute($dsc);

    return $task->fetchrow_hashref;
}


sub get_args {
    my $self = shift;

    return $self->{args};
}


sub get_root_task {
    my $self = shift;

    return $self->{root_task};
}


sub find_task {
    my $self = shift;
    my ($path, @tasks) = @_;

    my $str_path = $path;
    my @path = (split(' > ', $path), '');

    unless (@tasks) {
        $self->{find_task_by_root}->execute($self->{root_task}->{id},
                                            shift @path);
        while (my $row = $self->{find_task_by_root}->fetchrow_hashref) {
            push @tasks, $row;
        }
    }

    my @task_queue = (undef, @tasks);

    my $desc;
    while (@path && @task_queue) {
        my $task = shift @task_queue;
        unless ($task) {
            $desc = shift @path;
            push @task_queue, undef;
            next;
        }

        if (ref $task ne "HASH")
        {
          print "ERROR: can't find task for $str_path path. Task is $task.\n";
        }

        my $id = $task->{id};
        $self->{find_task}->execute($id, $desc);
        while (my $row = $self->{find_task}->fetchrow_hashref) {
            push @task_queue, $row;
        }
    }

    pop @task_queue;
    return @task_queue;
}


sub find_failed {
    my $self = shift;
    my ($task) = @_;

    my $result;

    while (1) {
        $self->{find_failed_task}->execute($task->{id});
        $task = $self->{find_failed_task}->fetchrow_hashref;

        last unless $task && ! $self->{find_failed_task}->fetchrow_arrayref;

        $result = $task;
    } 

    return $result;
}

sub get_files_list {
  my $self = shift;
  my ($task) = @_;

  $self->{files_list}->execute($task->{id});

  my @files_list = @{$self->{files_list}->fetchall_arrayref({})};
  return @files_list;
}

sub get_file_content {
  my $self = shift;
  my ($file) = @_;

  $self->{get_file_content}->execute($file->{id});
  my ($content) = $self->{get_file_content}->fetchrow_array;
  return $content;
}

sub get_file_history {
  my $self = shift;
  my @file = @_;
  my ($name, $path, $task, $limit) =
    (ref $file[0] eq "HASH" ?
      ($file[0]->{name},
       $file[0]->{fullpath},
       $file[0]->{task_id},
       (scalar @file > 1 ? $file[1] : 100)) :
      @file);

  $limit = 100 unless $limit;

  $self->{file_history}->execute($name, $path, $task, 0, $limit);
  my @files_list = @{$self->{file_history}->fetchall_arrayref({})};
  return @files_list;
}

sub get_stdout {
    my $self = shift;
    my ($task) = @_;

    $self->{get_stdout}->execute($task->{id});
    my ($gzipped, $stdout) = $self->{get_stdout}->fetchrow_array;

    return "(not a command)" unless defined $stdout;

    if ($gzipped) {
        use Compress::Zlib;
        $stdout = Compress::Zlib::memGunzip($stdout);
    }

    return $stdout;
}


sub get_stderr {
    my $self = shift;
    my ($task) = @_;

    $self->{get_stderr}->execute($task->{id});
    my ($gzipped, $stderr) = $self->{get_stderr}->fetchrow_array;

    return "(not a command)" unless defined $stderr;

    if ($gzipped) {
        use Compress::Zlib;
        $stderr = Compress::Zlib::memGunzip($stderr);
    }

    return $stderr;
}


sub task_href {
    my $self = shift;
    my ($task) = @_;

    my $task_id = (ref $task eq 'HASH' ? $task->{id} : $task);

    return "?task=$task_id";
}


sub report_href {
    my $self = shift;
    my ($report_id) = @_;

    return "?report=$report_id";
}

sub file_href {
  my $self = shift;
  my ($file) = @_;
  my $file_id = $file;
  $file_id = $file->{id} if ref $file eq 'HASH';
  return "/taskbot/browse-file.cgi?id=$file_id";
}

sub files_href {
  my $self = shift;
  my ($task) = @_;
  my $task_id = (ref $task eq 'HASH' ? $task->{id} : $task);
  return "/taskbot/browse-file.cgi?task=$task_id";
}

sub test_doc_href {
    my $self = shift;
    my ($test_name, $commit, $repo) = @_;

    $repo = "" unless defined $repo;

    return "/docs/AutoTest/docs.cgi?commit=$commit&test=$test_name&repo=$repo"
}


sub commit_href {
    my $self = shift;
    my ($repo, $commit) = @_;

    return "/git/gitweb.cgi"
      . "?a=commitdiff;p=$repo/.git;h=$commit";
}

sub atcommons_doc_href {
    my $self = shift;
    open(my $fh, '<', "$ENV{TASKBOT_VAR}/revision/adserver/autotest-commons")
      or die "open($ENV{TASKBOT_VAR}/revision/adserver/autotest-commons): $!";
    my $revision = <$fh>;
    close($fh);
    return "/docs/AutoTest/docs.cgi?commit=$revision";
}

sub test_history_href {
  my $self = shift;
  my ($test, $run_name, $root_task) = @_;
  return "/taskbot/testhistory.cgi?test=$test;root_task=$root_task;run_name=$run_name";
}

sub qa_test_plan_href
{
  my $self = shift;
  my ($test_plan) = @_;
  return "https://confluence.ocslab.com/display/QA/$test_plan";
}

sub jira_href {
  return "https://jira.corp.foros.com/browse";
}

sub jira_codes {
  return [qw(ADSC ADDB ENVDEV REQ PXS UCS TDOC)];
}

sub db_dump_href {
    my $self = shift;
    my ($test) = @_;

    use URI::Escape;
    
    my $prefix_ = $ENV{TASKBOT_USER};

    if ( defined $ENV{RUN_NUMBER} )
    {
      $prefix_ .= "-$ENV{RUN_NUMBER}";
    }

    my ($db, $user, $password, $prefix) =
      map { uri_escape($_) }
        @ENV{qw(TASKBOT_DB_URL TASKBOT_DB_USER TASKBOT_DB_PASSWORD)},
          "$prefix_";

    return "/db/dump.cgi"
      . "?db=$db;user=$user;password=$password;namespace=UT"
      . ";prefix=$prefix;test=$test;action=Dump";
}


sub _compress_maybe {
    my $self = shift;
    my ($buf) = @_;

    if (length($buf) >= $self->{config}->{gzip_threshold}) {
        use Compress::Zlib;
        my $buf_gz = Compress::Zlib::memGzip($buf);
        if (length($buf_gz) <= length($buf) * $self->{config}->{gzip_ratio}) {
            return (1, $buf_gz);
        }
    }

    return (0, $buf);
}


sub add_report {
    my $self = shift;
    my ($html, $ref_to_src) = @_;

    my ($gzipped, $buffer) = $self->_compress_maybe($html);

    if (length($buffer) > 65535) # size of MySQL Text
    {
      $buffer = "Report size is too long (" . length($buffer) . ").<br>";
      $buffer .= "Source data you can find <a href=\"$ref_to_src\">here</a>.<br>"
        if $ref_to_src;
      $gzipped = 0;
    }

    $self->{insert_report}->execute($self->{root_task}->{id},
                                    $gzipped, $buffer);

    return $self->{dbh}->last_insert_id((undef) x 4);
}


sub add_to_report {
    my $self = shift;
    my ($report_id, $html) = @_;

    $self->{insert_to_report}->execute($self->_compress_maybe($html), $report_id);
}


sub add_history {
    my $self = shift;
    my ($color, $html) = @_;

    $html = "<td bgcolor=$color>$html</td>" if defined $color;

    unless ($self->{history_count}++) {
        eval {
            $self->{dbh}->do(q[
                INSERT INTO history (task_id, link_task_id, html_table_row)
                VALUES (?, ?, ?)
            ], undef, $self->{root_task}->{id}, $self->{link_task_id}, $html);
        };
        return unless $@;
    }
    $self->{add_history}->execute($html, $self->{root_task}->{id});
}


sub get_previous_run {
    my $self = shift;
    my ($task) = @_;

    $task = $self->{root_task} unless defined $task;

    $self->{prev_root_task}->execute(@$task{qw(id description)});
    return $self->{prev_root_task}->fetchrow_hashref;
}


sub generate {
    my $self = shift;
    my ($report_file) = @_;

    Taskbot::Base::do_file($report_file);

    Taskbot::Base::report($self);

    $self->{dbh}->commit;
}


1;

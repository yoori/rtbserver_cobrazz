
use warnings;
use strict;

{
  package EmailNotification;

  use constant SENDMAIL_CMD => "sendmail -t";
  use constant FAIL_STR     => "FAILED!!!";
  use constant OK_STR       => "OK";
  use constant REPLY_TO     => "server\@ocslab.com";
  
  sub result_2_str {
    my $result = shift;
    if ( ! $result )
    {
      return FAIL_STR;
    }
    return OK_STR;
  }

  sub users_2_str {
    my $users    = shift;
    my $user;
    my $buffer="";
    my $idx=0;
    foreach $user (sort @$users)
    {
      if ($buffer ne "")
      {
        $buffer.=", ";
      }
      $buffer.=$user;
    }
    return $buffer;
  }

  sub docs_2_content 
  {
    my $docs    = shift;
    my $buffer  = "";
    my $idx     = 0;
    my $doc;
    foreach $doc (sort @$docs)
    {
      $idx++;
      $buffer .= "$idx" . ". " .  "$doc\n";
    }
    return $buffer;
  }


  sub send_mail {
    my $users         = shift;
    my $doc_name      = shift;
    my $documents     = shift;
    my $test_result   = shift;
    my $user_list_len = scalar(@$users);
    my $result_str    = result_2_str($test_result);
    my $to_str        = users_2_str(\@$users);
    my $docs_list     = docs_2_content(\@$documents);
    if ($user_list_len > 0)
    {
      my $cmd       = SENDMAIL_CMD;
      open(SENDMAIL, "|$cmd" );
      print SENDMAIL "Reply-to: " . REPLY_TO . "\n";
      print SENDMAIL "Subject: Test on '$doc_name' - $result_str\n";
      print SENDMAIL "To: $to_str\n";
      print SENDMAIL "Content-type: text/plain\n\n";
      print SENDMAIL "Test results:\n$docs_list";
      close(SENDMAIL);
    }
  }
}

1;


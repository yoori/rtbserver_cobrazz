
package CCGKeywordUpdate::Case;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub new
{
  my $self = shift;
  my ($ns, $case_name, $args) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{case_name} = $case_name;
  $self->{ns} = $ns->sub_namespace($case_name);
  $self->create($args);

  return $self;
}

sub create
{
  my ($self, $args) = @_;

  my %args_copy = %$args;

  my $size = $self->{ns}->create(
    CreativeSize => { name => "Size" });

  $args_copy{name} = 'Campaign';
  $args_copy{template_id} = DB::Defaults::instance()->text_template;
  $args_copy{size_id} = $size;

  if (not defined $args->{original_keyword})
  {
    $args_copy{ccg_keyword_id} = undef;
  }
  else
  {
    $args_copy{original_keyword} = 
     substr($args->{original_keyword}, 0, 1) eq '-'? 
       "-" . make_autotest_name(
         $self->{ns}, substr($args->{original_keyword}, 0, 1)):
           make_autotest_name(
             $self->{ns}, $args->{original_keyword});
  }
    
  my $campaign = $self->{ns}->create(
     TextAdvertisingCampaign => \%args_copy);

  $self->{ns}->output("Account", $campaign->{account_id});
  $self->{ns}->output("CCG", $campaign->{ccg_id});
  if (defined $args->{original_keyword})
  {
    $self->{ns}->output("Channel", 
      $campaign->{CCGKeyword}->channel_id);
    $self->{ns}->output("KWD", 
      $args_copy{original_keyword});
    $self->{ns}->output("CCGKeyword", 
      $campaign->{CCGKeyword}->{ccg_keyword_id});
  }

  $self->{ns}->output("CLICK", $args->{ccgkeyword_click_url})
    if (defined $args->{ccgkeyword_click_url});
  $self->{ns}->output("MAXCPC", $args->{max_cpc_bid})
    if (defined $args->{max_cpc_bid});
}

1;


package CCGKeywordUpdate;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub add_ccgkeyword
{
  my ($self, $ns) = @_;

  my $case = new CCGKeywordUpdate::Case($ns, "ADD", {});

  $case->{ns}->output("CHANNEL_NAME",
    make_autotest_name($case->{ns}, 'Channel'));
  $case->{ns}->output("KWD",
    make_autotest_name($case->{ns}, 'KWD'));
}

sub activate_ccgkeyword
{
  my ($self, $ns) = @_;
  new CCGKeywordUpdate::Case(
    $ns, "ACTIVATE", 
    { ccgkeyword_status => 'D',
      original_keyword => 'KWD' } );
}

sub deactivate_ccgkeyword
{
  my ($self, $ns) = @_;
  new CCGKeywordUpdate::Case(
    $ns, "DEACTIVATE", 
    { original_keyword => 'KWD' } );
}

sub deactivate_channel
{
  my ($self, $ns) = @_;
  new CCGKeywordUpdate::Case(
    $ns, "DEACTIVATECHANNEL", 
    { original_keyword => 'KWD' } );
}

sub change_ccgkeyword
{
  my ($self, $ns) = @_;
  my $case = new CCGKeywordUpdate::Case(
    $ns, "CHANGE", 
    { original_keyword => '-KWD',
      ccgkeyword_click_url => 'http://www.CCGKeywordUpdate.com',
      max_cpc_bid => 10 } );

  $case->{ns}->output("NEW_CLICK", 'http://www.NewCCGKeywordUpdate.com');
  $case->{ns}->output("NEW_MAXCPC", 20);
  $case->{ns}->output("NEW_KWD",
    make_autotest_name($case->{ns}, 'NEWKWD'));
}

sub init {
  my ($self, $ns) = @_;

  $self->add_ccgkeyword($ns);
  $self->activate_ccgkeyword($ns);
  $self->deactivate_ccgkeyword($ns);
  $self->deactivate_channel($ns);
  $self->change_ccgkeyword($ns);
}

1;

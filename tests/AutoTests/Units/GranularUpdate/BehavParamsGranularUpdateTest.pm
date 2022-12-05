package BehavParamsGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub case_empty
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("EMPTY");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'empty',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "keywordEmpty") ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});

}

sub case_assign
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("ASSIGN");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'assign',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "KeywordAssign") ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});

}

sub case_change
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("CHANGE");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'change',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "KeywordChange"),
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        minimum_visits => 1,
        time_from => 0,
        time_to => 600),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        minimum_visits => 1,
        time_from => 0,
        time_to => 1200)]

  ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});
  $ns->output("PAGE", $ch->{behavioral_parameters}[0]->{behav_params_id});
  $ns->output("SEARCH", $ch->{behavioral_parameters}[1]->{behav_params_id});

}

sub case_remove
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("REMOVE");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'remove',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "KeywordRemove"),
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        minimum_visits => 1,
        time_from => 0,
        time_to => 1200),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S',
        minimum_visits => 1,
        time_from => 0,
        time_to => 1200)]

  ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});
  $ns->output("PAGE", $ch->{behavioral_parameters}->[0]->{behav_params_id});
  $ns->output("SEARCH", $ch->{behavioral_parameters}->[1]->{behav_params_id});

}

sub case_list
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("LIST");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'list',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "KeywordList"),
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        minimum_visits => 1,
        time_from => 0,
        time_to => 1200)]

  ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});
  $ns->output("PAGE", $ch->{behavioral_parameters}->[0]->{behav_params_id});

}

sub case_del
{
  my ($self, $namespace, $acc) = @_;
  my $ns = $namespace->sub_namespace("DEL");

  my $ch = $ns->create(DB::BehavioralChannel->blank(
    name => 'del',
    account_id => $acc,
    keyword_list => make_autotest_name($ns, "KeywordDel"),
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        minimum_visits => 1,
        time_from => 0,
        time_to => 1200)]

  ));

  $ns->output("CHANNEL", $ch->channel_id);
  $ns->output("KEYWORD_LIST", $ch->{keyword_list_});
  $ns->output("PAGE", $ch->{behavioral_parameters}->[0]->{behav_params_id});

}

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $bp_list_name = make_autotest_name($ns, "1");
  $ns->output("BPListName", $bp_list_name);

  $self->case_empty($ns, $acc);
  $self->case_assign($ns, $acc);
  $self->case_change($ns, $acc);
  $self->case_remove($ns, $acc);
  $self->case_list($ns, $acc);
  $self->case_del($ns, $acc);

}

1;

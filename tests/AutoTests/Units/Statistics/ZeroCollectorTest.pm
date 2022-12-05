
package ZeroCollectorTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;
  my $account = 
    $ns->create(Account => {
      name => 'Advertiser',
      role_id => DB::Defaults::instance()->advertiser_role });

  my $list = $ns->create(BehavioralParametersList => 
                         { name => 'List',
                           threshold => 100 });

  my $channel_gn = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel-GN',
      keyword_list => 
        make_autotest_name($ns, 'Kwd'),
      account_id => $account,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          behav_params_list_id => 
           $list->behav_params_list_id) ]));

  my $channel_ru = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel-RU',
      keyword_list => 
        make_autotest_name($ns, 'Kwd'),
      account_id => $account,
      country_code => "RU",
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          behav_params_list_id => 
           $list->behav_params_list_id) ]));

  my $keyword = make_autotest_name($ns, "kwd");
  my $trigger = $ns->create(DB::BehavioralChannel::Trigger->blank(
        trigger_type => 'K',
        normalized_trigger => 
          DB::BehavioralChannelBase::normalize_keyword_($keyword),
        qa_status => 'A',
        country_code => DB::Defaults::instance()->country()->{country_code},
        channel_type => 'D'));

  $ns->output('Account', $account);
  $ns->output('Prefix', $ns->namespace . '-Channel');
  $ns->output('Channel/RU', $channel_ru->channel_id());  
  $ns->output('Channel/GN', $channel_gn->channel_id());  
  $ns->output('Currency', DB::Defaults::instance()->currency());
  $ns->output('TriggerID', $trigger->trigger_id());
  $ns->output('KEYWORD', $keyword);
  $ns->output('List', $list);  
}

1;

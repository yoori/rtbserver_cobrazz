package ChannelTriggerImpStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init_case
{
  my ($ns, $advertiser, $case_name, $triggers_in_channel, $match_triggers) = @_;

  my $keyword_prefix = make_autotest_name($ns, "Keyword-${case_name}");

  my $keyword_list = '';
  for(my $i = 0; $i < 20; ++$i)
  {
    $keyword_list .= "$keyword_prefix-$i\n";
  }

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => $case_name,
    account_id => $advertiser,
    keyword_list => $keyword_list,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P',
        time_to => 3600) ]));

  my $campaign = $ns->create(DisplayCampaign => {
    name => $case_name,
    account_id => $advertiser,
    campaigncreativegroup_cpm => 100,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{ name => 'Site' }],
    channel_id => $channel
    });

  my $tag = $ns->create(PricedTag => {
    name => $case_name,
    site_id => $campaign->{Site}[0]->{site_id}});

  # replication of ChannelTrigger table isn't stable : and all channel triggers can't be
  #   selected at postgres side
  my $all_channel_trigger_ids = '';
  my $nonmatch_channel_trigger_ids = '';
  my $match_channel_trigger_ids = '';
  my $request_keywords = '';

  my $keyword_channel_triggers = $channel->keyword_channel_triggers();
  my $i = 0;

  foreach my $chtr_id(@$keyword_channel_triggers)
  {
    if(exists $match_triggers->{$i})
    {
      $match_channel_trigger_ids .= (length($match_channel_trigger_ids) > 0 ? "," : "") .
        $chtr_id->channel_trigger_id();

      $request_keywords .= (length($request_keywords) > 0 ? ',' : '') .
        $chtr_id->original_trigger();
    }
    else
    {
      $nonmatch_channel_trigger_ids .= (length($nonmatch_channel_trigger_ids) > 0 ? "," : "") .
        $chtr_id->channel_trigger_id();
    }

    $all_channel_trigger_ids .= (length($all_channel_trigger_ids) > 0 ? "," : "") .
      $chtr_id->channel_trigger_id();

    ++$i;
  }

  $ns->output("ChannelTriggerIds/$case_name", $all_channel_trigger_ids);
  $ns->output("MatchChannelTriggerIds/$case_name", $match_channel_trigger_ids);
  $ns->output("NonMatchChannelTriggerIds/$case_name", $nonmatch_channel_trigger_ids);
  $ns->output("Keyword/$case_name", $request_keywords);
  $ns->output("ChannelId/$case_name", $channel->{channel_id});
  $ns->output("CcId/$case_name", $campaign->{cc_id});
  $ns->output("Tid/$case_name", $tag);
}

sub channel_types
{
  my ($namespace, $advertiser) = @_;

  my $ns = $namespace->sub_namespace('Types');

  my $publisher = $ns->create(
    Publisher => { 
      name => 'Publisher',
      pricedtag_adjustment => 0.9 } );

 $ns->output("TAG", $publisher->{tag_id});

  my @channels = (
     #1
     [ 'DB::BehavioralChannel', ['P'] ],
     #2 
     [ 'DB::BehavioralChannel', ['P'] ],
     #3 
     [ 'DB::BehavioralChannel', ['P'] ],
     #4  
     [ 'DB::BehavioralChannel', ['P'] ],
     #5 
     [ 'DB::BehavioralChannel', ['P', 'R'] ],
     #6 
     [ 'DB::BehavioralChannel', ['S'], 'K' ],
     #7 
     [ 'DB::BehavioralChannel', ['S'], 'K' ],
     #8 
     [ 'DB::BehavioralChannel', ['S'], 'K' ],
     #9 
     [ 'DB::BehavioralChannel', ['R'] ] );

  my @chs = ();

  my $index = 0;
  for my $c (@channels)
  {
    my ($type, $types, $channel_type) = @$c;

    my $keyword = make_autotest_name($ns, 'kwd' . ++$index);
    
    my @params = map {
      DB::BehavioralChannel::BehavioralParameter->blank(
         time_to => 10*60,
         trigger_type => $_) } @$types;

    my %args = (
      name => "Channel" . $index,
      account_id => $advertiser,
      behavioral_parameters => \@params );

   if (defined $channel_type)
   {
     $args{channel_type} = $channel_type;
     $args{namespace} = $channel_type;
   }

   if (grep {$_ eq 'P'} @$types)
   {
     $args{keyword_list} = $keyword;
   }

   if (grep {$_ eq 'S'} @$types)
   {
     $args{search_list} = $keyword;
   }

   if (grep {$_ eq 'R'} @$types)
   {
     $args{url_kwd_list} = $keyword;
   }
 
    my $channel = $ns->create(
        $type->blank(%args));

    push @chs, $channel;

    $ns->output("Channel/$index", $channel);
    for my $t (@$types)
    {
      $ns->output("Channel/$index/$t", $channel->channel_id() . $t);
    }
    $ns->output("Keyword/$index", $keyword);
  }

  my $expression1 = 
    $ns->create(
      DB::ExpressionChannel->blank(
        name => 'Expression1',
        account_id => $advertiser,
        expression => 
          $chs[0]->channel_id . "|" .
          $chs[1]->channel_id . "|" .
          $chs[2]->channel_id . "|" .
          $chs[3]->channel_id . "|" .
          $chs[8]->channel_id ));

  my $expression2 = 
    $ns->create(
      DB::ExpressionChannel->blank(
        name => 'Expression2',
        account_id => $advertiser,
        expression => 
          $chs[0]->channel_id . "|" .
          $chs[1]->channel_id ));

  $ns->output("Expression/1", $expression1);
  $ns->output("Expression/2", $expression2);

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'DisplayCampaign',
    account_id => $advertiser,
    channel_id => $expression1,
    campaigncreativegroup_cpm => 10,
    site_links => [
      {site_id => $publisher->{site_id}} ] });  

  $ns->output("CC/Display", $campaign->{cc_id});
  
  my $text_campaign = $ns->create(
    TextAdvertisingCampaign => { 
      name => "TextCampaign",
      size_id => DB::Defaults::instance()->size(),
      creative_size_type_id => DB::Defaults::instance()->other_size_type,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_ctr => 0.01,
      ccg_keyword_id => undef,
      site_links => [ {site_id => $publisher->{site_id} }] 
});

  $ns->create(CCGKeyword => {
    name => "TextKWD-1",
    ccg_id => $text_campaign->{ccg_id},
    original_keyword => 
      $chs[5]->search_channel_triggers()->[0]->{original_trigger},
    max_cpc_bid => undef,
    channel_id => $chs[5] });


  $ns->create(CCGKeyword => {
    name => "TextKWD-2",
    ccg_id => $text_campaign->{ccg_id},
    original_keyword => 
       $chs[6]->search_channel_triggers()->[0]->{original_trigger},
    max_cpc_bid => undef,
    channel_id => $chs[6] });

  $ns->create(CCGKeyword => {
    name => "TextKWD-3",
    ccg_id => $text_campaign->{ccg_id},
    original_keyword => 
      $chs[7]->search_channel_triggers()->[0]->{original_trigger},
    max_cpc_bid => undef,
    channel_id => $chs[7] });

  $ns->output("CC/Text", $text_campaign->{cc_id});
}

sub init
{
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser'});

  init_case($ns, $advertiser, "OnlyImpsCase", 20, { 2 => 1 });
  init_case($ns, $advertiser, "OnlyImpsCaseTempo", 20, { 2 => 1 });
  channel_types($ns, $advertiser);
}

1;

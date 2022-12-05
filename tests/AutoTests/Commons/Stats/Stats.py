#! /usr/bin/env python
# $Id: Stats.py 180589 2016-05-11 14:56:01Z artem_nikitin $

from StatsBase import integer, float, daily, hourly
from StatsBase import boolean, string, number, pgdate, yesno, exclude
from StatsBase import sum, min, max, count
from StatsBase import Base, SubBase
from StatsBase import Stats, process_stats

statsHeader = 'StatsList.hpp'
statsSource = 'StatsList.cpp'

BasicStats = Base('BasicStats')
BasicDiffStats = Base('DiffStats')
BasicChannelStats = Base('BasicChannelStats')

MaxLastAppearanceDiff = 32

def pg_user_stats_control_sum(date_field, count_field):
  return \
"""SUM(
 ((%s - to_date('01-01-1970', 'DD.MM.YYYY')) * (%d + 1) + 
 least(%s - nullif(last_appearance_date, '-infinity'), %d))
 * %s)""" % \
(date_field, MaxLastAppearanceDiff, date_field,MaxLastAppearanceDiff, count_field)

def pg_negative_control_sum(count_field):
  return \
"""SUM(CASE WHEN %s < 0 THEN %s ELSE 0 END)""" % (count_field, count_field)

Stats('ChannelInventoryStats',
      'ChannelInventory', BasicDiffStats, 
      [number('channel_id'),
       number('colo_id'),
       pgdate('sdate')]).fields = [
  sum('active_users', 'active_user_count'),
  sum('total_users', 'total_user_count'),
  sum('sum_ecpm', 'sum_ecpm'),
  sum('hits', 'hits'),
  sum('hits_urls', 'hits_urls'),
  sum('hits_kws', 'hits_kws'),
  sum('hits_search_kws', 'hits_search_kws')
]

Stats('ChannelInventoryByCPMStats',
      'ChannelInventoryByCpm', BasicDiffStats,
      [number('channel_id'),
       pgdate('sdate'),
       number('creative_size_id'),
       number('colo_id'),
       string('country_code'),
       number('ecpm', format = float)]).fields = [
  sum('user_count'),
  sum('impops')
]

Stats('ChannelOverlapUserStats',
      'ChannelOverlapUserStats', BasicDiffStats,
      [number('channel1', 'channel1_id'),
       number('channel2', 'channel2_id'),
       pgdate('sdate', format = daily)]).fields = [
  sum('users', 'unique_users')]

# AdvertiserUserStats
Stats('AdvertiserUserStats',
      'AdvertiserUserStats', BasicDiffStats,
      [number('adv_account_id'),
       pgdate('last_appearance_date'),
       pgdate('adv_sdate')]).fields = [
  sum('unique_users', 'unique_users'),
  sum('text_unique_users', 'text_unique_users'),
  sum('display_unique_users', 'display_unique_users'),
  number('control_sum',  pg_user_stats_control_sum('adv_sdate', 'unique_users') )]

# CampaignUserStats
Stats('CampaignUserStats',
      'CampaignUserStats', BasicDiffStats,
      [number('campaign_id'),
       number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('adv_sdate')]).fields = [
  sum('unique_users', 'unique_users'),
  number('control_sum', pg_user_stats_control_sum('adv_sdate', 'unique_users'))]

# CCGUserStats
Stats('CCGUserStats',
      'CCGUserStats', BasicDiffStats,
      [number('ccg_id'),
       number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('adv_sdate')]).fields = [
  sum('unique_users', 'unique_users'),
  number('control_sum', pg_user_stats_control_sum('adv_sdate', 'unique_users'))]

# CCUserStats
Stats('CCUserStats',
      'CCUserStats', BasicDiffStats,
      [number('cc_id'),
       number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('adv_sdate')]).fields = [
  sum('unique_users', 'unique_users'),
  number('control_sum', pg_user_stats_control_sum('adv_sdate', 'unique_users'))]

Stats('SiteUserStats',
      'SiteUserStats', BasicDiffStats,
      [number('site_id'),
       number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('isp_sdate')]).fields = [
  sum('unique_users', 'unique_users'),
  number('control_sum', pg_user_stats_control_sum('isp_sdate', 'unique_users'))]

Stats('HourlyStats',
      ['RequestStatsHourly', 'RequestStatsHourlyTest'], BasicDiffStats,
      [number('cc_id'),
       number('tag_id'),
       number('colo_id'),
       number('adv_account_id'),
       number('pub_account_id'),
       number('isp_account_id'),
       number('num_shown'),
       boolean('fraud_correction'),
       pgdate('stimestamp', format = hourly)]).fields = [
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('actions', 'actions'),
  sum('requests', 'requests'),
  sum('pub_amount', 'pub_amount'),
  sum('pub_comm_amount', 'pub_comm_amount'),
  sum('adv_amount', 'adv_amount'),
  sum('adv_comm_amount', 'adv_comm_amount'),
  sum('isp_amount', 'isp_amount')]

# ChannelImpInventory
Stats('ChannelImpInventory',
      'ChannelImpInventory', BasicDiffStats,
      [number('channel_id'),
       string('ccg_type'),
       number('colo_id'),
       pgdate('sdate')]).fields = [
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('actions', 'actions'),
  sum('revenue', 'revenue'),
  sum('impops_user_count', 'impops_user_count'),
  sum('imps_user_count', 'imps_user_count'),
  sum('imps_value', 'imps_value'),
  sum('imps_other', 'imps_other'),
  sum('imps_other_user_count', 'imps_other_user_count'),
  sum('imps_other_value', 'imps_other_value'),
  sum('impops_no_imp', 'impops_no_imp'),
  sum('impops_no_imp_user_count','impops_no_imp_user_count'),
  sum('impops_no_imp_value', 'impops_no_imp_value')]

# UserProperties
Stats('UserPropertiesStats',
      """UserPropertyStatsHourly stat
Left join userproperty up on (up.user_property_id = stat.user_property_id)""",
      BasicDiffStats,
      [string('name'),
       string('value'),
       string('user_status'),
       number('colo_id'),
       pgdate('stimestamp', format = hourly),
       pgdate('sdate', format = hourly),
       number('hour')]).fields = [
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('actions', 'actions'),
  sum('requests', 'requests'),
  sum('imps_unverified', 'imps_unverified'),
  sum('profiling_requests', 'profiling_requests')]

# CMPRequestStatsHourly
Stats('CMPRequestStats',
      'CMPRequestStatsHourly',
      BasicDiffStats,
      [pgdate('sdate', format = hourly),
       pgdate('adv_sdate', format = daily),
       pgdate('cmp_sdate', format = daily),
       number('adv_account_id'),
       number('cmp_account_id'),
       number('ccg_id'),
       number('colo_id'),
       number('channel_id'),
       number('channel_rate_id'),
       number('currency_exchange_id'),
       boolean('fraud_correction'),
       number('cc_id'),
       string('country_code'),
       boolean('walled_garden')]).fields = [
  sum('imps', 'imps'),
  sum('adv_amount_cmp', 'adv_amount_cmp'),
  sum('cmp_amount', 'cmp_amount'),
  sum('cmp_amount_global', 'cmp_amount_global'),
  sum('clicks', 'clicks')]

# PageLoadsDaily
Stats('PageLoadsDaily',
      'PageLoadsDaily',
      BasicDiffStats,
      [pgdate('country_sdate', format = daily),
       number('colo_id'),
       number('site_id'),
       string('country_code'),
       string('tag_group')]).fields = [
  sum('page_loads', 'page_loads'),
  min('page_loads_min', 'page_loads'),
  max('page_loads_max', 'page_loads'),
  sum('utilized_page_loads', 'utilized_page_loads'),
  min('utilized_page_loads_min', 'utilized_page_loads'),
  max('utilized_page_loads_max', 'utilized_page_loads')]

# ExpressionPerformance
Stats('ExpressionPerformanceStats',
      'ExpressionPerformance',
      BasicDiffStats,
      [pgdate('sdate', format = daily),
       number('cc_id'),
       number('colo_id'),
       string('expression')]).fields = [
  sum('imps', 'imps_verified'),
  sum('clicks', 'clicks'),
  sum('actions', 'actions')]

# PublisherInventory
Stats('PublisherInventory',
      'PublisherInventory',
      BasicDiffStats,
      [pgdate('pub_sdate', format = daily),
       number('tag_id'),
       number('cpm', format = float)]).fields = [
sum('imps', 'imps'),
sum('requests', 'requests'),
sum('revenue', 'revenue')]

Stats('ColoUserStats',
      'ColoUserStats', BasicDiffStats,
      [number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('isp_sdate')]).fields = [
  sum('unique_users'),
  sum('network_unique_users'),
  sum('profiling_unique_users'),
  sum('unique_hids'),
  number('control_unique_users', pg_user_stats_control_sum('isp_sdate', 'unique_users')),
  number('control_network_unique_users', pg_user_stats_control_sum('isp_sdate', 'network_unique_users')),
  number('control_profiling_unique_users', pg_user_stats_control_sum('isp_sdate', 'profiling_unique_users')),
  number('control_unique_hids', pg_user_stats_control_sum('isp_sdate', 'unique_hids')),
  number('neg_control_unique_users', pg_negative_control_sum('unique_users')),
  number('neg_control_network_unique_users', pg_negative_control_sum('network_unique_users')),
  number('neg_control_profiling_unique_users', pg_negative_control_sum('profiling_unique_users')),
  number('neg_control_unique_hids', pg_negative_control_sum('unique_hids'))]

Stats('CreatedUserStats',
      'CreatedUserStats', BasicDiffStats,
      [number('colo_id'),
       pgdate('last_appearance_date'),
       pgdate('create_date'),
       pgdate('isp_sdate')]).fields = [
  sum('unique_users'),
  sum('network_unique_users'),
  sum('profiling_unique_users'),
  sum('unique_hids'),
  number('control_unique_users_1', pg_user_stats_control_sum('isp_sdate', 'unique_users')),
  number('control_unique_users_2', pg_user_stats_control_sum('create_date', 'unique_users')),
  number('control_network_unique_users_1', pg_user_stats_control_sum('isp_sdate', 'network_unique_users')),
  number('control_network_unique_users_2', pg_user_stats_control_sum('create_date', 'network_unique_users')),
  number('control_profiling_unique_users_1', pg_user_stats_control_sum('isp_sdate', 'profiling_unique_users')),
  number('control_profiling_unique_users_2', pg_user_stats_control_sum('create_date', 'profiling_unique_users')),
  number('control_unique_hids_1', pg_user_stats_control_sum('isp_sdate', 'unique_hids')),
  number('control_unique_hids_2', pg_user_stats_control_sum('create_date', 'unique_hids')),
  number('neg_control_unique_users', pg_negative_control_sum('unique_users')),
  number('neg_control_network_unique_users', pg_negative_control_sum('network_unique_users')),
  number('neg_control_profiling_unique_users', pg_negative_control_sum('profiling_unique_users')),
  number('neg_control_unique_hids', pg_negative_control_sum('unique_hids'))]

Stats('GlobalColoUserStats',
      'GlobalColoUserStats', BasicDiffStats,
      [number('colo_id'),
       pgdate('create_date'),
       pgdate('last_appearance_date'),
       pgdate('global_sdate')]).fields = [
  sum('unique_users'),
  sum('network_unique_users'),
  sum('profiling_unique_users'),
  sum('unique_hids'),
  number('control_unique_users_1', pg_user_stats_control_sum('global_sdate', 'unique_users')),
  number('control_unique_users_2', pg_user_stats_control_sum('create_date', 'unique_users')),
  number('control_network_unique_users_1', pg_user_stats_control_sum('global_sdate', 'network_unique_users')),
  number('control_network_unique_users_2', pg_user_stats_control_sum('create_date', 'network_unique_users')),
  number('control_profiling_unique_users_1', pg_user_stats_control_sum('global_sdate', 'profiling_unique_users')),
  number('control_profiling_unique_users_2', pg_user_stats_control_sum('create_date', 'profiling_unique_users')),
  number('control_unique_hids_1', pg_user_stats_control_sum('global_sdate', 'unique_hids')),
  number('control_unique_hids_2', pg_user_stats_control_sum('create_date', 'unique_hids')),
  number('neg_control_unique_users', pg_negative_control_sum('unique_users')),
  number('neg_control_network_unique_users', pg_negative_control_sum('network_unique_users')),
  number('neg_control_profiling_unique_users', pg_negative_control_sum('profiling_unique_users')),
  number('neg_control_unique_hids', pg_negative_control_sum('unique_hids'))]

# ChannelInventoryEstimStats
Stats('ChannelInventoryEstimStats',
      'ChannelInventoryEstimStats',
      BasicDiffStats,
      [pgdate('sdate', format = daily),
       number('colo_id'),
       number('channel_id'),
       number('match_level', format = float)]).fields = [
sum('users_regular', 'users_regular'),
sum('users_from_now', 'users_from_now')]

# WebStats
Stats('WebStats',
      """WebStats stat
  Left join weboperation wbo on (wbo.web_operation_id = stat.web_operation_id)
  Left join userproperty upo on (upo.name = 'OsVersion' and stat.os_property_id = upo.user_property_id)
  Left join userproperty upb on (upb.name = 'BrowserVersion' and stat.browser_property_id = upb.user_property_id)""",
      BasicDiffStats,
      [pgdate('stimestamp', format = hourly),
       pgdate('country_sdate', format = daily),
       number('colo_id'),
       string('ct'),
       string('curct'),
       string('browser', 'upb.value'),
       string('os', 'upo.value'),
       string('app', 'wbo.app'),
       string('source', 'wbo.source'),
       string('operation', 'wbo.operation'),
       string('result'),
       string('user_status'),
       boolean('test'),
       number('cc_id'),
       number('tag_id')]).fields = [sum('count', 'count')]

# OptOutStats
Stats('OptOutStats',
      'OptOutStats',
      BasicDiffStats,
      [pgdate('isp_sdate', format = hourly),
       number('colo_id'),
       string('referer'),
       string('operation'),
       number('status'),
       string('test')]).fields = [sum('count', 'count')]

# PassbackStats
Stats('PassbackStats',
      'PassbackStats',
      BasicDiffStats,
      [pgdate('sdate', format = hourly),
       number('tag_id'),
       number('colo_id')]).fields = [sum('requests', 'requests')]

# ChannelPerformance
Stats('ChannelPerformance',
      'ChannelUsageStatsTotal',
      BasicDiffStats,
      [number('channel_id'),
       pgdate('last_use', format = daily, do_not_init = True)]).fields =[
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('actions', 'actions'),
  sum('revenue', 'revenue') ]

# CCGKeywordStatsHourly
Stats('CCGKeywordStatsHourly',
      'CCGKeywordStatsHourly',
      BasicDiffStats,
      [pgdate('sdate', format = hourly),
       pgdate('adv_sdate', format = daily),
       number('ccg_keyword_id'),
       number('currency_exchange_id'),
       number('colo_id'),
       number('cc_id')]).fields =[
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('adv_amount', 'adv_amount'),
  sum('adv_comm_amount', 'adv_comm_amount'),
  sum('pub_amount_adv', 'pub_amount_adv') ]

# CCGKeywordStatsTotal
Stats('CCGKeywordStatsTotal',
      'CCGKeywordStatsTotal',
      BasicDiffStats,
      [number('ccg_keyword_id'),
       number('cc_id')]).fields =[
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('adv_amount', 'adv_amount'),
  sum('adv_comm_amount', 'adv_comm_amount'),
  sum('pub_amount_adv', 'pub_amount_adv') ]

# CCGKeywordStatsDaily
Stats('CCGKeywordStatsDaily',
      'CCGKeywordStatsDaily',
      BasicDiffStats,
      [pgdate('adv_sdate', format = daily),
       number('ccg_keyword_id'),
       number('cc_id')]).fields =[
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('adv_amount', 'adv_amount'),
  sum('adv_comm_amount', 'adv_comm_amount'),
  sum('pub_amount_adv', 'pub_amount_adv') ]

# CCGKeywordStatsToW
Stats('CCGKeywordStatsToW',
      'CCGKeywordStatsToW',
      BasicDiffStats,
      [number('day_of_week'),
       number('hour'),
       number('ccg_keyword_id'),
       number('cc_id')]).fields =[
  sum('imps', 'imps'),
  sum('clicks', 'clicks'),
  sum('adv_amount', 'adv_amount'),
  sum('adv_comm_amount', 'adv_comm_amount'),
  sum('pub_amount_adv', 'pub_amount_adv') ]

# ActionStats
Stats('ActionStats',
      'ActionStats',
      BasicDiffStats,
      [number('action_request_id'),
       number('action_id'),
       number('cc_id'),
       number('tag_id'),
       number('colo_id'),
       string('country_code'),
       string('action_referrer_url'),
       pgdate('action_date', format = hourly),
       pgdate('imp_date', format = hourly),
       pgdate('click_date', format = hourly),
       number('cur_value', format = float),
       string('order_id'),
       ]).fields =[ count() ]

# ChannelUsageStats
Stats('ChannelUsageStats',
      'ChannelUsageStatsHourly',
      BasicDiffStats,
      [number('channel_id'),
       number('colo_id'),
       number('ccg_id'),
       pgdate('sdate', format = hourly)]).fields = [
    sum('imps'),
    sum('clicks'),
    sum('actions'),
    sum('revenue') ]

# ChannelIdBasedStats
Stats('ChannelIdBasedStats', [
  Stats('ChannelImpInventory', 'ChannelImpInventory', BasicDiffStats, fields = [
    sum('imps'),
    sum('clicks'),
    sum('actions'),
    sum('revenue'),
    sum('impops_user_count'),
    sum('imps_user_count'),
    sum('imps_value'),
    sum('imps_other'),
    sum('imps_other_user_count'),
    sum('imps_other_value'),
    sum('impops_no_imp'),
    sum('impops_no_imp_user_count'),
    sum('impops_no_imp_value')]),
  Stats('ChannelInventory', 'ChannelInventory',  BasicDiffStats, fields = [
    sum('sum_ecpm'),
    sum('active_user_count'),
    sum('total_user_count'),
    sum('hits'),
    sum('hits_urls'),
    sum('hits_kws'),
    sum('hits_search_kws'),
    sum('hits_url_kws')]),
  Stats('ChannelInventoryByCpm', 'ChannelInventoryByCpm', BasicDiffStats, fields = [
    sum('user_count')])
  ],
  BasicDiffStats,
  [number('channel_id'),
   exclude('exclude_colo', 'colo_id')])

Stats('ColoIdBasedStats', [
  Stats('ActionRequests', 'ActionRequests', BasicDiffStats, fields = [
    sum('action_request_count')]),
  Stats('ActionStats', 'ActionStats',  BasicDiffStats, fields = [
    count('action_request_id')]),
  Stats('CCGKeywordStatsHourly', 'CCGKeywordStatsHourly', BasicDiffStats, fields = [
    sum('imps'),
    sum('clicks'),
    sum('adv_amount'),
    sum('adv_comm_amount'),
    sum('pub_amount_adv')]),
  Stats('CCGUserStats', 'CCGUserStats', BasicDiffStats, fields = [
    sum('unique_users')]),
  Stats('CMPRequestStatsHourly', 'CMPRequestStatsHourly', BasicDiffStats, fields = [
    sum('imps'),
    sum('adv_amount_cmp'),
    sum('cmp_amount'),
    sum('cmp_amount_global'),
    sum('clicks')]),
  Stats('CampaignUserStats', 'CampaignUserStats', BasicDiffStats, fields = [
    sum('unique_users')]),
  Stats('CCUserStats', 'CCUserStats', BasicDiffStats, fields = [
    sum('unique_users')]),
  Stats('ChannelTriggerStats', 'ChannelTriggerStats', BasicDiffStats, fields = [
    sum('hits')]),
  Stats('ChannelUsageStats', 'ChannelUsageStatsHourly', BasicDiffStats, fields = [
    sum('imps'),
    sum('clicks'),
    sum('actions'),
    sum('revenue')]),
  Stats('ColoUserStats', 'ColoUserStats', BasicDiffStats, fields = [
    sum('unique_users'),
    sum('network_unique_users')]),
  Stats('ExpressionPerformance', 'ExpressionPerformance', BasicDiffStats, fields = [
    sum('imps_verified'),
    sum('clicks'),
    sum('actions')]),
  Stats('GlobalColoUserStats', 'GlobalColoUserStats', BasicDiffStats, fields = [
    sum('unique_users'),
    sum('network_unique_users')]),
  Stats('PassbackStats', 'PassbackStats', BasicDiffStats, fields = [
    sum('requests')]),
  Stats('RequestStatsHourly', 'RequestStatsHourly', BasicDiffStats, fields = [
    sum('imps'),
    sum('requests'),
    sum('clicks'),
    sum('actions'),
    sum('adv_amount'),
    sum('adv_amount_global'),
    sum('pub_amount'),
    sum('pub_amount_global'),
    sum('isp_amount'),
    sum('isp_amount_global'),
    sum('adv_comm_amount'),
    sum('pub_comm_amount'),
    sum('adv_comm_amount_global'),
    sum('pub_comm_amount_global'),
    sum('adv_invoice_comm_amount'),
    sum('passbacks')]),
  Stats('SiteChannelStats', 'SiteChannelStats', BasicDiffStats, fields = [
    sum('imps'),
    sum('adv_revenue'),
    sum('pub_revenue')]),
  Stats('UserPropertyStatsHourly', 'UserPropertyStatsHourly', BasicDiffStats, fields = [
    sum('requests'),
    sum('imps'),
    sum('clicks'),
    sum('actions'),
    sum('imps_unverified'),
    sum('profiling_requests')]),
  Stats('TagAuctionStats', 'TagAuctionStats', BasicDiffStats, fields = [
    sum('requests')])
  ],
  BasicDiffStats,
  [number('colo_id')])

Stats('SiteIdBasedStats', [
  Stats('SiteUserStats', 'SiteUserStats', BasicDiffStats, fields = [
    sum('unique_users')]),
  Stats('PageLoadsDaily', 'PageLoadsDaily', BasicDiffStats, fields = [
    sum('page_loads'),
    sum('utilized_page_loads')])],
  BasicDiffStats,
  [number('site_id'),
   exclude('exclude_colo', 'colo_id')])

Stats('TagIdBasedStats', [
  Stats('TagAuctionStats', 'TagAuctionStats', BasicDiffStats, fields = [
    sum('requests')]),
  Stats('PublisherInventory', 'PublisherInventory', BasicDiffStats, fields = [
    sum('imps'),
    sum('requests'),
    sum('revenue')], keys = [number('tag_id')])],
  BasicDiffStats,
  [number('tag_id'),
   exclude('exclude_colo', 'colo_id')])

# ChannelCountStats
Stats('ChannelCountStats',
      'ChannelCountStats',
      BasicDiffStats,
      [ number('colo_id'),
        string('channel_type'),
        number('channel_count'),
        pgdate('isp_sdate', format = daily)]).fields = [
    sum('users_count') ]

# AdvertiserStatsDaily
Stats('AdvertiserStatsDaily',
      'AdvertiserStatsDaily',
      BasicDiffStats,
      [ pgdate('adv_sdate', format = daily),
        number('cc_id'),
        number('creative_id'),
        number('campaign_id'),
        number('ccg_id'),
        number('adv_account_id'),
        number('agency_account_id'),
        string('country_code'),
        boolean('walled_garden')]).fields = [
  sum('requests'),
  sum('imps'),
  sum('clicks'),
  sum('actions'),
  sum('adv_amount'),
  sum('adv_comm_amount'),
  sum('pub_amount_adv'),
  sum('pub_comm_amount_adv'),
  sum('adv_amount_cmp')]

# PublisherStatsDaily
Stats('PublisherStatsDaily',
      'PublisherStatsDaily',
      BasicDiffStats,
      [ pgdate('pub_sdate', format = daily),
        number('pub_account_id'),
        number('site_id'),
        number('size_id'),
        number('tag_id'),
        number('num_shown'),
        string('country_code'),
        boolean('walled_garden')]).fields = [
  sum('requests'),
  sum('imps'),
  sum('clicks'),
  sum('actions'),
  sum('pub_amount'),
  sum('pub_comm_amount'),
  sum('pub_credited_imps')]

# ActionRequests
Stats('ActionRequests',
      'ActionRequests',
      BasicDiffStats,
      [ number('action_id'),
        number('colo_id'),
        string('country_code'),
        pgdate('action_date', format = hourly),
        string('action_referrer_url'),
        string('user_status')]).fields = [
  sum('count', 'action_request_count'),
  sum('cur_value')]

# SiteChannelStats
Stats('SiteChannelStats',
      'SiteChannelStats',
      BasicDiffStats,
      [ pgdate('sdate', format = daily),
        number('tag_id'),
        number('channel_id'),
        number('colo_id')]).fields = [
  sum('imps'),
  sum('adv_revenue'),
  sum('pub_revenue')]

# ChannelTriggerStats
Stats('ChannelTriggerStats',
      'ChannelTriggerStats',
      BasicDiffStats,
      [ pgdate('sdate', format = daily),
        number('colo_id'),
        number('channel_id'),
        number('channel_trigger_id'),
        string('trigger_type')]).fields = [
  sum('hits'),
  sum('imps', 'approximated_imps'),
  sum('clicks', 'approximated_clicks')]

# CcgStatsDaily
Stats('CcgAuctionStatsDaily',
      'CcgAuctionStatsDaily',
      BasicDiffStats,
      [ pgdate('adv_sdate', format = daily),
        number('colo_id'),
        number('ccg_id')]).fields = [
  sum('auctions_lost')]
  
process_stats()

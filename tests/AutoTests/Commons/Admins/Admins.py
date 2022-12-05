#! /usr/bin/env python
# $Id: Admins.py 182371 2017-03-29 07:45:40Z jurij_kuznecov $

from AdminsBase import Cmd, Admin, process_admins
from AdminsBase import number, unumber, string, field
from AdminsBase import flag, money, precision, string_list

admHeader = 'AdminsList.hpp'
admSource = 'AdminsList.cpp'

########## Fields

# field functions return field for expected whis is not present in admin output
#  parameters are name, type, default value
# other fields:
#  number  = unsigned long
#  unumber = unsigned int
#  string  =  std::string
#  flag = bool as presence without value
#        flag('name_of_param', 'flag's_string_to_use', default = 'true or false if need')

########## Constructors

# field(parameters) construction
# (name, getter = False, default = None, switch = None, may_empty = False)
# name = field name
# getter = string to use as cmd parameter
#    or boolean: True = getter is empty, False = field is free variable
#    (get, set)
# default = default value
# switch  = what field to use to swith field name (field name, name to switch)
# may_empty = this field can have no value - empty string

# Cmd construction ( admin = '', aspect = '', fields = [], modificators = [],
#     skip = 0, options = []):
# admin = admin name (enum value)
#   if tuple - first = enum value, second = enum name
#   for dinamic selection in construction, second is default value
# aspect = not valued parameter
# fields = parameters to construct admin command (valued)
# modificators = additional aspects to construct with
#   (will be present in constructor)
#   for dinamic selection in construction
# skip = how many lines skip from admin output
# options = additional options (aspects will not be present in constructor)

########### Admins

# @class CampaignAdmin
# @brief CampaignAdmin with aspect 'campaign'.
# Examine loaded by AdServer campaigns.
# CampaignAdmin command call string:
# CampaignAdmin show campaign=$ccg_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('CampaignAdmin',
      Cmd('CampaignManager', 'campaign',
          [number('campaign', 'id=')],
          ['--expand']),
      checker_name = 'CampaignChecker').fields = [
  number('id'),
  number('account_id'),
  string('account_currency_id'),
  string('campaign_group_name'),
  string('channels'),
  money('ecpm'),
  string('delivery_threshold'),
  string('status', True),
  string('eval_status', True),
  string('timestamp'),
  string('exclude_pub_accounts'),
  string('exclude_tags'),
  string('date_start'),
  string('date_end'),
  money('budget'),
  string('daily_budget'),
  string('cmp_date_start'),
  string('cmp_date_end'),
  string('max_pub_share'),
  string('channel'),
  string('flags'),
  string('country'),
  string_list('sites'),
  field('force_remote_present', 'bool', 'false')
]


# @class AccountAdmin
# @brief CampaignAdmin with aspect 'account'.
# Examine loaded by AdServer accountss.
# CampaignAdmin command call string:
# CampaignAdmin account account_id=$account_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('AccountAdmin',
      Cmd('CampaignManager', 'account',
          [number('account', 'account_id=')], []),
      checker_name = 'AccountChecker').fields = [
  number('account_id'),
  number('agency_account_id'),
  number('internal_account_id'),
  string('status', True),
  string('eval_status', True),
  string('flags'),
  string('at_flags'),
  string('text_adserving'),
  number('currency_id'),
  string('country'),
  money('commision'),
  money('budget'),
  money('paid_amount'),
  string('time_offset'),
  string('walled_garden_accounts'),
  string('auction_rate'),
  string('use_pub_pixels'),
  string('pub_pixel_optin'),
  string('pub_pixel_optout'),
  string('timestamp'),
  field('force_remote_present', 'bool', 'false')]

# @class StatAccountAdmin
# @brief CampaignAdmin with aspect 'stat-account'.
# CampaignAdmin call string:
# CampaignAdmin stat-account server=corbaloc::$campaign_server_host:$campaign_server_port/CampaignServer

Admin('StatAccountAdmin',
      Cmd('CampaignServer', 'stat_account',
          [number('id', 'id=')]),
      checker_name = 'StatAccountChecker').fields = [
  number('account_id'),
  money('amount'),
  money('comm_amount'),
  money('daily_amount'),
  money('daily_comm_amount')
]

# @class CreativeAdmin
# @brief CampaignAdmin with aspect 'creative'.
# Examine loaded by AdServer creatives.
# CampaignAdmin command call string:
# CampaignAdmin show creative=$cc_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('CreativeAdmin',
      Cmd('CampaignManager', 'creative',
          [number('ccid', 'ccid=')]),
      checker_name='CreativeChecker').fields = [
  number('ccid'),
  number('campaign_id'),
  string('creative_format'),
  string('sizes'),
  string('categories'),
  string('weight'),
  string('status', True)
]

# @class ActionAdmin
# @brief CampaignAdmin with aspect 'action_adv'.
# Examine loaded actions.
# CampaignAdmin call string:
# CampaignAdmin show adv-action=$action_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('ActionAdmin',
      Cmd('CampaignManager', 'adv_action',
          [number('action', 'action_id=')]),
      checker_name='ActionChecker').fields = [
  number('action_id'),
  string('timestamp'),
  string('ccg_ids')
]

# @class FreqCapAdmin
# @brief CampaignAdmin with aspect 'freq-cap'.
# Examines frequency caps objects.
# CampaignAdmin call string:
# CampaignAdmin show freq-cap=$freq_cap_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('FreqCapAdmin',
      Cmd('CampaignManager', 'freq_cap',
          [number('freqcap', 'id=')]),
      checker_name='FreqCapChecker').fields = [
  number('id'),
  string('window_time'),
  string('window_limit')
]

# @class ColocationAdmin
# @brief CampaignAdmin with aspect 'colocations'.
# Examine loaded by AdServer colocations.
# CampaignAdmin call string:
# CampaignAdmin show colocations colo_id=$colo_id manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('ColocationAdmin',
      Cmd('CampaignManager', 'colocations',
          [number('colo', 'colo_id=')]),
      checker_name='ColocationChecker').fields = [
  number('colo_id'),
  number('colo_rate_id'),
  number('account_id'),
  number('revenue_share')
]

# @class GlobalsAdmin
# @brief CampaignAdmin with aspect 'globals'.
# Examine globals AdServer parameters.
# CampaignAdmin call string:
# CampaignAdmin show globals manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('GlobalsAdmin',
      Cmd('CampaignManager', 'globals'),
      checker_name='GlobalsChecker').fields = [
  number('currency_exchange_id'),
  number('foros_min_margin', True),
  number('foros_min_fixed_margin', True),
  string('global_params_timestamp', True),
  string('master_stamp', True)
]

# @class CreativeTemplatesAdmin
# @brief CampaignAdmin with aspect 'creative_templates'.
# Examine crative templates.
# CampaignAdmin call string:
# CampaignAdmin creative_templates manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('CreativeTemplatesAdmin',
      Cmd('CampaignManager', 'creative_templates'),
      checker_name = 'CreativeTemplatesChecker').fields = [
  string('creative_format'),
  string('creative_size'),
  string('app_format'),
  string('track_impression'),
  string('template_file'),
  string('type'),
  string('timestamp'),
  string('status')
]

# @class CreativeCategoryAdmin
# @brief CampaignAdmin with aspect 'creative_categories'.
# Examine creative categories.
# CampaignAdmin call string:
# CampaignAdmin show creative_categories manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('CreativeCategoryAdmin',
      Cmd('CampaignManager', 'creative_categories',
          [number('category', 'creative_category_id=')]),
      checker_name = 'CreativeCategoryChecker').fields = [
  number('creative_category_id'),
  string('timestamp')
]

# @class ChannelCategoryAdmin
# @brief CampaignAdmin with aspect 'category_channel'.
# Examine channel categories.
# CampaignAdmin call string:
# CampaignAdmin show category_channel manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager
Admin('ChannelCategoryAdmin',
      Cmd('CampaignManager', 'category_channel',
          [number('category', 'channel_id=')]),
      checker_name = 'ChannelCategoryChecker').fields = [
  number('channel_id'),
  number('parent_channel_id'),
  string('timestamp')
]

# @class MarginAdmin
# @brief CampaignAdmin with aspect 'margin'.
# Examine margin rules.
# CampaignAdmin call string:
# CampaignAdmin show margin manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager
Admin('MarginAdmin',
      Cmd('CampaignManager', 'margin',
          [number('margin', 'margin_rule_id=')]),
      checker_name = 'MarginChecker').fields = [
  number('margin_rule_id'),
  number('account_id'),
  string('type'),
  string('sort_order'),
  string('fixed_margin'),
  string('relative_margin'),
  string('isp_accounts'),
  string('publisher_accounts'),
  string('advertiser_accounts')
]

# @class SiteAdmin
# @brief CampaignAdmin with aspect 'sites'.
# Examine sites.
# CampaignAdmin call string:
# CampaignAdmin show sites manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('SiteAdmin',
      Cmd('CampaignManager', 'sites',
          [number('site', 'site_id=')]),
      checker_name = 'SiteChecker').fields = [
  number('site_id'),
  string('status'),
  string('approved_creative_categories'),
  string('rejected_creative_categories'),
  string('approved_creatives'),
  string('account_id'),
  string('noads_timeout')
]

# @class TagAdmin
# @brief CampaignAdmin with aspect 'tags'.
# Examine tags.
# CampaignAdmin call string:
# CampaignAdmin show tags manager=corbaloc::$campaign_manager_host:$campaign_manager_port/CampaignManager

Admin('TagAdmin',
      Cmd('CampaignManager', 'tags',
          [number('tag', 'tag_id=')]),
      checker_name = 'TagChecker').fields = [
  number('tag_id'),
  number('site_id'),
  string('sizes'),
  string('accepted_categories'),
  string('rejected_categories'),
  string('imp_track_pixel')
]

# @class CampaignAdminSimpleChannel
# @brief CampaignAdmin with aspect 'simple-channel'.
# Examine simple channels.
# CampaignAdmin call string:
# CampaignAdmin simple-channel server=corbaloc::$campaign_server_host:$campaign_server_port/CampaignServer

Admin('SimpleChannelAdmin',
      Cmd('CampaignServer', 'simple_channel',
          [number('id', 'channel_id=')]),
      checker_name = 'SimpleChannelChecker').fields = [
  number('channel_id'),
  string('country_code'),
  string('status', True),
  string('behav_param_list_id'),
  string('categories'),
  string('threshold'),
  string('timestamp')
]

# @class CampaignAdminExprChannels
# @brief CampaignAdmin with aspect 'expression-channels'.
# Examine expression channels.
# CampaignAdmin call string:
# CampaignAdmin show expression-channels mabager=corbaloc::$campaign_server_host:$campaign_server_port/CampaignServer

Admin('ExpressionChannelAdmin',
      Cmd('CampaignServer', 'expression_channel',
          [number('channel_id', 'channel_id=')]),
      checker_name = 'ExpressionChannelChecker').fields = [
  number('channel_id'),
  string('discover_query'),
  string('discover_annotation'),
  string('expression'),
  string('imp_revenue'),
  string('status', True)
]

# @class CampaignAdminStatCcg
# @brief CampaignAdmin with aspect 'stat-ccg'.
# Examine expression channels.
# CampaignAdmin call string:
# CampaignAdmin show stat-ccg server=corbaloc::$campaign_server_host:$campaign_server_port/CampaignServer

Admin('CampaignAdminStatCcg',
      Cmd('CampaignServer', 'stat_ccg',
          [number('id', 'id=')])).fields = [
  number('ccg_id'),
  string('impressions'),
  string('clicks'),
  string('actions'),
  string('amount'),
  string('comm_amount'),
  string('daily_amount'),
  string('daily_comm_amount'),
  string('creatives'),
  string('publishers'),
  string('tags')
]

Admin('SearchEngineAdmin',
      Cmd('CampaignServer', 'search_engines',
          [number('id', 'id=')]),
      checker_name = 'SearchEngineChecker').fields = [
  string('id'),
  string('regexp'),
  string('encoding'),
  string('decoding_depth'),
  string('timestamp')
]

# @class BaseProfileAdmin
# @brief UserInfoAdmin with aspect 'print-base'.
# Examine base user profile.
# UserInfoAdmin command call string:
# UserInfoAdmin print --uid=<UUID>
#   --manager=corbaloc::$user_info_manager_host:$user_info_manager_port/UserInfoManager

Admin('BaseProfileAdmin',
      Cmd(('UserInfoManagerController', 'UserInfoSrv'), 'print-base',
          [string('uid', '--uid=', switch = ('temp', '--tuid=')),
           field('temp', 'bool', 'false')], skip = 1),
      checker_name = 'BaseProfileChecker').fields = [
  string('version'),
  string('create_time'),
  string('history_time'),
  string('ignore_fraud_time'),
  string('last_request_time'),
  string('session_start_time'),
  string('colo_timestamps'),
  string('page_ht_candidates'),
  string('page_history_matches'),
  string('page_history_visits'),
  string('page_session_matches'),
  string('search_ht_candidates'),
  string('search_history_matches'),
  string('search_history_visits'),
  string('search_session_matches'),
  string('url_ht_candidates'),
  string('url_history_matches'),
  string('url_history_visits'),
  string('url_session_matches'),
  string('audience_channels')
]

# @class AdditionalProfileAdmin
# @brief UserInfoAdmin with aspect 'print'.
# Examine additional user profile.
# UserInfoAdmin command call string:
# UserInfoAdmin print --uid=<UUID>
#   --manager=corbaloc::$user_info_manager_host:$user_info_manager_port/UserInfoManager

Admin('AdditionalProfileAdmin',
      Cmd(('UserInfoManagerController', 'UserInfoSrv'), 'print-add',
          [string('uid', '--uid=', switch = ('temp', '--tuid=')),
           field('temp', 'bool', 'false')], skip = 1),
      checker_name = 'AdditionalProfileChecker').fields = [
  string('version'),
  string('create_time'),
  string('history_time'),
  string('ignore_fraud_time'),
  string('last_request_time'),
  string('session_start_time'),
  string('colo_timestamps'),
  string('page_ht_candidates'),
  string('page_history_matches'),
  string('page_history_visits'),
  string('page_session_matches'),
  string('search_ht_candidates'),
  string('search_history_matches'),
  string('search_history_visits'),
  string('search_session_matches'),
  string('url_ht_candidates'),
  string('url_history_matches'),
  string('url_history_visits'),
  string('url_session_matches')
]

# @class HistoryProfileAdmin
# @brief UserInfoAdmin with aspect 'print-history'.
# Examine history user profile.
# UserInfoAdmin command call string:
# UserInfoAdmin print --uid=<UUID>
#   --manager=corbaloc::$user_info_manager_host:$user_info_manager_port/UserInfoManager

Admin('HistoryProfileAdmin',
      Cmd(('UserInfoManagerController', 'UserInfoSrv'), 'print-history',
          [string('uid', '--uid=', switch = ('temp', '--tuid=')),
           field('temp', 'bool', 'false')], skip = 1),
      checker_name = 'HistoryProfileChecker').fields = [
  string('page_channels'),
  string('search_channels'),
  string('url_channels')
]

# @class FreqCapProfileAdmin
# @brief UserInfoAdmin with aspect 'print-freq-cap'.
# Examine frequency cap user profile.
# UserInfoAdmin command call string:
# UserInfoAdmin print-freq-cap --uid=<UUID>
#   --manager=corbaloc::$user_info_manager_host:$user_info_manager_port/UserInfoManager

Admin('FreqCapProfileAdmin',
      Cmd(('UserInfoManagerController', 'UserInfoSrv'), 'print-freq-cap',
          [string('uid', '--uid=', switch = ('temp', '--tuid=')),
           field('temp', 'bool', 'false')], skip = 1),
      checker_name = 'FreqCapProfileChecker').fields = [
  string('fc_id'),
  string('virtual'),
  string('total_impressions'),
  string('last_impressions')
]

# @class ExpressionMatcherAdmin
# @brief ExpressionMatcherAdmin with aspect 'print'.
# ExpressionMatcherAdmin call string:
# ExpressionMatcherAdmin print '<UUID>' \
#     -r corbaloc::$expression_matcher_host:$expression_matcher_port/ExpressionMatcher

Admin('InventoryProfileAdmin',
      Cmd('ExpressionMatcher', 'print',
          [string('uid', '--user_id=')]),
      checker_name='InventoryProfileChecker').fields = [
  string('user_id'),
  number('imp_count'),
  string_list('total_channels'),
  string('last_request_time')
]

Admin('UserTriggerMatchProfileAdmin',
      Cmd('ExpressionMatcher', 'print-user-trigger-match',
          [string('uid', '--user_id='),
           flag('temp', '--temporary', 'false')]),
      checker_name = 'UserTriggerMatchProfileChecker').fields = [
  string('user_id'),
  string('page_matches'),
  string('search_matches'),
  string('url_matches'),
  string('requests')
]

Admin('RequestTriggerMatchProfileAdmin',
      Cmd('ExpressionMatcher', 'print-request-trigger-match',
          [string('uid', '--user_id=')]),
      checker_name = 'RequestTriggerMatchProfileChecker').fields = [
string('request_id'),
string('time'),
string('page_matches'),
string('search_matches'),
string('url_matches'),
string('click_done')
]

# @class RequestInfoAdminRequest
# @brief RequestInfoAdmin with aspect 'print-request'.
# RequestInfoAdmin call string:
# RequestInfoAdmin print-request '<REQUESTID>' \
#     -r corbaloc::$request_info_manager_host:$request_info_manager_port/RequestInfoManager

Admin('RequestProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-request',
          [string('requestid', True, may_empty = True)], options = ['--align']),
      checker_name='RequestProfileChecker').fields = [
  string('fraud')
]

Admin('PassbackProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-passback',
          [string('requestid', True, may_empty = True)], options = ['--align']),
      checker_name='PassbackProfileChecker').fields = [
  number('tag_id'),
  number('colo_id'),
  string('user_status'),
  string('time'),
  string('done'),
  string('verified')
]

# @class RequestInfoAdminFraud
# @brief RequestInfoAdmin with aspect 'print-fraud'.
# RequestInfoAdmin call string:
# RequestInfoAdmin print-fraud '<UUID>' \
#     -r corbaloc::$request_info_manager_host:$request_info_manager_port/RequestInfoManager

Admin('FraudProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-fraud',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='FraudProfileChecker').fields = [
  string('fraud_time')
]

Admin('ReachProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-reach',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='ReachProfileChecker').fields = [
  string('last_request_day'),
  string('total_appear_campaigns'),
  string('total_appear_ccgs'),
  string('total_appear_creatives'),
  string('total_appear_advs'),
  string('total_appear_display_advs')
]

Admin('ActionProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-action',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='ActionProfileChecker').fields = [
  string('action_markers'),
  string('wait_markers'),
  string('custom_action_markers'),
  string('custom_wait_actions'),
  string('custom_done_actions')
]

Admin('SiteReachProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-site-reach',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='SiteReachProfileChecker').fields = [
  string('last_request_day'),
  string('daily_appear_lists'),
  string('monthly_appear_lists')
]

Admin('TagRequestGroupProfileAdmin',
      Cmd(('RequestInfoManager', 'RequestInfoSrv'), 'print-tag-request-groups',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='TagRequestGroupProfileChecker').fields = [
  string('country'),
  string('colo_id'),
  string('site_id'),
  string('page_load_id'),
  string('referer_hash'),
  string('min_time'),
  string('max_time'),
  string('tags'),
  string('ad_shown')
]

Admin('RequestKeywordProfileAdmin',
      Cmd('RequestInfoManager', 'print-request-keyword-match',
          [string('requestid', True, may_empty = True)], options = ['--align']),
      checker_name='RequestKeywordProfileChecker').fields = [
  string('time'),
  string('colo_id'),
  string('tag_id'),
  string('ccg_id'),
  string('cc_id'),
  string('channel_id'),
  string('position'),
  string('num_shown'),
  string('page_last_match'),
  string('page_hourly_matches'),
  string('page_daily_matches'),
  string('page_weekly_matches'),
  string('search_last_match'),
  string('search_hourly_matches'),
  string('search_daily_matches'),
  string('search_weekly_matches'),
  string('ccg_imps'),
  string('cc_imps'),
  string('channel_imps'),
  string('click_done')
]

Admin('UserKeywordProfileAdmin',
      Cmd('RequestInfoManager', 'print-user-keyword-match',
          [string('uuid', True, may_empty = True)], options = ['--align']),
      checker_name='UserKeywordProfileChecker').fields = [
  string('page_matches'),
  string('search_matches'),
  string('ccg_imps'),
  string('cc_imps'),
  string('channel_imps'),
  string('requests')
]

Admin('ChannelCheckAdmin',
      Cmd(('ChannelManagerController', 'ChannelSrv'),
          'check', skip = 3)).fields = [
  string('id'),
  string('version')
]

# @class CCGKeywordAdmin
# @brief ChannelAdmin with aspect 'pos_ccg'.
# Examine keywords associations with CCGs.
# ChannelAdmin call string:
# ChannelAdmin pos_ccg  -r manager=corbaloc::$channel_server_host:$channel_server_port/ChannelUpdate

Admin('CCGKeywordAdmin',
      Cmd(('ChannelManagerController', 'ChannelSrv'),
          'all_ccg', [number('ccg_keyword_id', '--show=ccg_keyword_id=')]),
      checker_name = 'CCGKeywordChecker' ).fields = [
  string('ccg_keyword_id'),
  string('ccg_id'),
  string('channel_id'),
  money('max_cpc'),
  money('ctr'),
  string('click_url'),
  string('original_keyword')
]

# @class TriggerAdmin
# @brief Check triggers loaded by channel server

Admin('TriggerAdmin',
      Cmd(('ChannelManagerController', 'ChannelSrv'),
          'update', [string('trigger', '-i')], skip = 7),
      checker_name='TriggerChecker').fields = [
  string('channel_id'),
  string_list('url'),
  string('neg_url'),
  string_list('page_word'),
  string('neg_page_word'),
  string_list('search_word'),
  string('neg_search_word'),
  string_list('url_keyword'),
  string('neg_url_keyword'),
  string('stamp')
]

# @class ChannelSearchAdmin
# @brief ChannelSearchAdmin with aspect 'search'.
# Examine channels loaded by AdServer.
# ChannelSearchAdmin command call string:
# ChannelSearchAdmin search -r corbaloc::$channel_search_server_host:$channel_search_server_port/ChannelSearch \
#  --phrase <search phrase>

Admin('ChannelSearchAdmin',
      Cmd(('ChannelSearch', 'ChannelSrv'),
          'search', [string('phrase', '--phrase=')]),
      checker_name = 'ChannelSearchChecker').fields = [
  string('channel_id')]

# @class ChannelMatchAdmin
# @brief ChannelAdmin with aspect 'match'.
# Examine channels loaded by AdServer.
# ChannelAdmin command call string:
# ChannelAdmin search -r corbaloc::$channel_search_server_host:$channel_search_server_port/ChannelSearch \
#  --phrase <search phrase>

# Admin('ChannelMatchAdmin',
#       Cmd(('ChannelManagerController', 'ChannelSrv'),
#           'match', [string('trigger', ['-p', '-e', '-u'])],
#           options = ['-d', '-f', '-l']))

process_admins()

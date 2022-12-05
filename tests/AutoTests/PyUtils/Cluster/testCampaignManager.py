#!/usr/bin/env python

import CampaignManagerUtils, time, RandomAuctionTest, InstantiateTokensTest, re, base64
from CORBATest import *
from OrbUtil import time2orb, decimal2orb, orb2decimal
from Util import currentTime
from FunTest import tlog
from OrbTestSuite import main
from Rec import Rec
from MockCampaignServer import CampaignServerTestMixin, AccountInfo, CurrencyInfo, \
     CampaignInfo, CampaignEcpmInfo, SimpleChannelKey, BehavParameter, BehavParamInfo, \
     SiteInfo, TagInfo, CreativeInfo, ExpressionChannelInfo, ExpressionInfo, \
     CreativeTemplateInfo, CreativeTemplateFileInfo, TagPricingInfo, ColocationInfo, \
     CampaignDeliveryLimitsInfo, CTT_TEXT, CTT_XSLT, GeoChannelInfo, OptionValueInfo, \
     CreativeOptionInfo, AppFormatInfo, SizeInfo, CreativeSizeInfo, TagSizeInfo
from CampaignManagerUtils import CCGKeywordInfo, createSite, createTag

PORTION_NUMBER = 20 # It is a server hard-coded value
                    # see PORTION_NUMBER in
                    # CampaignSvcs/CampaignManager/CampaignConfigSource.cpp
DAY = 24*60*60

class CampaignManagerTest(CORBAFunTest, CampaignServerTestMixin):
  'CampaignManager'

  def setUp( self ):
    self.LOGLEVEL = 100
    self.CAMPAIGNSRV_PORT  = self.orbPort
    CampaignServerTestMixin.setUp( self )
    self.__prepareConfig()
    self.setUpServers(CampaignManager)
    self.startProc()
    self.CampaignObject = self.CampaignManager.getObject("CampaignManager",
                                                           CampaignManagerUtils.CampaignManager)

  def tearDown( self ):
    self.tearDownServers()

  def __prepareConfig( self ):
    now       = currentTime()
    yesterday = (int(time.time()) - DAY, 0)
    tommorow  = (int(time.time()) + DAY, 0)
    self.colocations = [ColocationInfo(1,                              # colo_id
                                       "Colocation#1",                 # colo_name
                                       0,                              # colo_rate_id
                                       0,                              # at_flags
                                       0,                              # ad_serving
                                       False,                          # hid_profile
                                       1,                              # account_id
                                       decimal2orb(1.0),               # revenue_share
                                       [],                             # tokens
                                       time2orb(now)                   # timestamp
                                       )]
    self.simple_channels = [SimpleChannelKey(15,                       # channel_id
                                             "gn",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             1,                        # behav_param_list_id
                                             "",                       # str_behav_param_list_id 
                                             [],                       # categories
                                             1,                        # threshold
                                             0,                        # discover
                                             [],                       # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(now)             # timestamp
                                             )]
    self.expression_channels = [ExpressionChannelInfo(16,              # channel_id
                                                      "ExprChannel#1", # name
                                                      1,               # account_id,
                                                      "gn",            # country_code
                                                      0,               # flags
                                                      'A',             # status
                                                      'B',             # type
                                                      True,            # is_public
                                                      "",              # language
                                                      0,               # freq_cap_id
                                                      0,               # parent_channel_id;
                                                      time2orb(now),   # timestamp
                                                      "",              # discover_query
                                                      "",              # discover_annotation
                                                      0,               # channel_rate_id
                                                      decimal2orb(1.0),# imp_revenue
                                                      decimal2orb(2.0),# click_revenue
                                                      # expression
                                                      ExpressionInfo('S', # operation
                                                                     15,  # channel_id
                                                                     []   # sub_channels
                                                                     ),
                                                      1                # threshold
                                                      ),
                                ExpressionChannelInfo(15393282,        # channel_id
                                                      "adscopenrtbtext11", # name
                                                      1,               # account_id,
                                                      "gn",            # country_code
                                                      0,               # flags
                                                      'A',             # status
                                                      'K',             # type
                                                      False,           # is_public
                                                      "en",            # language
                                                      0,               # freq_cap_id
                                                      0,               # parent_channel_id;
                                                      time2orb(now),   # timestamp
                                                      "",              # discover_query
                                                      "",              # discover_annotation
                                                      0,               # channel_rate_id
                                                      decimal2orb(1.0),# imp_revenue
                                                      decimal2orb(2.0),# click_revenue
                                                      # expression
                                                      ExpressionInfo('S', # operation
                                                                     15393282,  # channel_id
                                                                     []   # sub_channels
                                                                     ),
                                                      1                # threshold
                                                      )]
                                                      
    self.behav_params = [BehavParamInfo(1,                             # id             
                                        1,                             # threshold
                                        time2orb(now),                 # timestamp
                                        # bp_seq
                                        [BehavParameter(1,                # minimum_visits
                                                        0,                # time_from
                                                        0,                # time_to
                                                        1,                # weight
                                                        "P"               # trigger_type
                                                        )]
                                        )]
    self.currencies = [CurrencyInfo(decimal2orb(1.0),                 # rate
                                    1,                                # currency_id
                                    1,                                # currency_exchange_id
                                    int(time.time()) + 2*DAY,         # effective_date
                                    2,                                # fraction_digits
                                    "USD",                            # currency_code
                                    time2orb(now)                     # timestamp
                                    )]
    self.accounts = [AccountInfo(1,                                   # account_id
                                 0,                                   # agency_account_id
                                 0,                                   # internal_account_id
                                 0,                                   # role_id
                                 0,                                   # flags
                                 0,                                   # at_flags  
                                 'O',                                 # text_adserving
                                 1,                                   # currency_id
                                 "gb",                                # country
                                 time2orb((2,0)),                     # time_offset
                                 decimal2orb(0.0),                    # commision
                                 decimal2orb(0.0),                    # budget
                                 decimal2orb(0.0),                    # paid_amount
                                 [],                                  # walled_garden_accounts
                                 0,                                   # auction_rate
                                 False,                               # use_pub_pixels
                                 "",                                  # pub_pixel_optin
                                 "",                                  # pub_pixel_optout
                                 'A',                                 # status
                                 'A',                                 # eval_status
                                 time2orb(now)                        # timestamp
                                 ),
                     AccountInfo(2,                                   # account_id
                                 0,                                   # agency_account_id
                                 0,                                   # internal_account_id
                                 0,                                   # role_id
                                 1,                                   # flags
                                 0,                                   # at_flags  
                                 'O',                                 # text_adserving
                                 1,                                   # currency_id
                                 "gb",                                # country
                                 time2orb((2,0)),                     # time_offset
                                 decimal2orb(0.0),                    # commision
                                 decimal2orb(0.0),                    # budget
                                 decimal2orb(0.0),                    # paid_amount
                                 [],                                  # walled_garden_accounts
                                 0,                                   # auction_rate
                                 False,                               # use_pub_pixels
                                 "",                                  # pub_pixel_optin
                                 "",                                  # pub_pixel_optout
                                 'A',                                 # status
                                 'A',                                 # eval_status
                                 time2orb(now)                        # timestamp
                                 ),
                      AccountInfo(3,                                   # account_id
                                 0,                                   # agency_account_id
                                 0,                                   # internal_account_id
                                 0,                                   # role_id
                                 0,                                   # flags
                                 0,                                   # at_flags  
                                 'O',                                 # text_adserving
                                 1,                                   # currency_id
                                 "gb",                                # country
                                 time2orb((2,0)),                     # time_offset
                                 decimal2orb(0.0),                    # commision
                                 decimal2orb(0.0),                    # budget
                                 decimal2orb(0.0),                    # paid_amount
                                 [],                                  # walled_garden_accounts
                                 0,                                   # auction_rate
                                 False,                               # use_pub_pixels
                                 "",                                  # pub_pixel_optin
                                 "",                                  # pub_pixel_optout
                                 'A',                                 # status
                                 'A',                                 # eval_status
                                 time2orb(now)                        # timestamp
                                 )]
    self.campaigns = [CampaignInfo(1,             # campaign id
                                   1,             # ccg_id
                                   1,             # ccg_rate_id
                                   'M',           # ccg_rate_type
                                   0,             # fc_id
                                   0,             # group_fc_id
                                   0,             # priority
                                   32768,         # flags, 32768 - mean none
                                   'A',           # marketplace
                                   # expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   # stat_expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   "gn",          # country   
                                   [1],           # sites
                                   'A',           # status
                                   'A',           # eval_status
                                   [],            # dailyrun_intervals
                                   # creatives
                                   [CreativeInfo(1,                              # cc id
                                                 1,                              # creative id
                                                 0,                              # fc_id 
                                                 1,                              # weight
                                                 [
                                                   CreativeSizeInfo(
                                                     0,  # size_id
                                                     0,  # up_expand_space
                                                     0,  # right_expand_space
                                                     0,  # down_expand_space
                                                     0,  # down_expand_space
                                                     []) # tokens
                                                  ],
                                                 "unit-test",                    # format
                                                  # click_url
                                                 OptionValueInfo(0,              # option_id
                                                                 ""                # value
                                                                 ),
                                                  # html_url
                                                 OptionValueInfo(104,                # option_id
                                                                 ""                # value
                                                                 ),
                                                 0,                              # order_set_id
                                                 [],                             # categories  
                                                 [],                             # tokens
                                                 'A',                           # status
                                                 ''                               # version_id
                                                 )],
                                   1,             # account_id 
                                   1,             # advertiser_id
                                   [],            # exclude_pub_accounts
                                   [],            # exclude_tags
                                   decimal2orb(0.5), # imp_revenue
                                   decimal2orb(2.0), # click_revenue
                                   decimal2orb(3.0), # action_revenue
                                   decimal2orb(0.0), # commision
                                   'D',           # ccg_type
                                   'C',           # target type
                                   # campaign_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   # ccg_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   0,                       # start_user_group_id
                                   99,                      # end_user_group_id
                                   decimal2orb(1000.0),     # max_pub_share
                                   0,                       # ctr_reset_id
                                   0,                       # random_imps
                                   1,                       # mode
                                   0,                       # seq_set_rotate_imps
                                   time2orb(0),             # min_uid_age
                                   [],                      # colocations
                                   time2orb(now)            # timestamp
                                   ),
                      CampaignInfo(2,             # campaign id
                                   2,             # ccg_id
                                   1,             # ccg_rate_id
                                   'M',           # ccg_rate_type
                                   0,             # fc_id
                                   0,             # group_fc_id
                                   0,             # priority
                                   32768,         # flags, 32768 - mean none
                                   'A',           # marketplace
                                   # expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   # stat_expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   "gn",          # country   
                                   [1],           # sites
                                   'A',           # status
                                   'A',           # eval_status
                                   [],            # dailyrun_intervals
                                   # creatives
                                   [CreativeInfo(2,                              # cc id
                                                 2,                              # creative id
                                                 0,                              # fc_id 
                                                 1,                              # weight
                                                 [
                                                   CreativeSizeInfo(
                                                     1,  # size_id
                                                     0,  # up_expand_space
                                                     0,  # right_expand_space
                                                     0,  # down_expand_space
                                                     0,  # down_expand_space
                                                     []) # tokens
                                                 ],
                                                 "Image",                    # format
                                                  # click_url
                                                 OptionValueInfo(203,                # option_id
                                                                 "http://www.clickurl.com?and=query_tag_##TAGID##_site_##SITEID##_pub_##PUBID##_ccg_##CGID##_cmp_##CID##" # value
                                                                 ),
                                                  # html_url
                                                 OptionValueInfo(104,                # option_id
                                                                 ""                # value
                                                                 ),
                                                 0,                              # order_set_id
                                                 [],                             # categories  
                                                 [OptionValueInfo(101, ""),
                                                  OptionValueInfo(102, "http://www.displayurl.com")], # tokens
                                                 'A',                            # status
                                                 ''                                # version_id
                                                 ),
                                   CreativeInfo(3,                              # cc id
                                                3,                              # creative id
                                                0,                              # fc_id 
                                                1,                              # weight
                                                [
                                                  CreativeSizeInfo(
                                                    1,  # size_id
                                                    0,  # up_expand_space
                                                    0,  # right_expand_space
                                                    0,  # down_expand_space
                                                    0,  # down_expand_space
                                                    []) # tokens
                                                ],
                                                "Image",                    # format
                                                # click_url
                                                OptionValueInfo(203,                # option_id
                                                               "http://www.test.com/adv_##ADVID##_site_##SITEID=##_pub_##PUBID=##_ccg_##CGID##_cmp_##CID##" # value
                                                               ),
                                                # html_url
                                                OptionValueInfo(104,                # option_id
                                                                ""                # value
                                                                ),
                                                0,                              # order_set_id
                                                [],                             # categories  
                                                [OptionValueInfo(101, ""),
                                                 OptionValueInfo(102, "http://www.displayurl.com")], # tokens
                                                'A',                            # status
                                                ''                                # version_id
                                                )],
                                   1,             # account_id 
                                   1,             # advertiser_id
                                   [],            # exclude_pub_accounts
                                   [],            # exclude_tags
                                   decimal2orb(0.5), # imp_revenue
                                   decimal2orb(2.0), # click_revenue
                                   decimal2orb(3.0), # action_revenue
                                   decimal2orb(0.0), # commision
                                   'D',           # ccg_type
                                   'C',           # target type
                                   # campaign_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   # ccg_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   0,                       # start_user_group_id
                                   99,                      # end_user_group_id
                                   decimal2orb(1000.0),     # max_pub_share
                                   0,                       # ctr_reset_id
                                   0,                       # random_imps
                                   1,                       # mode
                                   0,                       # seq_set_rotate_imps
                                   time2orb(0),             # min_uid_age
                                   [],                      # colocations
                                   time2orb(now)            # timestamp
                                   ),
                      CampaignInfo(10306897,      # campaign id
                                   2,             # ccg_id
                                   1,             # ccg_rate_id
                                   'C',           # ccg_rate_type
                                   0,             # fc_id
                                   0,             # group_fc_id
                                   0,             # priority
                                   32768,         # flags, 32768 - mean none
                                   'A',           # marketplace
                                   # expression
                                   ExpressionInfo('-',  # operation
                                                  0,    # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   # stat_expression
                                   ExpressionInfo('-',  # operation
                                                  0,    # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   "gn",          # country   
                                   [1],           # sites
                                   'A',           # status
                                   'A',           # eval_status
                                   [],            # dailyrun_intervals
                                   # creatives
                                   [CreativeInfo(10336933,                       # cc id
                                                 12345,                          # creative id
                                                 0,                              # fc_id 
                                                 10000,                          # weight
                                                 [
                                                   CreativeSizeInfo(
                                                     0,  # size_id
                                                     0,  # up_expand_space
                                                     0,  # right_expand_space
                                                     0,  # down_expand_space
                                                     0,  # down_expand_space
                                                     []) # tokens
                                                 ],
                                                 "Text",                         # creative format
                                                  # click_url
                                                 OptionValueInfo(1,                # option_id
                                                                 "http://aa.zz.rubypub.net/Text1"  # value
                                                                 ),
                                                  # html_url
                                                 OptionValueInfo(104,              # option_id
                                                                 ""                # value
                                                                 ),
                                                 1,                              # order_set_id
                                                 [],                             # categories  
                                                 [OptionValueInfo(102, "http://www.displayurl.com")], # tokens
                                                 'A',                            # status
                                                 ''                                # version_id
                                                 )],
                                   1,             # account_id 
                                   1,             # advertiser_id
                                   [],            # exclude_pub_accounts
                                   [],            # exclude_tags
                                   decimal2orb(0.5), # imp_revenue
                                   decimal2orb(2.0), # click_revenue
                                   decimal2orb(3.0), # action_revenue
                                   decimal2orb(0.0), # commision
                                   'T',           # ccg_type
                                   'K',           # target type
                                   # campaign_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   # ccg_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   0,                       # start_user_group_id
                                   99,                      # end_user_group_id
                                   decimal2orb(1.0),        # max_pub_share
                                   0,                       # ctr_reset_id
                                   0,                       # random_imps
                                   1,                       # mode
                                   0,                       # seq_set_rotate_imps
                                   time2orb(0),             # min_uid_age
                                   [],                      # colocations
                                   time2orb(now)            # timestamp
                                   ),
                      CampaignInfo(10000,         # campaign id
                                   1000,          # ccg_id
                                   1,             # ccg_rate_id
                                   'M',           # ccg_rate_type
                                   0,             # fc_id
                                   0,             # group_fc_id
                                   0,             # priority
                                   32768,         # flags, 32768 - mean none
                                   'A',           # marketplace
                                   # expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   # stat_expression
                                   ExpressionInfo('-',  # operation
                                                  16,   # channel_id
                                                  [],   # sub_channels
                                                  ),
                                   "gn",          # country   
                                   [1],           # sites
                                   'A',           # status
                                   'A',           # eval_status
                                   [],            # dailyrun_intervals
                                   # creatives
                                   [CreativeInfo(986887,                         # cc id
                                                 986887,                         # creative id
                                                 0,                              # fc_id 
                                                 1,                              # weight
                                                 [
                                                   CreativeSizeInfo(
                                                     0,  # size_id
                                                     0,  # up_expand_space
                                                     0,  # right_expand_space
                                                     0,  # down_expand_space
                                                     0,  # down_expand_space
                                                     []) # tokens
                                                  ],
                                                 "unit-test",                    # format
                                                  # click_url
                                                 OptionValueInfo(203,                # option_id
                                                                 "http://www.clickurl.com?cid=##CID##&cgid=##CGID##" # value
                                                                 ),
                                                  # html_url
                                                 OptionValueInfo(104,              # option_id
                                                                 ""                # value
                                                                 ),
                                                 0,                              # order_set_id
                                                 [],                             # categories  
                                                 [],                             # tokens
                                                 'W',                            # status
                                                 ''                               # version_id
                                                 )],
                                   0,             # account_id 
                                   0,             # advertiser_id
                                   [],            # exclude_pub_accounts
                                   [],            # exclude_tags
                                   decimal2orb(0.5), # imp_revenue
                                   decimal2orb(2.0), # click_revenue
                                   decimal2orb(3.0), # action_revenue
                                   decimal2orb(0.0), # commision
                                   'D',           # ccg_type
                                   'C',           # target type
                                   # campaign_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   # ccg_delivery_limits
                                   CampaignDeliveryLimitsInfo(time2orb(yesterday),   # date_start
                                                              time2orb(tommorow),    # date_end
                                                              decimal2orb(1000.0),   # budget
                                                              decimal2orb(1000.0),   # daily_budget
                                                              'A'                    # delivery_pacing
                                                              ),
                                   0,                       # start_user_group_id
                                   99,                      # end_user_group_id
                                   decimal2orb(1000.0),     # max_pub_share
                                   0,                       # ctr_reset_id
                                   0,                       # random_imps
                                   1,                       # mode
                                   0,                       # seq_set_rotate_imps
                                   time2orb(0),             # min_uid_age
                                   [],                      # colocations
                                   time2orb(now)            # timestamp
                                   )] 
    self.ecpms = [CampaignEcpmInfo(1,                       # ccg_id 
                                   decimal2orb(20.0),       # ecpm
                                   decimal2orb(0.01),       # ctr
                                   time2orb(now)            # timestamp
                                   )]

    self.creative_templates = [CreativeTemplateInfo(1,             # id
                                                    time2orb(now), # timestamp
                                                    # files
                                                    [CreativeTemplateFileInfo('unit-test', # creative_format
                                                                              "468x60",    # creative_size
                                                                              'unit-test', # app_format
                                                                              'unit-test-mime', # mime_format
                                                                              False,       # track_impr
                                                                              CTT_TEXT,    # type
                                                                              "test.html"  # template_file
                                                                              )],
                                                    [], # tokens
                                                    []  # hidden_tokens
                                                    ),
                               CreativeTemplateInfo(2,             # id
                                                    time2orb(now), # timestamp
                                                    # files
                                                    [CreativeTemplateFileInfo('Image', # creative_format
                                                                              "250x250",    # creative_size
                                                                              'js', # app_format
                                                                              'text/javascript', # mime_format
                                                                              False,       # track_impr
                                                                              CTT_TEXT,    # type
                                                                              "creative2.js"  # template_file
                                                                              ),
                                                     CreativeTemplateFileInfo('Text',      # creative_format
                                                                              '468x60',    # creative_size
                                                                              'html',      # app_format
                                                                              'text/html', # mime_format
                                                                              True,        # track_impr
                                                                              CTT_XSLT,    # type
                                                                              "textad.xsl" # template_file
                                                                              )],
                                                    [], # tokens
                                                    []  # hidden_tokens
                                                    ),
                               CreativeTemplateInfo(3,
                                                    time2orb(now), # timestamp
                                                    # files
                                                    [CreativeTemplateFileInfo('Text', # creative_format
                                                                              "728x90",    # creative_size
                                                                              'js', # app_format
                                                                              'text/javascript', # mime_format
                                                                              False,       # track_impr
                                                                              CTT_TEXT,    # type
                                                                              "creative2.js"  # template_file
                                                                              )],
                                                    [], # tokens
                                                    []  # hidden_tokens
                                                  )]

    self.sites = [createSite(site_id = 1, account_id = 1),
                  createSite(site_id = 2, account_id = 2)]

    self.tags = [createTag(tag_id = 1, site_id = 1,
                           sizes = [
                             TagSizeInfo(
                               0,  # size_id
                               1,  # max_text_creatives
                               []) # tokens
                           ]),
                 createTag(tag_id = 2, site_id = 2,
                           sizes = [
                             TagSizeInfo(
                               1,  # size_id
                               1,  # max_text_creatives
                               []) # tokens
                           ]),
                 createTag(tag_id = 213542, site_id = 2,
                           sizes = [
                             TagSizeInfo(
                               1,  # size_id
                               2,  # max_text_creatives
                               []) # tokens
                           ],
                           hidden_tokens = [
                             OptionValueInfo(44492, "Hidden def value"), # ADSC_HIDDEN_PB_1
                             OptionValueInfo(44493, "Hidden def value (RANDOM = ##RANDOM##)") # ADSC_HIDDEN_PB_2
                           ],
                           passback = "0000/0021/3542/20131120-085023588.html",
                           passback_type = "html"),
                 createTag(tag_id = 10213614, site_id = 2,
                           sizes = [
                             TagSizeInfo(
                               0,  # size_id
                               2,  # max_text_creatives
                               []) # tokens
                           ]),
                 createTag(tag_id = 213483, site_id = 1,
                           sizes = [
                             TagSizeInfo(
                               0,  # size_id
                               0,  # max_text_creatives
                               []) # tokens
                           ],
                           auction_max_ecpm_share = decimal2orb(0.0),
                           auction_prop_probability_share = decimal2orb(1.0),
                           auction_random_share = decimal2orb(0.0)),
                 createTag(tag_id = 213484, site_id = 1,
                           sizes = [
                             TagSizeInfo(
                               0,  # size_id
                               1,  # max_text_creatives
                               []) # tokens
                           ],
                           auction_max_ecpm_share = decimal2orb(0.0),
                           auction_prop_probability_share = decimal2orb(1.0),
                           auction_random_share = decimal2orb(0.0),
                           tag_pricings = [TagPricingInfo(
                                             "gn",               # country_code
                                             'T',                # ccg_type
                                             '-',                # ccg_rate_type
                                             1,                  # site_rate_id
                                             decimal2orb(1000.0),# imp_revenue
                                             decimal2orb(0.0) # revenue_share
                                             )])
                 ]

    self.activate_geo_channels     = [GeoChannelInfo(
                                      15,           # channel_id
                                      "gn",         # country_code
                                      [],           # geoip_targets
                                      time2orb(now) # timestamp
                                      ),
                                      GeoChannelInfo(
                                      16,           # channel_id
                                      "gn",         # country_code
                                      [],           # geoip_targets
                                      time2orb(now) # timestamp
                                      )]

    self.activate_creative_options = [CreativeOptionInfo(
                                      101,          # option_id
                                      "DESTURL",    # token
                                      'L',          # type
                                      [],           # token_relations
                                      time2orb(0.0) # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      102,          # option_id
                                      "DISPLAY_URL",# token
                                      'L',          # type
                                      [],           # token_relations
                                      time2orb(0.0) # timestamp
                                      ),
                                      CreativeOptionInfo(
                                        104,          # option_id
                                        "HTML_URL",   # token
                                        'L',          # type
                                        [],           # token_relations
                                        time2orb(0.0) # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      44492,              # option_id
                                      "ADSC_HIDDEN_PB_1", # token
                                      'S',                # type
                                      ["RANDOM"],         # token_relations
                                      time2orb(0.0)       # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      44493,              # option_id
                                      "ADSC_HIDDEN_PB_2", # token
                                      'S',                # type
                                      ["RANDOM"],         # token_relations
                                      time2orb(0.0)       # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      201,                # option_id
                                      "CGID",             # token
                                      'S',                # type
                                      [],                 # token_relations
                                      time2orb(0.0)       # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      202,                # option_id
                                      "CID",              # token
                                      'S',                # type
                                      [],                 # token_relations
                                      time2orb(0.0)       # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      203,                # option_id
                                      "CRCLICK",          # token
                                      'L',                # type
                                      ["RANDOM", "KEYWORD", "CGID", "CID", "PUBID", "SITEID", "TAGID", "ADVID"], # token_relations
                                      time2orb(0.0)       # timestamp
                                      ),
                                      CreativeOptionInfo(
                                      204,                # option_id
                                      "CRCLICK",          # token
                                      'L',                # type
                                      ["RANDOM", "KEYWORD"], # token_relations
                                      time2orb(0.0)       # timestamp
                                      )]

    self.app_formats = [AppFormatInfo(
                        "tokens_html",  # app_format
                        "text/html",    # mime_format
                        time2orb(0.0)   # timestamp
                        ),
                        AppFormatInfo(
                        "js",  # app_format
                        "text/javascript",    # mime_format
                        time2orb(0.0)   # timestamp
                        )]
    
    self.sizes = [SizeInfo(
                    0,             # size_id
                    "468x60",      # protocol_name
                    0,             # size_type_id
                    468,           # width
                    60,            # height
                    time2orb(0.0), # timestamp
                  ),
                  SizeInfo(
                    1,             # size_id
                    "250x250",     # protocol_name
                    0,             # size_type_id
                    250,           # width
                    250,           # height
                    time2orb(0.0), # timestamp
                  ),
                  SizeInfo(
                    2,             # size_id
                    "728x90",      # protocol_name
                    0,             # size_type_id
                    728,           # width
                    90,            # height
                    time2orb(0.0), # timestamp
                  )]
    RandomAuctionTest.prepareConfig(self)
    InstantiateTokensTest.prepareConfig(self)

  def run( self ):
    res = self.test(1, self.testConnect)
    res = self.test(1, self.testGetCampaignCreative, res)
    res = self.test(1, self.testTestRequestTokenInstantiate, res)
    res = self.test(1, self.testDestinationURL, res)
    res = self.test(1, self.testHiddenOptionsInPassback, res)
    res = self.test(1, self.testPreviewModeAndPassbackAll, res)
    res = self.test(1, self.testTextKeywordTargetedCCG, res)
    res = self.test(1, self.testGetClickUrl, res)
    res = self.test(1, self.testClickTokensInstantiate, res)
    res = self.test(1, self.testRequestToInvalidCampaign, res)
    res = self.test(1, self.testPropProbAuctionWithMaxTextCreativeIsZero, res)
    res = self.test(1, self.testPropProbAuctionAllTextFilteredByCPM, res)
    res = self.test(1, RandomAuctionTest.baseTest, res)
    res = self.test(1, InstantiateTokensTest.instantiateClickTokensForCreativeWithEmptyClickUrl, res)

  def testConnect( self ):
    "connect"
    expCalls = []
    for i in range(PORTION_NUMBER):
      expCalls.append('CampaignServer_get_config')
    self.checkCallSequence(expCalls = expCalls)

  def testGetCampaignCreative( self ):
    "get campaign creative"
    request = CampaignManagerUtils.createRequest(tag_id = 1,
                                                 channels = [16],
                                                 debug_ccg = 1,
                                                 only_display_ad = 0)
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    self.compare([Rec(_class = CampaignManagerUtils.CreativeSelectResult,
                      ccid=1L, cmp_id=1L, ecpm=decimal2orb(20.0),
                      # Revenue = campaign imp revenue
                      revenue = decimal2orb(0.0))],
                 response.ad_slots[0].selected_creatives,
                 "selected creatives")
    self.compare(orb2decimal(response.ad_slots[0].debug_info.selected_creatives[0].imp_revenue), 0.5, 'imp revenue')

  def testTestRequestTokenInstantiate ( self ):
    "request token instantiate"
    request = CampaignManagerUtils.createRequest(tag_id = 2,
                                                 preview_ccid = 2,
                                                 format = "js")
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives),'selected creatives')

    tokens = dict()
    for line in response.ad_slots[0].creative_body.split('\n'):
      matchObj = re.match(r"^(.+) = (.*)$", line)
      if (matchObj):
        tokens[matchObj.group(1)] = matchObj.group(2)

    self.assertEqual('1', tokens['TEST_REQUEST'])
    self.assertEqual('WmZMaElpdzlSWmlKWmdLYwyMHMk3XihvzsrwljDM6hkwWTs7quBXpWrx0f9Nfx6fM', tokens['GREQUESTID'])

  def testDestinationURL ( self ):
    "destination url"
    request = CampaignManagerUtils.createRequest(tag_id = 2,
                                                 preview_ccid = 2,
                                                 format = "js")
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives),'selected creatives')
    self.assertEqual('http://www.displayurl.com', response.ad_slots[0].selected_creatives[0].destination_url)

  def testHiddenOptionsInPassback( self ):
    "hidden options in passback"
    fe_address = ConfigVars.HOST + ":" + str(ConfigVars.PORT_BASE + 80)
    request = CampaignManagerUtils.createRequest(tag_id = 213542,
                                                 random = 112233,
                                                 format = "tokens_html",
                                                 required_passback = True)
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    expected = """</head>
<body>
<h4>Passback specific</h4>
<p>PASSBACK_TYPE = html</p>
<p>PASSBACK_CODE = </p>
<p>PASSBACK_URL = http://%s/tags/0000/0021/3542/20131120-085023588.html</p>
<p>PASSBACK_PIXEL = http://%s/services/passback?requestid=WmZMaElpdzlSWmlKWmdLYw..&random=112233&testrequest=1&h=40</p>
<h4>System</h4>
<p>TAGWIDTH = 250</p>
<p>TAGHEIGHT = 250</p>
<p>TAGSIZE = 250x250</p>
<p>TAGID = 213542</p>
<p>RANDOM = 112233</p>
<p>ADSERVER = http://%s</p>
<p>ADIMAGE-SERVER = http://%s</p>
<p>REFERER = </p>
<p>UID = </p>
<p>COHORT = </p>
<p>APP_FORMAT = tokens_html</p>
<p>COLOID = 1</p>
<p>TESTREQUEST = 1</p>
<p>USERSTATUS = 1</p>
<p>PUBPIXELS = 0</p>
<p>PUBPIXELSOPTIN = </p>
<p>PUBPIXELSOPTOUT = </p>
<h4>Hidden</h4>
<p>ADSC_HIDDEN_PB_1 = Hidden def value</p>
<p>ADSC_HIDDEN_PB_2 = Hidden def value (RANDOM = 112233)</p>
<h4>Tag specific</h4>
<p>PUBL_TAG_TRACK_PIXEL = </p>
<p>AD_FOOTER_ENABLED = </p>
<p>MAX_ADS_PER_TAG = </p>
<h4>Tag-template specific</h4>
<p>DESCRIPTION_COLOR = </p>
</body>
</html>""" % (fe_address, fe_address, fe_address, fe_address)
    self.assertEqual(expected, response.ad_slots[0].creative_body, 'creative_body')

  def testPreviewModeAndPassbackAll( self ):
    "preview mode and passback"
    request = CampaignManagerUtils.createRequest(tag_id = 2,
                                                 preview_ccid = 2,
                                                 format = "js",
                                                 test_request = True,
                                                 required_passback = True)
    request.ad_slots[0].passback = True
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots), 'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives), 'selected creatives')

    tokens = dict()
    for line in response.ad_slots[0].creative_body.split('\n'):
      matchObj = re.match(r"^(.+) = (.*)$", line)
      if (matchObj):
        tokens[matchObj.group(1)] = matchObj.group(2)

    self.assertEqual('1', tokens['TEST_REQUEST'])

  def testTextKeywordTargetedCCG( self ):
    "text keyword targeted CCG"
    request = CampaignManagerUtils.createRequest(
      publisher_account_id = 2,
      source_id = 'url',
      client = 'openrtb',
      client_version = '1',
      request_type = 3, # OpenRTB
      user_status = 2, # OptIn
      format = "html",
      debug_ccg = 10306897,
      ccg_keywords = [
        CCGKeywordInfo(
          11078847,             # ccg_keyword_id
          10306898,             # ccg_id
          15393280,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "",                   # click_url
          "ADSCOpenRTBText2"),  # original_keyword
        CCGKeywordInfo(
          11078849,             # ccg_keyword_id
          10306897,             # ccg_id
          15393282,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "http://keyword-click.com/Text11",  # click_url
          "ADSCOpenRTBText11")  # original_keyword
        ],
        only_display_ad = False
    )

    request.ad_slots[0].size = '468x60'

    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots), 'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives), 'selected creatives')
    self.assertEqual('http://www.displayurl.com', response.ad_slots[0].selected_creatives[0].destination_url)

  def testGetClickUrl( self ):
    "get click url"
    click_info = CampaignManagerUtils.ClickInfo(
      time2orb(currentTime()),
      1, # colo_id
      2, # tag_id
      1, # tag_size_id
      2, # ccid
      2, # keyword_id
      0, # creative_id
      "ZfLhIiw9RZiJZgKchTKxFA..",
      CampaignManagerUtils.UserIdHashModInfo(False, 0),
      "", # relocate
      "",  # referer
      True # log_click
      )
    result = self.CampaignObject.get_click_url(click_info)
    self.assertEqual('http://www.clickurl.com/?and=query_tag_2_site_2_pub_2_ccg_2_cmp_2', result[1].url)

    click_info = CampaignManagerUtils.ClickInfo(
      time2orb(currentTime()),
      1, # colo_id
      0, # tag_id
      1, # tag_size_id
      3, # ccid
      2, # keyword_id
      0, # creative_id
      "ZfLhIiw9RZiJZgKchTKxFA..",
      CampaignManagerUtils.UserIdHashModInfo(False, 0),
      "", # relocate
      "",  # referer
      True # log_click
      )
    result = self.CampaignObject.get_click_url(click_info)
    self.assertEqual('http://www.test.com/adv_1_site__pub__ccg_2_cmp_2', result[1].url)

  def testClickTokensInstantiate( self ):
    "click tokens instantiate"
    # request 1
    request = CampaignManagerUtils.createRequest(tag_id = 2,
                                                 preview_ccid = 2,
                                                 format = "js")

    request.ad_slots[0].size = '250x250'
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives),'selected creatives')

    tokens = dict()
    for line in response.ad_slots[0].creative_body.split('\n'):
      matchObj = re.match(r"^(.+) = (.*)$", line)
      if (matchObj):
        tokens[matchObj.group(1)] = matchObj.group(2)

    self.assertEqual('1', tokens['TEST_REQUEST'])
    self.assertEqual('http://www.clickurl.com?and=query_tag_2_site_2_pub_2_ccg_2_cmp_2', tokens['CRCLICK'])
    self.assertEqual('2', tokens['CID'])
    self.assertEqual('2', tokens['CGID'])
    self.assertEqual('', tokens['PUBPRECLICK'])
    request_id = base64.urlsafe_b64encode(response.ad_slots[0].selected_creatives[0].request_id).replace("==", "..")
    self.assertEqual('http://' + ConfigVars.HOST + ":" + str(ConfigVars.PORT_BASE + 80) + '/services/AdClickServer/' + \
      'ccid*eql*2*amp*requestid*eql*' + request_id + '*amp*cmi*eql*1_1*amp*colo*eql*1*amp*tid*eql*2*amp*tsid*eql*1*amp*h*eql*40*amp*u*eql*MDEyMzQ1Njc4OTAxMjM0NQ..', tokens['CLICK0'])

    # request 2
    request = CampaignManagerUtils.createRequest(publisher_account_id = 2,
                                                 preview_ccid = 2,
                                                 format = "js",
                                                 request_type = 3,
                                                 preclick_url = "http://preclick.com?param=value")

    request.ad_slots[0].size = '250x250'
    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots),'ad slot size')
    self.assertEqual(1, len(response.ad_slots[0].selected_creatives),'selected creatives')
    
    tokens = dict()
    for line in response.ad_slots[0].creative_body.split('\n'):
      matchObj = re.match(r"^(.+) = (.*)$", line)
      if (matchObj):
        tokens[matchObj.group(1)] = matchObj.group(2)

    self.assertEqual('1', tokens['TEST_REQUEST'])
    self.assertEqual('http://www.clickurl.com?and=query_tag_2_site_2_pub_2_ccg_2_cmp_2', tokens['CRCLICK'])
    self.assertEqual('2', tokens['CID'])
    self.assertEqual('2', tokens['CGID'])
    self.assertEqual('http://preclick.com?param=value', tokens['PUBPRECLICK'])
    request_id = base64.urlsafe_b64encode(response.ad_slots[0].selected_creatives[0].request_id).replace("==", "..")
    self.assertEqual('http://' + ConfigVars.HOST + ":" + str(ConfigVars.PORT_BASE + 80) + '/services/AdClickServer/' + \
      'ccid*eql*2*amp*requestid*eql*' + request_id + '*amp*cmi*eql*1_1*amp*colo*eql*1*amp*tid*eql*2*amp*tsid*eql*1*amp*h*eql*40*amp*u*eql*MDEyMzQ1Njc4OTAxMjM0NQ..', tokens['CLICK0'])

  def testRequestToInvalidCampaign( self ):
    "request to invalid campaign"
    click_info = CampaignManagerUtils.ClickInfo(
      time2orb(currentTime()),
      1, # colo_id
      2, # tag_id
      1, # tag_size_id
      986887, # ccid
      2, # keyword_id
      0, # creative_id
      "ZfLhIiw9RZiJZgKchTKxFA..",
      CampaignManagerUtils.UserIdHashModInfo(False, 0),
      "",  # relocate
      "",  # referer
      True # log_click
      )
    
    result = self.CampaignObject.get_click_url(click_info)
    self.assertEqual('http://www.clickurl.com/?cid=1000&cgid=10000', result[1].url)

    request = CampaignManagerUtils.createRequest(publisher_account_id = 2,
                                                 preview_ccid = 986887,
                                                 format = "js",
                                                 request_type = 2)

    request.ad_slots[0].size = '250x250'
    response = self.CampaignObject.get_campaign_creative(request)
    self.assertEqual(1, len(response.ad_slots), 'ad slot size')
    self.assertEqual(0, len(response.ad_slots[0].selected_creatives), 'selected creatives')

    instantiate_ad_info = CampaignManagerUtils.InstantiateAdInfo(
      common_info = request.common_info,
      format = "js",
      publisher_account_id = 0,
      tag_id = 2,
      tag_size_id = 1,
      creative_id = 0,
      creatives = [CampaignManagerUtils.InstantiateCreativeInfo(
                     ccid = 986887, 
                     ccg_keyword_id = 0,
                     request_id = "")],
      user_id_hash_mod = CampaignManagerUtils.UserIdHashModInfo(False, 0),
      merged_user_id = "",
      pubpixel_accounts = [],
      tanx_price = "",
      open_price = "",
      openx_price = "",
      liverail_price = "",
      baidu_price = "")

    try:
      instantiate_ad_result = self.CampaignObject.instantiate_ad(instantiate_ad_info)
    except CampaignManagerUtils.ImplementationException as e:
      self.assertEqual("CampaignManagerImpl::instantiate_ad(): creative not found", e.description, 'exception')

  def testPropProbAuctionWithMaxTextCreativeIsZero( self ):
    "campaign selection prop probability auction with max_text_creatives=0"

    request = CampaignManagerUtils.createRequest(
      tag_id = 213483,
      format = "html",
      debug_ccg = 10306897,
      ccg_keywords = [
        CCGKeywordInfo(
          11078847,             # ccg_keyword_id
          10306898,             # ccg_id
          15393280,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "",                   # click_url
          "ADSCOpenRTBText2"),  # original_keyword
        CCGKeywordInfo(
          11078849,             # ccg_keyword_id
          10306897,             # ccg_id
          15393282,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "http://keyword-click.com/Text11",  # click_url
          "ADSCOpenRTBText11")  # original_keyword
        ],
        only_display_ad = False
    )

    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots), 'ad slot size')
    self.assertEqual(0, len(response.ad_slots[0].selected_creatives),'selected creatives')

  def testPropProbAuctionAllTextFilteredByCPM( self ):
    "campaign selection prop probability auction when all text candidates are filtered by tag cpm"

    request = CampaignManagerUtils.createRequest(
      tag_id = 213484,
      format = "html",
      debug_ccg = 10306897,
      ccg_keywords = [
        CCGKeywordInfo(
          11078847,             # ccg_keyword_id
          10306898,             # ccg_id
          15393280,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "",                   # click_url
          "ADSCOpenRTBText2"),  # original_keyword
        CCGKeywordInfo(
          11078849,             # ccg_keyword_id
          10306897,             # ccg_id
          15393282,             # channel_id
          decimal2orb(0.0),     # max_cpc
          decimal2orb(0.0),     # ctr
          "http://keyword-click.com/Text11",  # click_url
          "ADSCOpenRTBText11")  # original_keyword
        ],
        only_display_ad = False
    )

    response = self.CampaignObject.get_campaign_creative(request)
    FunTest.tlog(10, "Trace response: %s" % response)
    self.assertEqual(1, len(response.ad_slots), 'ad slot size')
    self.assertEqual(0, len(response.ad_slots[0].selected_creatives),'selected creatives')

if __name__ == '__main__':
 main()


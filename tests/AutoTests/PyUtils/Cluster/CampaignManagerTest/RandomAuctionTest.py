import FunTest, time, math
from Util import currentTime
from OrbUtil import time2orb, decimal2orb, orb2decimal
from MockCampaignServer import TagSizeInfo
from CampaignManagerUtils import CCGKeywordInfo, createTag, createRequest, createCampaign
from MockCampaignServer import CreativeInfo, CreativeSizeInfo, OptionValueInfo, \
     ExpressionInfo, ExpressionChannelInfo

campaignManager = None

def prepareConfig(config):
  global campaignManager
  campaignManager = config
  now = currentTime()

  config.tags.extend([
    createTag(
      tag_id = 213110, 
      site_id = 1,
      auction_max_ecpm_share = decimal2orb(0.0),
      sizes = [
        TagSizeInfo(
          2,  # size_id
          1,  # max_text_creatives
          []) # tokens
     ])
  ])
  config.campaigns.extend([
    createCampaign(
      account_id = 1,
      campaign_id = 306227,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'D',
      creatives = [CreativeInfo(
        334026,                         # cc id
        318617,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    ),
    createCampaign(
      account_id = 1,
      campaign_id = 306228,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'D',
      creatives = [CreativeInfo(
        334027,                         # cc id
        318618,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    ),
    createCampaign(
      account_id = 1,
      campaign_id = 306230,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'T',
      creatives = [CreativeInfo(
        334028,                         # cc id
        318619,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    ),
    createCampaign(
      account_id = 3,
      campaign_id = 306245,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'T',
      creatives = [CreativeInfo(
        334029,                         # cc id
        318620,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    ),
    createCampaign(
      account_id = 3,
      campaign_id = 306246,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'T',
      creatives = [CreativeInfo(
        334030,                         # cc id
        318621,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    ),
    createCampaign(
      account_id = 3,
      campaign_id = 306247,
      mode = 0,
      ccg_rate_type = 'C',
      marketplace = 'O',
      ccg_type = 'T',
      creatives = [CreativeInfo(
        334031,                         # cc id
        318622,                         # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          2,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Text",                         # format
        OptionValueInfo(0, "http://www.foros.com"), # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [],                             # tokens
        'A',                            # status
        ''                              # version_id
      )]
    )
  ])

# ccg_id  account_id  ccg type
# 306227              D
# 306228              D
# 306230  1           T
# 306245  3           T
# 306246  3           T
# 306247  3           T 
#
# [EXPECTED]
# ccgs 306227, 306228, 306230 match with 25% probability each
# ccgs 306245, 306246, 306247 match with 25%/3 probability each (same 'text candidates group')
#
def baseTest ():
  "base random auction test"
  global campaignManager

  request = createRequest(
      tag_id = 213110,
      format = "js",
      debug_ccg = 306230,
      only_display_ad = False,
      channels = [16],
    )

  result = dict()
  N = 1000

  for x in range(0, N):
    response = campaignManager.CampaignObject.get_campaign_creative(request)
    campaignManager.assertEqual(1, len(response.ad_slots), 'ad slot size')
    campaignManager.assertEqual(1, len(response.ad_slots[0].selected_creatives),'selected creatives')
    #print response.ad_slots[0].debug_info.trace_ccg
    cmp_id = response.ad_slots[0].selected_creatives[0].cmp_id
  
    if cmp_id in result.keys():
      result[cmp_id] += 1
    else:
      result[cmp_id] = 1

  # check probabilities inside of 0.998 confidence interval
  quantile = 3.090

  campaignManager.assertEqual(306227 in result.keys(), True)
  p = 0.25
  v = result[306227]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

  campaignManager.assertEqual(306228 in result.keys(), True)
  p = 0.25
  v = result[306228]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

  campaignManager.assertEqual(306230 in result.keys(), True)
  p = 0.25
  v = result[306230]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

  campaignManager.assertEqual(306245 in result.keys(), True)
  p = 0.25 / 3.0
  v = result[306245]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

  campaignManager.assertEqual(306246 in result.keys(), True)
  p = 0.25 / 3.0
  v = result[306246]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

  campaignManager.assertEqual(306247 in result.keys(), True)
  p = 0.25 / 3.0
  v = result[306247]
  campaignManager.assert_(abs(v - p * N) < quantile * math.sqrt(p * (1 - p) * N), "expected = {0}, actual = {1}".format(p * N, v))

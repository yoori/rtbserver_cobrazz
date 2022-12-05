import FunTest, time, re, ConfigVars
from Util import currentTime
from OrbUtil import time2orb, decimal2orb, orb2decimal
from MockCampaignServer import TagSizeInfo
from CampaignManagerUtils import createRequest, createCampaign
from MockCampaignServer import CreativeInfo, CreativeSizeInfo, OptionValueInfo, \
     ExpressionInfo, ExpressionChannelInfo

campaignManager = None

def prepareConfig(config):
  global campaignManager
  campaignManager = config
  now = currentTime()

  config.expression_channels.extend([
    ExpressionChannelInfo(
      17,              # channel_id
      "ExprChannel#17", # name
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
      )]
    )
  config.campaigns.extend([
    createCampaign(
      account_id = 1,
      campaign_id = 100,
      ccg_id = 100,
      mode = 1,
      ccg_type = 'T',
      ccg_rate_type = 'M',
      expression = ExpressionInfo(
        '-',  # operation
        17,   # channel_id
        []),
      creatives = [CreativeInfo(
        101,                            # cc id
        101,                            # creative id
        0,                              # fc_id 
        1,                              # weight
        [CreativeSizeInfo(
          1,  # size_id
          0,  # up_expand_space
          0,  # right_expand_space
          0,  # down_expand_space
          0,  # down_expand_space
          []) # tokens
        ],
        "Image",                        # format
        OptionValueInfo(0, ""),         # click_url
        OptionValueInfo(104, ""),       # html_url
        0,                              # order_set_id
        [],                             # categories  
        [OptionValueInfo(102, "http://www.displayurl.com")], # tokens
        'A',                            # status
        ''                              # version_id
      )]
    )])

def parseTokens(str):
  tokens = dict()
  for line in str.split('\n'):
    matchObj = re.match(r"^(.+) = (.*)$", line)
    if (matchObj):
      tokens[matchObj.group(1)] = matchObj.group(2)
  return tokens

def instantiateClickTokensForCreativeWithEmptyClickUrl():
  "instantiate CLICK and CLICK0 for creative with empty click_url"
  global campaignManager

  request = createRequest(
    tag_id = 2,
    format = "js",
    debug_ccg = 100,
    only_display_ad = False,
    channels = [17],
    ad_instantiate_type = 2) # AIT_BODY
  request.ad_slots[0].size = '250x250'
  response = campaignManager.CampaignObject.get_campaign_creative(request)

  campaignManager.assertEqual(1, len(response.ad_slots),'ad slot size')
  #print response.ad_slots[0].debug_info.trace_ccg
  campaignManager.assertEqual(1, len(response.ad_slots[0].selected_creatives), 'selected creatives')
  #print response.ad_slots[0].creative_body
  tokens = parseTokens(response.ad_slots[0].creative_body)
  campaignManager.assertEqual('', tokens['CLICK'])
  campaignManager.assertEqual('', tokens['CLICK0'])

  request = createRequest(
    tag_id = 2,
    format = "js",
    debug_ccg = 100,
    only_display_ad = False,
    channels = [17],
    ad_instantiate_type = 2, # AIT_BODY
    request_type = 2, # AR_TANX
    publisher_account_id = 2)
  request.ad_slots[0].size = '250x250'
  response = campaignManager.CampaignObject.get_campaign_creative(request)

  campaignManager.assertEqual(1, len(response.ad_slots),'ad slot size')
  campaignManager.assertEqual(1, len(response.ad_slots[0].selected_creatives), 'selected creatives')
  campaignManager.assertEqual('http://' + ConfigVars.HOST + ":" + str(ConfigVars.PORT_BASE + 80) + '/services/AdClickServer', response.ad_slots[0].selected_creatives[0].click_url)

  request = createRequest(
    tag_id = 2,
    format = "js",
    debug_ccg = 100,
    only_display_ad = False,
    channels = [17],
    ad_instantiate_type = 2, # AIT_BODY
    request_type = 4, # AR_OPENRTB_WITH_CLICKURL
    publisher_account_id = 2)
  request.ad_slots[0].size = '250x250'
  response = campaignManager.CampaignObject.get_campaign_creative(request)

  campaignManager.assertEqual(1, len(response.ad_slots),'ad slot size')
  campaignManager.assertEqual(1, len(response.ad_slots[0].selected_creatives), 'selected creatives')
  tokens = parseTokens(response.ad_slots[0].creative_body)
  campaignManager.assertEqual('', tokens['CLICK'])
  campaignManager.assertEqual('', tokens['CLICK0'])

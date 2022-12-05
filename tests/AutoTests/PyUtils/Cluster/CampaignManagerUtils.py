
import time, CORBACommons
from Util import currentTime
from OrbUtil import time2orb, orb2time, decimal2orb
from AdServer.CampaignSvcs_v340 import CampaignManager
from MockCampaignServer import SiteInfo, TagPricingInfo, TagInfo, CampaignInfo, \
     ExpressionInfo, CreativeInfo, CreativeSizeInfo, OptionValueInfo, CampaignDeliveryLimitsInfo

REQUEST_ID  = "ZfLhIiw9RZiJZgKchTKxFA.."
USER_ID = "0123456789012345"

DAY = 24*60*60
now = currentTime()
yesterday = (int(time.time()) - DAY, 0)
tommorow  = (int(time.time()) + DAY, 0)

RequestParams = CampaignManager.RequestParams
CreativeSelectResult = CampaignManager.CreativeSelectResult
GeoInfo = CampaignManager.GeoInfo
TriggerMatchResult = CampaignManager.TriggerMatchResult
AdSlotInfo =  CampaignManager.AdSlotInfo
CommonAdRequestInfo = CampaignManager.CommonAdRequestInfo
CCGKeywordInfo = CampaignManager.CCGKeywordInfo
ClickInfo = CampaignManager.ClickInfo
UserIdHashModInfo = CampaignManager.UserIdHashModInfo
InstantiateAdInfo = CampaignManager.InstantiateAdInfo
InstantiateCreativeInfo = CampaignManager.InstantiateCreativeInfo
ImplementationException = CampaignManager.ImplementationException


US_UNDEFINED = 1
US_OPTIN = 2
US_OPTOUT = 3
US_PROBE = 4
US_TEMPORARY = 5
US_NONE = 6


def createRequest(current_time = time2orb(currentTime()),
                  tag_id = 0,
                  ext_tag_id = "",
                  request_id = REQUEST_ID,
                  full_freq_caps = [],
                  full_virtual_freq_caps = [],
                  seq_orders = [],
                  location = [ GeoInfo(
                                      "gn", # country_code
                                      "",   # region
                                      ""    # city
                                      )
                              ],
                  coord_location = [],
                  colo_id = 1,
                  format = "unit-test",
                  household_id = USER_ID,
                  user_id = USER_ID,
                  signed_user_id = "",
                  merged_user_id = USER_ID,
                  track_user_id = "",
                  tag_inventory = False,
                  user_status = US_OPTIN,
                  user_agent = "Unknown",
                  referer_hash = 0,
                  client = "Test client",
                  client_version = "1.0",
                  peer_ip = "192.168.1.1",
                  platform_ids = [],
                  platform = "Win",
                  full_platform = "Win 5.1 PI=2",
                  web_browser = "Mozilla 5.0",
                  cohort = '', # current value from cookies,
                  hpos = 256, # UNDEFINED_PUB_POSITION_BOTTOM
                  search_engine_id = 0,

                  page_keywords_present = True,
                  profiling_available = True,
                  fraud = False,
                  test_request = False,
                  log_as_test = True,
                  disable_fraud_detection = False,

                  enabled_notice = False,
                  fill_track_pixel = False,
                  fill_iurl = False,
                  random = 0,
                  original_url = "http://www.test.ocslab.com",

                  channels = [],
                  hid_channels = [],
                  geo_channels = [],
                  ccg_keywords = [],
                  hid_ccg_keywords = [],

                  publisher_account_id = 0,
                  publisher_site_id = 0,
                  request_type = 1, # AR_NORMAL
                  external_user_id = "",
                  source_id = "",
                  creative_instantiate_type = "unsecure",
                  trigger_match_result = TriggerMatchResult([],[],[],[], []),

                  client_create_time = time2orb(currentTime()),
                  client_last_request = time2orb(currentTime()),
                  session_start = time2orb(currentTime()),
                  exclude_pubpixel_accounts = [],
                  required_passback = False,
                  profile_referer = False,
                  ip_hash = "",
                  tag_delivery_factor = 0,
                  page_load_id = 0,
                  preview_ccid = 0,
                  request_token = "",
                  preclick_url = "",
                  click_prefix_url = "",
                  impr_track_url = "",
                  search_words = "",
                  page_keywords = "",
                  url_keywords = "",
                  debug_ccg = 0,
                  passback_type = "",
                  passback_url = "",
                  need_debug_info = True,
                  only_display_ad = True,
                  security_token = "",
                  pub_impr_track_url = "",
                  referer="",
                  urls=[],
                  ad_instantiate_type=1, # url
                  profiling_type = 2     # PT_ALL
                  ):
  return RequestParams(
    CommonAdRequestInfo(time2orb(currentTime()),
                        request_id,
                        creative_instantiate_type,
                        request_type,
                        random,
                        test_request,
                        log_as_test,
                        colo_id,
                        publisher_account_id,
                        publisher_site_id,
                        external_user_id,
                        source_id,
                        location,
                        coord_location,
                        referer,
                        urls,
                        security_token,
                        pub_impr_track_url,  
                        preclick_url,
                        click_prefix_url,
                        original_url,
                        track_user_id,
                        user_id,
                        user_status,
                        signed_user_id,
                        peer_ip,                        
                        cohort,
                        hpos,
                        passback_type,
                        passback_url),
    enabled_notice,
    fill_track_pixel,
    fill_iurl,
    ad_instantiate_type,
    current_time,
    only_display_ad,
    full_freq_caps,
    full_virtual_freq_caps,
    seq_orders,
    household_id,
    merged_user_id,
    tag_inventory,
    user_agent,
    referer_hash,
    client,
    client_version,
    platform_ids,
    platform,
    full_platform,
    web_browser,
    search_engine_id,
    search_words,
    page_keywords_present,
    profiling_available,
    fraud,
    channels,
    hid_channels,
    geo_channels,
    ccg_keywords,
    hid_ccg_keywords,
    trigger_match_result,
    client_create_time,
    session_start,
    exclude_pubpixel_accounts,
    tag_delivery_factor,
    page_load_id,
    preview_ccid,
    [AdSlotInfo(1,        # ad_slot_id;
                format,   #format
                tag_id,   # tag_id;
                "",       # size
                ext_tag_id, # ext_tag_id;
                decimal2orb(0.0), # min_ecpm
                "USD",    # min_ecpm_currency_code
                False,   # passback
                0,       # up_expand_space;
                0,       # right_expand_space;
                0,       # left_expand_space;
                0,       # tag_visibility;
                0,       # down_expand_space;
                0,       # video_min_duration;
                2147483647, # video_max_duration;
                [],      # exclude_categories
                debug_ccg, # debug_ccg,
                []       # allowed_durations
                )],
    required_passback,
    profile_referer,
    ip_hash,
    profiling_type,
    disable_fraud_detection,
    need_debug_info,
    page_keywords,
    url_keywords)

def createSite(site_id,
               account_id,
               status = 'A',
               freq_cap_id = 0,
               noads_timeout = 0,
               approved_creative_categories = [],
               rejected_creative_categories = [],
               approved_creatives = [],
               rejected_creatives = [],
               flags = 0,
               timestamp = time2orb(currentTime())
               ):
  return SiteInfo(
    site_id,
    status,
    freq_cap_id,
    noads_timeout,
    approved_creative_categories,
    rejected_creative_categories,
    approved_creatives,
    rejected_creatives,
    flags,
    account_id,
    timestamp)

def createTag(tag_id,
              site_id,
              status = 'A',
              sizes = [],
              imp_track_pixel = "",
              passback = "",
              passback_type = "",
              flags = 0,
              marketplace = 'O',
              adjustment = decimal2orb(1.0),
              tag_pricings = [TagPricingInfo("gn",               # country_code
                                             'D',                # ccg_type
                                             'M',                # ccg_rate_type
                                             1,                  # site_rate_id
                                             decimal2orb(0.0001),# imp_revenue
                                             decimal2orb(0.0) # revenue_share
                                             )],
              accepted_categories = [],
              rejected_categories = [],
              allow_expandable = False,
              tokens = [],
              hidden_tokens = [],
              passback_tokens = [],
              template_tokens = [],
              auction_max_ecpm_share = decimal2orb(1.0),
              auction_prop_probability_share = decimal2orb(0.0),
              auction_random_share = decimal2orb(0.0),
              tag_pricings_timestamp = time2orb(currentTime()),
              timestamp = time2orb(currentTime())):
  return TagInfo(
    tag_id,
    site_id,
    status,
    sizes,
    imp_track_pixel,
    passback,
    passback_type,
    flags,
    marketplace,
    adjustment,
    tag_pricings , 
    accepted_categories,
    rejected_categories,
    allow_expandable,
    tokens,
    hidden_tokens,
    passback_tokens,
    template_tokens,
    auction_max_ecpm_share,
    auction_prop_probability_share,
    auction_random_share,
    tag_pricings_timestamp,
    timestamp)

def createCampaign(
    campaign_id,
    ccg_id = 1,
    ccg_rate_id = 1,
    ccg_rate_type = 'A',
    fc_id = 0,
    group_fc_id = 0,
    priority = 0,
    flags = 32768,
    marketplace = 'A',
    expression = ExpressionInfo(
      '-',  # operation
      16,   # channel_id
      []),  # sub_channels
    stat_expression = ExpressionInfo(
      '-',  # operation
      16,   # channel_id
      []),  # sub_channels
    country = "gn",
    sites = [1],
    status = 'A',
    eval_status = 'A',
    dailyrun_intervals = [],
    creatives = [],
    account_id = 1,
    advertiser_id = 1,
    exclude_pub_accounts = [],
    exclude_tags= [],
    imp_revenue = decimal2orb(0.5),
    click_revenue = decimal2orb(2.0),
    action_revenue = decimal2orb(3.0),
    commision = decimal2orb(0.0),
    ccg_type = 'D',
    target_type = 'C',
    campaign_delivery_limits = CampaignDeliveryLimitsInfo(
      time2orb(yesterday),   # date_start
      time2orb(tommorow),    # date_end
      decimal2orb(1000.0),   # budget
      decimal2orb(1000.0),   # daily_budget
      'A'),                  # delivery_pacing
    ccg_delivery_limits = CampaignDeliveryLimitsInfo(
      time2orb(yesterday),   # date_start
      time2orb(tommorow),    # date_end
      decimal2orb(1000.0),   # budget
      decimal2orb(1000.0),   # daily_budget
      'A'),                  # delivery_pacing
    start_user_group_id = 0,
    end_user_group_id = 99,
    max_pub_share = decimal2orb(1000.0),
    ctr_reset_id = 0,
    random_imps = 0,
    mode = 1,
    seq_set_rotate_imps = 0,
    min_uid_age = time2orb(0),
    colocations = [],
    timestamp = time2orb(now)):
  return CampaignInfo(
    campaign_id,
    ccg_id,
    ccg_rate_id,
    ccg_rate_type,
    fc_id,
    group_fc_id,
    priority,
    flags,
    marketplace,
    expression,
    stat_expression,
    country,   
    sites,
    status,
    eval_status,
    dailyrun_intervals,
    creatives,
    account_id, 
    advertiser_id,
    exclude_pub_accounts,
    exclude_tags,
    imp_revenue,
    click_revenue,
    action_revenue,
    commision,
    ccg_type,
    target_type,
    campaign_delivery_limits,
    ccg_delivery_limits,
    start_user_group_id, 
    end_user_group_id,
    max_pub_share,
    ctr_reset_id,
    random_imps,
    mode,
    seq_set_rotate_imps,
    min_uid_age,
    colocations,
    timestamp
  )

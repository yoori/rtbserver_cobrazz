
import struct
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time, decimal2orb
from FunTest import tlog
import AdServer.CampaignSvcs_v340, AdServer__POA.CampaignSvcs_v340
from TestComparison import ComparisonMixin
from CORBATestObj import CORBATestObj

from ProbeObj import ProbeObjMixin


SearchEnginesConfig = AdServer.CampaignSvcs_v340.DetectorsConfig
SimpleChannelAnswer = AdServer.CampaignSvcs_v340.SimpleChannelAnswer
ChannelServerChannelAnswer = AdServer.CampaignSvcs_v340.ChannelServerChannelAnswer
CampaignConfigUpdateInfo = AdServer.CampaignSvcs_v340.CampaignConfigUpdateInfo
CurrencyInfo = AdServer.CampaignSvcs_v340.CurrencyInfo
AccountInfo = AdServer.CampaignSvcs_v340.AccountInfo
SimpleChannelKey = AdServer.CampaignSvcs_v340.SimpleChannelKey
CSSimpleChannel = AdServer.CampaignSvcs_v340.CSSimpleChannel
BehavParameter = AdServer.CampaignSvcs_v340.BehavParameter
BehavParamInfo = AdServer.CampaignSvcs_v340.BehavParamInfo
BehaveInfo = AdServer.CampaignSvcs_v340.BehaveInfo
CampaignInfo = AdServer.CampaignSvcs_v340.CampaignInfo
CampaignEcpmInfo = AdServer.CampaignSvcs_v340.CampaignEcpmInfo
SiteInfo = AdServer.CampaignSvcs_v340.SiteInfo
TagInfo = AdServer.CampaignSvcs_v340.TagInfo
CreativeInfo = AdServer.CampaignSvcs_v340.CreativeInfo
ExpressionChannelInfo = AdServer.CampaignSvcs_v340.ExpressionChannelInfo
ExpressionInfo =  AdServer.CampaignSvcs_v340.ExpressionInfo
CreativeTemplateInfo = AdServer.CampaignSvcs_v340.CreativeTemplateInfo
CreativeTemplateFileInfo = AdServer.CampaignSvcs_v340.CreativeTemplateFileInfo
TagPricingInfo = AdServer.CampaignSvcs_v340.TagPricingInfo
ColocationInfo = AdServer.CampaignSvcs_v340.ColocationInfo
CampaignDeliveryLimitsInfo = AdServer.CampaignSvcs_v340.CampaignDeliveryLimitsInfo
CTT_NONE = AdServer.CampaignSvcs_v340.CTT_NONE
CTT_TEXT = AdServer.CampaignSvcs_v340.CTT_TEXT
CTT_XSLT = AdServer.CampaignSvcs_v340.CTT_XSLT
CampaignServer=AdServer.CampaignSvcs_v340.CampaignServer
GeoChannelInfo=AdServer.CampaignSvcs_v340.GeoChannelInfo
OptionValueInfo=AdServer.CampaignSvcs_v340.OptionValueInfo
CreativeOptionInfo = AdServer.CampaignSvcs_v340.CreativeOptionInfo
AppFormatInfo = AdServer.CampaignSvcs_v340.AppFormatInfo
SizeInfo = AdServer.CampaignSvcs_v340.SizeInfo
CreativeSizeInfo = AdServer.CampaignSvcs_v340.CreativeSizeInfo
TagSizeInfo = AdServer.CampaignSvcs_v340.TagSizeInfo


ObjectKey   = "CampaignServer_v340"

class CampaignServerObj(CORBATestObj):

  def __init__( self, test):
    CORBATestObj.__init__( self, 'CampaignServer',
                           AdServer__POA.CampaignSvcs_v340.CampaignServer,
                           test)

class CampaignServerTestMixin(ComparisonMixin):

  def setUp( self ):
    self.server_id                   = 1
    self.global_freq_cap_id          = 0
    self.currency_exchange_id        = 0
    self.max_keyword_ecpm            = 0
    self.tanx_publisher_account_id   = 0
    self.google_publisher_account_id = 0
    self.app_formats                 = []
    self.delete_app_formats          = []
    self.sizes                       = []
    self.delete_sizes                = []
    self.accounts                    = [] 
    self.deleted_accounts            = []
    self.activate_creative_options   = []
    self.delete_creative_options     = []
    self.campaigns                   = []
    self.deleted_campaigns           = []
    self.ecpms                       = []
    self.deleted_ecpms               = []
    self.sites                       = []
    self.deleted_sites               = []
    self.tags                        = []
    self.deleted_tags                = []
    self.frequency_caps              = []
    self.deleted_frequency_caps      = []
    self.colocations                 = []
    self.deleted_colocations         = []
    self.creative_templates          = []
    self.deleted_creative_templates  = []
    self.currencies                  = []
    self.discover_channels           = []
    self.expression_channels         = []
    self.deleted_expression_channels = []
    self.campaign_keywords           = []
    self.deleted_keywords            = []
    self.creative_categories         = []
    self.deleted_creative_categories = []
    self.adv_actions                 = []
    self.deleted_adv_actions         = []
    self.simple_channels             = []
    self.chsv_channels               = []
    self.deleted_simple_channels     = []
    self.category_channels           = []
    self.deleted_category_channels   = []
    self.behav_params                = []
    self.deleted_behav_params        = []
    self.key_behav_params            = []
    self.deleted_key_behav_params    = []
    self.fraud_conditions            = []
    self.deleted_fraud_conditions    = []
    self.search_engines              = []
    self.deleted_search_engines      = []
    self.web_browsers                = []
    self.deleted_web_browsers        = []
    self.platforms                   = []
    self.deleted_platforms           = []
    self.activate_geo_channels       = []
    self.delete_geo_channels         = []
    self.activate_geo_coord_channels = []
    self.delete_geo_coord_channels   = []
    self.activate_block_channels     = []
    self.delete_block_channels       = []
    self.string_dictionaries         = []
    self.delete_string_dictionaries  = []
    self.cost_limit                  = 0.0
    self.tzOffset                    = time2orb(0) # Timezone offset
    self.masterStamp                 = time2orb(currentTime())
    self.passbackDict                = {} # tag_id => passback URL
    self.activateCampaignServer()

  def activateCampaignServer( self ):
    campaignServer = CampaignServerObj(self)
    self.MockCampaignServerIOR = self.bindObject(ObjectKey, campaignServer)
    tlog(10, "CampaignServer '%s' activated" % ObjectKey)

  def getCampaignServer( self ):
    return self.getObject( ObjectKey, AdServer.CampaignSvcs_v340.CampaignServer)

  def CampaignServer_get_config( self, get_config_settings ):
    tlog(10, "CampaignServer.get_config")
    config = CampaignConfigUpdateInfo(self.server_id,            # server_id
                                      time2orb(currentTime()),   # master_stamp
                                      time2orb(currentTime()),   # first_load_stamp
                                      time2orb(currentTime()),   # finish_load_stamp
                                      time2orb(currentTime()),   # current_time
                                      self.global_freq_cap_id,   
                                      self.currency_exchange_id, 
                                      self.max_keyword_ecpm,
                                      self.tanx_publisher_account_id,
                                      self.google_publisher_account_id,
                                      time2orb(currentTime()),   # fraud_user_deactivate_period
                                      decimal2orb(self.cost_limit),
                                      time2orb(currentTime()),   # global_params_timestamp
                                      [], # self.app_formats,
                                      [], # self.delete_app_formats,
                                      [], # self.sizes
                                      [], # self.delete_sizes
                                      [], # self.accounts,
                                      [], # self.deleted_accounts,
                                      [], # self.activate_creative_options,
                                      [], # self.delete_creative_options,
                                      [], # self.campaigns,
                                      [], # self.deleted_campaigns,
                                      [], # self.ecpms,
                                      [], # self.deleted_ecpms,
                                      [], # self.sites,
                                      [], # self.deleted_sites,
                                      [], # self.tags,
                                      [], # self.deleted_tags,
                                      [], # self.frequency_caps,
                                      [], # self.deleted_frequency_caps,
                                      [], # self.simple_channels,
                                      [], # self.deleted_simple_channels,
                                      time2orb(currentTime()),   # geo_channels_timestamp
                                      [], # self.activate_geo_channels
                                      [], # self.delete_geo_channels
                                      [], # self.activate_geo_coord_channels
                                      [], # self.delete_geo_coord_channels
                                      [], # self.activate_block_channels
                                      [], # self.delete_block_channels
                                      [], # self.colocations,
                                      [], # self.deleted_colocations,
                                      [], # self.creative_templates,
                                      [], # self.deleted_creative_templates,
                                      [], # self.currencies,
                                      [], # self.expression_channels,
                                      [], # self.deleted_expression_channels,
                                      [], # self.campaign_keywords,
                                      [], # self.deleted_keywords,
                                      [], # self.creative_categories,
                                      [], # self.deleted_creative_categories,
                                      [], # self.adv_actions,
                                      [], # self.deleted_adv_actions,
                                      [], # self.category_channels,
                                      [], # self.deleted_category_channels,
                                      [], # self.behav_params,
                                      [], # self.deleted_behav_params,
                                      [], # self.key_behav_params,
                                      [], # self.deleted_key_behav_params,
                                      [], # self.fraud_conditions,
                                      [], # self.deleted_fraud_conditions,
                                      [], # self.search_engines,
                                      [], # self.deleted_search_engines,
                                      [], # self.web_browsers,
                                      [], # self.deleted_web_browsers,
                                      [], # self.platforms,
                                      [], # self.deleted_platforms,
                                      [], # self.string_dictionaries,
                                      []) # self.delete_string_dictionaries
    if get_config_settings.portion == 0:
      config = CampaignConfigUpdateInfo(self.server_id,            # server_id
                                      time2orb(currentTime()),   # master_stamp
                                      time2orb(currentTime()),   # first_load_stamp
                                      time2orb(currentTime()),   # finish_load_stamp
                                      time2orb(currentTime()),   # current_time
                                      self.global_freq_cap_id,   
                                      self.currency_exchange_id, 
                                      self.max_keyword_ecpm,
                                      self.tanx_publisher_account_id,
                                      self.google_publisher_account_id,
                                      time2orb(currentTime()),   # fraud_user_deactivate_period
                                      decimal2orb(self.cost_limit),
                                      time2orb(currentTime()),   # global_params_timestamp
                                      self.app_formats,
                                      self.delete_app_formats,
                                      self.sizes,
                                      self.delete_sizes,
                                      self.accounts,
                                      self.deleted_accounts,
                                      self.activate_creative_options,
                                      self.delete_creative_options,
                                      self.campaigns,
                                      self.deleted_campaigns,
                                      self.ecpms,
                                      self.deleted_ecpms,
                                      self.sites,
                                      self.deleted_sites,
                                      self.tags,
                                      self.deleted_tags,
                                      self.frequency_caps,
                                      self.deleted_frequency_caps,
                                      self.simple_channels,
                                      self.deleted_simple_channels,
                                      time2orb(currentTime()),   # geo_channels_timestamp
                                      self.activate_geo_channels,
                                      self.delete_geo_channels,
                                      self.activate_geo_coord_channels,
                                      self.delete_geo_coord_channels,
                                      self.activate_block_channels,
                                      self.delete_block_channels,
                                      self.colocations,
                                      self.deleted_colocations,
                                      self.creative_templates,
                                      self.deleted_creative_templates,
                                      self.currencies,
                                      self.expression_channels,
                                      self.deleted_expression_channels,
                                      self.campaign_keywords,
                                      self.deleted_keywords,
                                      self.creative_categories,
                                      self.deleted_creative_categories,
                                      self.adv_actions,
                                      self.deleted_adv_actions,
                                      self.category_channels,
                                      self.deleted_category_channels,
                                      self.behav_params,
                                      self.deleted_behav_params,
                                      self.key_behav_params,
                                      self.deleted_key_behav_params,
                                      self.fraud_conditions,
                                      self.deleted_fraud_conditions,
                                      self.search_engines,
                                      self.deleted_search_engines,
                                      self.web_browsers,
                                      self.deleted_web_browsers,
                                      self.platforms,
                                      self.deleted_platforms,
                                      self.string_dictionaries,
                                      self.delete_string_dictionaries)
    tlog(10, "CampaignServer.config: %s" % config)
    return config

  def get_behav_info( self, id):
    for param in self.behav_params:
      if param.id == id:
        def behav2info(behav):
          return BehaveInfo(
            (behav.time_to == 0 and behav.time_from == 0), # is_context
            behav.trigger_type,                            # trigger_type
            behav.weight                                   # weight
            )
        return map(behav2info, param.bp_seq)

  def get_chsv_channels( self ):
    def simple2chsv ( simple ):
      return CSSimpleChannel(simple.channel_id,   # channel_id
                             simple.country_code, # country_code
                             simple.language,     # language
                             'B',                 # channel_type
                             simple.status,       # status
                             self.get_behav_info(simple.behav_param_list_id) # behave_info
                             )
    return map(simple2chsv, self.simple_channels)
                                    
  def CampaignServer_update_stat( self ):
    tlog(10, "CampaignServer.update_stat")
                                    
  def CampaignServer_need_config( self, req_timestamp ):
    tlog(10, "CampaignServer.need_config")
    return True

  def CampaignServer_get_ecpms( self, request_timestamp ):
    tlog(10, "CampaignServer.get_ecpms")
    return self.ecpms

  def CampaignServer_get_expression_channels(self,
                              channel_types,
                              portion,
                              portions_number):
    tlog(10, "CampaignServer.get_expression_channels")
    return self.expression_channels

  def CampaignServer_get_discover_channels( self, portion, portions_number ):
    tlog(10, "CampaignServer.get_discover_channels")
    return self.discover_channels

  def CampaignServer_get_tag_passback( self, tag_id ):
    tlog(10, "CampaignServer.get_tag_passback")
    return self.passbackDict.get(tag_id, "")

  def CampaignServer_simple_channels( self, get_config_settings ):
    tlog(10, "CampaignServer.simple_channels")
    return SimpleChannelAnswer(self.simple_channels,
                               self.behav_params,
                               self.key_behav_params,
                               self.tzOffset,
                               self.masterStamp,
                               decimal2orb(self.cost_limit))

  def CampaignServer_chsv_simple_channels( self, params ):
    tlog(10, "CampaignServer.chsv_simple_channels#%d.%d %s" % (params.portion, params.portions_number, self.get_chsv_channels()))
    return ChannelServerChannelAnswer(self.get_chsv_channels(),
                                      decimal2orb(self.cost_limit))

  def CampaignServer_fraud_conditions( self ):
    tlog(10, "CampaignServer.fraud_conditions")
    return self.fraud_conditions
        
  def CampaignServer_detectors( self, request_timestamp ):
    tlog(10, "CampaignServer.search_engines %s" % time2str(orb2time(request_timestamp)))
    return DetectorsConfig( time2orb(currentTime()),
                            self.search_engines,
                            self.web_browsers,
                            self.platforms)


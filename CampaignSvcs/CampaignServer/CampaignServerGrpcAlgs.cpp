#include "CampaignServerGrpcAlgs.hpp"

#include <stdexcept>

namespace AdServer::CampaignSvcs::CampaignGrpc
{
  namespace
  {
    void validate_bytes(const std::string& source, std::size_t size, bool optional = false)
    {
      if (source.size() != size && !(optional && source.empty()))
      {
        throw std::invalid_argument("Invalid packed time/decimal width");
      }
    }
  }

  void validate(const rpc::AppFormatInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::SizeInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CreativeOptionInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::OptionValueInfo&) {}

  void validate(const rpc::CountryInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::ExternalCategoryInfo&) {}

  void validate(const rpc::CreativeCategoryInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.external_categories())
    {
      validate(value);
    }
  }

  void validate(const rpc::CreativeSizeInfo& source)
  {
    for (const auto& value : source.tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::CreativeInfo& source)
  {
    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    for (const auto& value : source.sizes())
    {
      validate(value);
    }

    validate(source.click_url());
    validate(source.html_url());
    for (const auto& value : source.tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::ChannelWeight&) {}

  void validate(const rpc::DeletedIdInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::DeletedStringIdInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CampaignEcpmInfo& source)
  {
    validate_bytes(source.ecpm(), 9);
    validate_bytes(source.ctr(), 9);
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::ExpressionInfo& source)
  {
    if (source.operation() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    for (const auto& value : source.sub_channels())
    {
      validate(value);
    }
  }

  void validate(const rpc::ExpressionChannelInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    validate_bytes(source.imp_revenue(), 9, !source.channel_rate_id());
    validate_bytes(source.click_revenue(), 9, !source.channel_rate_id());
    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    validate(source.expression());
  }

  void validate(const rpc::BehavioralParameterInfo& source)
  {
    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::WeeklyRunInterval&) {}

  void validate(const rpc::CampaignDeliveryLimitsInfo& source)
  {
    validate_bytes(source.date_start(), 8);
    validate_bytes(source.date_end(), 8);
    validate_bytes(source.budget(), 9, true);
    validate_bytes(source.daily_budget(), 9, true);
    if (source.delivery_pacing() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::TagDeliveryInfo&) {}

  void validate(const rpc::ContractInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CampaignInfo& source)
  {
    validate_bytes(source.imp_revenue(), 9);
    validate_bytes(source.click_revenue(), 9);
    validate_bytes(source.action_revenue(), 9);
    validate_bytes(source.commision(), 9);
    validate_bytes(source.max_pub_share(), 9);
    validate_bytes(source.min_uid_age(), 8);
    validate_bytes(source.min_ctr_goal(), 9);
    validate_bytes(source.timestamp(), 8);
    validate_bytes(source.min_vtr_goal(), 9);
    if (source.ccg_rate_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.marketplace() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.eval_status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.ccg_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.target_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    validate(source.expression());
    validate(source.stat_expression());
    for (const auto& value : source.weekly_run_intervals())
    {
      validate(value);
    }

    for (const auto& value : source.creatives())
    {
      validate(value);
    }

    for (const auto& value : source.exclude_tags())
    {
      validate(value);
    }

    validate(source.campaign_delivery_limits());
    validate(source.ccg_delivery_limits());
  }

  void validate(const rpc::TagPricingInfo& source)
  {
    validate_bytes(source.imp_revenue(), 9);
    validate_bytes(source.revenue_share(), 9);
    if (source.ccg_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.ccg_rate_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::TemplateOptionsInfo& source)
  {
    for (const auto& value : source.tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::TagSizeInfo& source)
  {
    for (const auto& value : source.tokens())
    {
      validate(value);
    }

    for (const auto& value : source.hidden_tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::TagInfo& source)
  {
    validate_bytes(source.adjustment(), 9);
    validate_bytes(source.auction_max_ecpm_share(), 9);
    validate_bytes(source.auction_prop_probability_share(), 9);
    validate_bytes(source.auction_random_share(), 9);
    validate_bytes(source.cost_coef(), 9);
    validate_bytes(source.tag_pricings_timestamp(), 8);
    validate_bytes(source.timestamp(), 8);
    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.marketplace() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    for (const auto& value : source.sizes())
    {
      validate(value);
    }

    for (const auto& value : source.tag_pricings())
    {
      validate(value);
    }

    for (const auto& value : source.tokens())
    {
      validate(value);
    }

    for (const auto& value : source.hidden_tokens())
    {
      validate(value);
    }

    for (const auto& value : source.passback_tokens())
    {
      validate(value);
    }

    for (const auto& value : source.template_tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::SiteInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::ColocationInfo& source)
  {
    validate_bytes(source.revenue_share(), 9);
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::ColocationFlags&) {}

  void validate(const rpc::FreqCapInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CreativeTemplateFileInfo& source)
  {
    if (!rpc::CreativeTemplateType_IsValid(source.type()))
    {
      throw std::invalid_argument("Invalid creative template type");
    }
  }

  void validate(const rpc::CreativeTemplateInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.files())
    {
      validate(value);
    }

    for (const auto& value : source.tokens())
    {
      validate(value);
    }

    for (const auto& value : source.hidden_tokens())
    {
      validate(value);
    }
  }

  void validate(const rpc::CurrencyInfo& source)
  {
    validate_bytes(source.rate(), 9);
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CampaignKeywordInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::AdvActionInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::AccountInfo& source)
  {
    validate_bytes(source.time_offset(), 8);
    validate_bytes(source.commision(), 9);
    validate_bytes(source.budget(), 9);
    validate_bytes(source.paid_amount(), 9);
    validate_bytes(source.self_service_commission(), 9);
    validate_bytes(source.timestamp(), 8);
    if (source.text_adserving() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.eval_status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::CategoryChannelLocalizationInfo&) {}

  void validate(const rpc::CategoryChannelInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.localizations())
    {
      validate(value);
    }
  }

  void validate(const rpc::PriceRange&) {}

  void validate(const rpc::BehavParameter& source)
  {
    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::BehavParamInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.bp_seq())
    {
      validate(value);
    }
  }

  void validate(const rpc::KeyBehavParamInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.bp_seq())
    {
      validate(value);
    }
  }

  void validate(const rpc::BriefBehavParamInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.bp_seq())
    {
      validate(value);
    }
  }

  void validate(const rpc::BriefKeyBehavParamInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.bp_seq())
    {
      validate(value);
    }
  }

  void validate(const rpc::SimpleChannelKey& source)
  {
    validate_bytes(source.timestamp(), 8);
    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::BriefSimpleChannelKey&) {}

  void validate(const rpc::GeoIPRegionCityInfo&) {}

  void validate(const rpc::GeoChannelInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.geoip_targets())
    {
      validate(value);
    }
  }

  void validate(const rpc::GeoCoordChannelInfo& source)
  {
    validate_bytes(source.longitude(), 9);
    validate_bytes(source.latitude(), 9);
    validate_bytes(source.radius(), 9);
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::BlockChannelInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::FraudConditionInfo& source)
  {
    validate_bytes(source.period(), 8);
    validate_bytes(source.timestamp(), 8);
    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::SearchEngineRegExpInfo&) {}

  void validate(const rpc::SearchEngineInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.regexps())
    {
      validate(value);
    }
  }

  void validate(const rpc::WebBrowserDetectorInfo&) {}

  void validate(const rpc::WebBrowserInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.detectors())
    {
      validate(value);
    }
  }

  void validate(const rpc::PlatformDetectorInfo&) {}

  void validate(const rpc::PlatformInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.detectors())
    {
      validate(value);
    }
  }

  void validate(const rpc::WebOperationInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::CampaignConfigUpdateInfo& source)
  {
    validate_bytes(source.master_stamp(), 8);
    validate_bytes(source.first_load_stamp(), 8);
    validate_bytes(source.finish_load_stamp(), 8);
    validate_bytes(source.current_time(), 8);
    validate_bytes(source.fraud_user_deactivate_period(), 8);
    validate_bytes(source.cost_limit(), 9);
    validate_bytes(source.global_params_timestamp(), 8);
    validate_bytes(source.geo_channels_timestamp(), 8);
    for (const auto& value : source.app_formats())
    {
      validate(value);
    }

    for (const auto& value : source.delete_app_formats())
    {
      validate(value);
    }

    for (const auto& value : source.sizes())
    {
      validate(value);
    }

    for (const auto& value : source.delete_sizes())
    {
      validate(value);
    }

    for (const auto& value : source.countries())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_countries())
    {
      validate(value);
    }

    for (const auto& value : source.accounts())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_accounts())
    {
      validate(value);
    }

    for (const auto& value : source.activate_creative_options())
    {
      validate(value);
    }

    for (const auto& value : source.delete_creative_options())
    {
      validate(value);
    }

    for (const auto& value : source.campaigns())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_campaigns())
    {
      validate(value);
    }

    for (const auto& value : source.ecpms())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_ecpms())
    {
      validate(value);
    }

    for (const auto& value : source.sites())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_sites())
    {
      validate(value);
    }

    for (const auto& value : source.tags())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_tags())
    {
      validate(value);
    }

    for (const auto& value : source.frequency_caps())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_freq_caps())
    {
      validate(value);
    }

    for (const auto& value : source.simple_channels())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_simple_channels())
    {
      validate(value);
    }

    for (const auto& value : source.activate_geo_channels())
    {
      validate(value);
    }

    for (const auto& value : source.delete_geo_channels())
    {
      validate(value);
    }

    for (const auto& value : source.activate_geo_coord_channels())
    {
      validate(value);
    }

    for (const auto& value : source.delete_geo_coord_channels())
    {
      validate(value);
    }

    for (const auto& value : source.activate_block_channels())
    {
      validate(value);
    }

    for (const auto& value : source.delete_block_channels())
    {
      validate(value);
    }

    for (const auto& value : source.colocations())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_colocations())
    {
      validate(value);
    }

    for (const auto& value : source.creative_templates())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_templates())
    {
      validate(value);
    }

    for (const auto& value : source.currencies())
    {
      validate(value);
    }

    for (const auto& value : source.expression_channels())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_expression_channels())
    {
      validate(value);
    }

    for (const auto& value : source.campaign_keywords())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_keywords())
    {
      validate(value);
    }

    for (const auto& value : source.creative_categories())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_creative_categories())
    {
      validate(value);
    }

    for (const auto& value : source.adv_actions())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_adv_actions())
    {
      validate(value);
    }

    for (const auto& value : source.category_channels())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_category_channels())
    {
      validate(value);
    }

    for (const auto& value : source.behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.key_behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_key_behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.fraud_conditions())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_fraud_conditions())
    {
      validate(value);
    }

    for (const auto& value : source.search_engines())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_search_engines())
    {
      validate(value);
    }

    for (const auto& value : source.web_browsers())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_web_browsers())
    {
      validate(value);
    }

    for (const auto& value : source.platforms())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_platforms())
    {
      validate(value);
    }

    for (const auto& value : source.web_operations())
    {
      validate(value);
    }

    for (const auto& value : source.delete_web_operations())
    {
      validate(value);
    }

    for (const auto& value : source.contracts())
    {
      validate(value);
    }

    for (const auto& value : source.deleted_contracts())
    {
      validate(value);
    }
  }

  void validate(const rpc::CreativeStatInfo& source)
  {
    validate_bytes(source.impressions(), 17);
    validate_bytes(source.clicks(), 17);
    validate_bytes(source.actions(), 17);
  }

  void validate(const rpc::PublisherCCGStatInfo& source)
  {
    validate_bytes(source.amount(), 9);
  }

  void validate(const rpc::TagCCGStatInfo& source)
  {
    validate_bytes(source.cur_pub_isp_amount(), 9);
    validate_bytes(source.cur_adv_amount(), 9);
    validate_bytes(source.cur_adv_comm_amount(), 9);
    validate_bytes(source.prev_pub_isp_amount(), 9);
    validate_bytes(source.prev_adv_amount(), 9);
    validate_bytes(source.prev_adv_comm_amount(), 9);
  }

  void validate(const rpc::CtrResetStatInfo& source)
  {
    validate_bytes(source.impressions(), 17);
  }

  void validate(const rpc::CCGStatInfo& source)
  {
    validate_bytes(source.impressions(), 17);
    validate_bytes(source.clicks(), 17);
    validate_bytes(source.actions(), 17);
    validate_bytes(source.amount(), 9);
    validate_bytes(source.comm_amount(), 9);
    validate_bytes(source.daily_amount(), 9);
    validate_bytes(source.daily_comm_amount(), 9);
    validate_bytes(source.prev_hour_amount(), 9);
    validate_bytes(source.prev_hour_comm_amount(), 9);
    validate_bytes(source.current_hour_amount(), 9);
    validate_bytes(source.current_hour_comm_amount(), 9);
    for (const auto& value : source.creatives())
    {
      validate(value);
    }

    for (const auto& value : source.publishers())
    {
      validate(value);
    }

    for (const auto& value : source.tags())
    {
      validate(value);
    }

    for (const auto& value : source.ctr_resets())
    {
      validate(value);
    }
  }

  void validate(const rpc::CampaignStatInfo& source)
  {
    validate_bytes(source.amount(), 9);
    validate_bytes(source.comm_amount(), 9);
    validate_bytes(source.daily_amount(), 9);
    validate_bytes(source.daily_comm_amount(), 9);
    for (const auto& value : source.ccgs())
    {
      validate(value);
    }
  }

  void validate(const rpc::AmountStatInfo& source)
  {
    validate_bytes(source.amount(), 9);
    validate_bytes(source.comm_amount(), 9);
    validate_bytes(source.daily_amount(), 9);
    validate_bytes(source.daily_comm_amount(), 9);
  }

  void validate(const rpc::StatInfo& source)
  {
    for (const auto& value : source.accounts())
    {
      validate(value);
    }

    for (const auto& value : source.campaigns())
    {
      validate(value);
    }
  }

  void validate(const rpc::DayAmount& source)
  {
    validate_bytes(source.day(), 8);
    validate_bytes(source.amount(), 9);
  }

  void validate(const rpc::AmountDistributionInfo& source)
  {
    validate(source.prev_days_amount());
    for (const auto& value : source.day_amounts())
    {
      validate(value);
    }
  }

  void validate(const rpc::AccountAmountDistributionInfo& source)
  {
    validate(source.amount_distribution());
  }

  void validate(const rpc::DayAmountCount& source)
  {
    validate_bytes(source.day(), 8);
    validate_bytes(source.amount(), 9);
    validate_bytes(source.imps(), 17);
    validate_bytes(source.clicks(), 17);
  }

  void validate(const rpc::AmountCountDistributionInfo& source)
  {
    validate(source.prev_days_amount_count());
    for (const auto& value : source.day_amount_counts())
    {
      validate(value);
    }
  }

  void validate(const rpc::CampaignAmountDistributionInfo& source)
  {
    validate(source.amount_count_distribution());
  }

  void validate(const rpc::CCGAmountDistributionInfo& source)
  {
    validate(source.amount_count_distribution());
  }

  void validate(const rpc::BillStatInfo& source)
  {
    for (const auto& value : source.accounts())
    {
      validate(value);
    }

    for (const auto& value : source.campaigns())
    {
      validate(value);
    }

    for (const auto& value : source.ccgs())
    {
      validate(value);
    }
  }

  void validate(const rpc::DiscoverChannel& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::SimpleChannelAnswer& source)
  {
    validate_bytes(source.timezone_offset(), 8);
    validate_bytes(source.master_stamp(), 8);
    validate_bytes(source.cost_limit(), 9);
    for (const auto& value : source.simple_channels())
    {
      validate(value);
    }

    for (const auto& value : source.behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.key_behav_params())
    {
      validate(value);
    }
  }

  void validate(const rpc::BriefSimpleChannelAnswer& source)
  {
    validate_bytes(source.timezone_offset(), 8);
    validate_bytes(source.master_stamp(), 8);
    for (const auto& value : source.simple_channels())
    {
      validate(value);
    }

    for (const auto& value : source.behav_params())
    {
      validate(value);
    }

    for (const auto& value : source.key_behav_params())
    {
      validate(value);
    }
  }

  void validate(const rpc::BehaveInfo& source)
  {
    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }
  }

  void validate(const rpc::CSSimpleChannel& source)
  {
    if (source.channel_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    for (const auto& value : source.behave_info())
    {
      validate(value);
    }
  }

  void validate(const rpc::ChannelServerChannelAnswer& source)
  {
    validate_bytes(source.cost_limit(), 9);
    for (const auto& value : source.simple_channels())
    {
      validate(value);
    }
  }

  void validate(const rpc::CampaignGetConfigSettings& source)
  {
    validate_bytes(source.timestamp(), 8);
    validate_bytes(source.geo_channels_timestamp(), 8);
  }

  void validate(const rpc::FraudConditionConfig& source)
  {
    validate_bytes(source.deactivate_period(), 8);
    for (const auto& value : source.rules())
    {
      validate(value);
    }
  }

  void validate(const rpc::DetectorsConfig& source)
  {
    validate_bytes(source.timestamp(), 8);
    for (const auto& value : source.engines())
    {
      validate(value);
    }

    for (const auto& value : source.web_browsers())
    {
      validate(value);
    }

    for (const auto& value : source.platforms())
    {
      validate(value);
    }
  }

  void validate(const rpc::FreqCapConfigInfo& source)
  {
    for (const auto& value : source.freq_caps())
    {
      validate(value);
    }
  }

  void validate(const rpc::ExpressionChannelCCGInfo&) {}

  void validate(const rpc::ChannelTriggersInfo&) {}

  void validate(const rpc::ExpressionChannelsInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
    validate_bytes(source.first_load_stamp(), 8);
    for (const auto& value : source.expression_channels())
    {
      validate(value);
    }

    for (const auto& value : source.expression_channel_ccgs())
    {
      validate(value);
    }

    for (const auto& value : source.activate_channel_triggers())
    {
      validate(value);
    }

    for (const auto& value : source.delete_simple_channels())
    {
      validate(value);
    }

    for (const auto& value : source.activate_actions())
    {
      validate(value);
    }

    for (const auto& value : source.delete_actions())
    {
      validate(value);
    }
  }

  void validate(const rpc::AccountDeliveryInfo& source)
  {
    validate_bytes(source.time_offset(), 8);
    validate_bytes(source.budget(), 9);
  }

  void validate(const rpc::CampaignDeliveryInfo& source)
  {
    validate_bytes(source.time_offset(), 8);
    validate(source.delivery_limits());
  }

  void validate(const rpc::CCGDeliveryInfo& source)
  {
    validate_bytes(source.time_offset(), 8);
    validate_bytes(source.imp_amount(), 9);
    validate_bytes(source.click_amount(), 9);
    validate(source.delivery_limits());
  }

  void validate(const rpc::GetExpressionChannelsInfo& source)
  {
    validate_bytes(source.timestamp(), 8);
  }

  void validate(const rpc::GetSimpleChannelsInfo&) {}

  void validate(const rpc::PassbackInfo&) {}

  void validate(const rpc::ColocationPropInfo& source)
  {
    validate_bytes(source.time_offset(), 8);
  }

  void validate(const rpc::DeliveryLimitConfigInfo& source)
  {
    for (const auto& value : source.accounts())
    {
      validate(value);
    }

    for (const auto& value : source.campaigns())
    {
      validate(value);
    }

    for (const auto& value : source.ccgs())
    {
      validate(value);
    }
  }
}

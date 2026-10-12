#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <Commons/WireTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>
#include <stdexcept>
#include <CampaignSvcs/CampaignServer/StatTypes.hpp>
#include <CampaignSvcs/CampaignServer/BillStatTypes.hpp>

namespace AdServer::CampaignSvcs
{
  struct DiscoverChannel
  {
    std::uint32_t channel_id{};
    std::string query{};
    std::string annotation{};
    std::string language{};
    ULongSeq categories{};
    TimestampInfo timestamp{};
  };

  using DiscoverChannelSeq = std::vector<DiscoverChannel>;

  struct SimpleChannelAnswer
  {
    SimpleChannelKeySeq simple_channels{};
    BehavParamInfoSeq behav_params{};
    KeyBehavParamInfoSeq key_behav_params{};
    TimestampInfo timezone_offset{};
    TimestampInfo master_stamp{};
    AdServer::Commons::DecimalInfo cost_limit{};
  };

  struct BriefSimpleChannelAnswer
  {
    BriefSimpleChannelKeySeq simple_channels{};
    BriefBehavParamInfoSeq behav_params{};
    BriefKeyBehavParamInfoSeq key_behav_params{};
    AdServer::Commons::FixedTimestampInfo timezone_offset = std::string(8, '\0');
    AdServer::Commons::FixedTimestampInfo master_stamp = std::string(8, '\0');
  };

  struct BehaveInfo
  {
    bool is_context{};
    char trigger_type{};
    std::uint32_t weight{};
  };

  using BehaveInfoSeq = std::vector<BehaveInfo>;

  struct CSSimpleChannel
  {
    std::uint32_t channel_id{};
    std::string country_code{};
    std::string language{};
    char channel_type{};
    char status{};
    BehaveInfoSeq behave_info{};
  };

  using CSSimpleChannelSeq = std::vector<CSSimpleChannel>;

  struct ChannelServerChannelAnswer
  {
    CSSimpleChannelSeq simple_channels{};
    AdServer::Commons::DecimalInfo cost_limit{};
  };

  struct CampaignGetConfigSettings
  {
    TimestampInfo timestamp{};
    std::uint32_t server_id{};
    std::uint32_t portion{};
    std::uint32_t portions_number{};
    std::uint32_t colo_id{};
    std::string version{};
    std::string campaign_statuses{};
    std::string channel_statuses{};
    std::string country{};
    bool no_deleted{};
    bool provide_only_tags{};
    bool provide_channel_triggers{};
    TimestampInfo geo_channels_timestamp{};
  };

  struct FraudConditionConfig
  {
    FraudConditionSeq rules{};
    TimestampInfo deactivate_period{};
  };

  struct DetectorsConfig
  {
    TimestampInfo timestamp{};
    SearchEngineSeq engines{};
    WebBrowserSeq web_browsers{};
    PlatformSeq platforms{};
  };

  struct FreqCapConfigInfo
  {
    FreqCapSeq freq_caps{};
    CampaignIdSeq campaign_ids{};
  };

  struct ExpressionChannelCCGInfo
  {
    std::uint32_t channel_id{};
    CCGIdSeq ccgs{};
  };

  using ExpressionChannelCCGSeq = std::vector<ExpressionChannelCCGInfo>;

  struct ChannelTriggersInfo
  {
    std::uint32_t channel_id{};
    std::uint32_t page_min_visits{};
    std::uint32_t search_min_visits{};
    std::uint32_t url_min_visits{};
    std::uint32_t url_keyword_min_visits{};
    std::uint32_t page_time_to{};
    std::uint32_t search_time_to{};
    std::uint32_t url_time_to{};
    std::uint32_t url_keyword_time_to{};
    ULongSeq page_triggers{};
    ULongSeq search_triggers{};
    ULongSeq url_triggers{};
    ULongSeq url_keyword_triggers{};
  };

  using ChannelTriggersSeq = std::vector<ChannelTriggersInfo>;

  struct ExpressionChannelsInfo
  {
    TimestampInfo timestamp{};
    TimestampInfo first_load_stamp{};
    ExpressionChannelSeq expression_channels{};
    ExpressionChannelCCGSeq expression_channel_ccgs{};
    ChannelIdSeq overlap_channel_ids{};
    ChannelTriggersSeq activate_channel_triggers{};
    DeletedIdSeq delete_simple_channels{};
    AdvActionInfoSeq activate_actions{};
    DeletedIdSeq delete_actions{};
  };

  struct AccountDeliveryInfo
  {
    std::uint32_t account_id{};
    AdServer::Commons::TimestampInfo time_offset{};
    bool active{};
    AdServer::Commons::DecimalInfo budget{};
  };

  using AccountDeliverySeq = std::vector<AccountDeliveryInfo>;

  struct CampaignDeliveryInfo
  {
    std::uint32_t campaign_id{};
    AdServer::Commons::TimestampInfo time_offset{};
    bool active{};
    CampaignDeliveryLimitsInfo delivery_limits{};
  };

  using CampaignDeliverySeq = std::vector<CampaignDeliveryInfo>;

  struct CCGDeliveryInfo
  {
    std::uint32_t ccg_id{};
    std::uint32_t campaign_id{};
    AdServer::Commons::TimestampInfo time_offset{};
    bool active{};
    CampaignDeliveryLimitsInfo delivery_limits{};
    AdServer::Commons::DecimalInfo imp_amount{};
    AdServer::Commons::DecimalInfo click_amount{};
  };

  using CCGDeliverySeq = std::vector<CCGDeliveryInfo>;

  namespace CampaignServer
  {
    class ImplementationException : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    class NotReady : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    class NotSupport : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    struct GetExpressionChannelsInfo
    {
      AdServer::Commons::TimestampInfo timestamp{};
      std::uint32_t portion{};
      std::uint32_t portions_number{};
      std::string channel_types{};
      std::string channel_statuses{};
      bool provide_ccg_links{};
      bool provide_channel_triggers{};
      bool provide_overlap_channel_ids{};
    };

    struct GetSimpleChannelsInfo
    {
      std::uint32_t portion{};
      std::uint32_t portions_number{};
      std::string channel_statuses{};
    };

    struct PassbackInfo
    {
      std::string passback_type{};
      std::string passback_code{};
      std::string passback_url{};
      std::uint32_t width{};
      std::uint32_t height{};
      std::string size{};
      std::string mime_format{};
      bool active{};
    };

    struct ColocationPropInfo
    {
      bool found{};
      std::uint32_t colo_id{};
      AdServer::Commons::TimestampInfo time_offset{};
    };

    struct DeliveryLimitConfigInfo
    {
      AccountDeliverySeq accounts{};
      CampaignDeliverySeq campaigns{};
      CCGDeliverySeq ccgs{};
    };

  }
}

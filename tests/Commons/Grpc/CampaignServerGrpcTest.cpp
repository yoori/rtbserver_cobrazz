#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/grpcpp.h>
#include <CampaignSvcs/CampaignServerClient/CampaignServerClient.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServerGrpc.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServerGrpcAlgs.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServerPool.hpp>

using namespace AdServer::CampaignSvcs;
namespace rpc = adserver::campaign::server;

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  const std::unordered_map<std::string, int> packed_sizes =
  {
    {"AppFormatInfo.timestamp", 8},
    {"SizeInfo.timestamp", 8},
    {"CreativeOptionInfo.timestamp", 8},
    {"CountryInfo.timestamp", 8},
    {"CreativeCategoryInfo.timestamp", 8},
    {"DeletedIdInfo.timestamp", 8},
    {"DeletedStringIdInfo.timestamp", 8},
    {"CampaignEcpmInfo.ecpm", 9},
    {"CampaignEcpmInfo.ctr", 9},
    {"CampaignEcpmInfo.timestamp", 8},
    {"ExpressionChannelInfo.timestamp", 8},
    {"ExpressionChannelInfo.imp_revenue", 9},
    {"ExpressionChannelInfo.click_revenue", 9},
    {"CampaignDeliveryLimitsInfo.date_start", 8},
    {"CampaignDeliveryLimitsInfo.date_end", 8},
    {"CampaignDeliveryLimitsInfo.budget", 9},
    {"CampaignDeliveryLimitsInfo.daily_budget", 9},
    {"ContractInfo.timestamp", 8},
    {"CampaignInfo.imp_revenue", 9},
    {"CampaignInfo.click_revenue", 9},
    {"CampaignInfo.action_revenue", 9},
    {"CampaignInfo.commision", 9},
    {"CampaignInfo.max_pub_share", 9},
    {"CampaignInfo.min_uid_age", 8},
    {"CampaignInfo.min_ctr_goal", 9},
    {"CampaignInfo.timestamp", 8},
    {"CampaignInfo.min_vtr_goal", 9},
    {"TagPricingInfo.imp_revenue", 9},
    {"TagPricingInfo.revenue_share", 9},
    {"TagInfo.adjustment", 9},
    {"TagInfo.auction_max_ecpm_share", 9},
    {"TagInfo.auction_prop_probability_share", 9},
    {"TagInfo.auction_random_share", 9},
    {"TagInfo.cost_coef", 9},
    {"TagInfo.tag_pricings_timestamp", 8},
    {"TagInfo.timestamp", 8},
    {"SiteInfo.timestamp", 8},
    {"ColocationInfo.revenue_share", 9},
    {"ColocationInfo.timestamp", 8},
    {"FreqCapInfo.timestamp", 8},
    {"CreativeTemplateInfo.timestamp", 8},
    {"CurrencyInfo.rate", 9},
    {"CurrencyInfo.timestamp", 8},
    {"CampaignKeywordInfo.timestamp", 8},
    {"AdvActionInfo.timestamp", 8},
    {"AccountInfo.time_offset", 8},
    {"AccountInfo.commision", 9},
    {"AccountInfo.budget", 9},
    {"AccountInfo.paid_amount", 9},
    {"AccountInfo.self_service_commission", 9},
    {"AccountInfo.timestamp", 8},
    {"CategoryChannelInfo.timestamp", 8},
    {"BehavParamInfo.timestamp", 8},
    {"KeyBehavParamInfo.timestamp", 8},
    {"BriefBehavParamInfo.timestamp", 8},
    {"BriefKeyBehavParamInfo.timestamp", 8},
    {"SimpleChannelKey.timestamp", 8},
    {"GeoChannelInfo.timestamp", 8},
    {"GeoCoordChannelInfo.longitude", 9},
    {"GeoCoordChannelInfo.latitude", 9},
    {"GeoCoordChannelInfo.radius", 9},
    {"GeoCoordChannelInfo.timestamp", 8},
    {"BlockChannelInfo.timestamp", 8},
    {"FraudConditionInfo.period", 8},
    {"FraudConditionInfo.timestamp", 8},
    {"SearchEngineInfo.timestamp", 8},
    {"WebBrowserInfo.timestamp", 8},
    {"PlatformInfo.timestamp", 8},
    {"WebOperationInfo.timestamp", 8},
    {"CampaignConfigUpdateInfo.master_stamp", 8},
    {"CampaignConfigUpdateInfo.first_load_stamp", 8},
    {"CampaignConfigUpdateInfo.finish_load_stamp", 8},
    {"CampaignConfigUpdateInfo.current_time", 8},
    {"CampaignConfigUpdateInfo.fraud_user_deactivate_period", 8},
    {"CampaignConfigUpdateInfo.cost_limit", 9},
    {"CampaignConfigUpdateInfo.global_params_timestamp", 8},
    {"CampaignConfigUpdateInfo.geo_channels_timestamp", 8},
    {"CreativeStatInfo.impressions", 17},
    {"CreativeStatInfo.clicks", 17},
    {"CreativeStatInfo.actions", 17},
    {"PublisherCCGStatInfo.amount", 9},
    {"TagCCGStatInfo.cur_pub_isp_amount", 9},
    {"TagCCGStatInfo.cur_adv_amount", 9},
    {"TagCCGStatInfo.cur_adv_comm_amount", 9},
    {"TagCCGStatInfo.prev_pub_isp_amount", 9},
    {"TagCCGStatInfo.prev_adv_amount", 9},
    {"TagCCGStatInfo.prev_adv_comm_amount", 9},
    {"CtrResetStatInfo.impressions", 17},
    {"CCGStatInfo.impressions", 17},
    {"CCGStatInfo.clicks", 17},
    {"CCGStatInfo.actions", 17},
    {"CCGStatInfo.amount", 9},
    {"CCGStatInfo.comm_amount", 9},
    {"CCGStatInfo.daily_amount", 9},
    {"CCGStatInfo.daily_comm_amount", 9},
    {"CCGStatInfo.prev_hour_amount", 9},
    {"CCGStatInfo.prev_hour_comm_amount", 9},
    {"CCGStatInfo.current_hour_amount", 9},
    {"CCGStatInfo.current_hour_comm_amount", 9},
    {"CampaignStatInfo.amount", 9},
    {"CampaignStatInfo.comm_amount", 9},
    {"CampaignStatInfo.daily_amount", 9},
    {"CampaignStatInfo.daily_comm_amount", 9},
    {"AmountStatInfo.amount", 9},
    {"AmountStatInfo.comm_amount", 9},
    {"AmountStatInfo.daily_amount", 9},
    {"AmountStatInfo.daily_comm_amount", 9},
    {"DayAmount.day", 8},
    {"DayAmount.amount", 9},
    {"DayAmountCount.day", 8},
    {"DayAmountCount.amount", 9},
    {"DayAmountCount.imps", 17},
    {"DayAmountCount.clicks", 17},
    {"DiscoverChannel.timestamp", 8},
    {"SimpleChannelAnswer.timezone_offset", 8},
    {"SimpleChannelAnswer.master_stamp", 8},
    {"SimpleChannelAnswer.cost_limit", 9},
    {"BriefSimpleChannelAnswer.timezone_offset", 8},
    {"BriefSimpleChannelAnswer.master_stamp", 8},
    {"ChannelServerChannelAnswer.cost_limit", 9},
    {"CampaignGetConfigSettings.timestamp", 8},
    {"CampaignGetConfigSettings.geo_channels_timestamp", 8},
    {"FraudConditionConfig.deactivate_period", 8},
    {"DetectorsConfig.timestamp", 8},
    {"ExpressionChannelsInfo.timestamp", 8},
    {"ExpressionChannelsInfo.first_load_stamp", 8},
    {"AccountDeliveryInfo.time_offset", 8},
    {"AccountDeliveryInfo.budget", 9},
    {"CampaignDeliveryInfo.time_offset", 8},
    {"CCGDeliveryInfo.time_offset", 8},
    {"CCGDeliveryInfo.imp_amount", 9},
    {"CCGDeliveryInfo.click_amount", 9},
    {"GetExpressionChannelsInfo.timestamp", 8},
    {"ColocationPropInfo.time_offset", 8},
  };

  const std::unordered_set<std::string> character_fields =
  {
    "CreativeOptionInfo.type",
    "CreativeInfo.status",
    "ExpressionInfo.operation",
    "ExpressionChannelInfo.status",
    "ExpressionChannelInfo.type",
    "BehavioralParameterInfo.trigger_type",
    "CampaignDeliveryLimitsInfo.delivery_pacing",
    "CampaignInfo.ccg_rate_type",
    "CampaignInfo.marketplace",
    "CampaignInfo.status",
    "CampaignInfo.eval_status",
    "CampaignInfo.ccg_type",
    "CampaignInfo.target_type",
    "TagPricingInfo.ccg_type",
    "TagPricingInfo.ccg_rate_type",
    "TagInfo.status",
    "TagInfo.marketplace",
    "SiteInfo.status",
    "AccountInfo.text_adserving",
    "AccountInfo.status",
    "AccountInfo.eval_status",
    "BehavParameter.trigger_type",
    "SimpleChannelKey.status",
    "FraudConditionInfo.type",
    "BehaveInfo.trigger_type",
    "CSSimpleChannel.channel_type",
    "CSSimpleChannel.status",
  };

  void fill(google::protobuf::Message& message, int depth = 0)
  {
    using F = google::protobuf::FieldDescriptor;
    const auto* descriptor = message.GetDescriptor();
    const auto* reflection = message.GetReflection();
    for (int i = 0; i < descriptor->field_count(); ++i)
    {
      const auto* field = descriptor->field(i);
      const std::string key = descriptor->name() + "." + field->name();
      const bool repeated = field->is_repeated();
      switch (field->cpp_type())
      {
        case F::CPPTYPE_MESSAGE:
          if (!repeated || depth < 8)
          {
            auto* child = repeated ? reflection->AddMessage(&message, field) :
              reflection->MutableMessage(&message, field);
            fill(*child, depth + 1);
          }
          break;
        case F::CPPTYPE_STRING:
        {
          const std::string value = field->type() == F::TYPE_BYTES ?
            std::string(packed_sizes.at(key), '\x81') : std::string("value\0", 6) + "кириллица";
          if (repeated)
          {
            reflection->AddString(&message, field, value);
          }
          else
          {
            reflection->SetString(&message, field, value);
          }
          break;
        }
        case F::CPPTYPE_UINT32:
        {
          const std::uint32_t value = field->name() == "portion" ? 1 :
            character_fields.count(key) ? 175 : 4000000001U;
          if (repeated)
          {
            reflection->AddUInt32(&message, field, value);
          }
          else
          {
            reflection->SetUInt32(&message, field, value);
          }
          break;
        }
        case F::CPPTYPE_INT32:
          reflection->SetInt32(&message, field, -2000000001);
          break;
        case F::CPPTYPE_UINT64:
          reflection->SetUInt64(&message, field, 9000000000001ULL);
          break;
        case F::CPPTYPE_INT64:
          reflection->SetInt64(&message, field, -9000000000001LL);
          break;
        case F::CPPTYPE_BOOL:
          reflection->SetBool(&message, field, true);
          break;
        case F::CPPTYPE_ENUM:
          reflection->SetEnum(&message, field, field->enum_type()->value(1));
          break;
        default:
          throw std::runtime_error("Unhandled campaign field");
      }
    }
  }

  template<class Proto>
  Proto fixture()
  {
    Proto result;
    fill(result);
    return result;
  }

  template<class Native, class Proto>
  Native* native_fixture()
  {
    auto value = std::make_unique<Native>();
    CampaignGrpc::unpack(fixture<Proto>(), *value);
    return value.release();
  }

  template<class Native, class Proto>
  void roundtrip()
  {
    const auto source = fixture<Proto>();
    Native native;
    CampaignGrpc::unpack(source, native);
    Proto restored;
    CampaignGrpc::pack(native, restored);
    CampaignGrpc::pack(native, restored);
    require(google::protobuf::util::MessageDifferencer::Equals(source, restored),
      source.GetDescriptor()->name().c_str());
  }

  template<class Proto, class Native>
  void check_value(const Native& value)
  {
    Proto actual;
    CampaignGrpc::pack(value, actual);
    require(google::protobuf::util::MessageDifferencer::Equals(fixture<Proto>(), actual),
      actual.GetDescriptor()->name().c_str());
  }

  class Source final : public CampaignServerApi
  {
  public:
    std::atomic<int> error{0};
    std::atomic<int> calls{0};
    std::atomic<int> delay_ms{0};
    std::atomic<int> large_count{0};
    void check();
    CampaignConfigUpdateInfo* get_config(
      const CampaignGetConfigSettings& get_config_settings) override;
    bool need_config(const TimestampInfo& req_timestamp) override;
    EcpmSeq* get_ecpms(const TimestampInfo& request_timestamp) override;
    ExpressionChannelsInfo* get_expression_channels(
      const CampaignServer::GetExpressionChannelsInfo& params) override;
    CampaignServer::PassbackInfo* get_tag_passback(
      std::uint32_t tag_id, const char* app_format) override;
    BriefSimpleChannelAnswer* brief_simple_channels(
      const CampaignServer::GetSimpleChannelsInfo& params) override;
    SimpleChannelAnswer* simple_channels(
      const CampaignServer::GetSimpleChannelsInfo& params) override;
    ChannelServerChannelAnswer* chsv_simple_channels(
      const CampaignServer::GetSimpleChannelsInfo& params) override;
    FraudConditionConfig* fraud_conditions() override;
    FreqCapConfigInfo* freq_caps() override;
    ColocationFlagsSeq* get_colocation_flags() override;
    DetectorsConfig* detectors(const TimestampInfo& request_timestamp) override;
    void update_stat() override;
    StatInfo* get_stat() override;
    BillStatInfo* get_bill_stat() override;
    CampaignServer::ColocationPropInfo* get_colocation_prop(std::uint32_t colo_id) override;
    CampaignServer::DeliveryLimitConfigInfo* get_delivery_limit_config() override;
  };

  void Source::check()
  {
    ++calls;
    const int delay = delay_ms;
    if (delay)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    switch (error.load())
    {
      case 1: throw CampaignServer::NotReady("not ready");
      case 2: throw CampaignServer::NotSupport("not supported");
      case 3: throw CampaignServer::ImplementationException("domain failure");
      case 4: throw std::runtime_error("unexpected failure");
    }
  }
  CampaignConfigUpdateInfo* Source::get_config(const CampaignGetConfigSettings& get_config_settings)
  {
    check();
    check_value<rpc::CampaignGetConfigSettings>(get_config_settings);
    auto result = std::unique_ptr<CampaignConfigUpdateInfo>(
      native_fixture<CampaignConfigUpdateInfo, rpc::CampaignConfigUpdateInfo>());
    if (large_count)
    {
      result->sizes.resize(large_count);
      for (std::uint32_t i = 0; i < result->sizes.size(); ++i)
      {
        CampaignGrpc::unpack(fixture<rpc::SizeInfo>(), result->sizes[i]);
      }
    }

    return result.release();
  }

  bool Source::need_config(const TimestampInfo& req_timestamp)
  {
    check();
    require(req_timestamp.size() == 8 && static_cast<unsigned char>(req_timestamp[0]) == 129,
      "request timestamp");
    return true;
  }

  EcpmSeq* Source::get_ecpms(const TimestampInfo& request_timestamp)
  {
    check();
    require(request_timestamp.size() == 8 &&
      static_cast<unsigned char>(request_timestamp[0]) == 129,
      "request timestamp");
    auto result = std::make_unique<EcpmSeq>();
    result->resize(1);
    CampaignGrpc::unpack(fixture<rpc::CampaignEcpmInfo>(), (*result)[0]);
    return result.release();
  }

  ExpressionChannelsInfo* Source::get_expression_channels(
    const CampaignServer::GetExpressionChannelsInfo& params)
  {
    check();
    check_value<rpc::GetExpressionChannelsInfo>(params);
    return native_fixture<ExpressionChannelsInfo, rpc::ExpressionChannelsInfo>();
  }

  CampaignServer::PassbackInfo* Source::get_tag_passback(
    std::uint32_t tag_id, const char* app_format)
  {
    check();
    require(tag_id == 4000000001U, "request id");
    require(std::string(app_format) == "value", "request string");
    return native_fixture<CampaignServer::PassbackInfo, rpc::PassbackInfo>();
  }

  BriefSimpleChannelAnswer* Source::brief_simple_channels(
    const CampaignServer::GetSimpleChannelsInfo& params)
  {
    check();
    check_value<rpc::GetSimpleChannelsInfo>(params);
    return native_fixture<BriefSimpleChannelAnswer, rpc::BriefSimpleChannelAnswer>();
  }

  SimpleChannelAnswer* Source::simple_channels(const CampaignServer::GetSimpleChannelsInfo& params)
  {
    check();
    check_value<rpc::GetSimpleChannelsInfo>(params);
    return native_fixture<SimpleChannelAnswer, rpc::SimpleChannelAnswer>();
  }

  ChannelServerChannelAnswer* Source::chsv_simple_channels(
    const CampaignServer::GetSimpleChannelsInfo& params)
  {
    check();
    check_value<rpc::GetSimpleChannelsInfo>(params);
    return native_fixture<ChannelServerChannelAnswer, rpc::ChannelServerChannelAnswer>();
  }

  FraudConditionConfig* Source::fraud_conditions()
  {
    check();
    return native_fixture<FraudConditionConfig, rpc::FraudConditionConfig>();
  }

  FreqCapConfigInfo* Source::freq_caps()
  {
    check();
    return native_fixture<FreqCapConfigInfo, rpc::FreqCapConfigInfo>();
  }

  ColocationFlagsSeq* Source::get_colocation_flags()
  {
    check();
    auto result = std::make_unique<ColocationFlagsSeq>();
    result->resize(1);
    CampaignGrpc::unpack(fixture<rpc::ColocationFlags>(), (*result)[0]);
    return result.release();
  }

  DetectorsConfig* Source::detectors(const TimestampInfo& request_timestamp)
  {
    check();
    require(request_timestamp.size() == 8 &&
      static_cast<unsigned char>(request_timestamp[0]) == 129,
      "request timestamp");
    return native_fixture<DetectorsConfig, rpc::DetectorsConfig>();
  }

  void Source::update_stat()
  {
    check();
  }

  StatInfo* Source::get_stat()
  {
    check();
    return native_fixture<StatInfo, rpc::StatInfo>();
  }

  BillStatInfo* Source::get_bill_stat()
  {
    check();
    return native_fixture<BillStatInfo, rpc::BillStatInfo>();
  }

  CampaignServer::ColocationPropInfo* Source::get_colocation_prop(std::uint32_t colo_id)
  {
    check();
    require(colo_id == 4000000001U, "request id");
    return native_fixture<CampaignServer::ColocationPropInfo, rpc::ColocationPropInfo>();
  }

  CampaignServer::DeliveryLimitConfigInfo* Source::get_delivery_limit_config()
  {
    check();
    return native_fixture<CampaignServer::DeliveryLimitConfigInfo, rpc::DeliveryLimitConfigInfo>();
  }

  void test_calls(CampaignServerClient& client)
  {
    {
      std::unique_ptr<CampaignGetConfigSettings> get_config_settings(
        native_fixture<CampaignGetConfigSettings, rpc::CampaignGetConfigSettings>());
      std::unique_ptr<CampaignConfigUpdateInfo> result(client.get_config(*get_config_settings));
      check_value<rpc::CampaignConfigUpdateInfo>(*result);
    }

    {
      TimestampInfo req_timestamp;
      req_timestamp.resize(8);
      std::fill_n(req_timestamp.data(), 8, 129);
      require(client.need_config(req_timestamp), "need_config");
    }

    {
      TimestampInfo request_timestamp;
      request_timestamp.resize(8);
      std::fill_n(request_timestamp.data(), 8, 129);
      std::unique_ptr<EcpmSeq> result(client.get_ecpms(request_timestamp));
      require(result->size() == 1, "sequence response");
      check_value<rpc::CampaignEcpmInfo>((*result)[0]);
    }

    {
      std::unique_ptr<CampaignServer::GetExpressionChannelsInfo> params(
        native_fixture<CampaignServerApi::GetExpressionChannelsInfo,
          rpc::GetExpressionChannelsInfo>());
      std::unique_ptr<ExpressionChannelsInfo> result(client.get_expression_channels(*params));
      check_value<rpc::ExpressionChannelsInfo>(*result);
    }

    {
      std::unique_ptr<CampaignServer::PassbackInfo> result(
        client.get_tag_passback(4000000001U, "value"));
      check_value<rpc::PassbackInfo>(*result);
    }

    {
      std::unique_ptr<CampaignServer::GetSimpleChannelsInfo> params(
        native_fixture<CampaignServer::GetSimpleChannelsInfo, rpc::GetSimpleChannelsInfo>());
      std::unique_ptr<BriefSimpleChannelAnswer> result(client.brief_simple_channels(*params));
      check_value<rpc::BriefSimpleChannelAnswer>(*result);
    }

    {
      std::unique_ptr<CampaignServer::GetSimpleChannelsInfo> params(
        native_fixture<CampaignServer::GetSimpleChannelsInfo, rpc::GetSimpleChannelsInfo>());
      std::unique_ptr<SimpleChannelAnswer> result(client.simple_channels(*params));
      check_value<rpc::SimpleChannelAnswer>(*result);
    }

    {
      std::unique_ptr<CampaignServer::GetSimpleChannelsInfo> params(
        native_fixture<CampaignServer::GetSimpleChannelsInfo, rpc::GetSimpleChannelsInfo>());
      std::unique_ptr<ChannelServerChannelAnswer> result(client.chsv_simple_channels(*params));
      check_value<rpc::ChannelServerChannelAnswer>(*result);
    }

    {
      std::unique_ptr<FraudConditionConfig> result(client.fraud_conditions());
      check_value<rpc::FraudConditionConfig>(*result);
    }

    {
      std::unique_ptr<FreqCapConfigInfo> result(client.freq_caps());
      check_value<rpc::FreqCapConfigInfo>(*result);
    }

    {
      std::unique_ptr<ColocationFlagsSeq> result(client.get_colocation_flags());
      require(result->size() == 1, "sequence response");
      check_value<rpc::ColocationFlags>((*result)[0]);
    }

    {
      TimestampInfo request_timestamp;
      request_timestamp.resize(8);
      std::fill_n(request_timestamp.data(), 8, 129);
      std::unique_ptr<DetectorsConfig> result(client.detectors(request_timestamp));
      check_value<rpc::DetectorsConfig>(*result);
    }

    {
      client.update_stat();
    }

    {
      std::unique_ptr<StatInfo> result(client.get_stat());
      check_value<rpc::StatInfo>(*result);
    }

    {
      std::unique_ptr<BillStatInfo> result(client.get_bill_stat());
      check_value<rpc::BillStatInfo>(*result);
    }

    {
      std::unique_ptr<CampaignServer::ColocationPropInfo> result(
        client.get_colocation_prop(4000000001U));
      check_value<rpc::ColocationPropInfo>(*result);
    }

    {
      std::unique_ptr<CampaignServer::DeliveryLimitConfigInfo> result(
        client.get_delivery_limit_config());
      check_value<rpc::DeliveryLimitConfigInfo>(*result);
    }

  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc == 2)
    {
      CampaignServerClient client(argv[1]);
      try
      {
        std::unique_ptr<FreqCapConfigInfo> result(client.freq_caps());
      }
      catch (const CampaignServer::NotReady&)
      {
        std::cout << "PASS: live CampaignServer business RPC reports NotReady\n";
        return 0;
      }

      throw std::runtime_error("Expected NotReady from unconfigured CampaignServer");
    }

    const auto zero_time = std::string(8, '\0');
    require(BriefBehavParamInfo{}.timestamp == zero_time, "Default behavior timestamp changed");
    require(BriefKeyBehavParamInfo{}.timestamp == zero_time, "Default key timestamp changed");
    require(BriefSimpleChannelAnswer{}.timezone_offset == zero_time,
      "Default timezone changed");
    require(BriefSimpleChannelAnswer{}.master_stamp == zero_time,
      "Default master timestamp changed");

    roundtrip<AppFormatInfo, rpc::AppFormatInfo>();
    roundtrip<SizeInfo, rpc::SizeInfo>();
    roundtrip<CreativeOptionInfo, rpc::CreativeOptionInfo>();
    roundtrip<OptionValueInfo, rpc::OptionValueInfo>();
    roundtrip<CountryInfo, rpc::CountryInfo>();
    roundtrip<ExternalCategoryInfo, rpc::ExternalCategoryInfo>();
    roundtrip<CreativeCategoryInfo, rpc::CreativeCategoryInfo>();
    roundtrip<CreativeSizeInfo, rpc::CreativeSizeInfo>();
    roundtrip<CreativeInfo, rpc::CreativeInfo>();
    roundtrip<ChannelWeight, rpc::ChannelWeight>();
    roundtrip<DeletedIdInfo, rpc::DeletedIdInfo>();
    roundtrip<DeletedStringIdInfo, rpc::DeletedStringIdInfo>();
    roundtrip<CampaignEcpmInfo, rpc::CampaignEcpmInfo>();
    roundtrip<ExpressionInfo, rpc::ExpressionInfo>();
    roundtrip<ExpressionChannelInfo, rpc::ExpressionChannelInfo>();
    {
      auto source = fixture<rpc::ExpressionChannelInfo>();
      source.set_channel_rate_id(0);
      source.clear_imp_revenue();
      source.clear_click_revenue();
      ExpressionChannelInfo native;
      CampaignGrpc::unpack(source, native);
      require(native.imp_revenue.empty() && native.click_revenue.empty(),
        "Channel without a rate must preserve absent revenues");
      rpc::ExpressionChannelInfo restored;
      CampaignGrpc::pack(native, restored);
      require(source.SerializeAsString() == restored.SerializeAsString(),
        "Channel without a rate roundtrip");

      for (const auto rate_id : {0U, 1U})
      {
        for (const auto* field_name : {"imp_revenue", "click_revenue"})
        {
          for (const auto& invalid : {std::string(), std::string("x"), std::string(10, 'x')})
          {
            if (!rate_id && invalid.empty())
            {
              continue;
            }

            auto malformed = fixture<rpc::ExpressionChannelInfo>();
            malformed.set_channel_rate_id(rate_id);
            const auto* field = malformed.GetDescriptor()->FindFieldByName(field_name);
            malformed.GetReflection()->SetString(&malformed, field, invalid);
            bool rejected = false;
            try
            {
              CampaignGrpc::unpack(malformed, native);
            }
            catch (const std::invalid_argument&)
            {
              rejected = true;
            }

            require(rejected, "Malformed channel revenue must be rejected");
          }
        }
      }
    }

    roundtrip<BehavioralParameterInfo, rpc::BehavioralParameterInfo>();
    roundtrip<WeeklyRunInterval, rpc::WeeklyRunInterval>();
    roundtrip<CampaignDeliveryLimitsInfo, rpc::CampaignDeliveryLimitsInfo>();
    roundtrip<TagDeliveryInfo, rpc::TagDeliveryInfo>();
    roundtrip<ContractInfo, rpc::ContractInfo>();
    roundtrip<CampaignInfo, rpc::CampaignInfo>();
    roundtrip<TagPricingInfo, rpc::TagPricingInfo>();
    roundtrip<TemplateOptionsInfo, rpc::TemplateOptionsInfo>();
    roundtrip<TagSizeInfo, rpc::TagSizeInfo>();
    roundtrip<TagInfo, rpc::TagInfo>();
    roundtrip<SiteInfo, rpc::SiteInfo>();
    roundtrip<ColocationInfo, rpc::ColocationInfo>();
    roundtrip<ColocationFlags, rpc::ColocationFlags>();
    roundtrip<FreqCapInfo, rpc::FreqCapInfo>();
    roundtrip<CreativeTemplateFileInfo, rpc::CreativeTemplateFileInfo>();
    roundtrip<CreativeTemplateInfo, rpc::CreativeTemplateInfo>();
    roundtrip<CurrencyInfo, rpc::CurrencyInfo>();
    roundtrip<CampaignKeywordInfo, rpc::CampaignKeywordInfo>();
    roundtrip<AdvActionInfo, rpc::AdvActionInfo>();
    roundtrip<AccountInfo, rpc::AccountInfo>();
    roundtrip<CategoryChannelLocalizationInfo, rpc::CategoryChannelLocalizationInfo>();
    roundtrip<CategoryChannelInfo, rpc::CategoryChannelInfo>();
    roundtrip<PriceRangeInfo, rpc::PriceRange>();
    roundtrip<BehavParameter, rpc::BehavParameter>();
    roundtrip<BehavParamInfo, rpc::BehavParamInfo>();
    roundtrip<KeyBehavParamInfo, rpc::KeyBehavParamInfo>();
    roundtrip<BriefBehavParamInfo, rpc::BriefBehavParamInfo>();
    roundtrip<BriefKeyBehavParamInfo, rpc::BriefKeyBehavParamInfo>();
    roundtrip<SimpleChannelKey, rpc::SimpleChannelKey>();
    roundtrip<BriefSimpleChannelKey, rpc::BriefSimpleChannelKey>();
    roundtrip<GeoIPRegionCityInfo, rpc::GeoIPRegionCityInfo>();
    roundtrip<GeoChannelInfo, rpc::GeoChannelInfo>();
    roundtrip<GeoCoordChannelInfo, rpc::GeoCoordChannelInfo>();
    roundtrip<BlockChannelInfo, rpc::BlockChannelInfo>();
    roundtrip<FraudConditionInfo, rpc::FraudConditionInfo>();
    roundtrip<SearchEngineRegExpInfo, rpc::SearchEngineRegExpInfo>();
    roundtrip<SearchEngineInfo, rpc::SearchEngineInfo>();
    roundtrip<WebBrowserDetectorInfo, rpc::WebBrowserDetectorInfo>();
    roundtrip<WebBrowserInfo, rpc::WebBrowserInfo>();
    roundtrip<PlatformDetectorInfo, rpc::PlatformDetectorInfo>();
    roundtrip<PlatformInfo, rpc::PlatformInfo>();
    roundtrip<WebOperationInfo, rpc::WebOperationInfo>();
    roundtrip<CampaignConfigUpdateInfo, rpc::CampaignConfigUpdateInfo>();
    roundtrip<CreativeStatInfo, rpc::CreativeStatInfo>();
    roundtrip<PublisherCCGStatInfo, rpc::PublisherCCGStatInfo>();
    roundtrip<TagCCGStatInfo, rpc::TagCCGStatInfo>();
    roundtrip<CtrResetStatInfo, rpc::CtrResetStatInfo>();
    roundtrip<CCGStatInfo, rpc::CCGStatInfo>();
    roundtrip<CampaignStatInfo, rpc::CampaignStatInfo>();
    roundtrip<AmountStatInfo, rpc::AmountStatInfo>();
    roundtrip<StatInfo, rpc::StatInfo>();
    roundtrip<DayAmount, rpc::DayAmount>();
    roundtrip<AmountDistributionInfo, rpc::AmountDistributionInfo>();
    roundtrip<AccountAmountDistributionInfo, rpc::AccountAmountDistributionInfo>();
    roundtrip<DayAmountCount, rpc::DayAmountCount>();
    roundtrip<AmountCountDistributionInfo, rpc::AmountCountDistributionInfo>();
    roundtrip<CampaignAmountDistributionInfo, rpc::CampaignAmountDistributionInfo>();
    roundtrip<CCGAmountDistributionInfo, rpc::CCGAmountDistributionInfo>();
    roundtrip<BillStatInfo, rpc::BillStatInfo>();
    roundtrip<DiscoverChannel, rpc::DiscoverChannel>();
    roundtrip<SimpleChannelAnswer, rpc::SimpleChannelAnswer>();
    roundtrip<BriefSimpleChannelAnswer, rpc::BriefSimpleChannelAnswer>();
    roundtrip<BehaveInfo, rpc::BehaveInfo>();
    roundtrip<CSSimpleChannel, rpc::CSSimpleChannel>();
    roundtrip<ChannelServerChannelAnswer, rpc::ChannelServerChannelAnswer>();
    roundtrip<CampaignGetConfigSettings, rpc::CampaignGetConfigSettings>();
    roundtrip<FraudConditionConfig, rpc::FraudConditionConfig>();
    roundtrip<DetectorsConfig, rpc::DetectorsConfig>();
    roundtrip<FreqCapConfigInfo, rpc::FreqCapConfigInfo>();
    roundtrip<ExpressionChannelCCGInfo, rpc::ExpressionChannelCCGInfo>();
    roundtrip<ChannelTriggersInfo, rpc::ChannelTriggersInfo>();
    roundtrip<ExpressionChannelsInfo, rpc::ExpressionChannelsInfo>();
    roundtrip<AccountDeliveryInfo, rpc::AccountDeliveryInfo>();
    roundtrip<CampaignDeliveryInfo, rpc::CampaignDeliveryInfo>();
    roundtrip<CCGDeliveryInfo, rpc::CCGDeliveryInfo>();
    roundtrip<CampaignServer::GetExpressionChannelsInfo, rpc::GetExpressionChannelsInfo>();
    roundtrip<CampaignServer::GetSimpleChannelsInfo, rpc::GetSimpleChannelsInfo>();
    roundtrip<CampaignServer::PassbackInfo, rpc::PassbackInfo>();
    roundtrip<CampaignServer::ColocationPropInfo, rpc::ColocationPropInfo>();
    roundtrip<CampaignServer::DeliveryLimitConfigInfo, rpc::DeliveryLimitConfigInfo>();
    Source source;
    CampaignServerGrpcService service(source);
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    require(server && port, "server startup");
    const auto endpoint = "127.0.0.1:" + std::to_string(port);
    CampaignServerClient client(endpoint);
    test_calls(client);
    require(source.calls == 17, "all 17 RPCs");

    for (int error = 1; error <= 4; ++error)
    {
      source.error = error;
      bool caught = false;
      try
      {
        client.update_stat();
      }
      catch (const CampaignServer::NotReady&)
      {
        caught = error == 1;
      }
      catch (const CampaignServer::NotSupport&)
      {
        caught = error == 2;
      }
      catch (const CampaignServer::ImplementationException&)
      {
        caught = error >= 3;
      }

      require(caught, "error mapping");
    }

    source.error = 0;
    source.delay_ms = 150;
    CampaignServerClient impatient(endpoint, std::chrono::milliseconds(20));
    const int before = source.calls;
    bool timed_out = false;
    try
    {
      impatient.update_stat();
    }
    catch (const CampaignServer::ImplementationException& e)
    {
      timed_out = std::string(e.what()).find("gRPC 4:") != std::string::npos;
    }

    require(timed_out, "deadline propagation");
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    require(source.calls == before + 1, "no automatic retries");
    source.delay_ms = 0;

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto stub = rpc::CampaignServer::NewStub(
      grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args));
    grpc::ClientContext invalid_context;
    rpc::get_configRequest invalid_request;
    rpc::get_configResponse invalid_response;
    require(stub->get_config(&invalid_context, invalid_request, &invalid_response).error_code() ==
      grpc::StatusCode::INVALID_ARGUMENT, "reject malformed request");

    auto settings = std::unique_ptr<CampaignGetConfigSettings>(
      native_fixture<CampaignGetConfigSettings, rpc::CampaignGetConfigSettings>());
    source.large_count = 150000;
    std::unique_ptr<CampaignConfigUpdateInfo> large(client.get_config(*settings));
    require(large->sizes.size() == 150000, "large response truncated");
    rpc::CampaignConfigUpdateInfo wire;
    CampaignGrpc::pack(*large, wire);
    require(wire.ByteSizeLong() > 4 * 1024 * 1024, "large response must exceed 4 MiB");
    source.large_count = 0;

    CampaignServerPool pool({endpoint, endpoint}, Generics::Time(0, 20000));
    auto first = pool.get_object(0);
    require(first.operator->() == pool.get_object(0).operator->(), "persistent selection");
    first.release_bad(String::SubString("failed"));
    auto second = pool.get_object(0);
    require(second.operator->() == pool.get_object(1).operator->(), "failover");
    second->update_stat();
    second.release_bad(String::SubString("failed"));
    bool all_bad = false;
    try
    {
      pool.get_object(0);
    }
    catch (const CampaignServerPool::Exception&)
    {
      all_bad = true;
    }

    require(all_bad, "all bad endpoints must not busy-loop");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    require(pool.get_object(0).operator->() == first.operator->(), "endpoint recovery");

    rpc::CampaignEcpmInfo malformed;
    malformed.set_ecpm("x");
    CampaignEcpmInfo native;
    bool rejected = false;
    try
    {
      CampaignGrpc::unpack(malformed, native);
    }
    catch (const std::invalid_argument&)
    {
      rejected = true;
    }

    require(rejected, "malformed packed decimal");
    server->Shutdown();
    server->Wait();
    bool unavailable = false;
    try
    {
      client.update_stat();
    }
    catch (const CampaignServer::ImplementationException&)
    {
      unavailable = true;
    }

    require(unavailable, "unavailable endpoint");
    std::cout << "PASS: all campaign values, 17 RPCs, errors, deadlines, large responses, pool\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }

  return 1;
}
